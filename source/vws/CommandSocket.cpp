/*
 * Copyright (C) 2024 Bruce Beisel
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

#include "CommandSocket.h"

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "json.hpp"

#include "EventManager.h"
#include "ResponseHandler.h"
#include "VantageLogger.h"

using json = nlohmann::json;
using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
commandThreadEntry(CommandSocket * cs) {
    cs->mainLoop();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::CommandSocket(int port, EventManager & evtMgr) : port(port),
                                                                terminating(false),
                                                                commandThread(NULL),
                                                                listenFd(-1),
                                                                eventManager(evtMgr),
                                                                logger(VantageLogger::getLogger("CommandSocket")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::~CommandSocket() {
    if (listenFd != -1) {
        close(listenFd);
        listenFd = -1;
    }

    for (int fd : socketFdList)
        close(fd);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::mainLoop() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Entering command socket thread" << endl;
    struct timeval tv;
    while (!terminating) {
        fd_set fd_read;
        int nfds = listenFd;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&fd_read); // @suppress("Symbol is not resolved") @suppress("Statement has no effect")
        FD_SET(listenFd, &fd_read);

        logger.log(VantageLogger::VANTAGE_DEBUG3) << "Adding " << socketFdList.size() << " to fd mask" << endl;

        for (int fd : socketFdList)
            FD_SET(fd, &fd_read);

        //
        // This single if statement works because socketFdList is sorted
        //
        if (socketFdList.size() > 0)
            nfds = std::max(socketFdList[socketFdList.size() - 1], listenFd);

        nfds++;

        logger.log(VantageLogger::VANTAGE_DEBUG3) << "Entering select()  nfds = " << nfds << endl;
        int n = select(nfds, &fd_read, NULL, NULL, &tv);
        logger.log(VantageLogger::VANTAGE_DEBUG3) << "select()  returned  " << n << endl;

        if (FD_ISSET(listenFd, &fd_read))
            acceptConnection();

        for (int fd : socketFdList) {
            if (FD_ISSET(fd, &fd_read)) {
                logger.log(VantageLogger::VANTAGE_DEBUG3) << "Received data on fd " << fd << endl;
                readCommand(fd);
            }
        }
    }
    logger.log(VantageLogger::VANTAGE_INFO) << "Exiting command socket thread" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::terminate() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Received request to terminate command socket thread" << endl;
    terminating = true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::join() {
    if (commandThread != NULL) {
        commandThread->join();
        delete commandThread;
        commandThread = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandSocket::initialize() {
    if (!createListenSocket())
        return false;

    commandThread = new thread(commandThreadEntry, this);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::closeSocket(int fd) {
    //
    // Since the list is sorted, deleting a single item will not change the sort order
    //
    socketFdList.erase(std::find(socketFdList.begin(), socketFdList.end(), fd));
    close(fd);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Closed socket that used fd = " << fd << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::readCommand(int fd) {
    char buffer[10240];
    int readPosition = 0;

    //
    // First read the header
    //
    while (readPosition < HEADER_SIZE) {
        int nbytes = read(fd, &buffer[readPosition], HEADER_SIZE - readPosition);
        //
        // If 0 bytes are read that most likely means the other end has closed the socket
        //
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Attempted read of command header indicates the socket has been closed by the other end, closing socket. Read returned 0." << endl;
            closeSocket(fd);
            return;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command header, closing socket. Read returned -1." << endl;
            closeSocket(fd);
            return;
        }
        else {
            readPosition += nbytes;
        }
    }

    if (strncmp(buffer, HEADER_TEXT, strlen(HEADER_TEXT)) != 0) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Command does not start with header text. Received '" << buffer << "'" << endl;
        return;
    }

    //
    // Now pull out the size of the upcoming command
    //
    int messageLength = atoi(&buffer[strlen(HEADER_TEXT) + 1]);
    if (messageLength < MIN_COMMAND_LENGTH) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Command length in header is too small. Received " << messageLength << endl;
        closeSocket(fd);
        return;
    }

    //
    // Read the command body
    //
    readPosition = 0;
    while (readPosition < messageLength) {
        int nbytes = read(fd, &buffer[readPosition], messageLength - readPosition);
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to read command body, closing socket. Read returned 0." << endl;
            closeSocket(fd);
            return;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command body, closing socket. Read returned -1." << endl;
            closeSocket(fd);
            return;
        }
        else {
            readPosition += nbytes;
        }
    }

    //
    // NULL terminate the string
    //
    buffer[readPosition] = '\0';

    //
    // At this point the command is in buffer. Use auto-conversion to get it into the required std::string
    //
    CommandData cd;
    cd.fd = fd;
    cd.command = buffer;
    cd.responseHandler = this;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Queuing command " << cd.command << " that was received on fd " << cd.fd << endl;
    eventManager.queueEvent(cd);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::handleCommandResponse(const CommandData & commandData, const std::string & responseArg) {
    const char * responseTerminator = "\n\n";
    string response(responseArg);
    response.append(responseTerminator);

    const char * responseBuffer = response.c_str();

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Writing response '" << response << "' on fd " << commandData.fd << endl;

    if (write(commandData.fd, responseBuffer, strlen(responseBuffer)) < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not write response to command server socket. fd = " << commandData.fd <<  endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandSocket::createListenSocket() {
    listenFd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenFd < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not create command server socket" << endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not configure command server socket" << endl;
        return false;
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(listenFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to bind command server socket" << endl;
        return false;
    }

    if (listen(listenFd, 3) < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to listen on command server socket" << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::acceptConnection() {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Accepting socket using listen fd = " << listenFd << endl;
    int fd = accept(listenFd, NULL, NULL);

    if (fd < 0) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Accept failed" << endl;
        return;
    }

    //
    // Set a read timeout of 250 milliseconds
    //
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    socketFdList.push_back(fd);
    std::sort(socketFdList.begin(), socketFdList.end());
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Accepted socket using fd = " << fd << endl;
}

}
