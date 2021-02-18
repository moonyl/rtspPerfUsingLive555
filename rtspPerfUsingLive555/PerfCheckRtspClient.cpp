#include <string>
#include "PerfCheckRtspClient.h"
#include "RawStreamPerfCheckSink.h"

StreamClientState::StreamClientState()
{

}
StreamClientState::~StreamClientState()
{

}

PerfCheckRtspClient *PerfCheckRtspClient::createNew(UsageEnvironment &env, const char *rtspURL, int verbosityLevel,
                                                    const char *applicationName, char const *user, char const *password,
                                                    portNumBits tunnelOverHTTPPortNum)
{
    return new PerfCheckRtspClient(env, rtspURL, user, password, verbosityLevel,
                                   applicationName, tunnelOverHTTPPortNum);
}

PerfCheckRtspClient::PerfCheckRtspClient(UsageEnvironment &env, const char *rtspURL,
                                         char const* user, char const *password, int verbosityLevel,
                                         const char *applicationName, portNumBits tunnelOverHTTPPortNum) :
                                         RTSPClient(env,rtspURL, verbosityLevel, applicationName,
                                                    tunnelOverHTTPPortNum, -1)
{
    if (user)   {
        fCurrentAuthenticator.setUsernameAndPassword(user, password);
    }
}

PerfCheckRtspClient::~PerfCheckRtspClient()
{

}

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

unsigned int PerfCheckRtspClient::rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.
#define RTSP_CLIENT_VERBOSITY_LEVEL 0 // by default, print verbose output from each "RTSPClient"

static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);

RTSPClient* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, char const *user, char const *password) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).
    RTSPClient* rtspClient = PerfCheckRtspClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName,
                                                            user, password);
    if (rtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return nullptr;
    }

    ++PerfCheckRtspClient::rtspClientCount;

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
    return rtspClient;
}


static void setupNextSubsession(RTSPClient* rtspClient);

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((PerfCheckRtspClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            // it is possible to be confused owing to Load Balancer configuration
            delete[] resultString;
            break;
        }

        char* const sdpDescription = resultString;
        //env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient, ExitCode::Retry);
}

static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

#define REQUEST_STREAMING_OVER_TCP True

void setupNextSubsession(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((PerfCheckRtspClient*)rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            //env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
//            if (scs.subsession->rtcpIsMuxed()) {
//                env << "client port " << scs.subsession->clientPortNum();
//            } else {
//                env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
//            }
//            env << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

#include <nlohmann/json.hpp>
#include <iostream>
#include <exception>

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((PerfCheckRtspClient*)rtspClient)->scs; // alias

    if (exitCode == ExitCode::Retry)    {
        openURL(env, "rtspPerfUsingLive555", rtspClient->url());
    }

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

//    try {
//        using json = nlohmann::json;
//        json state;
//
//        state["name"] = rtspClient->name();
//        state["url"] = rtspClient->url();
//        state["byUser"] = exitCode == 0;
//
//        json j;
//        j["result"] = "disconnected";
//        j["state"] = state;
//        std::cout << j.dump() << std::endl;
//    } catch(std::exception &e)   {
//        env << "exception, closing: name: " << rtspClient->name() << ", url: " << rtspClient->url() << "\n";
//        env << e.what() << "\n";
//    }

    env << *rtspClient << "Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

    if (--PerfCheckRtspClient::rtspClientCount == 0) {
        // The final stream has ended, so exit the application now.
        // (Of course, if you're embedding this code into your own application, you might want to comment this out,
        // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
        exit(exitCode);
    }
}

static void subsessionAfterPlaying(void* clientData);
static void subsessionByeHandler(void* clientData, char const* reason);

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((PerfCheckRtspClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << scs.subsession->clientPortNum();
        } else {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        if (!std::string("video").compare(scs.subsession->mediumName())) {
//            if (RawStreamPerfCheckSink::connectCount == 0)  {
//                int64_t uSecsToDelay = 3+1000*1000;
//                env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)calculateStatics, rtspClient);
//            }
            auto* sink = RawStreamPerfCheckSink::createNew(env, *scs.subsession, rtspClient->url());
            PerfCheckRtspClient* client = dynamic_cast<PerfCheckRtspClient*>(rtspClient);
            client->setPerfSink(sink);
            scs.subsession->sink = sink;
            //env << "fps: " << scs.subsession->videoFPS() << "\n";
        } else {
//            env << *rtspClient << "don't care of the \"" << *scs.subsession
//                << "\" subsession: " << "\n";
            break;
        }

        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }



//        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\"";
    if (reason != NULL) {
        env << " (reason:\"" << reason << "\")";
        delete[] (char*)reason;
    }
    env << " on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}


static void streamTimerHandler(void* clientData);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    Boolean success = False;

    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((PerfCheckRtspClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }

        env << *rtspClient << "Started playing session";
        if (scs.duration > 0) {
            env << " (for up to " << scs.duration << " seconds)";
        }
        env << "...\n";

        success = True;

//        try {
//            using json = nlohmann::json;
//            json j;
//            j["result"] = "added";
//            json state;
//            state["name"] = rtspClient->name();
//            state["url"] = rtspClient->url();
//            j["state"] = state;
//
//            std::cout << j.dump() << std::endl;
//        } catch(std::exception &e)  {
//            env << "exception, client added, name: " << rtspClient->name() << ", url: " << rtspClient->url() << "\n";
//            env << e.what() << "\n";
//        }

    } while (0);
    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}

void streamTimerHandler(void* clientData) {
    PerfCheckRtspClient* rtspClient = (PerfCheckRtspClient*)clientData;
    StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    // Shut down the stream:
    shutdownStream(rtspClient);
    //rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}