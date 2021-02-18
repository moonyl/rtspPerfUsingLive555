//
// Created by moony on 21. 1. 6..
//

#pragma once

#include <boost/lockfree/spsc_queue.hpp>

class RemoteCommandInterpreter
{
    using TaskCommandQueue = boost::lockfree::spsc_queue<TaskCommand>;
    TaskCommandQueue &q;

public:
    RemoteCommandInterpreter(TaskCommandQueue &q);
    bool handleRequest(const std::string& request);
};
