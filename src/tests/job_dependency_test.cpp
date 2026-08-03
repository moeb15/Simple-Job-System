#include "tests/job_dependency_test.hpp"
#include "timer.hpp"
#include <iostream>


namespace
{
    void JobA(Job* j, void* d)
    {
        printf("JobA\n");
    }

    void JobB(Job* j, void* d)
    {
        printf("JobB\n");
    }

    void JobC(Job* j, void* d)
    {
        printf("JobC\n");
    }

    void JobD(Job* j, void* d)
    {
        printf("JobD\n");
    }

    JobFunction jobFuncA;
    JobFunction jobFuncB;
    JobFunction jobFuncC;
    JobFunction jobFuncD;
}

void JobDependencyTest::Initialize()
{
    jobFuncA.Bind<JobA>();
    jobFuncB.Bind<JobB>();
    jobFuncC.Bind<JobC>();
    jobFuncD.Bind<JobD>();
}

void JobDependencyTest::Run()
{
    printf("Beginning job dependency test\n");
    printf("A->B->C->D\n");

    Job* jobA = JobScheduler::Get()->SubmitJob(jobFuncA);
    Job* jobB = JobScheduler::Get()->SubmitJob(jobFuncB, EJobPriority::Normal, { jobA });
    Job* jobC = JobScheduler::Get()->SubmitJob(jobFuncC, EJobPriority::Normal, { jobB });
    Job* jobD = JobScheduler::Get()->SubmitJob(jobFuncD, EJobPriority::Normal, { jobC });

    while (JobScheduler::Get()->IsBusy()) {}

    printf("(A, B, D)->C\n");

    jobA = JobScheduler::Get()->SubmitJob(jobFuncA);
    jobB = JobScheduler::Get()->SubmitJob(jobFuncB);
    jobD = JobScheduler::Get()->SubmitJob(jobFuncD);
    jobC = JobScheduler::Get()->SubmitJob(jobFuncC, EJobPriority::Normal, { jobA, jobB, jobD });

    while (JobScheduler::Get()->IsBusy()) {}

    printf("(A, B)->(C, D)\n");

    jobA = JobScheduler::Get()->SubmitJob(jobFuncA);
    jobB = JobScheduler::Get()->SubmitJob(jobFuncB);
    jobC = JobScheduler::Get()->SubmitJob(jobFuncC, EJobPriority::High, { jobA, jobB });
    jobD = JobScheduler::Get()->SubmitJob(jobFuncD, EJobPriority::Low, { jobA, jobB });

    while (JobScheduler::Get()->IsBusy()) {}
}