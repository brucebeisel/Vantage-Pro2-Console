/*
 * Copyright (C) 2025 Bruce Beisel
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

#ifndef __CYGWIN__
#include <sys/eventfd.h>
#endif
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "json.hpp"

#include "CommandQueue.h"
#include "ResponseHandler.h"
#include "CommandData.h"
#include "CommandHandler.h"
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

/*
 * TODO There appears to be a bug where two web servers can step on each other resulting
 * in bad commands being received or somehow overridden. For instance, it appears
 * that one server can override the "query-current-weather" command so that it has
 * no arguments.
 */
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::CommandSocket(int port) : port(port),
                                         nextSocketSequence(100),
                                         responseEventFd(-1),
                                         terminating(false),
                                         commandThread(NULL),
                                         listenFd(-1),
                                         logger(VantageLogger::getLogger("CommandSocket")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandSocket::~CommandSocket() {
    if (listenFd != -1) {
        close(listenFd);
        listenFd = -1;
    }

    if (responseEventFd != -1) {
        close(responseEventFd);
        responseEventFd = -1;
    }

    for (SocketId socket : socketList)
        close(socket.fd);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::addCommandHandler(CommandHandler & handler) {
    commandHandlers.push_back(&handler);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::mainLoop() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Entering command socket thread with listen fd of " << listenFd << " and eventfd of " << responseEventFd << endl;
    struct timeval tv;
    while (!terminating) {
        try {
            fd_set readFdSet;
            int nfds = max(listenFd, responseEventFd);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            FD_ZERO(&readFdSet); // @suppress("Symbol is not resolved") @suppress("Statement has no effect")
            FD_SET(listenFd, &readFdSet);

            if (responseEventFd != -1)
                FD_SET(responseEventFd, &readFdSet);

            logger.log(VantageLogger::VANTAGE_DEBUG3) << "Adding " << socketList.size() << " file descriptors to read mask" << endl;

            for (const SocketId & socketId : socketList)
                FD_SET(socketId.fd, &readFdSet);

            //
            // This single if statement works because socketFdList is sorted
            //
            if (socketList.size() > 0)
                nfds = std::max(socketList[socketList.size() - 1].fd, nfds);

            nfds++;

            logger.log(VantageLogger::VANTAGE_DEBUG3) << "Entering select()  nfds = " << nfds << endl;
            int n = select(nfds, &readFdSet, NULL, NULL, &tv);
            logger.log(VantageLogger::VANTAGE_DEBUG3) << "select()  returned  " << n << endl;

            if (FD_ISSET(listenFd, &readFdSet))
                acceptConnection();

            //
            // Windows does not support eventfd, so just poll for responses to be processed.
            // This might mean a response will sit in the queue for one second or so, until
            // the select() call times out
            //
            if (responseEventFd != -1) {
                if (FD_ISSET(responseEventFd, &readFdSet))
                    sendCommandResponses();
            }
            else
                sendCommandResponses();

            for (const SocketId & socketId : socketList) {
                if (FD_ISSET(socketId.fd, &readFdSet)) {
                    readCommand(socketId);
                }
            }
        }
        catch (const std::exception & e) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught exception in CommandSocket::mainLoop. " << e.what() << endl;
        }
        catch (...) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught unknown exception from CommandSocket::mainLoop" << endl;
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
    if (commandThread != NULL && commandThread->joinable()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Joining the thread" << endl;
        commandThread->join();
        delete commandThread;
        commandThread = NULL;
    }
    else
        logger.log(VantageLogger::VANTAGE_WARNING) << "Ignoring join request. Thread was not created or is not running." << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandSocket::start() {
    if (!createListenSocket())
        return false;

#ifndef __CYGWIN__
    responseEventFd = eventfd(0, EFD_NONBLOCK);
#endif

    commandThread = new thread(commandThreadEntry, this);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::closeSocket(const SocketId & socket) {
    //
    // Since the list is sorted, deleting a single item will not change the sort order
    //
    socketList.erase(std::find_if(socketList.begin(), socketList.end(), [&socket](const SocketId & other) { return socket.sequence == other.sequence;}));

    close(socket.fd);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Closed socket that used fd = " << socket.fd << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::readCommand(const SocketId & socket) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Reading data on fd " << socket.fd << endl;

    char buffer[10240];
    int readPosition = 0;

    //
    // First read the header
    //
    while (readPosition < HEADER_SIZE) {
        int nbytes = read(socket.fd, &buffer[readPosition], HEADER_SIZE - readPosition);
        //
        // If 0 bytes are read that most likely means the other end has closed the socket
        //
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Attempted read of command header indicates the socket has been closed by the other end, closing socket. Read returned 0." << endl;
            closeSocket(socket);
            return;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command header, closing socket. Read returned -1." << endl;
            closeSocket(socket);
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
        closeSocket(socket);
        return;
    }

    //
    // Read the command body
    //
    readPosition = 0;
    while (readPosition < messageLength) {
        int nbytes = read(socket.fd, &buffer[readPosition], messageLength - readPosition);
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to read command body, closing socket. Read returned 0." << endl;
            closeSocket(socket);
            return;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command body, closing socket. Read returned -1." << endl;
            closeSocket(socket);
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
    CommandData commandData(*this, socket.sequence);
    commandData.setCommandFromJson(string(buffer));

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Offering command " << commandData.commandName << " that was received on fd " << socket.fd << endl;
    bool consumed = false;
    for (auto handler : commandHandlers) {
        consumed = consumed || handler->offerCommand(commandData);
    }

    //
    // If none of the command handlers consumed the command, then immediately send a failure response.
    // There is no need to queue the response as this is running on the socket thread.
    //
    if (!consumed) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Command " << commandData.commandName << " was not consumed by any command handlers. Command is being ignored as an unrecognized command." << endl;
        commandData.response.append(CommandData::buildFailureString("Unrecognized command"));
        sendCommandResponse(commandData);
    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::handleCommandResponse(const CommandData & commandData) {
    std::lock_guard<std::mutex> guard(mutex);
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Queuing response" << endl;
    responseQueue.push(commandData);

    if (responseEventFd != -1) {
        uint64_t eventId = 1;
        logger.log(VantageLogger::VANTAGE_DEBUG3) << "Triggering eventfd" << endl;
        if (write(responseEventFd, &eventId, sizeof(eventId)) < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Could not write to eventfd" <<  endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::sendCommandResponse(const CommandData & commandData) {
    //
    // Terminate the JSON element
    //
    const char * responseTerminator = "\n\n";
    string response(commandData.response);
    response.append(responseTerminator);

    const char * responseBuffer = response.c_str();

    //
    // Lookup the socket file descriptor
    //
    int fd = -1;
    for (const SocketId & socket : socketList) {
        if (socket.sequence == commandData.socketId)
            fd = socket.fd;
    }

    if (fd != -1) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Writing response on fd " << fd << " Response: '" << response << "'" << endl;

        if (write(fd, responseBuffer, strlen(responseBuffer)) < 0)
            logger.log(VantageLogger::VANTAGE_ERROR) << "Could not write response to command server socket. fd = " << fd <<  endl;
    }
    else
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not find socket with ID " << commandData.socketId << " to write response. Response: " << response << endl;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::sendCommandResponses() {
    if (responseEventFd != -1) {
        uint64_t eventId = 0;
        read(responseEventFd, &eventId, sizeof(eventId));
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Read " << eventId << " from eventfd" << endl;
    }

    std::lock_guard<std::mutex> guard(mutex);

    while (!responseQueue.empty()) {
        CommandData commandData = responseQueue.front();
        responseQueue.pop();
        sendCommandResponse(commandData);
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

    logger.log(VantageLogger::VANTAGE_INFO) << "Listening for connections on fd " << listenFd << endl;

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

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        int en = errno;
        logger.log(VantageLogger::VANTAGE_WARNING) << "setsockopt() failed. Error: " << strerror(en) << endl;
    }

    SocketId socket;
    socket.fd = fd;
    socket.sequence = nextSocketSequence++;

    socketList.push_back(socket);
    std::sort(socketList.begin(), socketList.end(), [&](SocketId & s1, SocketId & s2) {return s1.fd < s2.fd; });
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Accepted socket using fd = " << fd << " with assigned sequence = " << socket.sequence << endl;
}

}
