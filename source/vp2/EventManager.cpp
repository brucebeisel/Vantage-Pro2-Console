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
//#include <sys/signalfd.h>
#include <signal.h>
#include "CommandHandler.h"
#include "ResponseHandler.h"
#include "EventManager.h"

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
EventManager::EventManager(CommandHandler & ch) : commandHandler(ch) {
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
    commandQueue.push(event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
EventManager::processNextEvent() {
    CommandData event;
    std::string response;
    if (consumeEvent(event)) {
        commandHandler.handleCommand(event.command, response);
        event.responseHandler->handleCommandResponse(event, response);
        // TODO what to do with the response?
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
