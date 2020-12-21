
#pragma once

#include <string>

struct TaskCommand {
    enum Type {
        Connect,
        RemoveOne,
        Disconnect,
        Quit
    };
    Type type;
    //std::string streamId;
    std::string url;
    std::string user;
    std::string password;
};
