/*
 * Copyright (C) 2023 Bruce Beisel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <iostream>
#include "json.hpp"
#include "EventManager.h"
#include "CommandSocket.h"

using json = nlohmann::json;
using namespace std;

namespace vws {

void
jsonKeyValue(json object, std::string & key, std::string & value) {
    auto iterator = object.begin();
    key = iterator.key();
    value = iterator.value();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::CommandSocket(EventManager & evtMgr) : listenFd(-1), eventManager(evtMgr), maxSocket(-1) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::~CommandSocket() {
    if (listenFd != -1) {
        close(listenFd);
        listenFd = -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandSocket::dataAvailable() {
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_except;
    int nfds = maxSocket + 1;

    int n = select(nfds, &fd_read, &fd_write, &fd_except, NULL);

    for (int i = 0; i < socketFdList.size(); i++) {
        if (FD_ISSET(socketFdList[i], &fd_read)) {

        }

    }

    //return n == 1 && FD_ISSET(socketFd, &fd_read);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::processCommand(const std::string & commandJson) {

    /*
    try {
        json command = json::parse(commandJson.begin(), commandJson.end());
        string commandName = command.value("command", "unknown");
        json args = command.at("arguments");
        vector<pair<string,string>> argumentList;
        for (int i = 0; i < args.size(); i++) {
            json arg = args[i];
            std::string key, value;
            pair<string,string> argument;
            jsonKeyValue(arg, argument.first, argument.second);
            argumentList.push_back(argument);
        }

        cout <<     "Command: " << commandName << endl;
        cout <<     "    Arguments:" << endl;
        for (int i = 0; i < argumentList.size(); i++) {
            cout << "          [" << i << "]: " << argumentList[i].first << "=" << argumentList[i].second << endl;
        }

        switch (commandName) {
            case "backlight":
                break;

        }
    }
    catch (const std::exception & e) {
        cout << "Exception: " << e.what() << endl;
    }
    */

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::acceptConnection() {
    int fd = accept(listenFd, NULL, NULL);

    if (fd < 0)
        return;

    socketFdList.push_back(fd);
    std::sort(socketFdList.begin(), socketFdList.end());
    maxSocket = socketFdList[socketFdList.size() - 1];
}

}
