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
#ifndef COMMAND_QUEUE_H_
#define COMMAND_QUEUE_H_

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

namespace vws {
class VantageLogger;
class CommandData;

/**
 * Class to queue commands for a thread.
 */
class CommandQueue {
public:
    /**
     * Constructor.
     */
    CommandQueue();

    /**
     * Destructor.
     */
    virtual ~CommandQueue();

    /**
     * Check if there is a command on the queue. Note that in a multi-threaded environment
     * the return value may no longer be valid when the consumeCommand() method is called.
     *
     * @return True if the queue is not empty at the moment
     */
    bool isCommandAvailable() const;

    /**
     * Queue a command.
     *
     * @param command The command to be queued
     */
    void queueCommand(const CommandData & command);

    /**
     * Consume the command at the head of the queue with locking.
     *
     * @param command The command that was copied from the head of the queue
     * @return True if an command was actually copied. If false, the parameter command is not changed.
     */
    bool consumeCommand(CommandData & command);

    /**
     * Wait for a command to appear on the queue.
     *
     * @param command The command that was copied from the head of the queue
     * @return True if a command was actually copied. If false, the parameter command is not changed.
     */
    bool waitForCommand(CommandData & command);

    /**
     * Interrupt a thread that is waiting for a command.
     */
    void interrupt();

    //
    // Prevent all copying and moving
    //
    CommandQueue(const CommandQueue &) = delete;
    CommandQueue & operator=(const CommandQueue &) = delete;

private:
    /**
     * Get and pop the command at the head of the queue without locking.
     *
     * @param command The command that was copied from the head of the queue
     * @return True if an command was actually copied. If false, the parameter command is not changed.
     */
    bool retrieveNextCommand(CommandData & command);

    std::queue<CommandData> commandQueue;   // The queue on which to store commands
    mutable std::mutex      mutex;          // The mutex to protect the queue against multi-threaded contention
    std::condition_variable cv;             // The condition variable used for notifying a thread that a command is available
    VantageLogger &         logger;
};

}

#endif /* COMMAND_QUEUE_H_ */
