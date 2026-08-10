#include "tests/empty_job_test.hpp"
#include "timer.hpp"
#include <iostream>

namespace
{
    constexpr u32 JOB_COUNT{ 1 << 16 };

    void EmptyJob(Job* j = nullptr, void* d = nullptr)
    {
    }

    JobFunction jobFunc;
}

void EmptyJobTest::Initialize()
{
    jobFunc.Bind<EmptyJob>();
}

void EmptyJobTest::Run()
{
    std::cout << "Beginning empty job test, invoking " << JOB_COUNT << " function calls sequentially." << std::endl;

    i64 sequentialBegin = Timer::Get()->Now();
    for(u32 i = 0; i < JOB_COUNT; i++)
    {
        EmptyJob();
    }
    i64 sequentialEnd = Timer::Get()->Now();
    f64 sequentialMs = Timer::Get()->DeltaMiliseconds(sequentialBegin, sequentialEnd);

    std::cout << "Sequential Test Time: " << sequentialMs << " ms." << std::endl;

    std::cout << "Beginning empty job test, creating " << JOB_COUNT << " jobs." << std::endl;
    i64 jobBegin = Timer::Get()->Now();
    for(u32 i = 0; i < JOB_COUNT; i++)
    {
        JobScheduler::Get()->SubmitJob(jobFunc);
    }
    
    while(JobScheduler::Get()->IsBusy())
    {
        // do nothing
    }

    i64 jobEnd = Timer::Get()->Now();
    f64 jobMs = Timer::Get()->DeltaMiliseconds(jobBegin, jobEnd);
    std::cout << "JobSystem Test Time: " << jobMs << " ms." << std::endl;
}