#include "tests/job_nesting_test.hpp"
#include "timer.hpp"
#include <iostream>

namespace
{
    void JobLeaf(Job* j, void* d)
    {
        u32 value = *(u32*)d;
        printf("JobLeaf %d\n", value);
    }

    void JobNode(Job* j, void* d)
    {
        printf("JobNode\n");
        JobFunction jobFuncLeaf;
        jobFuncLeaf.Bind<&JobLeaf>();

        u32 test = 10;
        JobScheduler::Get()->SubmitJob(jobFuncLeaf, EJobPriority::Normal, {}, &test, sizeof(test));
    }

    void JobRoot(Job* j, void* d)
    {
        printf("JobRoot\n");

        JobFunction jobFuncNode;
        jobFuncNode.Bind<&JobNode>();

        JobScheduler::Get()->SubmitJob(jobFuncNode, EJobPriority::Normal);
    }

    JobFunction jobFuncRoot;
}

void JobNestingTest::Initialize()
{
    jobFuncRoot.Bind<JobRoot>();
}

void JobNestingTest::Run()
{
    printf("Beginning job nesting test\n");

    JobScheduler::Get()->SubmitJob(jobFuncRoot);

    while (JobScheduler::Get()->IsBusy()) {}
}