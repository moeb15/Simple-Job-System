#include "job_system.hpp"
#include "timer.hpp"
#include "tests/test.hpp"
#include <iostream>

#define NUM_TESTS 100
#define EMPTY_JOB_TEST 1
#define JOB_DEPENDENCY_TEST 1

#if EMPTY_JOB_TEST
#include "tests/empty_job_test.hpp"
#elif JOB_DEPENDENCY_TEST
#include "tests/job_dependency_test.hpp"
#endif

ITest* testPtr{ nullptr };

int main()
{
    Timer::Get()->Initialize();
    JobScheduler::Get()->Initialize();

#if EMPTY_JOB_TEST
    testPtr = new EmptyJobTest();
    testPtr->Initialize();
#elif JOB_DEPENDENCY_TEST
    testPtr = new JobDependencyTest();
    testPtr->Initialize();
#endif

    if(testPtr)
    {
        for(u32 i = 0; i < NUM_TESTS; i++)
        {
            printf("======================\n");
            printf("Test %d\n\n", i);
            testPtr->Run();
            printf("======================\n");

        }
        delete testPtr;
        testPtr = nullptr;
    }

    std::cin.get();

    JobScheduler::Get()->Shutdown();
    Timer::Get()->Shutdown();

    std::cin.get();

    return 0;
}