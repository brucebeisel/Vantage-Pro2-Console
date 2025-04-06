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

#include "ConsoleCommandHandler.h"

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>

#include "Weather.h"
#include "CalibrationAdjustmentsPacket.h"
#include "ConsoleDiagnosticReport.h"
#include "CommandData.h"
#include "HiLowPacket.h"
#include "VantageConfiguration.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"
#include "VantageStationNetwork.h"
#include "AlarmManager.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

struct ConsoleCommandEntry {
    std::string commandName;
    void (ConsoleCommandHandler::*handler)(CommandData &);
    bool (VantageWeatherStation::*consoleHandler)();
};

static const ConsoleCommandEntry consoleCommandList[] = {
    "backlight",                     &ConsoleCommandHandler::handleBacklight,                           NULL,
    "clear-active-alarms",           NULL,                                                              &VantageWeatherStation::clearActiveAlarms,
    "clear-alarm-thresholds",        NULL,                                                              &VantageWeatherStation::clearAlarmThresholds,
    "clear-console-archive",         NULL,                                                              &VantageWeatherStation::clearArchive,
    "clear-calibration-offsets",     NULL,                                                              &VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets,
    "clear-cumulative-values",       &ConsoleCommandHandler::handleClearCumulativeValue,                NULL,
    "clear-current-data",            NULL,                                                              &VantageWeatherStation::clearCurrentData,
    "clear-graph-points",            NULL,                                                              &VantageWeatherStation::clearGraphPoints,
    "clear-high-values",             &ConsoleCommandHandler::handleClearHighValues,                     NULL,
    "clear-low-values",              &ConsoleCommandHandler::handleClearLowValues,                      NULL,
    "console-diagnostics",           &ConsoleCommandHandler::handleQueryConsoleDiagnostics,             NULL,
    "get-timezones",                 &ConsoleCommandHandler::handleGetTimezones,                        NULL,
    "query-alarm-thresholds",        &ConsoleCommandHandler::handleQueryAlarmThresholds,                NULL,
    "query-active-alarms",           &ConsoleCommandHandler::handleQueryActiveAlarms,                   NULL,
    "query-archive-period",          &ConsoleCommandHandler::handleQueryArchivePeriod,                  NULL,
    "query-baro-cal-params",         &ConsoleCommandHandler::handleQueryBarometerCalibrationParameters, NULL,
    "query-cal-adjustments",         &ConsoleCommandHandler::handleQueryCalibrationAdjustments,         NULL,
    "query-configuration-data",      &ConsoleCommandHandler::handleQueryConfigurationData,              NULL,
    "query-console-time",            &ConsoleCommandHandler::handleQueryConsoleTime,                    NULL,
    "query-console-type",            &ConsoleCommandHandler::handleQueryConsoleType,                    NULL,
    "query-firmware",                &ConsoleCommandHandler::handleQueryFirmware,                       NULL,
    "query-highlows",                &ConsoleCommandHandler::handleQueryHighLows,                       NULL,
    "query-network-config",          &ConsoleCommandHandler::handleQueryNetworkConfiguration,           NULL,
    "query-network-status",          &ConsoleCommandHandler::handleQueryNetworkStatus,                  NULL,
    "query-receiver-list",           &ConsoleCommandHandler::handleQueryReceiverList,                   NULL,
    "query-station-list",            &ConsoleCommandHandler::handleQueryStationList,                    NULL,
    "query-used-transmitters",       &ConsoleCommandHandler::handleQueryMonitoredStations,              NULL,
    "query-today-network-status",    &ConsoleCommandHandler::handleQueryTodayNetworkStatus,             NULL,
    "query-units",                   &ConsoleCommandHandler::handleQueryUnits,                          NULL,
    "put-year-rain",                 &ConsoleCommandHandler::handlePutYearRain,                         NULL,
    "put-year-et",                   &ConsoleCommandHandler::handlePutYearET,                           NULL,
    "start-archiving",               NULL,                                                              &VantageWeatherStation::startArchiving,
    "stop-archiving",                NULL,                                                              &VantageWeatherStation::stopArchiving,
    "query-archiving-state",         &ConsoleCommandHandler::handleQueryArchivingState,                 NULL,
    "update-alarm-thresholds",       &ConsoleCommandHandler::handleUpdateAlarmThresholds,               NULL,
    "update-archive-period",         &ConsoleCommandHandler::handleUpdateArchivePeriod,                 NULL,
    "update-baro-reading-elevation", &ConsoleCommandHandler::handleUpdateBarometerReadingAndElevation,  NULL,
    "update-cal-adjustments",        &ConsoleCommandHandler::handleUpdateCalibrationAdjustments,        NULL,
    "update-configuration-data",     &ConsoleCommandHandler::handleUpdateConfigurationData,             NULL,
    "update-network-config",         &ConsoleCommandHandler::handleUpdateNetworkConfiguration,          NULL,
    "update-units",                  &ConsoleCommandHandler::handleUpdateUnits,                         NULL
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ConsoleCommandHandler::ConsoleCommandHandler(VantageWeatherStation & station,
                                             VantageConfiguration & configurator,
                                             VantageStationNetwork & stationNetwork,
                                             AlarmManager & alarmManager) : station(station),
                                                                            logger(VantageLogger::getLogger("ConsoleCommandHandler")),
                                                                            configurator(configurator),
                                                                            network(stationNetwork),
                                                                            alarmManager(alarmManager) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ConsoleCommandHandler::~ConsoleCommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ConsoleCommandHandler::offerCommand(const CommandData & commandData) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Being offered command " << commandData.commandName << endl;
    for (auto & entry : consoleCommandList) {
        if (commandData.commandName == entry.commandName) {
            commandQueue.queueCommand(commandData);
            logger.log(VantageLogger::VANTAGE_DEBUG3) << "Offer of command " << commandData.commandName << " accepted" << endl;
            return true;
        }
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Offer of command " << commandData.commandName << " rejected" << endl;

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleCommand(CommandData & commandData) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Processing command " << commandData << endl;

    //
    // If the station has not been opened and configured just ignore the command
    //
    if (!station.isOpen()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Ignoring command " << commandData << " because the weather station console is not open" << endl;
        commandData.response.append("\"Console not open\"");
        return;
    }

    for (auto & entry : consoleCommandList) {
        if (commandData.commandName == entry.commandName) {
            if (entry.handler != NULL) {
                (this->*entry.handler)(commandData);
            }
            else if (entry.consoleHandler != NULL)  {
                handleNoArgCommand(entry.consoleHandler, commandData);
            }
            else {
                logger.log(VantageLogger::VANTAGE_WARNING) << "handleCommand() command named '" << commandData.commandName << "' has no handler registered" << endl;
                commandData.response.append("\"Internal logic error\"");
            }

            return;
        }
    }

    logger.log(VantageLogger::VANTAGE_WARNING) << "handleCommand() received unexpected command named '" << commandData.commandName << "'" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleNoArgCommand(bool (VantageWeatherStation::*handler)(), CommandData & commandData) {
    if ((station.*handler)())
        commandData.response.append(SUCCESS_TOKEN);
    else
        commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
}

/******************************************************************************
 *                      TESTING COMMANDS                                      *
 ******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryArchivingState(CommandData & commandData) {
    ostringstream oss;
    bool archivingActive = station.getArchivingState();
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"archivingActive\" : \"" << boolalpha << archivingActive << "\" }";
    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryConsoleType(CommandData & commandData) {
    string consoleType;

    ostringstream oss;

    if (station.retrieveConsoleType(&consoleType))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"consoleType\" : \"" << consoleType << "\" }";
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryFirmware(CommandData & commandData) {
    string firmwareDate;
    string firmwareVersion;

    ostringstream oss;

    if (station.retrieveFirmwareDate(firmwareDate) && station.retrieveFirmwareVersion(firmwareVersion))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"firmwareVersion\" : \"" << firmwareVersion << "\", \"firmwareDate\" : \"" << firmwareDate << "\"}";
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryReceiverList(CommandData & commandData) {
    std::vector<StationId> sensorStations;

    ostringstream oss;
    if (station.retrieveReceiverList(sensorStations)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"receiverList\" : [";
        bool first = true;
        for (auto id : sensorStations) {
            if (!first) oss << ", "; else first = false;
            oss << id;
        }

        oss << " ] }";
    }
    else {
        oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryConsoleDiagnostics(CommandData & commandData) {
    ostringstream oss;

    ConsoleDiagnosticReport report;
    if (station.retrieveConsoleDiagnosticsReport(report))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << report.formatJSON();
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

/******************************************************************************
 *                      CURRENT DATA COMMANDS                                 *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryHighLows(CommandData & commandData) {
    ostringstream oss;

    HiLowPacket packet;
    if (station.retrieveHiLowValues(packet)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << packet.formatJSON();
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handlePutYearRain(CommandData & commandData) {
    Rainfall yearRain = -1.0; // Use negative number as the "not set" value

    ostringstream oss;

    for (auto arg : commandData.arguments) {
        if (arg.first == "value")
            yearRain = strtod(arg.second.c_str(), NULL);
    }

    if (yearRain >= 0 && station.putYearlyRain(yearRain))
        oss << SUCCESS_TOKEN;
    else
        oss << CommandData::buildFailureString("Invalid argument or command error");

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handlePutYearET(CommandData & commandData) {
    Evapotranspiration yearET = -1.0; // Use negative number as the "not set" value

    ostringstream oss;

    for (auto arg : commandData.arguments) {
        if (arg.first == "value")
            yearET = strtod(arg.second.c_str(), NULL);
    }

    if (yearET >= 0 && station.putYearlyET(yearET))
        oss << SUCCESS_TOKEN;
    else
        oss << CommandData::buildFailureString("Invalid argument or command error");

    commandData.response.append(oss.str());
}

/******************************************************************************
 *                      DOWNLOAD COMMANDS                                     *
 ******************************************************************************/
/******************************************************************************
 *                       EEPROM COMMANDS                                      *
 ******************************************************************************/
/******************************************************************************
 *                    CALIBRATION COMMANDS                                    *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryCalibrationAdjustments(CommandData & commandData) {
    ostringstream oss;

    CalibrationAdjustmentsPacket packet;

    if (station.retrieveCalibrationAdjustments(packet))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << packet.formatJSON();
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateCalibrationAdjustments(CommandData & commandData) {
    if (commandData.arguments.size() == 0) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
        return;
    }

    CalibrationAdjustmentsPacket packet;
    if (packet.parseJSON(commandData.arguments[0].second)) {
        if (station.updateCalibrationAdjustments(packet)) {
            commandData.response.append(SUCCESS_TOKEN);
        }
        else {
            commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
        }
    }
    else {
        commandData.response.append(CommandData::buildFailureString("Invalid calibration adjustment JSON"));
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryBarometerCalibrationParameters(CommandData & commandData) {
    ostringstream oss;

    BarometerCalibrationParameters baroCalParams;
    if (station.retrieveBarometerCalibrationParameters(baroCalParams)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"barometerCalibrationParameters\" : { "
            << " \"recentMeasurement\" : " << (baroCalParams.recentMeasurement / BAROMETER_SCALE) << ", "
            << " \"elevation\" : " << baroCalParams.elevation << ", "
            << " \"dewPoint\" : " << baroCalParams.dewPoint << ", "
            << " \"virtualTemperature\" : " << baroCalParams.avgTemperature12Hour << ", "
            << " \"humidityCorrectionFactor\" : " << baroCalParams.humidityCorrectionFactor << ", "
            << " \"correctionRatio\" : " << baroCalParams.correctionRatio << ", "
            << " \"offsetCorrectionFactor\" : " << baroCalParams.offsetCorrectionFactor << ", "
            << " \"fixedGain\" : " << baroCalParams.fixedGain << ", "
            << " \"fixedOffset\" : " << baroCalParams.fixedOffset
            << " } }";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateBarometerReadingAndElevation(CommandData & commandData) {
    const Pressure UNSET_PRESSURE(99.0);
    const int      UNSET_ELEVATION = -9999;

    Pressure baroReadingInHg(UNSET_PRESSURE);
    int elevationFeet = UNSET_ELEVATION;

    for (auto arg : commandData.arguments) {
        if (arg.first == "elevation") {
            elevationFeet = atoi(arg.second.c_str());
        }
        else if (arg.first == "baroReading") {
            baroReadingInHg = atof(arg.second.c_str());
        }
    }

    ostringstream oss;
    if (baroReadingInHg == UNSET_PRESSURE || elevationFeet == UNSET_ELEVATION) {
        oss << CommandData::buildFailureString("Missing argument");
    }
    else if (station.updateBarometerReadingAndElevation(baroReadingInHg, elevationFeet)) {
        oss << SUCCESS_TOKEN;
    }
    else {
        oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    commandData.response.append(oss.str());
}

/******************************************************************************
 *                    CLEARING COMMANDS                                       *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleClearCumulativeValue(CommandData & commandData) {
    CumulativeValue value;

    ostringstream oss;

    try {
        bool argFound = false;
        for (auto arg : commandData.arguments) {
            if (arg.first == "value") {
                value = cumulativeValueEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearCumulativeValue(value))
            oss << SUCCESS_TOKEN;
        else
            oss << CommandData::buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << CommandData::buildFailureString("Invalid argument exception");
        logger.log(VantageLogger::VANTAGE_WARNING) << "Caught exception: " << e.what() << endl;
    }

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleClearHighValues(CommandData & commandData) {
    ExtremePeriod extremePeriod;

    ostringstream oss;

    try {
        bool argFound = false;
        for (auto arg : commandData.arguments) {
            if (arg.first == "period") {
                extremePeriod = extremePeriodEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearHighValues(extremePeriod))
            oss << SUCCESS_TOKEN;
        else
            oss << CommandData::buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << CommandData::buildFailureString("Invalid value for period");
    }

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleClearLowValues(CommandData & commandData) {
    ExtremePeriod extremePeriod;

    ostringstream oss;

    try {
        bool argFound = false;
        for (auto arg : commandData.arguments) {
            if (arg.first == "period") {
                extremePeriod = extremePeriodEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearLowValues(extremePeriod))
            oss << SUCCESS_TOKEN;
        else
            oss << CommandData::buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << CommandData::buildFailureString("Invalid argument exception");
        logger.log(VantageLogger::VANTAGE_WARNING) << "Caught exception: " << e.what() << endl;
    }

    commandData.response.append(oss.str());
}

/******************************************************************************
 *                   CONFIGURATION COMMANDS                                   *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateArchivePeriod(CommandData & commandData) {
    int periodValue = 0;

    ostringstream oss;

    for (auto arg : commandData.arguments) {
        if (arg.first == "period")
            periodValue = atoi(arg.second.c_str());
    }

    ArchivePeriod period = static_cast<ArchivePeriod>(periodValue);

    if (period == ArchivePeriod::ONE_MINUTE || period == ArchivePeriod::FIVE_MINUTES || period == ArchivePeriod::TEN_MINUTES ||
        period == ArchivePeriod::FIFTEEN_MINUTES || period == ArchivePeriod::THIRTY_MINUTES || period == ArchivePeriod::ONE_HOUR ||
        period == ArchivePeriod::TWO_HOURS) {
        if (station.updateArchivePeriod(period))
            oss << SUCCESS_TOKEN;
        else
            oss << CommandData::buildFailureString("Command error");
    }
    else
        oss << CommandData::buildFailureString("Invalid argument");

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryConsoleTime(CommandData & commandData) {
    ostringstream oss;
    DateTimeFields consoleTime;

    if (station.retrieveConsoleTime(consoleTime))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"time\" : \"" << consoleTime.formatDateTime(true) << "\" } ";
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryArchivePeriod(CommandData & commandData) {
    ostringstream oss;
    ArchivePeriod period;

    if (station.retrieveArchivePeriod(period)) {
        int periodValue = static_cast<int>(period);
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"period\" : " << periodValue << " } ";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleBacklight(CommandData & commandData) {
    bool lampOn;
    bool success = true;
    ostringstream oss;

    if (commandData.arguments[0].first != "state")
        success = false;
    else {
        if (commandData.arguments[0].second == "on")
            lampOn = true;
        else if (commandData.arguments[0].second == "off")
            lampOn = false;
        else
            success = false;
    }

    if (success)
        success = station.controlConsoleLamp(lampOn);

    if (success)
        oss << SUCCESS_TOKEN;
    else
        oss << CommandData::buildFailureString("Console command error or invalid argument");

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateUnits(CommandData & commandData) {
    bool success = true;
    ostringstream oss;

    //
    // First get the current units settings, then change the ones in the JSON command
    //
    UnitsSettings unitsSettings;
    configurator.retrieveUnitsSettings(unitsSettings);
    string unitType;

    try {
        for (auto arg : commandData.arguments) {
            unitType = arg.second;
            if (arg.first == "baroUnits") {
                unitsSettings.setBarometerUnits(barometerUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "temperatureUnits") {
                unitsSettings.setTemperatureUnits(temperatureUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "elevationUnits") {
                unitsSettings.setElevationUnits(elevationUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "rainUnits") {
                unitsSettings.setRainUnits(rainUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "windUnits") {
                unitsSettings.setWindUnits(windUnitsEnum.stringToValue(arg.second));
            }
            else {
                oss << CommandData::buildFailureString("Invalid unit type argument " + arg.first);
                success = false;
                break;
            }
        }
    }
    catch (const std::invalid_argument & e) {
        success = false;
        oss << CommandData::buildFailureString("Invalid unit value argument " + unitType);
    }

    if (success) {
        if (configurator.updateUnitsSettings(unitsSettings))
            oss << SUCCESS_TOKEN;
        else
            oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryUnits(CommandData & commandData) {
    ostringstream oss;

    UnitsSettings unitsSettings;
    if (configurator.retrieveUnitsSettings(unitsSettings)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { " << unitsSettings.formatJSON() << " }";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryConfigurationData(CommandData & commandData) {
    ConsoleConfigurationData configData;

    ostringstream oss;
    if (configurator.retrieveAllConfigurationData(configData))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << configData.formatJSON();
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateConfigurationData(CommandData & commandData) {
    bool success = true;
    ostringstream oss;

    //
    // First get the current configuration, then change the ones in the JSON command
    //
    ConsoleConfigurationData configData;
    configurator.retrieveAllConfigurationData(configData);
    string argString;

    //
    // TODO Added argument processing for all configuration fields
    //
    try {
        for (auto arg : commandData.arguments) {
            argString = arg.second;
            if (arg.first == "baroUnits") {
                configData.unitsSettings.setBarometerUnits(barometerUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "temperatureUnits") {
                configData.unitsSettings.setTemperatureUnits(temperatureUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "elevationUnits") {
                configData.unitsSettings.setElevationUnits(elevationUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "rainUnits") {
                configData.unitsSettings.setRainUnits(rainUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "windUnits") {
                configData.unitsSettings.setWindUnits(windUnitsEnum.stringToValue(arg.second));
            }
            else if (arg.first == "latitude") {
                configData.positionData.latitude = atof(arg.second.c_str());
            }
            else if (arg.first == "longitude") {
                configData.positionData.longitude = atof(arg.second.c_str());
            }
            else if (arg.first == "elevation") {
                configData.positionData.elevation = atoi(arg.second.c_str());
            }
            else {
                oss << CommandData::buildFailureString("Invalid configuration type argument " + arg.first);
                success = false;
                break;
            }
        }
    }
    catch (const std::invalid_argument & e) {
        success = false;
        oss << CommandData::buildFailureString("Invalid unit value argument " + argString);
    }

    if (success) {
        if (configurator.updateAllConfigurationData(configData))
            oss << SUCCESS_TOKEN;
        else
            oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    commandData.response.append(oss.str());
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleGetTimezones(CommandData & commandData) {
    vector<string> timezoneList;
    configurator.getTimeZoneOptions(timezoneList);

    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"timezones\" : [ ";
    bool first = true;
    for (auto tzName : timezoneList) {
        if (!first) oss << ", "; else first = false;
        oss << "\"" << tzName << "\"";
    }

    oss << "] }";

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryStationList(CommandData & commandData) {
    ostringstream oss;
    StationList stationList;

    if (network.retrieveStationList(stationList)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << stationList.formatJSON();
        commandData.response.append(oss.str());
    }
    else
        commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryMonitoredStations(CommandData & commandData) {
    ostringstream oss;
    vector<StationId> monitoredStations;

    if (network.retrieveMonitoredStations(monitoredStations)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"monitoredStations\" : [";

        bool first = true;
        for (auto id : monitoredStations) {
            if (!first) oss << ", "; first = false;
            oss << id;
        }

        oss << "] }";
        commandData.response.append(oss.str());
    }
    else
        commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryNetworkConfiguration(CommandData & commandData) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
    oss << network.formatConfigurationJSON();

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateNetworkConfiguration(CommandData & commandData) {
    if (commandData.arguments.size() == 0) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
        return;
    }

    if (network.updateNetworkConfiguration(commandData.arguments[0].second)) {
        commandData.response.append(SUCCESS_TOKEN);
    }
    else {
        commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryAlarmThresholds(CommandData & commandData) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << alarmManager.formatAlarmThresholdsJSON();

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryActiveAlarms(CommandData & commandData) {
    commandData.response.append(SUCCESS_TOKEN).append(", ").append(DATA_TOKEN).append(" : ");
    commandData.response.append(alarmManager.formatActiveAlarmsJSON());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleUpdateAlarmThresholds(CommandData & commandData) {
    vector<AlarmManager::Threshold> thresholdList;
    AlarmManager::Threshold threshold;

    for (auto arg : commandData.arguments) {
        threshold.first = arg.first;
        threshold.second = atof(arg.second.c_str());
        thresholdList.push_back(threshold);
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Setting thresholds for " << thresholdList.size() << " alarms" << endl;
    if (alarmManager.setAlarmThresholds(thresholdList))
        commandData.response.append(SUCCESS_TOKEN);
    else
        commandData.response.append(CommandData::buildFailureString("Alarm Thresholds failed to be saved to console"));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleInitialization(CommandData & commandData) {
    commandData.response.append(SUCCESS_TOKEN);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryNetworkStatus(CommandData & commandData) {
    DateTimeFields startTime;
    DateTimeFields endTime;

    for (auto arg : commandData.arguments) {
        if (arg.first == "start-time") {
            startTime.parseDate(arg.second);
        }
        else if (arg.first == "end-time") {
            endTime.parseDate(arg.second);
        }
    }

    if (!startTime.isDateTimeValid() || !endTime.isDateTimeValid()) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query the network status with times: " << startTime.formatDateTime() << " - " << endTime.formatDateTime() << endl;

        ostringstream oss;
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << network.formatStatusJSON(startTime, endTime);

        commandData.response.append(oss.str());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ConsoleCommandHandler::handleQueryTodayNetworkStatus(CommandData & commandData) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
    oss << network.todayNetworkStatusJSON();

    commandData.response.append(oss.str());
}

} // End namespace
