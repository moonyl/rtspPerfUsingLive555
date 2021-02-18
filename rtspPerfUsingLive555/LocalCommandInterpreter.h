//
// Created by moony on 21. 1. 6..
//

#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include "TaskCommand.h"
#include "PerformanceEnvironment.h"
#include <string>

class LocalCommandInterpreter {
    using TaskCommandQueue = boost::lockfree::spsc_queue<TaskCommand>;
    TaskCommandQueue &q;
    PerformanceEnvironment &perfEnv;
    int _port = 7554; // 10554
    std::string _host = "172.27.58.251";

public:
    LocalCommandInterpreter(TaskCommandQueue &q, PerformanceEnvironment &perfEnv);

    bool handleRequest(const std::string& request);
};



