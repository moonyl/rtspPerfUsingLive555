//
// Created by admin on 2020-12-18.
//

#include "OnRemoveOneRequested.h"
#include <liveMedia.hh>
#include "PerfCheckRtspClient.h"


OnRemoveOneRequested::OnRemoveOneRequested(UsageEnvironment *env): env(env)
{}

void OnRemoveOneRequested::operator()()
{
    auto & table = MediaLookupTable::ourMedia(*env)->getTable();
    HashTable::Iterator* iter = HashTable::Iterator::create(table);
    char const* key;
    void* value;
    do {
        value = iter->next(key);
        if (value != nullptr)    {
            Medium* medium;
            auto found = Medium::lookupByName(*env, key, medium);
            if (found)  {
                if (medium->isRTSPClient()) {
                    RTSPClient* client = static_cast<RTSPClient*>(medium);
                    shutdownStream(client);
                    break;
                }
//                *env << "isSource: " << medium->isSource() << ", isSink: " << medium->isSink()
//                    << ", isRTCPInstance: " << medium->isRTCPInstance()
//                    << ", isRTSPClient: " << medium->isRTSPClient()
//                    << ", isRTSPServer: " << medium->isRTSPServer()
//                    << ", isMediaSession: " << medium->isMediaSession()
//                    << ", isServerMediaSession: " << medium->isServerMediaSession() << "\n";
            }
//            *env << "key: " << key << "\n";
//            *env << "entries: " << table.numEntries() << "\n";
        }
    } while(value != nullptr);
}
