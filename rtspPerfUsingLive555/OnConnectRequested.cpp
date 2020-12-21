//
// Created by admin on 2020-12-15.
//

#include "OnConnectRequested.h"
#include "PerfCheckRtspClient.h"

OnConnectRequested::OnConnectRequested(UsageEnvironment *env): env(env)
{}

void OnConnectRequested::operator()(DynamicTaskScheduler::RtspConnectInfo connect)
{
    *env << "url: " << connect.url.c_str()
         << ", user: " << connect.user.c_str()
         << ", password: " << connect.password.c_str() << "\n\n";

    // TODO : 생성을 수정할 것.
    openURL(*env, "rtspPerfUsingLive555", connect.url.c_str(), connect.user.c_str(), connect.password.c_str());
    //auto client = PerfCheckRtspClient::createNew(*env, "rtsp://192.168.15.111:554/onvif/profile3/media.smp", 1);
    //*env << client->name();
//    client->start();
}
