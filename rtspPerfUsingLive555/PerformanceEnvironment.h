//
// Created by moony on 21. 1. 6..
//

#pragma once
#include <deque>
#include <string>
#include <mutex>
#include <BasicUsageEnvironment.hh>

struct PerformanceEnvironment
{
    PerformanceEnvironment(TaskScheduler& taskScheduler);
    BasicUsageEnvironment* env;
    std::mutex statsMutex;
    std::deque<std::string> statsRecords;
    std::mutex abnRecMutex;
    std::deque<std::string> abnormalRecords;

    EventTriggerId listingTaskId;
    EventTriggerId removeAllId;
};