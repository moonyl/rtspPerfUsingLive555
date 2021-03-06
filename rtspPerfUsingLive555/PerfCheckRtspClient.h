//
// Created by admin on 2020-12-15.
//

#pragma once

#include <liveMedia.hh>
#include "RawStreamPerfCheckSink.h"

class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator* iter = nullptr;
    MediaSession* session = nullptr;
    MediaSubsession* subsession = nullptr;
    TaskToken streamTimerTask = nullptr;
    double duration = 0;
};

class PerfCheckRtspClient: public RTSPClient {

    RawStreamPerfCheckSink* videoSink{nullptr};

public:
    static PerfCheckRtspClient* createNew(UsageEnvironment& env, char const* rtspURL,
                                    int verbosityLevel = 0,
                                    char const* applicationName = nullptr,
                                    char const* user = nullptr, char const *password = nullptr,
                                    portNumBits tunnelOverHTTPPortNum = 0);
    void setPerfSink(RawStreamPerfCheckSink* sink) { videoSink = sink; }
    RawStreamPerfCheckSink* perfSink() const { return videoSink; }

protected:
    PerfCheckRtspClient(UsageEnvironment& env, char const* rtspURL, char const *user, char const *password,
                  int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
    virtual ~PerfCheckRtspClient();

public:
    StreamClientState scs;

    static unsigned int rtspClientCount;

};

RTSPClient* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL,
             char const *user = nullptr, char const *password = nullptr);


enum ExitCode
{
    ByUser,
    Error,
    Retry,
};
void shutdownStream(RTSPClient* rtspClient, int exitCode = ExitCode::Error);

