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

            for (const SocketId & socketId : socketList) {
                logger.log(VantageLogger::VANTAGE_DEBUG3) << "Adding fd " << socketId.fd << " to read mask" << endl;
                FD_SET(socketId.fd, &readFdSet);
                nfds = std::max(socketId.fd, nfds);
            }

            nfds++;

            logger.log(VantageLogger::VANTAGE_DEBUG3) << "Entering select()  nfds = " << nfds << endl;
            int n = select(nfds, &readFdSet, NULL, NULL, &tv);
            logger.log(VantageLogger::VANTAGE_DEBUG3) << "select()  returned  " << n << endl;

            if (n < 0) {
                logger.log(VantageLogger::VANTAGE_ERROR) << "select() returned an error (" << logger.strerror() << ")" << endl;
                continue;
            }

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

            for (vector<SocketId>::iterator it = socketList.begin(); it != socketList.end(); ) {
                if (FD_ISSET(it->fd, &readFdSet)) {
                    if (!readCommand(*it)) {
                        logger.log(VantageLogger::VANTAGE_DEBUG3) << "Closing socket " << *it <<  endl;
                        close(it->fd);
                        it = socketList.erase(it);
                    }
                    else
                        ++it;
                }
                else
                    ++it;
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
CommandSocket::dumpSocketList() const {
    cout << "Socket list: [";

    for (vector<SocketId>::const_iterator it = socketList.begin(); it != socketList.end(); ++it) {
        cout << *it << ", ";
    }

    cout << "]" << endl;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandSocket::readCommand(const SocketId & socketId) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Reading data from socket " << socketId << endl;

    char buffer[10240];
    int readPosition = 0;

    //
    // First read the header
    //
    while (readPosition < HEADER_SIZE) {
        int nbytes = read(socketId.fd, &buffer[readPosition], HEADER_SIZE - readPosition);
        //
        // If 0 bytes are read that most likely means the other end has closed the socket
        //
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Attempted read of command header indicates the socket has been closed by the other end, closing socket. Read returned 0." << endl;
            return false;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command header, closing socket. (" << logger.strerror() << ")" << endl;
            return false;
        }
        else {
            readPosition += nbytes;
        }
    }

    if (strncmp(buffer, HEADER_TEXT, strlen(HEADER_TEXT)) != 0) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Command does not start with header text. Received '" << buffer << "'" << endl;
        return true;
    }

    //
    // Now pull out the size of the upcoming command
    //
    int messageLength = atoi(&buffer[strlen(HEADER_TEXT) + 1]);
    if (messageLength < MIN_COMMAND_LENGTH) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Command length in header is too small. Received " << messageLength << endl;
        return false;
    }

    //
    // Read the command body
    //
    readPosition = 0;
    while (readPosition < messageLength) {
        int nbytes = read(socketId.fd, &buffer[readPosition], messageLength - readPosition);
        if (nbytes == 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to read command body, closing socket. Read returned 0." << endl;
            return false;
        }
        else if (nbytes < 0) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Read() returned an error while reading command body, closing socket. (" << logger.strerror() << ")" << endl;
            return false;
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
    CommandData commandData(*this, socketId.sequence);
    if (!commandData.setCommandFromJson(string(buffer))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Received invalid JSON command: '" << buffer << "'" << endl;
        return true;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Offering command " << commandData.commandName << " that was received on socket " << socketId << endl;
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

    return true;
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
            logger.log(VantageLogger::VANTAGE_WARNING) << "Could not write to eventfd (" << logger.strerror() << ")" <<  endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandSocket::sendCommandResponse(const CommandData & commandData) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Attempting to send response on socketId " << commandData.socketId << endl;

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

        if (write(fd, responseBuffer, strlen(responseBuffer)) < 0) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Write of response to command server socket failed (" << logger.strerror() << "). fd = " << fd <<  endl;
        }
    }
    else
        logger.log(VantageLogger::VANTAGE_ERROR) << "Discarding response because the socket with ID " << commandData.socketId << " could not be found. Response: " << response << endl;

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
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not create command server socket (" << logger.strerror() << ")" << endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not configure command server socket (" << logger.strerror() << ")" << endl;
        return false;
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(listenFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to bind command server socket (" << logger.strerror() << ")" << endl;
        return false;
    }

    if (listen(listenFd, 3) < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to listen on command server socket (" << logger.strerror() << ")" << endl;
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
        logger.log(VantageLogger::VANTAGE_WARNING) << "Accept failed (" << logger.strerror() << ")" << endl;
        return;
    }

    //
    // Set a read timeout of 250 milliseconds
    //
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "setsockopt() failed (" << logger.strerror() << ")" << endl;
    }

    SocketId socketId;
    socketId.fd = fd;
    socketId.sequence = nextSocketSequence++;

    socketList.push_back(socketId);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Accepted socket: " << socketId << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::ostream &
operator<<(std::ostream & os, const CommandSocket::SocketId & socketId) {
    os << "SocketID (Sequence: " << socketId.sequence << ", fd: " << socketId.fd << ")";
    return os;
}

}
