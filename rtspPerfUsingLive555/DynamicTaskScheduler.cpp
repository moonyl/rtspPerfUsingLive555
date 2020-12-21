#include "DynamicTaskScheduler.h"
#include <iostream>

DynamicTaskScheduler*
DynamicTaskScheduler::createNew(TaskCommandQueue& taskQueue, unsigned int maxSchedulerGranularity) {
    return new DynamicTaskScheduler(taskQueue, maxSchedulerGranularity);
}

DynamicTaskScheduler::DynamicTaskScheduler(TaskCommandQueue& taskQueue, unsigned int maxSchedulerGranularity) :
    BasicTaskScheduler(maxSchedulerGranularity), _taskQueue(taskQueue)
{}


void DynamicTaskScheduler::doEventLoop(volatile char* watchVariable) {

    while (1) {
        if (watchVariable != nullptr && *watchVariable != 0) break;
        SingleStep(0);
        TaskCommand command;
        while (_taskQueue.pop(command)) {
            std::cerr << "type: " << command.type << std::endl;
            if (command.type == TaskCommand::Connect) {
                //_onConnectSignal({ command.streamId, command.url, command.user, command.password });
                _onConnectSignal({ command.url, command.user, command.password });
            }
            else if (command.type == TaskCommand::RemoveOne)    {
                _onRemoveOneSignal();
            }
            else if (command.type == TaskCommand::Disconnect) {
                // TODO : how to disconnect
                //_onDisconnectSignal(command.streamId);
            }
            else if (command.type == TaskCommand::Quit) {
                std::cerr << "Thread finished" << std::endl;
                return;
            }
        
        }
    }

}


void DynamicTaskScheduler::addConnectListener(std::function<void(RtspConnectInfo)> handler) {
    _onConnectSignal.connect(handler);
}

void DynamicTaskScheduler::addDisconnectListener(std::function<void(std::string)> handler) {
    _onDisconnectSignal.connect(handler);
}

void DynamicTaskScheduler::addRemoveOneListener(std::function<void()> handler)  {
    _onRemoveOneSignal.connect(handler);
}