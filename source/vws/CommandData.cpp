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
#include "JsonUtils.h"

#include "json.hpp"

using namespace std;
using json = nlohmann::json;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandData::CommandData(): socketId(-1), responseHandler(NULL)  {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandData::CommandData(ResponseHandler & handler): socketId(-1), responseHandler(&handler)  {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandData::CommandData(ResponseHandler & handler, int sockId) : socketId(sockId), responseHandler(&handler) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CommandData::setCommandFromJson(const std::string  & commandJson) {
    commandName = "parse-error";
    try {
        json command = json::parse(commandJson.begin(), commandJson.end());
        commandName = command.value("command", "unknown");
        json args = command.at("arguments");
        for (int i = 0; i < args.size(); i++) {
            json arg = args[i];
            CommandArgument argument;
            JsonUtils::extractJsonKeyValue(arg, argument.first, argument.second);
            arguments.push_back(argument);
        }

        loadResponseTemplate();

        return true;
    }
    catch (const std::exception & e) {
        response.append(buildFailureString(string("Console processing error: ") + e.what()));
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandData::loadResponseTemplate() {
    response = "{ " + RESPONSE_TOKEN + " : \"" + commandName + "\", " + RESULT_TOKEN + " : ";
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CommandData::buildFailureString(const std::string & errorString) {
    std::string failure = FAILURE_TOKEN + "," + DATA_TOKEN + " : { " + ERROR_TOKEN + " : \"" + errorString + "\" }";
    return failure;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::ostream &
operator<<(std::ostream & os, const CommandData & commandData) {
    os << "Command Name: " << commandData.commandName << " socketId: " << commandData.socketId << " Arguments: ( ";
    for (auto arg : commandData.arguments)
        os << " [" << arg.first << "=" << arg.second << "], ";

    os << " )";

    return os;
}

}
