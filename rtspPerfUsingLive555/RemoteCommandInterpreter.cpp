//
// Created by moony on 21. 1. 6..
//
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "TaskCommand.h"
#include "RemoteCommandInterpreter.h"

bool RemoteCommandInterpreter::handleRequest(const std::string& request)
{
    using json = nlohmann::json;
    using jsonException = nlohmann::detail::exception;

    try {
        auto jsonObj = json::parse(request);
        if (jsonObj["cmd"].is_string()) {
            auto const cmd = jsonObj["cmd"].get<std::string>();
            if (cmd == "quit") {
                return false;
            }
            if (cmd == "removeOne")   {
                q.push({TaskCommand::RemoveOne});
                return true;
            }
            if (cmd == "add") {
                if (jsonObj["param"].is_object())   {
                    const auto param = jsonObj["param"].get<json>();
                    if (!param.contains("url"))  {
                        return true;
                    }
                    auto const url = param["url"].get<std::string>();
                    if (!url.empty()) {
                        auto const user = param.contains("user") ? param["user"].get<std::string>() : "" ;
                        auto const password = param.contains("password") ? param["password"].get<std::string>() : "";
                        q.push({TaskCommand::Connect, url, user, password});
                    }
                }
                return true;
            }
            if (cmd == "disconnect")    {
                if (jsonObj["param"].is_string()) {
                    auto const nameToDisconnect = jsonObj["param"].get<std::string>();
                    q.push({TaskCommand::Disconnect, nameToDisconnect});
                }
                return true;
            }
        }
    } catch (jsonException &e) {
        std::cerr << "exception, " << __func__ << ", " << __LINE__ << "\n";
        std::cerr << "request: " << request << "\n";
        std::cerr << e.what() << "\n";
    }
}

RemoteCommandInterpreter::RemoteCommandInterpreter(RemoteCommandInterpreter::TaskCommandQueue &q) : q(q) {}
