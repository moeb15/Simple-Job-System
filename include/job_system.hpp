#pragma once

#include "defines.hpp"
#include "function.hpp"
#include "ring_deque.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

static constexpr u32 MAX_JOB_THREADS{ 32 };
static constexpr u32 JOB_COUNT_LOG2{ 9 };
static constexpr u32 GLOBAL_QUEUE_JOB_COUNT_LOG2{ 12 };
static constexpr u32 JOB_ALLOCATOR_COUNT{ 1 << 16 };

struct Job;

using JobFunction = TFunction<void(Job*, void*)>;

enum class EJobState : u8
{
    Created,
    Queued,
    Running,
    Complete,
};

/**
 * @brief Represents a single Job the JobScheduler executes
 */
struct Job
{
    JobFunction func{};                                      // Job function to execute
    void* paramData{ nullptr };                              // Optional parameter data, allocates on heap
    std::vector<Job*> dependents{};                          // Jobs that depend on this job
    std::atomic<i32> unfinishedPrereqs{ 0 };                 // atomic counter for remaining dependencies
    std::atomic<EJobState> jobState{ EJobState::Created };   // atomic that signals the state of the job
};

/**
 * @brief Represents a single worker thread maintained by the JobScheduler
 */
struct JobThread
{
    bool Push(Job* job)
    {
        JS_ASSERT(std::this_thread::get_id() == owner);
        const bool result = jobQueue.PushOwner(job);
        if(result)
        {
            job->jobState.store(EJobState::Queued, std::memory_order_release);
        }
        return result;
    }

    u32 index{};                                       // JobThread index
    std::thread workerThread{};                        // Worker thread
    TRingDeque<Job, JOB_COUNT_LOG2> jobQueue{};        // Circular work-stealing deque of jobs
    std::atomic<std::thread::id> owner;                // Owning thread id
};

/**
 * @brief Schedules Jobs to executed across multiple JobThreads
 */
class JobScheduler
{
    JobScheduler() = default;
public:
    static JobScheduler* Get();

    bool Initialize();
    void Shutdown();

    /**
     * @brief Submits a Job to a worker threads deque if called on a separate thread
     * @brief places in global deque if called on the main thread, jobs without dependencies
     * @brief are queued as soon as there is space on the global/worker thread deque
     * 
     * @param func The function that the job will execute
     * @param prerequisites The prerequiste jobs that must complete before the currently submitted job
     * @param  paramData Optional parameter data that will be passed to func when the job executes, allocated on heap
     * @param paramDataSize The size of the optional parameter data
     */
    Job* SubmitJob(JobFunction func, std::initializer_list<Job*> prerequisites = {},
        void* paramData = nullptr, u32 paramDataSize = 0);
    
    /**
     * @brief Waits for all jobs in flight to finish executing
     */
    void Wait();
    EJobState GetJobState(Job* job);

private:
    /**
     * @brief Queues a job on a worker thread if called from a worker thread
     * @brief otherwise queues on the main thread
     * 
     * @param job Job to queued
     */
    void Enqueue(Job* job);

    /**
     * @brief Attempts to fetch a job from the given job thread index,
     * @brief if no job can be fetched, steals from global deque or worker threads
     * 
     * @param index Index of the current job thread
     */
    Job* FetchJob(i8 index);

    /**
     * @brief Attempts to execute a job if it is not currently running or complete
     * 
     * @param job Job to be executed
     */
    void Execute(Job* job);

    /**
     * @brief Performs clean up on an executed job, queues job dependents if possible
     * 
     * @param job Completed job
     */
    void Finish(Job* job);

    void JobThreadFunc(i8 idx);
    Job* AllocateJob();
    void FreeJob(Job* job);
    u32 RandomWorkerIndex();

private:
    JobThread m_JobThreads[MAX_JOB_THREADS];
    std::atomic<bool> m_IsRunning{ true };
    std::atomic<u32> m_JobsInFlight{ 0 };
    std::mutex m_SleepMutex;
    std::condition_variable m_WakeCV;
    u32 m_ThreadCount{ 0 };
    TRingDeque<Job, GLOBAL_QUEUE_JOB_COUNT_LOG2> m_GlobalQueue{};
    static thread_local i8 s_TLSIndex;
};