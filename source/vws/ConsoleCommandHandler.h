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
#ifndef CONSOLE_COMMAND_HANDLER_H_
#define CONSOLE_COMMAND_HANDLER_H_
#include <string>
#include <vector>
#include <utility>
#include "CommandData.h"
#include "CommandHandler.h"

namespace vws {
class VantageWeatherStation;
class VantageConfiguration;
class VantageLogger;
class VantageStationNetwork;
class AlarmManager;

/**
 * Handle the commands that arrive on the command socket.
 */
class ConsoleCommandHandler : public CommandHandler {
public:
    /**
     * Constructor.
     *
     * @param station               The weather station object that is used to send commands to the console
     * @param configurator          The object that manages many EEPROM related commands
     * @param network               The object that is used to read and write the weather station network data
     * @param alarmManager          The object that manages the alarm thresholds and alarm triggered states
     */
    ConsoleCommandHandler(VantageWeatherStation & station,
                          VantageConfiguration & configurator,
                          VantageStationNetwork & network,
                          AlarmManager & alarmManager);

    /**
     * Destructor.
     */
    virtual ~ConsoleCommandHandler();

    /**
     * Handle a command and write the response to the provided object.
     * Note that there will always be a response.
     *
     * @param       command  The command in JSON text form
     */
    virtual void handleCommand(CommandData & command);

    /**
     * Check if the command name can be processed by this command handler.
     *
     * @param commandName The name of the command
     * @return True if this command handler recognizes this command name
     */
    virtual bool offerCommand(const CommandData & commandData);

    /**
     * Generic handler that calls the provided member function and builds the response JSON
     *
     * @param handler     Pointer to the VantageWeatherStation member function that executes the command on the console
     * @param commandData The command data including the response data
     */
    void handleNoArgCommand(bool (VantageWeatherStation::*handler)(), CommandData & commandData);

    //
    // The following handler methods process commands received on the interface socket. Each method has a preceding comment
    // that indicates which console command is being processed by the handler. There are also comment blocks that
    // identify the sections of the Vantage Serial Protocol that document the command.
    // The methods all have the same signatures:
    //
    // void methodName(commandData);
    // @param [in/out] commandData The command data including the response data
    //
    ///////////////////////
    // Testing commands  //
    ///////////////////////

    // WRD<0x12><0x4d>
    void handleQueryConsoleType(CommandData & commandData);

    // NVER and VER
    void handleQueryFirmware(CommandData & commandData);

    // RECEIVERS
    // Note that this command will have inconsistent results. For a period of time after the console exits
    // the "Receiving From..." screen the receiver list will contain data. After the time period is over
    // the receiver list will be empty. Only putting the console back on the "Receiving From..." screen will
    // the receiver list populate with data again.
    //
    void handleQueryReceiverList(CommandData & commandData);

    // RXCHECK
    void handleQueryConsoleDiagnostics(CommandData & commandData);

    ///////////////////////////
    // Current Data Commands //
    ///////////////////////////

    // HILOWS
    void handleQueryHighLows(CommandData & commandData);

    // PUTRAIN
    void handlePutYearRain(CommandData & commandData);

    // PUTET
    void handlePutYearET(CommandData & commandData);

    /////////////////////
    // EEPROM Commands //
    /////////////////////

    // EEBWR
    void handleUpdateUnits(CommandData & commandData);


    // EEBRD
    void handleQueryUnits(CommandData & commandData);

    // EEBRD
    void handleQuerySensorStations(CommandData & commandData);

    void handleRequestSensorStationsStatus(CommandData & commandData);

    // EEBRD
    void handleQueryCalibrationAdjustments(CommandData & commandData);

    void handleUpdateCalibrationAdjustments(CommandData & commandData);

    //////////////////////////
    // Calibration Commands //
    //////////////////////////

    // BAR=
    void handleUpdateBarometerReadingAndElevation(CommandData & commandData);

    // BARDATA
    void handleQueryBarometerCalibrationParameters(CommandData & commandData);

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
    void handleClearCumulativeValue(CommandData & commandData);

    // CLRHIGHS
    void handleClearHighValues(CommandData & commandData);

    // CLRLOWS
    void handleClearLowValues(CommandData & commandData);

    // EEBRD
    void handleQueryArchivePeriod(CommandData & commandData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Configuration Commands                                                                             //
    // Note: the following commands do not require an argument and are handled using handleNoArgCommand() //
    //     1. Start Archiving   (START)                                                                   //
    //     2. Stop Archiving    (STOP)                                                                    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    // GETTIME
    void handleQueryConsoleTime(CommandData & commandData);

    // SETPER
    void handleUpdateArchivePeriod(CommandData & commandData);

    // NEWSETUP
    void handleInitialization(CommandData & commandData);

    // LAMPS
    void handleBacklight(CommandData & commandData);

    //
    // Other Commands
    //
    void handleQueryConfigurationData(CommandData & commandData);

    void handleUpdateConfigurationData(CommandData & commandData);

    void handleGetTimezones(CommandData & commandData);

    void handleQueryNetworkConfiguration(CommandData & commandData);

    void handleUpdateNetworkConfiguration(CommandData & commandData);

    void handleQueryAlarmThresholds(CommandData & commandData);

    void handleUpdateAlarmThresholds(CommandData & commandData);

    void handleQueryActiveAlarms(CommandData & commandData);

    void handleQueryNetworkStatus(CommandData & commandData);

    void handleQueryTodayNetworkStatus(CommandData & commandData);

private:
    VantageWeatherStation & station;
    VantageConfiguration &  configurator;
    VantageStationNetwork & network;
    AlarmManager &          alarmManager;
    VantageLogger &         logger;
};

}

#endif
