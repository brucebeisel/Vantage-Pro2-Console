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

#include "CommandHandler.h"

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include "json.hpp"

#include "Weather.h"
#include "ArchiveManager.h"
#include "ArchivePacket.h"
#include "CalibrationAdjustmentsPacket.h"
#include "HiLowPacket.h"
#include "VantageConfiguration.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"
#include "VantageStationNetwork.h"
#include "AlarmManager.h"
#include "CurrentWeatherManager.h"
#include "SummaryReport.h"
#include "WindRoseData.h"

using namespace std;
using json = nlohmann::json;

/*
 * Various strings used in the standard responses
 */
static const string RESPONSE_TOKEN = "\"response\"";
static const string RESULT_TOKEN = "\"result\"";
static const string DATA_TOKEN = "\"data\"";
static const string SUCCESS_TOKEN = "\"success\"";
static const string FAILURE_TOKEN = "\"failure\"";
static const string ERROR_TOKEN = "\"error\"";
static const string FAILURE_STRING = FAILURE_TOKEN + "," + DATA_TOKEN + " : { " + ERROR_TOKEN + " : ";
static const string CONSOLE_COMMAND_FAILURE_STRING = FAILURE_STRING + "\"Console command error\" }";

void
jsonKeyValue(json object, std::string & key, std::string & value) {
    auto iterator = object.begin();
    key = iterator.key();
    value = iterator.value();
}

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandHandler::CommandHandler(VantageWeatherStation & station,
                               VantageConfiguration & configurator,
                               ArchiveManager & archiveManager,
                               VantageStationNetwork & stationNetwork,
                               AlarmManager & alarmManager,
                               CurrentWeatherManager & cwManager) : station(station),
                                                                    logger(VantageLogger::getLogger("CommandHandler")),
                                                                    configurator(configurator),
                                                                    network(stationNetwork),
                                                                    alarmManager(alarmManager),
                                                                    currentWeatherManager(cwManager),
                                                                    archiveManager(archiveManager) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandHandler::~CommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleCommand(const std::string & commandJson, std::string & response) {
    string commandName = "parse-error";
    try {
        json command = json::parse(commandJson.begin(), commandJson.end());
        commandName = command.value("command", "unknown");
        json args = command.at("arguments");
        vector<pair<string,string>> argumentList;
        for (int i = 0; i < args.size(); i++) {
            json arg = args[i];
            std::string key, value;
            CommandArgument argument;
            jsonKeyValue(arg, argument.first, argument.second);
            argumentList.push_back(argument);
        }

        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Handling command: " << commandName << " with arguments:" << endl;
        for (int i = 0; i < argumentList.size(); i++) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "    [" << i << "]: " << argumentList[i].first << "=" << argumentList[i].second << endl;
        }

        //
        // Pre-load the response with the common fields
        //
        response = "{ " + RESPONSE_TOKEN + " : \"" + commandName + "\", " + RESULT_TOKEN + " : ";

        if (commandName == "backlight") {
            handleBacklight(commandName, argumentList, response);
        }
        else if (commandName == "clear-active-alarms") {
            handleNoArgCommand(&VantageWeatherStation::clearActiveAlarms, commandName, response);
        }
        else if (commandName == "clear-alarm-thresholds") {
            handleNoArgCommand(&VantageWeatherStation::clearAlarmThresholds, commandName, response);
        }
        else if (commandName == "clear-archive") {
            handleNoArgCommand(&VantageWeatherStation::clearArchive, commandName, response);
        }
        else if (commandName == "clear-calibration-offsets") {
            handleNoArgCommand(&VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets, commandName, response);
        }
        else if (commandName == "clear-cumulative-values") {
            handleClearCumulativeValue(commandName, argumentList, response);
        }
        else if (commandName == "clear-current-data") {
            handleNoArgCommand(&VantageWeatherStation::clearCurrentData, commandName, response);
        }
        else if (commandName == "clear-graph-points") {
            handleNoArgCommand(&VantageWeatherStation::clearGraphPoints, commandName, response);
        }
        else if (commandName == "clear-high-values") {
            handleClearHighValues(commandName, argumentList, response);
        }
        else if (commandName == "clear-highs") {
            handleClearHighValues(commandName, argumentList, response);
        }
        else if (commandName == "clear-low-values") {
            handleClearLowValues(commandName, argumentList, response);
        }
        else if (commandName == "clear-lows") {
            handleClearLowValues(commandName, argumentList, response);
        }
        else if (commandName == "console-diagnostics") {
            handleQueryConsoleDiagnostics(commandName, response);
        }
        else if (commandName == "get-timezones") {
            handleGetTimezones(commandName, response);
        }
        else if (commandName == "query-alarm-thresholds") {
            handleQueryAlarmThresholds(commandName, response);
        }
        else if (commandName == "query-archive") {
            handleQueryArchive(commandName, argumentList, response);
        }
        else if (commandName == "query-archive-summary") {
            handleQueryArchiveSummary(commandName, argumentList, response);
        }
        else if (commandName == "query-archive-period") {
            handleQueryArchivePeriod(commandName, response);
        }
        else if (commandName == "query-baro-cal-params") {
            handleQueryBarometerCalibrationParameters(commandName, response);
        }
        else if (commandName == "query-cal-adjustments") {
            handleQueryCalibrationAdjustments(commandName, response);
        }
        else if (commandName == "query-configuration-data") {
            handleQueryConfigurationData(commandName, response);
        }
        else if (commandName == "query-console-time") {
            handleQueryConsoleTime(commandName, response);
        }
        else if (commandName == "query-console-type") {
            handleQueryConsoleType(commandName, response);
        }
        else if (commandName == "query-current-weather") {
            handleQueryLoopArchive(commandName, argumentList, response);
        }
        else if (commandName == "query-firmware") {
            handleQueryFirmware(commandName, response);
        }
        else if (commandName == "query-highlows") {
            handleQueryHighLows(commandName, response);
        }
        else if (commandName == "query-network-config") {
            handleQueryNetworkConfiguration(commandName, response);
        }
        else if (commandName == "query-network-status") {
            handleQueryNetworkStatus(commandName, argumentList, response);
        }
        else if (commandName == "query-receiver-list") {
            handleQueryReceiverList(commandName, response);
        }
        else if (commandName == "query-today-network-status") {
            handleQueryTodayNetworkStatus(commandName, response);
        }
        else if (commandName == "query-units") {
            handleQueryUnits(commandName, response);
        }
        else if (commandName == "put-year-rain") {
            handlePutYearRain(commandName, argumentList, response);
        }
        else if (commandName == "put-year-et") {
            handlePutYearET(commandName, argumentList, response);
        }
        else if (commandName == "start-archiving") {
            handleNoArgCommand(&VantageWeatherStation::startArchiving, commandName, response);
        }
        else if (commandName == "stop-archiving") {
            handleNoArgCommand(&VantageWeatherStation::stopArchiving, commandName, response);
        }
        else if (commandName == "update-alarm-thresholds") {
            handleUpdateAlarmThresholds(commandName, argumentList, response);
        }
        else if (commandName == "update-archive-period") {
            handleUpdateArchivePeriod(commandName, argumentList, response);
        }
        else if (commandName == "update-baro-reading-elevation") {
            handleUpdateBarometerReadingAndElevation(commandName, argumentList, response);
        }
        else if (commandName == "update-cal-adjustments") {
            handleUpdateCalibrationAdjustments(commandName, argumentList, response);
        }
        else if (commandName == "update-configuration-data") {
            handleUpdateConfigurationData(commandName, argumentList, response);
        }
        else if (commandName == "update-network-config") {
            handleUpdateNetworkConfiguration(commandName, argumentList, response);
        }
        else if (commandName == "update-units") {
            handleUpdateUnits(commandName, argumentList, response);
        }
        else {
            response.append(buildFailureString("Unrecognized command"));
        }
    }
    catch (const std::exception & e) {
        response.append(buildFailureString(string("Console processing error: ") + e.what()));
        logger.log(VantageLogger::VANTAGE_WARNING) << "Caught exception while processing command: " << commandName << " Error: " << e.what() << endl;
    }

    //
    // Terminate the JSON element
    //
    response.append(" }");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleNoArgCommand(bool (VantageWeatherStation::*handler)(), const std::string & commandName, std::string & response) {
    if ((station.*handler)())
        response.append(SUCCESS_TOKEN);
    else
        response.append(CONSOLE_COMMAND_FAILURE_STRING);
}

