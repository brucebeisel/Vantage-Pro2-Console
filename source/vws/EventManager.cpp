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

#include "EventManager.h"

#include <signal.h>

#include "CommandHandler.h"
#include "ResponseHandler.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
EventManager::EventManager(CommandHandler & ch) : commandHandler(ch), logger(VantageLogger::getLogger("EventManager")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
EventManager::~EventManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::isEventAvailable() const {
    std::lock_guard<std::mutex> guard(mutex);
    return !commandQueue.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::queueEvent(const CommandData & event) {

    std::lock_guard<std::mutex> guard(mutex);
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Queuing event" << endl;
    commandQueue.push(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::processNextEvent() {
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Checking for event" << endl;
    CommandData event;
    std::string response;
    if (consumeEvent(event)) {
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Handling event with command '" << event.command << "'" << endl;
        commandHandler.handleCommand(event.command, response);
        event.response = response;
        event.responseHandler->handleCommandResponse(event);
    }
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::consumeEvent(CommandData & event) {
    std::lock_guard<std::mutex> guard(mutex);
    if (commandQueue.empty())
        return false;

    event = commandQueue.front();
    commandQueue.pop();

    return true;
}

}
