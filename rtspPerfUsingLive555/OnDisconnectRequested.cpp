//
// Created by admin on 2020-12-15.
//

#include "OnDisconnectRequested.h"
#include <liveMedia.hh>
#include "PerfCheckRtspClient.h"
#include <nlohmann/json.hpp>
#include <iostream>

OnDisconnectRequested::OnDisconnectRequested(UsageEnvironment *env): env(env)
{

}

void OnDisconnectRequested::operator()(std::string nameToDisconnect)
{
    *env << "nameToDisconnect: " << nameToDisconnect.c_str() << "\n\n";

    auto & table = MediaLookupTable::ourMedia(*env)->getTable();
    auto * medium = static_cast<Medium*>(table.Lookup(nameToDisconnect.c_str()));
    if (medium) {
        if (medium->isRTSPClient()) {
            RTSPClient *client = static_cast<RTSPClient *>(medium);
            *env << "shutdown stream\n";
            shutdownStream(client, ExitCode::ByUser);
        }
    }
}
