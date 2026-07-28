#include "timer.hpp"

#if defined(JS_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include <time.h>
#endif

namespace
{
    i64 int64_mul_div(i64 value, i64 numer, i64 denom)
    {
        const i64 q = value / denom;
        const i64 r = value % denom;
        return q * numer + r * numer / denom;
    }

    Timer* s_Timer{ nullptr };
}

#if defined(JS_PLATFORM_WINDOWS)
static LARGE_INTEGER s_Frequency;
#endif

Timer* Timer::Get()
{
    if (!s_Timer) s_Timer = new Timer();
    return s_Timer;
}

bool Timer::Initialize()
{
#if defined(JS_PLATFORM_WINDOWS)
    QueryPerformanceFrequency(&s_Frequency);
#endif

    return true;
}

void Timer::Shutdown()
{
    delete s_Timer;
    s_Timer = nullptr;
}

i64 Timer::Now()
{
#if defined(JS_PLATFORM_WINDOWS)
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    // convert to microseconds
    // const i64 microseconds_per_second = 1000000LL
    const i64 microseconds = int64_mul_div(time.QuadPart, 1000000LL, s_Frequency.QuadPart);
#else
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);

    const u64 now = tp.tv_sec * 1000000000 + tp.tv_nsec;
    const i64 microseconds = now / 1000;
#endif

    return microseconds;
}

f64 Timer::ToMicroseconds(i64 time)
{
    return (f64)time;
}

f64 Timer::ToMiliseconds(i64 time)
{
    return (f64)time / 1000.0;
}
f64 Timer::ToSeconds(i64 time)
{
    return (f64)time / 1000000.0;
}

i64 Timer::From(i64 startingTime)
{
    return Now() - startingTime;
}

i64 Timer::FromMicroseconds(i64 startingTime)
{
    return ToMicroseconds(From(startingTime));
}

i64 Timer::FromMiliseconds(i64 startingTime)
{
    return ToMiliseconds(From(startingTime));
}

i64 Timer::FromSeconds(i64 startingTime)
{
    return ToSeconds(From(startingTime));
}

f64 Timer::DeltaSeconds(i64 startingTime, i64 endingTime)
{
    return ToSeconds(endingTime - startingTime);
}

f64 Timer::DeltaMiliseconds(i64 startingTime, i64 endingTime)
{
    return ToMiliseconds(endingTime - startingTime);
}