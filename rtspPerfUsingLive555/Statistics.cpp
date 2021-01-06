//
// Created by moony on 21. 1. 4..
//
#include "Statistics.h"
#include <chrono>
#include <nlohmann/json.hpp>
#include <iostream>
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include "PerfCheckRtspClient.h"

double Statistics::InputFps = 30.0;
void Statistics::calculateStatistics(void *clientData)
{
    UsageEnvironment *env = static_cast<UsageEnvironment *>(clientData);

    static std::chrono::system_clock::time_point intervalStart = std::chrono::system_clock::now();
    std::chrono::duration<double> sec = std::chrono::system_clock::now() - intervalStart;
    int const interval = 3;
    if (sec.count() > interval) {
        int totalFrame = 0;

        auto &table = MediaLookupTable::ourMedia(*env)->getTable();
        HashTable::Iterator *iter = HashTable::Iterator::create(table);
        char const *key;
        void *value;

        int clientCount = 0;
        while ((value = iter->next(key)) != nullptr) {
            Medium *medium;
            auto found = Medium::lookupByName(*env, key, medium);
            if (found) {
                if (medium->isRTSPClient()) {
                    clientCount++;

                    auto *sink = (static_cast<PerfCheckRtspClient *>(medium))->perfSink();
                    totalFrame += sink->frameCount;
                    auto const frPerCh = sink->frameCount / sec.count();

                    if (frPerCh < 10)   {
                        try {
                            using json = nlohmann::json;
                            json j;
                            j["result"] = "abnormal";
                            json state;
                            state["name"] = medium->name();
                            state["frameRatePerCh"] = round(frPerCh * 10) / 10;
                            state["elapsed"] = sink->elapsed().count();

                            j["state"] = state;

                            std::cout << j.dump() << std::endl;
                        } catch(std::exception &e)  {
                            *env << "exception, " << __func__ << ", " << __LINE__ << "\n";
                            *env << e.what() << "\n";
                        }
                    }

                    sink->frameCount = 0;
                }
            }
        }

        auto const frameRate = totalFrame / sec.count();

        try {
            using json = nlohmann::json;
            json j;
            j["result"] = "stats";

            json state;
            state["counted"] = clientCount;
            state["measuredFrameRate"] = round(frameRate * 10) / 10;
            state["expected"] = round(PerfCheckRtspClient::rtspClientCount * InputFps * 10) / 10;
            j["state"] = state;

            std::cout << j.dump() << std::endl;
        } catch(std::exception& e)  {
            *env << "exception, " << __func__ << ", " << __LINE__ << "\n";
            *env << e.what() << "\n";
        }


        intervalStart = std::chrono::system_clock::now();
    }

    int64_t uSecsToDelay = 3 + 1000 * 1000;
    env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *) calculateStatistics, env);
}