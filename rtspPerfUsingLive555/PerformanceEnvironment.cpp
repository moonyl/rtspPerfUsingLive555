//
// Created by moony on 21. 1. 6..
//

#include <deque>
#include "LocalCommandInterpreter.h"
#include "RemoteCommandInterpreter.h"
#include "Statistics.h"
#include "RawStreamPerfCheckSink.h"
#include "OnDisconnectRequested.h"
#include "OnRemoveOneRequested.h"
#include "OnConnectRequested.h"
#include "PerfCheckRtspClient.h"
#include <nlohmann/json.hpp>
#include "DynamicTaskScheduler.h"
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include <chrono>
#include <thread>
#include <iostream>
#include "PerformanceEnvironment.h"

PerformanceEnvironment::PerformanceEnvironment(TaskScheduler &taskScheduler) : env(BasicUsageEnvironment::createNew(taskScheduler)) {}