/******************************************************************************
 *                      TESTING COMMANDS                                      *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleType(const std::string & commandName, std::string & response) {
    string consoleType;

    ostringstream oss;

    if (station.retrieveConsoleType(&consoleType))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"consoleType\" : \"" << consoleType << "\" }";
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryFirmware(const std::string & commandName, std::string & response) {
    string firmwareDate;
    string firmwareVersion;

    ostringstream oss;

    if (station.retrieveFirmwareDate(firmwareDate) && station.retrieveFirmwareVersion(firmwareVersion))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"firmwareVersion\" : \"" << firmwareVersion << "\", \"firmwareDate\" : \"" << firmwareDate << "\"}";
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryReceiverList(const std::string & commandName, std::string & response) {
    std::vector<StationId> sensorStations;

    ostringstream oss;
    if (station.retrieveReceiverList(sensorStations)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"receiverList\" : [";
        bool first = true;
        for (int id : sensorStations) {
            if (!first) oss << ", "; else first = false;
            oss << id;
        }

        oss << " ] }";
    }
    else {
        oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleDiagnostics(const std::string & commandName, std::string & response) {
    ostringstream oss;

    VantageWeatherStation::ConsoleDiagnosticReport report;
    if (station.retrieveConsoleDiagnosticsReport(report)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : {"
            << "\"consoleDiagnosticReport\" : { "
            << "\"totalPacketsReceived\" : " << report.packetCount << ", "
            << "\"totalPacketsMissed\" : " << report.missedPacketCount << ", "
            << "\"resyncCount\" : " << report.syncCount << ", "
            << "\"packetReceptionHwm\" : " << report.maxPacketSequence << ", "
            << "\"crcErrorCount\" : " << report.crcErrorCount
            << " } }";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

/******************************************************************************
 *                      CURRENT DATA COMMANDS                                 *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryHighLows(const std::string & commandName, std::string & response) {
    ostringstream oss;

    HiLowPacket packet;
    if (station.retrieveHiLowValues(packet)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << packet.formatJSON();
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handlePutYearRain(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    Rainfall yearRain = -1.0; // Use negative number as the "not set" value

    ostringstream oss;

    for (CommandArgument arg : argumentList) {
        if (arg.first == "value")
            yearRain = strtod(arg.second.c_str(), NULL);
    }

    if (yearRain > 0 && station.putYearlyRain(yearRain))
        oss << SUCCESS_TOKEN;
    else
        oss << buildFailureString("Invalid argument or command error");

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handlePutYearET(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    Evapotranspiration yearET = -1.0; // Use negative number as the "not set" value

    ostringstream oss;

    for (CommandArgument arg : argumentList) {
        if (arg.first == "value")
            yearET = strtod(arg.second.c_str(), NULL);
    }

    if (yearET > 0 && station.putYearlyET(yearET))
        oss << SUCCESS_TOKEN;
    else
        oss << buildFailureString("Invalid argument or command error");

    response.append(oss.str());
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
CommandHandler::handleQueryCalibrationAdjustments(const std::string & commandName, std::string & response) {
    ostringstream oss;

    CalibrationAdjustmentsPacket packet;

    if (station.retrieveCalibrationAdjustments(packet))
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << packet.formatJSON();
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateCalibrationAdjustments(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    if (argumentList.size() == 0) {
        response.append(buildFailureString("Missing argument"));
        return;
    }

    CalibrationAdjustmentsPacket packet;
    if (packet.parseJSON(argumentList[0].second)) {
        if (station.updateCalibrationAdjustments(packet)) {
            response.append(SUCCESS_TOKEN);
        }
        else {
            response.append(CONSOLE_COMMAND_FAILURE_STRING);
        }
    }
    else {
        response.append(buildFailureString("Invalid calibration adjustment JSON"));
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryBarometerCalibrationParameters(const std::string & commandName, std::string & response) {
    ostringstream oss;

    VantageWeatherStation::BarometerCalibrationParameters baroCalParams;
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

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateBarometerReadingAndElevation(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    ostringstream oss;

    Pressure baroReadingInHg = 99.0;
    int elevationFeet = -9999;

    for (CommandArgument arg : argumentList) {
        if (arg.first == "elevation") {
            elevationFeet = atoi(arg.second.c_str());
        }
        else if (arg.first == "baroReading") {
            baroReadingInHg = atof(arg.second.c_str());
        }
    }

    if (baroReadingInHg == 99.0 || elevationFeet == -9999) {
        oss << buildFailureString("Missing argument");
    }
    else if (station.updateBarometerReadingAndElevation(baroReadingInHg, elevationFeet)) {
        oss << SUCCESS_TOKEN;
    }
    else {
        oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    response.append(oss.str());
}

/******************************************************************************
 *                    CLEARING COMMANDS                                       *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearCumulativeValue(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    CumulativeValue value;

    ostringstream oss;

    try {
        bool argFound = false;
        for (CommandArgument arg : argumentList) {
            if (arg.first == "value") {
                value = cumulativeValueEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearCumulativeValue(value))
            oss << SUCCESS_TOKEN;
        else
            oss << buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << buildFailureString("Invalid argument exception");
        logger.log(VantageLogger::VANTAGE_WARNING) << "Caught exception: " << e.what() << endl;
    }

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearHighValues(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    ExtremePeriod extremePeriod;

    ostringstream oss;

    try {
        bool argFound = false;
        for (CommandArgument arg : argumentList) {
            if (arg.first == "period") {
                extremePeriod = extremePeriodEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearHighValues(extremePeriod))
            oss << SUCCESS_TOKEN;
        else
            oss << buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << buildFailureString("Invalid value for period");
    }

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearLowValues(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    ExtremePeriod extremePeriod;

    ostringstream oss;

    try {
        bool argFound = false;
        for (CommandArgument arg : argumentList) {
            if (arg.first == "period") {
                extremePeriod = extremePeriodEnum.stringToValue(arg.second);
                argFound = true;
            }
        }

        if (argFound && station.clearLowValues(extremePeriod))
            oss << SUCCESS_TOKEN;
        else
            oss << buildFailureString("Invalid argument or command error");
    }
    catch (std::exception & e) {
        oss << buildFailureString("Invalid argument exception");
        logger.log(VantageLogger::VANTAGE_WARNING) << "Caught exception: " << e.what() << endl;
    }

    response.append(oss.str());
}

/******************************************************************************
 *                   CONFIGURATION COMMANDS                                   *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateArchivePeriod(const string & commandName, const CommandArgumentList & argumentList, string & response) {
    int periodValue = 0;

    ostringstream oss;

    for (CommandArgument arg : argumentList) {
        if (arg.first == "period")
            periodValue = atoi(arg.second.c_str());
    }

    ArchivePeriod period = static_cast<ArchivePeriod>(periodValue);

    if ((period == ArchivePeriod::ONE_MINUTE || period == ArchivePeriod::FIVE_MINUTES || period == ArchivePeriod::TEN_MINUTES ||
        period == ArchivePeriod::FIFTEEN_MINUTES || period == ArchivePeriod::THIRTY_MINUTES || period == ArchivePeriod::ONE_HOUR ||
        period == ArchivePeriod::TWO_HOURS) && station.updateArchivePeriod(period))
        oss << SUCCESS_TOKEN;
    else
        oss << buildFailureString("Invalid argument or command error");

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleTime(const std::string & commandName, std::string & response) {
    ostringstream oss;
    DateTime consoleTime;

    if (station.retrieveConsoleTime(consoleTime)) {
        char timeString[100];
        struct tm tm = {0};
        Weather::localtime(consoleTime, tm);

        strftime(timeString, sizeof(timeString), "%Y-%m-%d %T", &tm);

        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"time\" : \"" << timeString << "\" } ";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryArchivePeriod(const std::string & commandName, std::string & response) {
    ostringstream oss;
    ArchivePeriod period;

    if (station.retrieveArchivePeriod(period)) {
        int periodValue = static_cast<int>(period);
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"period\" : " << periodValue << " } ";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleBacklight(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    bool lampOn;
    bool success = true;
    ostringstream oss;

    if (argumentList[0].first != "state")
        success = false;
    else {
        if (argumentList[0].second == "on")
            lampOn = true;
        else if (argumentList[0].second == "off")
            lampOn = false;
        else
            success = false;
    }

    if (success)
        success = station.controlConsoleLamp(lampOn);

    if (success)
        oss << SUCCESS_TOKEN;
    else
        oss << buildFailureString("Console command error or invalid argument");

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateUnits(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    bool success = true;
    ostringstream oss;

    //
    // First get the current units settings, then change the ones in the JSON command
    //
    UnitsSettings unitsSettings;
    configurator.retrieveUnitsSettings(unitsSettings);
    string unitType;

    try {
        for (CommandArgument arg : argumentList) {
            unitType = arg.second;
            if (arg.first == "baroUnits") {
                unitsSettings.baroUnits = barometerUnitsEnum.stringToValue(arg.second);
            }
            else if (arg.first == "temperatureUnits") {
                unitsSettings.temperatureUnits = temperatureUnitsEnum.stringToValue(arg.second);
            }
            else if (arg.first == "elevationUnits") {
                unitsSettings.elevationUnits = elevationUnitsEnum.stringToValue(arg.second);
            }
            else if (arg.first == "rainUnits") {
                unitsSettings.rainUnits = rainUnitsEnum.stringToValue(arg.second);
            }
            else if (arg.first == "windUnits") {
                unitsSettings.windUnits = windUnitsEnum.stringToValue(arg.second);
            }
            else {
                oss << buildFailureString("Invalid unit type argument " + arg.first);
                success = false;
                break;
            }
        }
    }
    catch (const std::invalid_argument & e) {
        success = false;
        oss << buildFailureString("Invalid unit value argument " + unitType);
    }

    if (success) {
        if (configurator.updateUnitsSettings(unitsSettings))
            oss << SUCCESS_TOKEN;
        else
            oss << CONSOLE_COMMAND_FAILURE_STRING;
    }

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryUnits(const std::string & commandName, std::string & response) {
    ostringstream oss;

    UnitsSettings unitsSettings;
    if (configurator.retrieveUnitsSettings(unitsSettings)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { ";
        oss << "\"baroUnits\" : \"" << barometerUnitsEnum.valueToString(unitsSettings.baroUnits) << "\", ";
        oss << "\"temperatureUnits\" : \"" << temperatureUnitsEnum.valueToString(unitsSettings.temperatureUnits) << "\", ";
        oss << "\"elevationUnits\" : \"" << elevationUnitsEnum.valueToString(unitsSettings.elevationUnits) << "\", ";
        oss << "\"rainUnits\" : \"" << rainUnitsEnum.valueToString(unitsSettings.rainUnits) << "\", ";
        oss << "\"windUnits\" : \"" << windUnitsEnum.valueToString(unitsSettings.windUnits) << "\" }";
    }
    else
        oss << CONSOLE_COMMAND_FAILURE_STRING;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConfigurationData(const std::string & commandName, std::string & response) {
    string data = configurator.retrieveAllConfigurationData();

    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << data;

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateConfigurationData(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    // TODO Implement this command
    response.append(SUCCESS_TOKEN);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    DateTime startTime = 0;
    DateTime endTime = 0;

    struct tm tm;

    for (CommandArgument arg : argumentList) {
        tm = {0};
        tm.tm_isdst = -1;
        if (arg.first == "start-time") {
            std::stringstream ss(arg.second);
            ss >> std::get_time(&tm, "%Y-%m-%dT%T");
            startTime = mktime(&tm);
        }
        else if (arg.first == "end-time") {
            std::stringstream ss(arg.second);
            ss >> std::get_time(&tm, "%Y-%m-%dT%T");
            endTime = mktime(&tm);
        }
    }

    if (startTime == 0 || endTime == 0) {
        response.append(buildFailureString("Missing argument"));
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query the archive with times: " << startTime << " - " << endTime << endl;
        vector<ArchivePacket> packets;
        archiveManager.queryArchiveRecords(startTime, endTime, packets);

        ostringstream oss;
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : [ ";

        bool first = true;
        for (ArchivePacket packet : packets) {
            if (!first) oss << ", "; else first = false;
            oss << packet.formatJSON();
        }

        oss << "]";
        response.append(oss.str());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryArchiveSummary(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    DateTime startTime = 0;
    DateTime endTime = 0;
    SummaryPeriod summaryPeriod;
    int speedBinCount = 0;
    Speed speedBinIncrement = 0.0;
    bool foundSummaryPeriodArgument = false;
    ProtocolConstants::WindUnits windUnits;
    bool foundWindUnits = false;
    struct tm tm;

    try {
        for (CommandArgument arg : argumentList) {
            tm = {0};
            tm.tm_isdst = -1;
            if (arg.first == "start-time") {
                std::stringstream ss(arg.second);
                ss >> std::get_time(&tm, "%Y-%m-%dT%T");
                startTime = mktime(&tm);
            }
            else if (arg.first == "end-time") {
                std::stringstream ss(arg.second);
                ss >> std::get_time(&tm, "%Y-%m-%dT%T");
                endTime = mktime(&tm);
            }
            else if (arg.first == "summary-period") {
                summaryPeriod = summaryPeriodEnum.stringToValue(arg.second);
                foundSummaryPeriodArgument = true;
            }
            else if (arg.first == "speed-bin-count") {
                speedBinCount = atoi(arg.second.c_str());
            }
            else if (arg.first == "speed-bin-increment") {
                speedBinIncrement = atof(arg.second.c_str());
            }
            else if (arg.first == "speed-units") {
                windUnits = windUnitsEnum.stringToValue(arg.second);
                foundWindUnits = true;
            }
        }

        if (startTime == 0 || endTime == 0 ||
            speedBinCount == 0 || speedBinIncrement == 0.0 ||
            !foundWindUnits ||
            !foundSummaryPeriodArgument)
            response.append(buildFailureString("Missing argument"));
        else {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query summaries from the archive with times: " << startTime << " - " << endTime << endl;
            WindRoseData windRoseData(windUnits, speedBinIncrement, speedBinCount);
            SummaryReport report(summaryPeriod, startTime, endTime, archiveManager, windRoseData);
            report.loadData();

            ostringstream oss;
            oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
            oss << report.formatJSON();
            response.append(oss.str());
        }
    }
    catch (const std::exception & e) {
        response.append(buildFailureString("Invalid summary period or wind speed unit"));
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryLoopArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    int hours = 1;
    for (CommandArgument arg : argumentList) {
        if (arg.first == "hours") {
            hours = atoi(arg.second.c_str());
        }
    }

    vector<CurrentWeather> list;
    currentWeatherManager.queryCurrentWeatherArchive(hours, list);
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : [ ";

    bool first = true;
    for (CurrentWeather cw : list) {
        if (!first) oss << ", "; else first = false;
        oss << cw.formatJSON();
    }

    oss << " ]";

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleGetTimezones(const std::string & commandName, std::string & response) {
    vector<string> timezoneList;
    configurator.getTimeZoneOptions(timezoneList);

    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"timezones\" : [ ";
    bool first = true;
    for (string tzName : timezoneList) {
        if (!first) oss << ", "; else first = false;
        oss << "\"" << tzName << "\"";
    }

    oss << "] }";

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryNetworkConfiguration(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
    oss << network.formatConfigurationJSON();

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateNetworkConfiguration(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    if (argumentList.size() == 0) {
        response.append(buildFailureString("Missing argument"));
        return;
    }

    if (network.updateNetworkConfiguration(argumentList[0].second)) {
        response.append(SUCCESS_TOKEN);
    }
    else {
        response.append(CONSOLE_COMMAND_FAILURE_STRING);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryAlarmThresholds(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : " << alarmManager.formatAlarmThresholdsJSON();

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateAlarmThresholds(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    for (auto arg : argumentList) {
        if (arg.second != "")
            alarmManager.setAlarmThreshold(arg.first, atof(arg.second.c_str()));
        else
            alarmManager.clearAlarmThreshold(arg.first);
    }

    response.append(SUCCESS_TOKEN);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleInitialization(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    response.append(SUCCESS_TOKEN);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryNetworkStatus(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    DateTime startTime = 0;
    DateTime endTime = 0;

    struct tm tm = {0};

    for (CommandArgument arg : argumentList) {
        if (arg.first == "start-time") {
            std::stringstream ss(arg.second);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            startTime = mktime(&tm);
        }
        else if (arg.first == "end-time") {
            std::stringstream ss(arg.second);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            endTime = mktime(&tm);
        }
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query the network status with times: " << startTime << " - " << endTime << endl;

    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
    oss << network.formatStatusJSON(startTime, endTime);

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryTodayNetworkStatus(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
    oss << network.todayNetworkStatusJSON();

    response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CommandHandler::buildFailureString(const std::string & errorString) {
    std::string failure = FAILURE_TOKEN + "," + DATA_TOKEN + " : { " + ERROR_TOKEN + " : \"" + errorString + "\" }";
    return failure;
}

} // End namespace
