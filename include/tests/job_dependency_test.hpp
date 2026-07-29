#pragma once

#include "tests/test.hpp"

class JobDependencyTest : public ITest
{
public:
    virtual void Initialize() override;
    virtual void Run() override;
};