#include "tests/job_nesting_test.hpp"
#include "timer.hpp"
#include <iostream>

namespace
{
    void JobLeaf(Job* j, void* d)
    {
        printf("JobLeaf\n");
    }

    void JobNode(Job* j, void* d)
    {
        printf("JobNode\n");
        JobFunction jobFuncLeaf;
        jobFuncLeaf.Bind<&JobLeaf>();

        JobScheduler::Get()->SubmitJob(jobFuncLeaf);
    }

    void JobRoot(Job* j, void* d)
    {
        printf("JobRoot\n");

        JobFunction jobFuncNode;
        jobFuncNode.Bind<&JobNode>();

        JobScheduler::Get()->SubmitJob(jobFuncNode);
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