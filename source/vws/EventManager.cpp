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

#include "CommandData.h"
#include "CommandHandler.h"
#include "ResponseHandler.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
EventManager::EventManager(/*CommandHandler & ch*/) : /*commandHandler(ch),*/ logger(VantageLogger::getLogger("EventManager")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
EventManager::~EventManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::isEventAvailable() const {
    std::scoped_lock<std::mutex> guard(mutex);
    return !commandQueue.empty();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::offerEvent(const CommandData & event) {
    bool eventAccepted = false;

    /*
    if (commandHandler.isCommandNameForHandler(event.commandName)) {
        queueEvent(event);
        eventAccepted = true;
    }
    */

    return eventAccepted;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::queueEvent(const CommandData & event) {
    {
        std::scoped_lock<std::mutex> guard(mutex);
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Queuing event" << endl;
        commandQueue.push(event);
    }

    cv.notify_all();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::processNextEvent() {
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Checking for event" << endl;
    CommandData event;
    std::string response;
    if (lockAndConsumeEvent(event)) {
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Handling event with command '" << event.commandName << "'" << endl;
        //commandHandler.handleCommand(event);
        event.response = response;
        event.responseHandler->handleCommandResponse(event);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::lockAndConsumeEvent(CommandData & event) {
    std::scoped_lock<std::mutex> guard(mutex);
    return consumeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::consumeEvent(CommandData & event) {
    if (commandQueue.empty())
        return false;

    event = commandQueue.front();
    commandQueue.pop();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
EventManager::waitForEvent(CommandData & event) {
    std::unique_lock<std::mutex> guard(mutex);

    cv.wait(guard);

    return consumeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::interrupt() {
    cv.notify_all();
}

}
