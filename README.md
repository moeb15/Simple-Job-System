# Job-System

A lock-free work-stealing job scheduler built from scratch in C++. Uses per-thread Chase-Lev deques, a global overflow queue, and a dependency graph for job ordering.

## Why I built this

I wanted to gain a deeper understanding of memory ordering, lock-free data structures, work stealing, and related problems like false sharing. This project is mostly a learning exercise, it builds upon the Job-System I use currently in my own engine.

## Architecture

- **Chase-Lev Deque:** Each worker thread owns a lock-free circular double ended queue. Threads push/pop from the bottom of the queue, while thieves CAS steal from the top.
- **Work Stealing:** Idle threads randomly select a victim thread and attempt to steal from the top of its deque. The index of the thread is selected randomly using a stateless hash function.
- **Global Queue:** The scheduler maintains a global queue, jobs submitted from the main thread are pushed into the global queue where they can be stolen from by worker threads, in the case where the global queue is full the main thread will execute jobs.
- **Dependency Graph:** Jobs can declare prerequisites, the scheduler tracks completion and automatically queues jobs once all dependencies are resolved.

## Design Decisions and Rationale

| Decision | Rationale |
|----------|-----------|
|Fixed-size global queue|The size of the deque is determined at compile time to avoid dynamic allocations, in the case where the global queue is full attempting to submit new jobs will force the main thread to execute jobs until the newly added job can be added.|
|Custom 'TFunction' instead of std::function for job callbacks|I opted for the TFunction because currently this is what I use in my own engine and it works fine, it's lightweight (16 bytes on 64 bit systems) and uses non-type template parameters to store function pointers and pointers to member functions, more details on its implemenation can be found in Game Engine Gems Volume 3 Chapter 13|
|Contiguous job storage block (65536 jobs)|The contiguous job block is thread_local, meaning each thread has its own copy, each thread also maintains the current index into the block, the rationale again is to avoid having to dynamically allocate a Job object each time, another option is to use a memory pool which would avoid the problem of overwriting a job, however for this project this functions just fine, in my engine Jobs are allocated using the custom dynamic alloctor (TLSF) used for all dynamic allocations so it avoids the issue of overwritten jobs.|
|Chase-Lev Deque over mutex based queues|Lock-free design avoids contention. The only atomic operations are on the deque's top/bottom indices.|


## Example

```cpp
namespace
{
    class Foo
    {
    public:
        void Bar(Job* job = nullptr, void* data = nullptr)
        {
            printf("FooBar1\n");
        }
    };

    void FooBar(Job* job = nullptr, void* data = nullptr)
    {
        printf("FooBar0\n");
    }

    Foo testObj;
    // using JobFunction = TFunction<void(Job*, void*)>;
    JobFunction globalFunc;
    JobFunction classFunc;
}

void DoWork()
{
    globalFunc.Bind<&FooBar>();
    classFunc.Bind<Foo, &Foo::Bar>(&testObj);

    Job* jobA = JobScheduler::Get()->SubmitJob(globalFunc);

    // jobB depends on jobA, it cannot begin executing until jobA completes
    Job* jobB = JobScheduler::Get()->SubmitJob(classFunc, { jobA });

    // the following would be the expected output assuming no other jobs are present
    // FooBar0
    // FooBar1

    // if other jobs are present, what can be guaranteed is that jobA will execute before jobB
}
```