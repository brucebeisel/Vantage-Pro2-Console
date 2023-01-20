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
#include <iostream>
#include "CommandHandler.h"
#include "json.hpp"
#include "VantageWeatherStation.h"
#include "VantageConfiguration.h"
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
            pair<string,string> argument;
            jsonKeyValue(arg, argument.first, argument.second);
            argumentList.push_back(argument);
        }

        cout <<     "Command: " << commandName << endl;
        cout <<     "    Arguments:" << endl;
        for (int i = 0; i < argumentList.size(); i++) {
            cout << "          [" << i << "]: " << argumentList[i].first << "=" << argumentList[i].second << endl;
        }

        if (commandName == "backlight") {
        }
        else if (commandName == "query-firmware") {
            handleQueryFirmwareCommand(commandName, responseJson);
        }
        else if (commandName == "query-receiver-list") {
            handleQueryReceiverListCommand(commandName, responseJson);
        }
        else if (commandName == "update-units") {
            handleUpdateUnitsCommand(commandName, argumentList, responseJson);
        }
    }
    catch (const std::exception & e) {
        cout << "Exception: " << e.what() << endl;
    }
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
        oss << " \"success\", \"data\" : { \"firmware-version\" : \"" << firmwareVersion << "\", \"firmware-date\" : \"" << firmwareDate << "\"}";
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
        oss << "\"success\", \"data\" : { \"receiver-list\" : [";
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
CommandHandler::handleUpdateUnitsCommand(const std::string & commandName, const vector<pair<string,string>> & argumentList, std::string & response) {

    BarometerUnits baroUnits = BarometerUnits::IN_HG;
    TemperatureUnits temperatureUnits = TemperatureUnits::TENTH_DEGREES;
    ElevationUnits elevationUnits = ElevationUnits::FEET;
    RainUnits rainUnits = RainUnits::INCHES;
    WindUnits windUnits = WindUnits::MPH;

    //
    // First get the current units settings, then change the ones in the JSON command
    //
    configurator.retrieveUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits);

    for (pair<string,string> arg : argumentList) {
        if (arg.first == "baro-units") {
        }
        else if (arg.first == "temperature-units") {
        }
        else if (arg.first == "elevation-units") {
        }
        else if (arg.first == "rain-units") {
            rainUnits = rainUnitsEnum.stringToValue(arg.second);
        }
        else if (arg.first == "wind-units") {
        }
        else {
             // Error
        }
    }

    configurator.updateUnitsSettings(baroUnits, temperatureUnits, elevationUnits, rainUnits, windUnits);
}
}
