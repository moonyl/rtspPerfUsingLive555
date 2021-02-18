#include "RawStreamPerfCheckSink.h"
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <cmath>


RawStreamPerfCheckSink *
RawStreamPerfCheckSink::createNew(UsageEnvironment &env, MediaSubsession &subsession, const char *streamId)
{
    return new RawStreamPerfCheckSink(env, subsession, streamId);
}

RawStreamPerfCheckSink::~RawStreamPerfCheckSink()
{
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void RawStreamPerfCheckSink::afterGettingFrame(void *clientData, unsigned int frameSize, unsigned int numTruncatedBytes,
                                               struct timeval presentationTime, unsigned int durationInMicroseconds)
{
    RawStreamPerfCheckSink* sink = (RawStreamPerfCheckSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);

}

#include <sstream>
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME
void RawStreamPerfCheckSink::afterGettingFrame(unsigned int frameSize, unsigned int numTruncatedBytes,
                                               struct timeval presentationTime, unsigned int durationInMicroseconds)
{
    // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
//    char frameKind[3]; // used to output the 'microseconds' part of the presentation time
//    sprintf(frameKind, "%02x", fReceiveBuffer[0] & 0x1f);
//    envir() << "frame kind: " << frameKind << "\n";

//    auto sec = presentationTime.tv_sec;
//    std::stringstream sstr;
//    sstr << "timestamp: " << presentationTime.tv_sec << ", " << presentationTime.tv_usec;
//    envir() << sstr.str().c_str() << "\n";
    auto kind = fReceiveBuffer[0] & 0x1f;
    if (kind == 0x1 || kind == 0x5 )    {
        frameCount++;
    }
    // Then continue, to request the next frame of data:
    continuePlaying();
}

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 700000

Boolean RawStreamPerfCheckSink::continuePlaying()
{
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}



RawStreamPerfCheckSink::RawStreamPerfCheckSink(UsageEnvironment &env,
                                               MediaSubsession &subsession, const char *streamId) :
                                               MediaSink(env),  fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
    startPoint = std::chrono::system_clock::now();

}
