#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/signals2.hpp>
#include <BasicUsageEnvironment.hh>
#include <string>
#include "TaskCommand.h"

class DynamicTaskScheduler : public BasicTaskScheduler {
public:
    struct RtspConnectInfo {
        //std::string streamId;
        std::string url;
        std::string user;
        std::string password;
    };
private:
    using TaskCommandQueue = boost::lockfree::spsc_queue<TaskCommand>;
    TaskCommandQueue& _taskQueue;
    boost::signals2::signal<void(RtspConnectInfo)> _onConnectSignal;
    boost::signals2::signal<void(std::string)> _onDisconnectSignal;
    boost::signals2::signal<void()> _onRemoveOneSignal;

public:
    static DynamicTaskScheduler*
        createNew(TaskCommandQueue& taskQueue, unsigned maxSchedulerGranularity = 10000/*microseconds*/);

    virtual ~DynamicTaskScheduler() {}

    virtual void doEventLoop(char volatile* watchVariable) override;

    void addConnectListener(std::function<void(RtspConnectInfo)> handler);
    
    void addDisconnectListener(std::function<void(std::string)> handler);

    void addRemoveOneListener(std::function<void()> handler);

protected:
    DynamicTaskScheduler(TaskCommandQueue& taskQueue, unsigned maxSchedulerGranularity);

};