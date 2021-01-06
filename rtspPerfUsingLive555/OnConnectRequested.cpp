//
// Created by admin on 2020-12-15.
//

#include "OnConnectRequested.h"
#include "PerfCheckRtspClient.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <liveMedia.hh>

OnConnectRequested::OnConnectRequested(UsageEnvironment *env): env(env)
{}

void OnConnectRequested::operator()(DynamicTaskScheduler::RtspConnectInfo connect)
{
    *env << "url: " << connect.url.c_str()
         << ", user: " << connect.user.c_str()
         << ", password: " << connect.password.c_str() << "\n\n";

    // TODO : 생성을 수정할 것.
    auto* client = openURL(*env, "rtspPerfUsingLive555", connect.url.c_str(),
                                     connect.user.c_str(), connect.password.c_str());

}
