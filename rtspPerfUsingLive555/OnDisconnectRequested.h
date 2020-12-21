//
// Created by admin on 2020-12-15.
//

#pragma once
#include <UsageEnvironment.hh>
#include <string>

class OnDisconnectRequested
{
    UsageEnvironment *env;

public:
    OnDisconnectRequested(UsageEnvironment *env);

    void operator()(std::string streamId);
};



