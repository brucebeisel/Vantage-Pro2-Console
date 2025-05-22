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
 * The data needed to process and respond to a command.
 */
struct CommandData {
    typedef std::pair<std::string,std::string> CommandArgument; // Each argument is a name/value pair
    typedef std::vector<CommandArgument> CommandArgumentList;   // The arguments are a list of name/value pairs

    /**
     * Constructor.
     */
    CommandData();

    /**
     * Constructor.
     *
     * @param handler The response handler that will be called after the command has been processed
     */
    explicit CommandData(ResponseHandler & handler);

    /**
     * Constructor.
     *
     * @param handler The response handler that will be called after the command has been processed
     * @param fd      The file descriptor on which to write the response
     *
     */
    CommandData(ResponseHandler & handler, int socketId);

    /**
     * Set the command name and arguments from the provided JSON.
     * This will also create a partial response string based on the command name.
     *
     * @param commandJson The command in JSON format
     * @return True if the JSON is valid
     */
    bool setCommandFromJson(const std::string  & commandJson);

    /**
     * Load the response template using the existing command name.
     */
    void loadResponseTemplate();

    /**
     * Utility to build a JSON error string that can be appended onto the response.
     *
     * @param errorString The error to be embedded into the error portion of the response
     */
    static std::string buildFailureString(const std::string & errorString);

    /**
     * ostream operator.
     *
     * @param os          The ostream
     * @param commandData The data to be formatted for output
     */
    friend std::ostream & operator<<(std::ostream & os, const CommandData & commandData);

    ResponseHandler *   responseHandler;  // The response handler that will process the response
    int                 socketId;         // The unique socket identifier on which to send the response
    std::string         commandName;      // The command that was processed
    CommandArgumentList arguments;        // The arguments as a list of name/value pairs
    std::string         response;         // The response to the command
};

}

#endif
