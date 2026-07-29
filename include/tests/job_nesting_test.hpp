#pragma once

#include "tests/test.hpp"

class JobNestingTest : public ITest
{
public:
    virtual void Initialize() override;
    virtual void Run() override;
};