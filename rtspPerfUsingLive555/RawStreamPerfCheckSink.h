//
// Created by admin on 2020-12-15.
//

#pragma once
#include <liveMedia.hh>

class RawStreamPerfCheckSink: public MediaSink {
public:
    static RawStreamPerfCheckSink* createNew(UsageEnvironment& env,
                                MediaSubsession& subsession, // identifies the kind of data that's being received
                                char const* streamId = NULL); // identifies the stream itself (optional)

private:
    RawStreamPerfCheckSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    // called only by "createNew()"
    virtual ~RawStreamPerfCheckSink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;

public:
    int frameCount = 0;
};

