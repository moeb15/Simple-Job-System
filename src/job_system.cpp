#include "job_system.hpp"
#include "timer.hpp"

namespace
{
    constexpr u32 JOB_QUEUE_SIZE{ 1 << 9 };
    constexpr u32 MAX_BACKOFF_MICROSECONDS{ 1000 };
    JobScheduler* s_JobScheduler{ nullptr };

    // https://blog.molecular-matters.com/2015/09/08/job-system-2-0-lock-free-work-stealing-part-2-a-specialized-allocator/
    thread_local u64 g_LocalIndex{ 0 };
    thread_local Job g_JobAllocator[JOB_ALLOCATOR_COUNT]{};

    // agx dynamics HashFunction.h
    u64 SplitMix64(u64 x)
    {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }
}

thread_local i8 JobScheduler::s_TLSIndex{ -1 };

JobScheduler* JobScheduler::Get()
{
    if (s_JobScheduler == nullptr) s_JobScheduler = new JobScheduler();
    return s_JobScheduler;
}

bool JobScheduler::Initialize()
{
    m_IsRunning.store(true, std::memory_order_release);

    m_ThreadCount = (u32)std::max(1u, (u32)std::thread::hardware_concurrency() - 1);
    m_ThreadCount = (u32)std::min(m_ThreadCount, (u32)MAX_JOB_THREADS);

    for (u32 i = 0; i < m_ThreadCount; i++)
    {
        m_JobThreads[i].index = i;
        m_JobThreads[i].workerThread = std::thread(&JobScheduler::JobThreadFunc, this, i);
    }

    return true;
}

void JobScheduler::Shutdown()
{
    m_IsRunning.store(false, std::memory_order_release);
    m_WakeCV.notify_all();

    for (u8 i = 0; i < m_ThreadCount; i++)
    {
        m_JobThreads[i].workerThread.join();
    }

    delete s_JobScheduler;
    s_JobScheduler = nullptr;
}

Job* JobScheduler::SubmitJob(JobFunction func, std::initializer_list<Job*> prerequisites, void* paramData, u32 paramDataSize)
{
    Job* newJob = AllocateJob();
    newJob->func = func;

    u32 count{ 0 };
    for (Job* prereq : prerequisites)
    {
        if (prereq->jobState.load(std::memory_order_acquire) == EJobState::Complete) continue;

        prereq->dependents.push_back(newJob);
        count++;
    }
    newJob->unfinishedPrereqs.store(count, std::memory_order_release);

    if (paramData && paramDataSize)
    {
        if (paramDataSize <= JOB_PARAM_BYTE_SIZE)
        {
            memcpy(&newJob->inlineData[0], paramData, paramDataSize);
        }
        else
        {
            void* data = malloc(paramDataSize);
            memcpy(data, paramData, paramDataSize);
            newJob->paramData = data;
        }
    }

    m_JobsInFlight.fetch_add(1, std::memory_order_relaxed);
    if (count == 0)
    {
        Enqueue(newJob);
    }

    return newJob;
}

