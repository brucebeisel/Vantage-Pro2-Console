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
#include <thread>
#include <vector>

#include "ResponseHandler.h"

namespace vws {
class VantageLogger;
class EventManager;

/**
 * The CommandSocket is a class that uses a thread to read commands from a TCP socket
 * and pass them onto the the event manager. This class will accept and manage multiple
 * connections.
 * The command must be formatted as follows:
 * VANTAGE ######\n
 * {command}
 *
 * where VANTAGE is a fixed string and ###### is a zero filled number indicating the length of the command that follows.
 */
class CommandSocket : ResponseHandler {
public:
    /**
     * Constructor.
     *
     * @param port   The port on which to bind the socket
     * @param evtMgr The event manager to which incoming command are sent for processing
     */
    CommandSocket(int port, EventManager & evtMgr);

    /**
     * Destructor.
     */
    virtual ~CommandSocket();

    /**
     * Format the console's response per the Vantage Weather Station protocol.
     * This is the implementation of the ResponseHandler interface.
     *
     * @param commandData The data that was the command
     * @param response    The response to be sent back to the client
     */
    virtual void handleCommandResponse(const CommandData & commandData, const std::string & response);

    /**
     * Initialize the object; creating the listen socket and spawning the reader thread.
     */
    bool initialize();

    /**
     * The main loop that read the commands from the socket.
     * This is also the thread entry point.
     */
    void mainLoop();

    /**
     * Mark the main loop for termination.
     * The join() method should be called next.
     */
    void terminate();

    /**
     * Join (in the pthread sense), the reader thread.
     */
    void join();

private:
    static constexpr int HEADER_SIZE = 15;
    static constexpr const char * HEADER_TEXT = "VANTAGE";
    static constexpr int MIN_COMMAND_LENGTH = 20; // Arbitrary number for quick error checks

    /**
     * Accept a new client socket connection.
     */
    void acceptConnection();

    /**
     * Create the socket for listening for new connections.
     */
    bool createListenSocket();

    /**
     * Read a command from one of the client sockets.
     *
     * @param fd The file descriptor from which to read
     */
    void readCommand(int fd);

    /**
     * Close the specified client socket.
     *
     * @param fd The file descriptor to close
     */
    void closeSocket(int fd);

    int              port;            // The port on which the console will listen for client connections
    int              listenFd;        // The file description on which this thread is listening
    std::vector<int> socketFdList;    // The list of client file descriptors currently open
    EventManager &   eventManager;    // The event manager to which commands are forwarded for processing
    bool             terminating;     // True if this thread's main loop should exit
    std::thread *    commandThread;   // The thread that reads the commands
    VantageLogger &  logger;
};

} /* namespace vws */

#endif /* COMMAND_SOCKET_H_ */
