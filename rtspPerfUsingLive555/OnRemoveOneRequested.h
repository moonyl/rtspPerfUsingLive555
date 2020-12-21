//
// Created by admin on 2020-12-18.
//

#pragma once

#include <UsageEnvironment.hh>

class OnRemoveOneRequested
{
    UsageEnvironment *env;

public:
    OnRemoveOneRequested(UsageEnvironment *env);

    void operator()();
};



