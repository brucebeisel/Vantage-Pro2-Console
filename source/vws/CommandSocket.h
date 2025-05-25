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
#ifndef COMMAND_SOCKET_H_
#define COMMAND_SOCKET_H_

#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>

#include "ResponseHandler.h"

namespace vws {
class VantageLogger;
class CommandHandler;

/**
 * The CommandSocket is a class that uses a thread to read commands from a TCP socket
 * and offer them onto the registered command handlers. This class will accept and manage multiple
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
    CommandSocket(int port);

    /**
     * Destructor.
     */
    virtual ~CommandSocket();

    /**
     * Add a command handler to the list of handlers that will be offered commands received on the socket.
     *
     * @param handler The handler to be added
     */
    void addCommandHandler(CommandHandler & handler);

    /**
     * Format the console's response per the Vantage Weather Station protocol.
     * This is the implementation of the ResponseHandler interface.
     *
     * @param commandData The data that was the command
     * @param response    The response to be sent back to the client
     */
    virtual void handleCommandResponse(const CommandData & commandData);

    /**
     * Actually send the response on the provided file descriptor.
     */
    void sendCommandResponse(const CommandData & commandData);

    /**
     * Initialize the object; creating the listen socket and spawning the socket read/write thread.
     */
    bool start();

    /**
     * The main loop that read the commands from the socket and write the responses.
     * This is also the thread entry point.
     */
    void mainLoop();

    /**
     * Mark the main loop for termination.
     * The join() method should be called next.
     */
    void terminate();

    /**
     * Join (in the pthread sense), the socket thread.
     */
    void join();

private:
    static constexpr int          HEADER_SIZE = 15;
    static constexpr const char * HEADER_TEXT = "VANTAGE";
    static constexpr int          MIN_COMMAND_LENGTH = 20; // Arbitrary number for quick error checks

    /**
     * Structure used to uniquely identify a socket to ensure that the response is sent on the same file
     * descriptor instance that the command was received on. Since file descriptors are reused there is
     * a possibility that the file descriptor value on which the command was received was closed and then
     * reopened.
     */
    struct SocketId {
        int sequence;
        int fd;
    };

    /**
     * Output the socket list on stdout.
     */
    void dumpSocketList() const;

    /**
     * ostream operator for SocketId.
     *
     * @param os       The ostream on which to output the SocketId
     * @param socketId The SocketId to output
     * @return The modified ostream
     */
    friend std::ostream & operator<<(std::ostream & os, const SocketId & socketId);

    /**
     * Accept a new client socket connection.
     */
    void acceptConnection();

    /**
     * Create the socket for listening for new connections.
     *
     * @return True if successful
     */
    bool createListenSocket();

    /**
     * Read a command from one of the client sockets.
     *
     * @param socket The socket from which to read
     * @return True if socket should remain open
     */
    bool readCommand(const SocketId & socket);

    /**
     * Send any pending responses.
     */
    void sendCommandResponses();

    int                           port;                // The port on which the console will listen for client connections
    int                           listenFd;            // The file description on which this thread is listening
    int                           nextSocketSequence;  // The sequence number for the next command socket accepted
    std::vector<SocketId>         socketList;          // The list of client file descriptors currently open
    std::vector<CommandHandler *> commandHandlers;     // The handlers that will be offered commands
    bool                          terminating;         // True if this thread's main loop should exit
    int                           responseEventFd;     // The file descriptor used to receive indications of an available response
    std::queue<CommandData>       responseQueue;       // The queue on which to store event responses
    mutable std::mutex            mutex;               // The mutex to protect the queue against multi-threaded contention
    std::thread *                 commandThread;       // The thread that reads the commands
    VantageLogger &               logger;
};

} /* namespace vws */

#endif /* COMMAND_SOCKET_H_ */
