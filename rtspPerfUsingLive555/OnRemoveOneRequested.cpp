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
                    shutdownStream(client, ExitCode::ByUser);
                    break;
                }
            }
        }
    } while(value != nullptr);
}
