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

    OnDisconnectRequested onDisconnectRequested{env};
    scheduler->addDisconnectListener(onDisconnectRequested);

    // Now, enter the event loop:
    std::thread rtspTaskThread([env]() {
        int64_t uSecsToDelay = 3 + 1000 * 1000;
        env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *) Statistics::calculateStatistics, env);
        env->taskScheduler().doEventLoop(); // does not return
    });

    std::string request;
    do {
        std::getline(std::cin, request);
        try {
            //std::cout << request;
            //auto requestCompare = "{\"url\":\"rtsp://192.168.15.105:554/onvif/profile3/media.smp\", \"user\":\"admin\", \"password\":\"q1w2e3r4@\"}";
            //std::cout << request.compare(requestCompare) << std::endl;
            auto jsonObj = json::parse(request);
            if (jsonObj["cmd"].is_string()) {
                auto const cmd = jsonObj["cmd"].get<std::string>();
                if (cmd == "quit") {
                    break;
                }
                if (cmd == "removeOne")   {
                    q.push({TaskCommand::RemoveOne});
                    continue;
                }
                if (cmd == "add") {
                    if (jsonObj["param"].is_object())   {
                        const auto param = jsonObj["param"].get<json>();
                        if (!param.contains("url"))  {
                            continue;
                        }
                        auto const url = param["url"].get<std::string>();
                        if (!url.empty()) {
                            auto const user = param.contains("user") ? param["user"].get<std::string>() : "" ;
                            auto const password = param.contains("password") ? param["password"].get<std::string>() : "";
                            q.push({TaskCommand::Connect, url, user, password});
                        }
                    }
                    continue;
                }
                if (cmd == "disconnect")    {
                    if (jsonObj["param"].is_string()) {
                        auto const nameToDisconnect = jsonObj["param"].get<std::string>();
                        q.push({TaskCommand::Disconnect, nameToDisconnect});
                    }
                    continue;
                }
            }
//
//            if (jsonObj["disconnect"].is_string()) {
//                auto const nameToDisconnect = jsonObj["disconnect"].get<std::string>();
//                q.push({TaskCommand::Disconnect, nameToDisconnect});
//                continue;
//            }


//            auto const url = jsonObj["url"].get<std::string>();
//            if (!url.empty()) {
//                auto const user = jsonObj["user"].is_null() ? "" : jsonObj["user"].get<std::string>();
//                auto const password = jsonObj["password"].is_null() ? "" : jsonObj["password"].get<std::string>();
//                q.push({TaskCommand::Connect, url, user, password});
//            }

        } catch (jsonException &e) {
            std::cerr << "exception, " << __func__ << ", " << __LINE__ << "\n";
            std::cerr << "request: " << request << "\n";
            std::cerr << e.what() << "\n";
        }
    } while (true);

    q.push({TaskCommand::Quit});

    rtspTaskThread.join();
    std::cout << "perf tester finished";

    return 0;
}


