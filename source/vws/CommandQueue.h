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
 * Class to handle events from the HTTP threads.
 */
class CommandQueue {
public:
    /**
     * Constructor.
     *
     * @param commandHandler The handler to which to send the events
     */
    CommandQueue();

    /**
     * Destructor.
     */
    virtual ~CommandQueue();

    /**
     * Check if there is an event on the queue. Note that in a multi-threaded environment
     * the return value may no longer be valid when the consumeEvent() method is called.
     *
     * @return True if the queue is not empty at the moment
     */
    bool isCommandAvailable() const;

    /**
     * Queue an event.
     *
     * @param event The event to be queued
     */
    void queueCommand(const CommandData & command);

    /**
     * Consume the event at the head of the queue without locking.
     *
     * @param event The event that was copied from the head of the queue
     * @return True if an event was actually copied. If false, the parameter event is not changed.
     */
    bool consumeCommand(CommandData & command);

    /**
     * Consume the event at the head of the queue with locking.
     *
     * @param event The event that was copied from the head of the queue
     * @return True if an event was actually copied. If false, the parameter event is not changed.
     */
    bool lockAndConsumeCommand(CommandData & command);

    /**
     * Wait for a command to appear on the queue.
     *
     * @param event The event that was copied from the head of the queue
     * @return True if an event was actually copied. If false, the parameter event is not changed.
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
    std::queue<CommandData> commandQueue;   // The queue on which to store events
    mutable std::mutex      mutex;          // The mutex to protect the queue against multi-threaded contention
    std::condition_variable cv;             // The mutex to protect the queue against multi-threaded contention
    VantageLogger &         logger;
};

}

#endif /* EVENT_MANAGER_H_ */
