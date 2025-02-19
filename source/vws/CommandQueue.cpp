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
#include "CommandQueue.h"

#include "CommandData.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandQueue::CommandQueue() : logger(VantageLogger::getLogger("CommandQueue")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandQueue::~CommandQueue() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::isCommandAvailable() const {
    std::scoped_lock<std::mutex> guard(mutex);
    return !commandQueue.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandQueue::queueCommand(const CommandData & command) {
    {
        std::scoped_lock<std::mutex> guard(mutex);
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Queuing command " << command.commandName << endl;
        commandQueue.push(command);
    }

    cv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::consumeCommand(CommandData & command) {
    std::scoped_lock<std::mutex> guard(mutex);
    return retrieveNextCommand(command);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::retrieveNextCommand(CommandData & command) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Attempting to consume command" << endl;
    if (commandQueue.empty())
        return false;

    command = commandQueue.front();
    commandQueue.pop();

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Consumed command " << command.commandName << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandQueue::waitForCommand(CommandData & command) {
    std::unique_lock<std::mutex> guard(mutex);

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Waiting for command" << endl;
    cv.wait(guard);

    return retrieveNextCommand(command);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandQueue::interrupt() {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Interrupting threads waiting for commands" << endl;
    cv.notify_all();
}

}
