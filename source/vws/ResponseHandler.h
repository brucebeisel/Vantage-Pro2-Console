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
#ifndef RESPONSE_HANDLER_H
#define RESPONSE_HANDLER_H

#include <string>

namespace vws {
class ResponseHandler;
class CommandData;

/**
 * The data needed to respond to a command.
struct CommandData {
    ResponseHandler * responseHandler;  // The response handler that will process the response
    int               fd;               // The file descriptor on which the command was received, so the response can be sent on the same file descriptor
    std::string       command;          // The command that was processed
    std::string       response;         // The response to the command
};
 */

/**
 * Interface class that handles the command response.
 */
class ResponseHandler {
public:
    /**
     * Virtual destructor.
     */
    virtual ~ResponseHandler() {};

    /**
     * Handle a command response.
     *
     * @param commandData The data that described the command and the source of the command
     */
    virtual void handleCommandResponse(const CommandData & commandData) = 0;
};
}
#endif
