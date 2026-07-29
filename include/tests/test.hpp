#pragma once

#include "job_system.hpp"

class ITest
{
public:
    virtual ~ITest() {}
    virtual void Initialize() {}
    virtual void Run() {}
};