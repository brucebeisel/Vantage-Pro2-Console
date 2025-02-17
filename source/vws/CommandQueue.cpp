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

#include "CommandData.h"
#include "CommandHandler.h"
#include "CommandQueue.h"
#include "ResponseHandler.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandQueue::CommandQueue(/*CommandHandler & ch*/) : /*commandHandler(ch),*/ logger(VantageLogger::getLogger("EventManager")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandQueue::~CommandQueue() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::isEventAvailable() const {
    std::scoped_lock<std::mutex> guard(mutex);
    return !commandQueue.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandQueue::queueEvent(const CommandData & event) {
    {
        std::scoped_lock<std::mutex> guard(mutex);
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Queuing event" << endl;
        commandQueue.push(event);
    }

    cv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::lockAndConsumeEvent(CommandData & event) {
    std::scoped_lock<std::mutex> guard(mutex);
    return consumeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::consumeEvent(CommandData & event) {
    if (commandQueue.empty())
        return false;

    event = commandQueue.front();
    commandQueue.pop();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::waitForEvent(CommandData & event) {
    std::unique_lock<std::mutex> guard(mutex);

    cv.wait(guard);

    return consumeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandQueue::interrupt() {
    cv.notify_all();
}

}
