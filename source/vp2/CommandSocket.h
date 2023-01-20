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
#ifndef COMMAND_SOCKET_H_
#define COMMAND_SOCKET_H_
#include <string>

namespace vws {

class EventManager;

class CommandSocket {
public:
    CommandSocket(EventManager & evtMgr);
    virtual ~CommandSocket();

    void processCommand(const std::string & commandJson);
    bool dataAvailable();

private:
    void acceptConnection();

    int              listenFd;
    std::vector<int> socketFdList;
    int              maxSocket;
    EventManager &   eventManager;
};

} /* namespace vws */

#endif /* COMMAND_SOCKET_H_ */
