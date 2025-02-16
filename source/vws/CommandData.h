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
#ifndef COMMAND_DATA_H
#define COMMAND_DATA_H

#include <string>
#include <utility>
#include <vector>

namespace vws {
class ResponseHandler;

static const std::string RESPONSE_TOKEN = "\"response\"";
static const std::string RESULT_TOKEN = "\"result\"";
static const std::string DATA_TOKEN = "\"data\"";
static const std::string SUCCESS_TOKEN = "\"success\"";
static const std::string FAILURE_TOKEN = "\"failure\"";
static const std::string ERROR_TOKEN = "\"error\"";
static const std::string FAILURE_STRING = FAILURE_TOKEN + "," + DATA_TOKEN + " : { " + ERROR_TOKEN + " : ";
static const std::string CONSOLE_COMMAND_FAILURE_STRING = FAILURE_STRING + "\"Console command error\" }";

/**
 * The data needed to respond to a command.
 */
struct CommandData {
    typedef std::pair<std::string,std::string> CommandArgument;
    typedef std::vector<CommandArgument> CommandArgumentList;

    CommandData();
    explicit CommandData(ResponseHandler & handler);
    CommandData(ResponseHandler & handler, int fd);

    bool setCommandFromJson(const std::string  & commandJson);

    ResponseHandler *   responseHandler;  // The response handler that will process the response
    int                 fd;               // The file descriptor on which the command was received, so the response can be sent on the same file descriptor
    std::string         commandName;      // The command that was processed
    CommandArgumentList arguments;
    std::string         response;         // The response to the command
};

}

#endif
