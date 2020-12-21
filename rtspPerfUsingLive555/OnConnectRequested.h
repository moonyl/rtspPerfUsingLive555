//
// Created by admin on 2020-12-15.
//

#pragma once
#include <UsageEnvironment.hh>
#include "DynamicTaskScheduler.h"

class OnConnectRequested
{
    UsageEnvironment *env;

public:
    OnConnectRequested(UsageEnvironment *env);

    void operator()(DynamicTaskScheduler::RtspConnectInfo connect);
};



