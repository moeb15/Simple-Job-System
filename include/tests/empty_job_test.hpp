#pragma once

#include "tests/test.hpp"

class EmptyJobTest : public ITest
{
public:
    virtual void Initialize() override;
    virtual void Run() override;
};