void JobScheduler::Wait()
{
    while (m_JobsInFlight.load(std::memory_order_acquire) > 0)
    {
        if (Job* job = FetchJob(-1))
        {
            Execute(job);
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

bool JobScheduler::IsBusy()
{
    return m_JobsInFlight.load(std::memory_order_acquire) > 0;
}

EJobState JobScheduler::GetJobState(Job* job)
{
    JS_ASSERT(job);
    return job->jobState.load(std::memory_order_acquire);
}

void JobScheduler::Enqueue(Job* job)
{
    u32 index{};

    // check if we're on a worker thread
    if (s_TLSIndex >= 0)
    {
        index = static_cast<u32>(s_TLSIndex);
        JS_ASSERT(m_JobThreads[index].index == index);

        // exponential backoff
        u32 backoff{ 0 };
        while (!m_JobThreads[index].Push(job))
        {
            if (backoff == 0)
            {
                std::this_thread::yield();
                backoff = 10;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::microseconds(backoff));
                backoff = std::min(backoff * 2, MAX_BACKOFF_MICROSECONDS);
            }
        }
    }
    else
    {
        // we are on the main thread so push to the global queue that workers can steal from,
        // if the global queue is full, have the main thread fetch jobs and execute them
        job->jobState.store(EJobState::Queued, std::memory_order_release);
        while (!m_GlobalQueue.PushOwner(job))
        {
            if (Job* j = m_GlobalQueue.PopOwner())
            {
                Execute(j);
            }
        }
    }
    m_WakeCV.notify_one();
}

Job* JobScheduler::FetchJob(i8 index)
{
    if (index >= 0)
    {
        if (Job* job = m_JobThreads[index].jobQueue.PopOwner()) return job;
    }

    // try stealing from global queue
    if (Job* job = m_GlobalQueue.Steal()) return job;

    // steal from random job thread
    static thread_local u64 localCounter = SplitMix64(static_cast<u64>(Timer::Get()->Now()));
    for (u32 i = 0; i < m_ThreadCount * 2; i++)
    {
        u64 randomValue = SplitMix64(++localCounter);
        u32 v = static_cast<u32>(randomValue % m_ThreadCount);
        if (v == index) continue;
        if (Job* job = m_JobThreads[v].jobQueue.Steal()) return job;
    }

    return nullptr;
}

void JobScheduler::Execute(Job* job)
{
    EJobState expected = EJobState::Queued;
    if (!job->jobState.compare_exchange_strong(
        expected,
        EJobState::Running,
        std::memory_order_acq_rel
    ))
    {
        JS_ASSERT(expected != EJobState::Created);
        return;
    }

    JS_ASSERT(job->func.IsValid());

    void* data{ nullptr };
    if (job->paramData)
    {
        data = job->paramData;
    }
    else
    {
        data = (void*)(&job->inlineData[0]);
    }

    job->func.Invoke(job, data);
    job->jobState.store(EJobState::Complete, std::memory_order_release);
    Finish(job);
}

void JobScheduler::Finish(Job* job)
{
    JS_ASSERT(job);

    // decrement unfinished prereq from each dependent job
    // and queue each one that is ready to be queued
    u32 numDependents = (u32)job->dependents.size();
    for (u32 i = 0; i < numDependents; i++)
    {
        Job* dependent = job->dependents[i];
        JS_ASSERT(dependent);
        if (dependent->unfinishedPrereqs.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            Enqueue(dependent);
        }
    }

    m_JobsInFlight.fetch_sub(1, std::memory_order_acq_rel);

    FreeJob(job);
}

void JobScheduler::JobThreadFunc(i8 idx)
{
    s_TLSIndex = idx;

    m_JobThreads[idx].owner = std::this_thread::get_id();

    while (m_IsRunning.load(std::memory_order_acquire))
    {
        if (Job* job = FetchJob(idx))
        {
            //printf("Worker %d executing job %p\n", idx, job);
            Execute(job);
        }
        else
        {
            std::unique_lock<std::mutex> lock{ m_SleepMutex };
            m_WakeCV.wait_for(lock, std::chrono::microseconds(10));
        }
    }
}

Job* JobScheduler::AllocateJob()
{
    const u64 index = g_LocalIndex++;

    Job* job = &g_JobAllocator[index & (JOB_ALLOCATOR_COUNT - 1)];

    job->func = {};
    job->dependents.clear();
    job->paramData = nullptr;
    job->unfinishedPrereqs.store(0, std::memory_order_relaxed);
    job->jobState.store(EJobState::Created, std::memory_order_relaxed);
    memset(&job->inlineData[0], 0, JOB_PARAM_BYTE_SIZE);

    return job;
}

void JobScheduler::FreeJob(Job* job)
{
    JS_ASSERT(job);

    memset(&job->inlineData[0], 0, JOB_PARAM_BYTE_SIZE);
    if (job->paramData)
    {
        free(job->paramData);
    }
}

u32 JobScheduler::RandomWorkerIndex()
{
    static thread_local u64 localIndexCounter = SplitMix64(static_cast<u64>(Timer::Get()->Now()));
    u64 randomValue = SplitMix64(++localIndexCounter);
    return static_cast<u32>(randomValue % m_ThreadCount);
}