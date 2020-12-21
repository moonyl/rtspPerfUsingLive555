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
#include "RawStreamPerfCheckSink.h"

using namespace std;
using json = nlohmann::json;
using jsonException = nlohmann::detail::exception;

struct RtspConnectInfo
{
    std::string streamId;
    std::string url;
    std::string user;
    std::string password;
};

static void calculateStatics(void* clientData);

int main()
{
    // Increase the maximum size of video frames that we can 'proxy' without truncation.
    // (Such frames are unreasonably large; the back-end servers should really not be sending frames this large!)
    //OutPacketBuffer::maxSize = 700000; // bytes

    using TaskCommandQueue = boost::lockfree::spsc_queue<TaskCommand>;
    TaskCommandQueue q{100};
    TaskCommandQueue &_taskQueue = q;
    //std::cout << _taskQueue.empty();
    DynamicTaskScheduler *scheduler = DynamicTaskScheduler::createNew(q);

    auto *env = BasicUsageEnvironment::createNew(*scheduler);

    *env << "LIVE555 Proxy Server\n"
         << "\t(LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING
         << "; licensed under the GNU LGPL)\n\n";

    OnConnectRequested onConnectRequested{env};
    scheduler->addConnectListener(onConnectRequested);

    OnRemoveOneRequested onRemoveOneRequested{env};
    scheduler->addRemoveOneListener(onRemoveOneRequested);

//    OnDisconnectRequested onConnectRequested{env};
//    scheduler->addDisconnectListener(onStreamRemoved);

    // Now, enter the event loop:
    std::thread rtspTaskThread([env]() {
        int64_t uSecsToDelay = 3+1000*1000;
        env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)calculateStatics, env);
        env->taskScheduler().doEventLoop(); // does not return
    });

    std::string request;
    do {
        std::getline(std::cin, request);
        if (request.compare("quit") == 0) {
            break;
        }
        if (request.compare("removeOne") == 0)  {
            q.push({TaskCommand::RemoveOne});
            continue;
        }
        try {
            //std::cout << request;
            //auto requestCompare = "{\"url\":\"rtsp://192.168.15.105:554/onvif/profile3/media.smp\", \"user\":\"admin\", \"password\":\"q1w2e3r4@\"}";
            //std::cout << request.compare(requestCompare) << std::endl;
            auto jsonObj = json::parse(request);
            if (jsonObj["url"].is_null())   {
                continue;
            }
            auto const url = jsonObj["url"].get<std::string>();
            if (!url.empty()) {
                auto const user = jsonObj["user"].is_null() ? "" : jsonObj["user"].get<std::string>();
                auto const password = jsonObj["password"].is_null() ? "" : jsonObj["password"].get<std::string>();
                q.push({TaskCommand::Connect, url, user, password});
            }

        } catch (jsonException &e) {
            std::cerr << e.what();
        }
    } while (true);

    q.push({TaskCommand::Quit});

    rtspTaskThread.join();

    return 0;
}

#include <chrono>
#include <nlohmann/json.hpp>
#include <iostream>
void calculateStatics(void* clientData) {
    UsageEnvironment* env = static_cast<UsageEnvironment*>(clientData);

    static std::chrono::system_clock::time_point intervalStart = std::chrono::system_clock::now();
    std::chrono::duration<double> sec = std::chrono::system_clock::now() - intervalStart;
    int const interval = 3;
    if (sec.count() > interval) {
        int totalFrame = 0;

        auto & table = MediaLookupTable::ourMedia(*env)->getTable();
        HashTable::Iterator* iter = HashTable::Iterator::create(table);
        char const* key;
        void* value;
        do {
            value = iter->next(key);
            if (value != nullptr)    {
                Medium* medium;
                auto found = Medium::lookupByName(*env, key, medium);
                if (found)  {
                    if (medium->isSink()) {
                        RawStreamPerfCheckSink* sink = static_cast<RawStreamPerfCheckSink*>(medium);
                        totalFrame += sink->frameCount;
                        auto const frPerCh = sink->frameCount / sec.count();
                        if (frPerCh < 10)  {
                            using json = nlohmann::json;
                            json j;
                            j["name"] = sink->name();
                            j["frameRatePerCh"] = round(frPerCh*10)/10;

                            std::cout << j.dump() << std::endl;
                            //std::cout << "name: " << sink->name() << ", " << sink->frameCount / sec.count() << std::endl;
                        }
                        sink->frameCount = 0;
                    }
                }
            }
        } while(value != nullptr);


        auto const frameRate = totalFrame / sec.count();
        using json = nlohmann::json;
        json j;
        //j["name"] = source()->name();
        j["connect"] = PerfCheckRtspClient::rtspClientCount;
        j["measuredFrameRate"] = round(frameRate*10)/10;
        j["expected"] = round(PerfCheckRtspClient::rtspClientCount * 23.9 * 10)/10;
        std::cout << j.dump() << std::endl;

        //RawStreamPerfCheckSink::frameCount = 0;
        intervalStart = std::chrono::system_clock::now();
    }


    int64_t uSecsToDelay = 3+1000*1000;

    env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)calculateStatics, env);
}