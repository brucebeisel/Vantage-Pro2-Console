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
#include <iostream>
#include "CommandHandler.h"
#include "json.hpp"
#include "VantageWeatherStation.h"
#include "VantageConfiguration.h"
#include "HiLowPacket.h"
#include "VantageEnums.h"

using namespace std;
using json = nlohmann::json;

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
CommandHandler::CommandHandler(VantageWeatherStation & station, VantageConfiguration & configurator) : station(station), configurator(configurator) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CommandHandler::~CommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleCommand(const std::string & commandJson, std::string & responseJson) {
    try {
        json command = json::parse(commandJson.begin(), commandJson.end());
        string commandName = command.value("command", "unknown");
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
        else if (commandName == "query-highlows") {
            handleQueryHighLows(commandName, responseJson);
        }
        else if (commandName == "update-archive-period") {
            handleUpdateArchivePeriod(commandName, argumentList, responseJson);
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
    }
    catch (const std::exception & e) {
        cout << "Exception: " << e.what() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleNoArgCommand(bool (VantageWeatherStation::*handler)(), const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    if ((station.*handler)())
        oss << "\"success\"";
    else
        oss << "\"failure\"";

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryConsoleType(const std::string & commandName, std::string & responseJson) {
    string consoleType;

    ostringstream oss;
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    if (station.retrieveConsoleType(&consoleType)) {
        oss << " \"success\", \"data\" : { \"consoleType\" : \"" << consoleType << "\" }";
    }
    else {
        oss << " \"failure\"";
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
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    if (station.retrieveFirmwareDate(&firmwareDate) && station.retrieveFirmwareVersion(&firmwareVersion)) {
        oss << " \"success\", \"data\" : { \"firmwareVersion\" : \"" << firmwareVersion << "\", \"firmwareDate\" : \"" << firmwareDate << "\"}";
    }
    else {
        oss << " \"failure\"";
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
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";
    if (station.retrieveReceiverList(&sensorStations)) {
        oss << "\"success\", \"data\" : { \"receiverList\" : [";
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
        oss << "\"failure\" }";
    }

    responseJson = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleQueryHighLows(const std::string & commandName, std::string & response) {
    ostringstream oss;
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    HiLowPacket packet;
    if (station.retrieveHiLowValues(packet)) {
        oss << "\"success\", \"data\" : ";
        oss << packet.formatJSON();
    }
    else
        oss << " \"failure\"";

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleUpdateArchivePeriod(const string & commandName, const CommandArgumentList & argumentList, string & response) {
    int periodValue = 0;

    ostringstream oss;
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    for (CommandArgument arg : argumentList) {
        if (arg.first == "period")
            periodValue = atoi(arg.first.c_str());
    }

    ArchivePeriod period = static_cast<ArchivePeriod>(periodValue);

    if ((period == ArchivePeriod::ONE_MINUTE || period == ArchivePeriod::FIVE_MINUTES || period == ArchivePeriod::TEN_MINUTES ||
        period == ArchivePeriod::FIFTEEN_MINUTES || period == ArchivePeriod::THIRTY_MINUTES || period == ArchivePeriod::ONE_HOUR ||
        period == ArchivePeriod::TWO_HOURS) && station.updateArchivePeriod(period))
        oss << "\"success\"";
    else
        oss << "\"failure\"";

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

    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";
    //"success", "info" : "Light is on|off" }

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
        oss << "\"success\"";
    else
        oss << "\"failure\"";

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
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";
    if (configurator.updateUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits))
        oss << " \"success\"";
    else
        oss << " \"failure\"";

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
    oss << "{ \"response\" : \"" << commandName << "\", \"result\" : ";

    if (configurator.retrieveUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits)) {
        oss << "\"success\", \"data\" : { ";
        oss << "\"baroUnits\" : \"" << barometerUnitsEnum.valueToString(baroUnits) << "\", ";
        oss << "\"temperatureUnits\" : \"" << temperatureUnitsEnum.valueToString(temperatureUnits) << "\", ";
        oss << "\"elevationUnits\" : \"" << elevationUnitsEnum.valueToString(elevationUnits) << "\", ";
        oss << "\"rainUnits\" : \"" << rainUnitsEnum.valueToString(rainUnits) << "\", ";
        oss << "\"windUnits\" : \"" << windUnitsEnum.valueToString(windUnits) << "\" }";
    }
    else
        oss << " \"failure\"";

    oss << " }";

    response = oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearCumulativeValueCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearHighValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CommandHandler::handleClearLowValuesCommand(const std::string & commandName, const CommandArgumentList & argumentList, std::string & response) {

}

} // End namespace
