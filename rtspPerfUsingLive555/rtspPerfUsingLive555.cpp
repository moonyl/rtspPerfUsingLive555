// rtspPerfUsingLive555.cpp : 애플리케이션의 진입점을 정의합니다.
//

#include <iostream>
#include <thread>
#include <chrono>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include "DynamicTaskScheduler.h"
#include <nlohmann/json.hpp>
#include "PerfCheckRtspClient.h"
#include "OnConnectRequested.h"
#include "OnRemoveOneRequested.h"
#include "OnDisconnectRequested.h"
#include "RawStreamPerfCheckSink.h"
#include "Statistics.h"
#include "RemoteCommandInterpreter.h"
#include "LocalCommandInterpreter.h"
#include "PerformanceEnvironment.h"

using namespace std;

using jsonException = nlohmann::detail::exception;

struct RtspConnectInfo
{
    std::string streamId;
    std::string url;
    std::string user;
    std::string password;
};

#include <deque>

void listClients(void *clientData) {
    //std::cout << __func__ << std::endl;
    UsageEnvironment *env = static_cast<UsageEnvironment*>(clientData);

    auto &table = MediaLookupTable::ourMedia(*env)->getTable();
    HashTable::Iterator *iter = HashTable::Iterator::create(table);
    char const *key;
    void *value;

    int clientCount = 0;
    std::map<std::string, std::list<std::string>> urlBasedNames;
    std::cout << "\n============== client list ===================\n";
    while ((value = iter->next(key)) != nullptr) {
        Medium *medium;
        auto found = Medium::lookupByName(*env, key, medium);
        if (found) {
            if (medium->isRTSPClient()) {
                PerfCheckRtspClient* client = static_cast<PerfCheckRtspClient*>(medium);
                std::cout << "name: " << client->name() << ", url: " << client->url()
                    << ", frameRate: " << client->perfSink()->frameRate() << std::endl;
                if (urlBasedNames.count(std::string(client->url()))) {
                    urlBasedNames.at(std::string(client->url())).push_back(client->name());
                } else {
                    urlBasedNames[std::string(client->url())] = std::list<std::string>{client->name()};
                }
            }
        }
    }
    std::cout << "\n============== url base ===================\n";
    for (auto const& item : urlBasedNames)   {
        std::cout << "url: " << item.first << ", count: " << item.second.size() << ", [ ";
        for (auto const &value: item.second)    {
            std::cout << value << ", ";
        }
        std::cout << "]" << std::endl;
    }
}

void removeAllClients(void *clientData) {
    UsageEnvironment *env = static_cast<UsageEnvironment*>(clientData);

    auto &table = MediaLookupTable::ourMedia(*env)->getTable();
    HashTable::Iterator *iter = HashTable::Iterator::create(table);
    char const *key;
    void *value;

    int clientCount = 0;
    std::map<std::string, std::list<std::string>> urlBasedNames;
    std::cout << "\n============== client list ===================\n";
    while ((value = iter->next(key)) != nullptr) {
        Medium *medium;
        auto found = Medium::lookupByName(*env, key, medium);
        if (found) {
            if (medium->isRTSPClient()) {
                RTSPClient* client = static_cast<RTSPClient*>(medium);
                shutdownStream(client);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    //std::cout << "check: " << RTSPClient::responseBufferSize << std::endl;
    RTSPClient::responseBufferSize = 40000;
    if (argc > 1)   {
        std::string fps{argv[1]};
        Statistics::InputFps = std::stod(fps);
        //std::cout << InputFps;
    }
    // Increase the maximum size of video frames that we can 'proxy' without truncation.
    // (Such frames are unreasonably large; the back-end servers should really not be sending frames this large!)
    //OutPacketBuffer::maxSize = 700000; // bytes

    using TaskCommandQueue = boost::lockfree::spsc_queue<TaskCommand>;
    TaskCommandQueue q{100};

    DynamicTaskScheduler *scheduler = DynamicTaskScheduler::createNew(q);

    PerformanceEnvironment perfEnv{*scheduler};
    //auto *env = BasicUsageEnvironment::createNew(*scheduler);
    auto *env = perfEnv.env;

    *env << "LIVE555 Proxy Server\n"
         << "\t(LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING
         << "; licensed under the GNU LGPL)\n\n";

    OnConnectRequested onConnectRequested{env};
    scheduler->addConnectListener(onConnectRequested);

    OnRemoveOneRequested onRemoveOneRequested{env};
    scheduler->addRemoveOneListener(onRemoveOneRequested);

    OnDisconnectRequested onDisconnectRequested{env};
    scheduler->addDisconnectListener(onDisconnectRequested);

    // Now, enter the event loop:
    //std::thread rtspTaskThread([env]() {
    std::thread rtspTaskThread([&perfEnv]() {
        int64_t uSecsToDelay = 3 + 1000 * 1000;
        auto *env = perfEnv.env;
        //env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *) Statistics::calculateStatistics, env);
        env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *) Statistics::calculateStatistics, &perfEnv);
        perfEnv.listingTaskId = env->taskScheduler().createEventTrigger((TaskFunc*) listClients);
        perfEnv.removeAllId = env->taskScheduler().createEventTrigger((TaskFunc*) removeAllClients);
        env->taskScheduler().doEventLoop(); // does not return
    });

    RemoteCommandInterpreter remoteInterpreter{q};
    LocalCommandInterpreter localInterpreter{q, perfEnv};
    std::string request;
    do {
        std::cout << "rtspPref > ";
        std::getline(std::cin, request);
        if (localInterpreter.handleRequest(request) == false) break;
        //if (remoteInterpreter.handleRequest(request) == false) break;
    } while (true);

    q.push({TaskCommand::Quit});

    rtspTaskThread.join();
    std::cout << "perf tester finished";

    return 0;
}


