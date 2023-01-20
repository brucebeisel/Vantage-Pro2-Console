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
#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_
#include <string>
#include <vector>
#include <utility>

namespace vws {
class VantageWeatherStation;
class VantageConfiguration;

class CommandHandler {
public:
    /**
     * Constructor.
     */
    CommandHandler(VantageWeatherStation & station, VantageConfiguration & configurator);

    /**
     * Destructor.
     */
    virtual ~CommandHandler();

    /**
     * Handle a command and write the response to the provided string.
     * Note that there will always be a response.
     *
     * @param command  The command in JSON text form
     * @param response The string to which the JSON response will be written
     */
    void handleCommand(const std::string & command, std::string & response);

private:
    void handleQueryFirmwareCommand(const std::string & commandName, std::string & response);

    void handleQueryReceiverListCommand(const std::string & commandName, std::string & response);

    void handleUpdateUnitsCommand(const std::string & commandName, const std::vector<std::pair<std::string,std::string>> & argumentList, std::string & response);

    VantageWeatherStation & station;
    VantageConfiguration & configurator;
};

}

#endif
