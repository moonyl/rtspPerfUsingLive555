//
// Created by moony on 21. 1. 6..
//

#include "LocalCommandInterpreter.h"
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LocalCommandInterpreter::LocalCommandInterpreter(LocalCommandInterpreter::TaskCommandQueue &q,
                                                 PerformanceEnvironment &perfEnv) : q(q), perfEnv(perfEnv) {}


template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

#include <chrono>
#include <thread>
bool LocalCommandInterpreter::handleRequest(const std::string &request) {
    auto const &seperated = split(request, ' ');
    if (seperated.empty())  {
        return true;
    }
    auto const &cmd = seperated[0];
    std::cout << "cmd: " << cmd << std::endl;
    if (cmd == "add")   {
        if (seperated.size() > 2)   {
            try {
                auto const start = std::stoi(seperated[1]);
                auto const length = std::stoi(seperated[2]);

                auto const repeat = seperated.size() > 3 ? std::stoi(seperated[3]) : 1;

                std::cout << "start: " << start << ", length: " << length << std::endl;
                for (auto rep = 0; rep < repeat; rep++) {
                    for (auto i = start; i < start + length; i++)  {
//                        auto url = string_format("rtsp://%s:%d/proxyStream-%d", _host.c_str(), _port, i);
                        auto url = string_format("rtsp://%s:%d/192.168.15.109:554/onvif/profile2/media.smp", _host.c_str(), _port);
                        std::cout << "url: " << url << std::endl;
                        q.push({TaskCommand::Connect, url, "", ""});
                        std::chrono::milliseconds dura(500);
                        std::this_thread::sleep_for( dura );
                   }
                }
            } catch(std::exception &e)  {
                std::cout << "exception: " << e.what() << std::endl;
            }
        } else {
            std::cout << "wrong parameter" << std::endl;
        }
    } else if (cmd == "stats")  {
        std::vector<std::string> result;
        {
            std::lock_guard<std::mutex> lockGuard(perfEnv.statsMutex);
            std::copy(perfEnv.statsRecords.begin(), perfEnv.statsRecords.end(), std::back_inserter(result));
        }

        double totalExpected = 0;
        double totalMeasured = 0;
        int index = 0;
        for (auto iter = result.crbegin(); iter != result.crend(); ++iter)    {
            std::cout << *iter << std::endl;
            if (index < 10) {
                auto j = json::parse(*iter);
                totalMeasured += j["state"]["measuredFrameRate"].get<double>();
                totalExpected += j["state"]["expected"].get<double>();
            }
            index++;
        }
        std::cout << "last 10, measured: " << totalMeasured << ", expected: " << totalExpected
            << ", performance: " << totalMeasured * 100 / totalExpected << std::endl;

    } else if (cmd == "abnormal") {
        std::vector<std::string> result;
        {
            std::lock_guard<std::mutex> lockGuard(perfEnv.abnRecMutex);
            std::copy(perfEnv.abnormalRecords.begin(), perfEnv.abnormalRecords.end(), std::back_inserter(result));
        }

        for (auto const &record: result)  {
            std::cout << record << std::endl;
        }
    } else if (cmd == "list")   {
        perfEnv.env->taskScheduler().triggerEvent(perfEnv.listingTaskId, perfEnv.env);
    } else if (cmd == "remove") {
        if (seperated.size() > 1) {
            for (auto iter = seperated.cbegin()+1; iter != seperated.cend(); iter++)    {
                q.push({TaskCommand::Disconnect, *iter});
            }
        }
    } else if (cmd == "port") {
        if (seperated.size() > 1) {
            _port = std::stoi(seperated[1]);
        }
    } else if (cmd == "host")   {
        if (seperated.size() > 1) {
            _host = seperated[1];
        }
    } else if (cmd == "rmall") {
        perfEnv.env->taskScheduler().triggerEvent(perfEnv.removeAllId, perfEnv.env);
    }

    std::cout << std::endl;
    return true;
}
