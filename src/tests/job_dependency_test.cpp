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
    Job* jobB = JobScheduler::Get()->SubmitJob(jobFuncB, { jobA });
    Job* jobC = JobScheduler::Get()->SubmitJob(jobFuncC, { jobB });
    Job* jobD = JobScheduler::Get()->SubmitJob(jobFuncD, { jobC });
    
    JobScheduler::Get()->Wait();

    printf("(A, B, D)->C\n");

    jobA = JobScheduler::Get()->SubmitJob(jobFuncA);
    jobB = JobScheduler::Get()->SubmitJob(jobFuncB);
    jobD = JobScheduler::Get()->SubmitJob(jobFuncD);
    jobC = JobScheduler::Get()->SubmitJob(jobFuncC, { jobA, jobB, jobD });

    JobScheduler::Get()->Wait();

    printf("(A, B)->(C, D)\n");

    jobA = JobScheduler::Get()->SubmitJob(jobFuncA);
    jobB = JobScheduler::Get()->SubmitJob(jobFuncB);
    jobC = JobScheduler::Get()->SubmitJob(jobFuncC, { jobA, jobB });
    jobD = JobScheduler::Get()->SubmitJob(jobFuncD, { jobA, jobB });

    JobScheduler::Get()->Wait();
}