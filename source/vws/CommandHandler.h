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
class ArchiveManager;
class VantageLogger;

class CommandHandler {
public:
    typedef std::pair<std::string,std::string> CommandArgument;
    typedef std::vector<CommandArgument> CommandArgumentList;

    /**
     * Constructor.
     */
    CommandHandler(VantageWeatherStation & station, VantageConfiguration & configurator, ArchiveManager & archiveManager);

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

    /**
     * Generic handler that calls the provided member function and builds the response JSON
     *
     * @param handle      Pointer to the VantageWeatherStation member function that executes the command on the console
     * @param commandName The name of the command from the JSON string
     * @param response    The string into which the response will be written
     */
    void handleNoArgCommand(bool (VantageWeatherStation::*handler)(), const std::string & commandName, std::string & response);

    //
    // Testing commands
    //
    void handleQueryConsoleType(const std::string & commandName, std::string & response);

    void handleQueryFirmwareCommand(const std::string & commandName, std::string & response);

    void handleQueryReceiverListCommand(const std::string & commandName, std::string & response);

    void handleQueryConsoleDiagnostics(const std::string & commandName, std::string & response);

    //
    // Current Data Commands
    //
    void handleQueryHighLows(const std::string & commandName, std::string & response);

    void handlePutYearRainCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handlePutYearETCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    //
    // EEPROM Commands
    //
    void handleUpdateUnitsCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleQueryUnitsCommand(const std::string & commandName, std::string & response);

    void handleQuerySensorStations(const std::string & commandName, std::string & response);

    void handleRequestSensorStationsStatus(const std::string & commandName, std::string & response);

    //
    // Clearing Commands. Note: the following commands do not require an argument and are handled using handleNoArgCommand()
    //     1. Clear Archive
    //     2. Clear Alarm Thresholds
    //     3. Clear Calibration Offsets
    //     4. Clear Graph Points
    //     5. Clear Active Alarms
    //     6. Clear Current Data
    //
    void handleClearCumulativeValueCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleClearHighValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleClearLowValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    //
    // Configuration Commands
    //
    void handleUpdateArchivePeriod(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleBacklightCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    //
    // Other Commands
    //
    void handleQueryArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    VantageWeatherStation & station;
    VantageConfiguration &  configurator;
    ArchiveManager &        archiveManager;
    VantageLogger &         logger;
};

}

#endif