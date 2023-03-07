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
class VantageStationNetwork;
class AlarmManager;
class CurrentWeatherManager;

class CommandHandler {
public:
    typedef std::pair<std::string,std::string> CommandArgument;
    typedef std::vector<CommandArgument> CommandArgumentList;

    /**
     * Constructor.
     *
     * @param station               The weather station object that is used to send commands to the console
     * @param configurator          The object that manages many EEPROM related commands
     * @param archiveManager        The object that is used to retrieve historical data from the archive
     * @param network               The object that is used to read and write the weather station network data
     * @param alarmManager          The object that manages the alarm thresholds and alarm triggered states
     * @param currentWeatherManager The object that is used to retrieve historical current weather data
     */
    CommandHandler(VantageWeatherStation & station,
                   VantageConfiguration & configurator,
                   ArchiveManager & archiveManager,
                   VantageStationNetwork & network,
                   AlarmManager & alarmManager,
                   CurrentWeatherManager & currentWeatherManager);

    /**
     * Destructor.
     */
    virtual ~CommandHandler();

    /**
     * Handle a command and write the response to the provided string.
     * Note that there will always be a response.
     *
     * @param       command  The command in JSON text form
     * @param [out] response The string to which the JSON response will be written
     */
    void handleCommand(const std::string & command, std::string & response);

private:

    /**
     * Generic handler that calls the provided member function and builds the response JSON
     *
     * @param handler         Pointer to the VantageWeatherStation member function that executes the command on the console
     * @param commandName     The name of the command from the JSON string
     * @param [out] response  The string into which the response will be written
     */
    void handleNoArgCommand(bool (VantageWeatherStation::*handler)(), const std::string & commandName, std::string & response);

    //
    // The following handler methods process commands received on the interface socket. Each method has a preceding comment
    // that indicates which console command is being processed by the handler. There are also comment blocks that
    // identify the sections of the Vantage Serial Protocol that document the command.
    // The methods have one of two signatures:
    //
    // void methodName(commandName, response)
    // @param commandName The name of the command to be processed
    // @param [out]       The response to the command
    //
    // void methodName(commandName, arguments, response)
    // @param commandName The name of the command to be processed
    // @param arguments   The arguments to the command
    // @param [out]       The response to the command
    //
    ///////////////////////
    // Testing commands  //
    ///////////////////////

    // WRD<0x12><0x4d>
    void handleQueryConsoleType(const std::string & commandName, std::string & response);

    // NVER and VER
    void handleQueryFirmware(const std::string & commandName, std::string & response);

    // RECEIVERS
    // Note that this command will have inconsistent results. For a period of time after the console exits
    // the "Receiving From..." screen the receiver list will contain data. After the time period is over
    // the receiver list will be empty. Only putting the console back on the "Receiving From..." screen will
    // the receiver list populate with data again.
    //
    void handleQueryReceiverList(const std::string & commandName, std::string & response);

    // RXCHECK
    void handleQueryConsoleDiagnostics(const std::string & commandName, std::string & response);

    ///////////////////////////
    // Current Data Commands //
    ///////////////////////////

    // HILOWS
    void handleQueryHighLows(const std::string & commandName, std::string & response);

    // PUTRAIN
    void handlePutYearRain(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // PUTET
    void handlePutYearET(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    /////////////////////
    // EEPROM Commands //
    /////////////////////

    // EEBWR
    void handleUpdateUnits(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);


    // EEBRD
    void handleQueryUnits(const std::string & commandName, std::string & response);

    // EEBRD
    void handleQuerySensorStations(const std::string & commandName, std::string & response);

    void handleRequestSensorStationsStatus(const std::string & commandName, std::string & response);

    // EEBRD
    void handleQueryCalibrationAdjustments(const std::string & commandName, std::string & response);

    void handleUpdateCalibrationAdjustments(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    //////////////////////////
    // Calibration Commands //
    //////////////////////////

    // BAR=
    void handleUpdateBarometerReadingAndElevation(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // BARDATA
    void handleQueryBarometerCalibrationParameters(const std::string & commandName, std::string & response);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clearing Commands.                                                                                 //
    // Note: the following commands do not require an argument and are handled using handleNoArgCommand() //
    //     1. Clear Archive             (CLRLOG)                                                          //
    //     2. Clear Alarm Thresholds    (CLRALM)                                                          //
    //     3. Clear Calibration Offsets (CLRCAL)                                                          //
    //     4. Clear Graph Points        (CLRGRA)                                                          //
    //     5. Clear Active Alarms       (CLRBITS)                                                         //
    //     6. Clear Current Data        (CLRDATA)                                                         //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    // CLRVAR
    void handleClearCumulativeValue(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // CLRHIGHS
    void handleClearHighValues(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // CLRLOWS
    void handleClearLowValues(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // EEBRD
    void handleQueryArchivePeriod(const std::string & commandName, std::string & response);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Configuration Commands                                                                             //
    // Note: the following commands do not require an argument and are handled using handleNoArgCommand() //
    //     1. Start Archiving   (START)                                                                   //
    //     2. Stop Archiving    (STOP)                                                                    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    // GETTIME
    void handleQueryConsoleTime(const std::string & commandName, std::string & response);

    // SETPER
    void handleUpdateArchivePeriod(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // NEWSETUP
    void handleInitialization(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    // LAMPS
    void handleBacklight(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    //
    // Other Commands
    //
    void handleQueryConfigurationData(const std::string & commandName, std::string & response);

    void handleUpdateConfigurationData(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleQueryArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleQueryLoopArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleGetTimezones(const std::string & commandName, std::string & response);

    void handleQueryNetworkConfiguration(const std::string & commandName, std::string & response);

    void handleQueryAlarmThresholds(const std::string & commandName, std::string & response);

    void handleUpdateAlarmThresholds(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleQueryNetworkStatus(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response);

    void handleQueryTodayNetworkStatus(const std::string & commandName, std::string & response);


    VantageWeatherStation & station;
    VantageConfiguration &  configurator;
    ArchiveManager &        archiveManager;
    VantageStationNetwork & network;
    AlarmManager &          alarmManager;
    CurrentWeatherManager & currentWeatherManager;
    VantageLogger &         logger;
};

}

#endif
