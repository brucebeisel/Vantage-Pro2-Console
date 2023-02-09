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

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include "json.hpp"

#include "CommandHandler.h"
#include "ArchiveManager.h"
#include "ArchivePacket.h"
#include "HiLowPacket.h"
#include "VantageConfiguration.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"

using namespace std;
using json = nlohmann::json;

static const string RESPONSE_TOKEN = "\"response\"";
static const string RESULT_TOKEN = "\"result\"";
static const string DATA_TOKEN = "\"data\"";
static const string SUCCESS_TOKEN = "\"success\"";
static const string FAILURE_TOKEN = "\"failure\"";

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
CommandHandler::CommandHandler(VantageWeatherStation & station, VantageConfiguration & configurator, ArchiveManager & archiveManager) : station(station),
                                                                                                                                        logger(VantageLogger::getLogger("CommandHandler")),
                                                                                                                                        configurator(configurator),
                                                                                                                                        archiveManager(archiveManager) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandHandler::~CommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleCommand(const std::string & commandJson, std::string & responseJson) {
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

        cout <<     "Command: " << commandName << endl;
        cout <<     "    Arguments:" << endl;
        for (int i = 0; i < argumentList.size(); i++) {
            cout << "          [" << i << "]: " << argumentList[i].first << "=" << argumentList[i].second << endl;
        }

        if (commandName == "query-console-type") {
            handleQueryConsoleType(commandName, responseJson);
        }
        else if (commandName == "query-firmware") {
            handleQueryFirmwareCommand(commandName, responseJson);
        }
        else if (commandName == "query-receiver-list") {
            handleQueryReceiverListCommand(commandName, responseJson);
        }
        else if (commandName == "console-diagnostics") {
            handleQueryConsoleDiagnostics(commandName, responseJson);
        }
        else if (commandName == "query-highlows") {
            handleQueryHighLows(commandName, responseJson);
        }
        else if (commandName == "put-year-rain") {
            handlePutYearRainCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "put-year-et") {
            handlePutYearETCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "update-archive-period") {
            handleUpdateArchivePeriod(commandName, argumentList, responseJson);
        }
        else if (commandName == "query-archive-period") {
            handleQueryArchivePeriod(commandName, responseJson);
        }
        else if (commandName == "update-units") {
            handleUpdateUnitsCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "query-units") {
            handleQueryUnitsCommand(commandName, responseJson);
        }
        else if (commandName == "clear-archive") {
            handleNoArgCommand(&VantageWeatherStation::clearArchive, commandName, responseJson);
        }
        else if (commandName == "clear-alarm-thresholds") {
            handleNoArgCommand(&VantageWeatherStation::clearAlarmThresholds, commandName, responseJson);
        }
        else if (commandName == "clear-calibration-offsets") {
            handleNoArgCommand(&VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets, commandName, responseJson);
        }
        else if (commandName == "clear-graph-points") {
            handleNoArgCommand(&VantageWeatherStation::clearGraphPoints, commandName, responseJson);
        }
        else if (commandName == "clear-cumulative-values") {
            handleClearCumulativeValueCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "clear-high-values") {
            handleClearHighValuesCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "clear-low-values") {
            handleClearLowValuesCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "clear-active-alarms") {
            handleNoArgCommand(&VantageWeatherStation::clearActiveAlarms, commandName, responseJson);
        }
        else if (commandName == "clear-current-data") {
            handleNoArgCommand(&VantageWeatherStation::clearCurrentData, commandName, responseJson);
        }
        else if (commandName == "backlight") {
            handleBacklightCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "clear-lows") {
            handleClearLowValuesCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "clear-highs") {
            handleClearHighValuesCommand(commandName, argumentList, responseJson);
        }
        else if (commandName == "query-archive") {
            handleQueryArchive(commandName, argumentList, responseJson);
        }
        else {
            ostringstream oss;
            oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : " << FAILURE_TOKEN << " }";
            responseJson = oss.str();
        }
    }
    catch (const std::exception & e) {
        ostringstream oss;
        oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : " << FAILURE_TOKEN << " }";
        responseJson = oss.str();
        cout << "Exception: " << e.what() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleNoArgCommand(bool (VantageWeatherStation::*handler)(), const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    if ((station.*handler)())
        oss << SUCCESS_TOKEN;
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

/******************************************************************************
 *                      TESTING COMMANDS                                      *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleType(const std::string & commandName, std::string & responseJson) {
    string consoleType;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    if (station.retrieveConsoleType(&consoleType)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"consoleType\" : \"" << consoleType << "\" }";
    }
    else {
        oss << FAILURE_TOKEN;
    }

    oss << " }";

    responseJson = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryFirmwareCommand(const std::string & commandName, std::string & responseJson) {
    string firmwareDate;
    string firmwareVersion;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    if (station.retrieveFirmwareDate(&firmwareDate) && station.retrieveFirmwareVersion(&firmwareVersion)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"firmwareVersion\" : \"" << firmwareVersion << "\", \"firmwareDate\" : \"" << firmwareDate << "\"}";
    }
    else {
        oss << FAILURE_TOKEN;
    }

    oss << " }";

    responseJson = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryReceiverListCommand(const std::string & commandName, std::string & responseJson) {
    std::vector<StationId> sensorStations;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";
    if (station.retrieveReceiverList(&sensorStations)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"receiverList\" : [";
        bool first = true;
        for (int id : sensorStations) {
            if (!first) {
                first = false;
                oss << ", ";
            }
            oss << id;
        }

        oss << " ] } }";
    }
    else {
        oss << FAILURE_TOKEN;
    }

    responseJson = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleDiagnostics(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    VantageWeatherStation::ConsoleDiagnosticReport report;
    if (station.retrieveConsoleDiagnosticsReport(report)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : {"
            << "\"consoleDiagnosticReport\" : { \"totalPacketsReceived\" : " << report.packetCount << ", "
            << "\"totalPacketsMissed\" : " << report.missedPacketCount << ", "
            << "\"resyncCount\" : " << report.syncCount << ", "
            << "\"packetReceptionHwm\" : " << report.maxPacketSequence << ", "
            << "\"crcErrorCount\" : " << report.crcErrorCount
            << " } }";
    }
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

/******************************************************************************
 *                      CURRENT DATA COMMANDS                                 *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryHighLows(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    HiLowPacket packet;
    if (station.retrieveHiLowValues(packet)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << packet.formatJSON();
    }
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handlePutYearRainCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    Rainfall yearRain = -1.0; // Use negative number as the "not set" value

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    for (CommandArgument arg : argumentList) {
        if (arg.first == "value")
            yearRain = strtod(arg.second.c_str(), NULL);
    }

    if (yearRain > 0 && station.putYearlyRain(yearRain))
        oss << SUCCESS_TOKEN;
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handlePutYearETCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    Evapotranspiration yearET = -1.0; // Use negative number as the "not set" value

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    for (CommandArgument arg : argumentList) {
        if (arg.first == "value")
            yearET = strtod(arg.second.c_str(), NULL);
    }

    if (yearET > 0 && station.putYearlyET(yearET))
        oss << SUCCESS_TOKEN;
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
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
/******************************************************************************
 *                    CLEARING COMMANDS                                       *
 ******************************************************************************/
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearCumulativeValueCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    CumulativeValue value;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

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
            oss << FAILURE_TOKEN;
    }
    catch (std::exception & e) {
        oss << FAILURE_TOKEN;
    }

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearHighValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    ExtremePeriod extremePeriod;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

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
            oss << FAILURE_TOKEN;
    }
    catch (std::exception & e) {
        oss << FAILURE_TOKEN;
    }

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearLowValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    ExtremePeriod extremePeriod;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

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
            oss << FAILURE_TOKEN;
    }
    catch (std::exception & e) {
        oss << FAILURE_TOKEN;
    }

    oss << " }";

    response = oss.str();
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
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

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
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryArchivePeriod(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    ArchivePeriod period;
    int periodValue = static_cast<int>(period);

    if (station.retrieveArchivePeriod(period)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"period\" : " << periodValue << " } ";
    }
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleBacklightCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
    bool lampOn;
    bool success = true;

    ostringstream oss;

    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

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
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateUnitsCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {

    BarometerUnits baroUnits;
    TemperatureUnits temperatureUnits;
    ElevationUnits elevationUnits;
    RainUnits rainUnits;
    WindUnits windUnits;

    //
    // First get the current units settings, then change the ones in the JSON command
    //
    configurator.retrieveUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits);

    for (CommandArgument arg : argumentList) {
        if (arg.first == "baroUnits") {
            baroUnits = barometerUnitsEnum.stringToValue(arg.second);
        }
        else if (arg.first == "temperatureUnits") {
            temperatureUnits = temperatureUnitsEnum.stringToValue(arg.second);
        }
        else if (arg.first == "elevationUnits") {
            elevationUnits = elevationUnitsEnum.stringToValue(arg.second);
        }
        else if (arg.first == "rainUnits") {
            rainUnits = rainUnitsEnum.stringToValue(arg.second);
        }
        else if (arg.first == "windUnits") {
            windUnits = windUnitsEnum.stringToValue(arg.second);
        }
        else {
             // TODO Error
        }
    }

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";
    if (configurator.updateUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits))
        oss << SUCCESS_TOKEN;
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryUnitsCommand(const std::string & commandName, std::string & response) {
    BarometerUnits baroUnits;
    TemperatureUnits temperatureUnits;
    ElevationUnits elevationUnits;
    RainUnits rainUnits;
    WindUnits windUnits;

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";

    if (configurator.retrieveUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits)) {
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { ";
        oss << "\"baroUnits\" : \"" << barometerUnitsEnum.valueToString(baroUnits) << "\", ";
        oss << "\"temperatureUnits\" : \"" << temperatureUnitsEnum.valueToString(temperatureUnits) << "\", ";
        oss << "\"elevationUnits\" : \"" << elevationUnitsEnum.valueToString(elevationUnits) << "\", ";
        oss << "\"rainUnits\" : \"" << rainUnitsEnum.valueToString(rainUnits) << "\", ";
        oss << "\"windUnits\" : \"" << windUnitsEnum.valueToString(windUnits) << "\" }";
    }
    else
        oss << FAILURE_TOKEN;

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryArchive(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {

    ostringstream oss;
    oss << "{ " << RESPONSE_TOKEN << " : \"" << commandName << "\", " << RESULT_TOKEN << " : ";
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { \"datasets\" : [ ";

    DateTime startTime = 0;
    DateTime endTime = 0;

    struct tm tm = {0};

    for (CommandArgument arg : argumentList) {
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

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query the archive with times: " << startTime << " - " << endTime << endl;
    vector<ArchivePacket> packets;
    archiveManager.queryArchiveRecords(startTime, endTime, packets);

    oss << " { \"time\" : [ ";
    bool first = true;
    for (ArchivePacket p : packets) {
        if (!first)
            oss << ", ";

        first = false;
        oss << p.getDateTime();
    }

    oss << " ]}, ";

    oss << " { \"outsideTemperature\" : [ ";
    first = true;
    for (ArchivePacket p : packets) {
        if (!first)
            oss << ", ";

        first = false;
        Measurement<Temperature> t = p.getOutsideTemperature();
        if (t.isValid())
            oss << p.getOutsideTemperature();
        else
            oss << "null";
    }

    oss << " ]} ";

    oss << "] } }";

    response = oss.str();
}

} // End namespace
