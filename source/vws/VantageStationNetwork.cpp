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

#include <sys/stat.h>
#include <iostream>

#include "VantageStationNetwork.h"
#include "BitConverter.h"
#include "VantageDecoder.h"
#include "VantageEepromConstants.h"
#include "VantageEnums.h"
#include "VantageWeatherStation.h"
#include "WeatherTypes.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;
using namespace ProtocolConstants;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::VantageStationNetwork(VantageWeatherStation & station, const string & file) : station(station),
                                                                                                     networkFile(file),
                                                                                                     windSensorStationId(UNKNOWN_STATION_ID),
                                                                                                     logger(VantageLogger::getLogger("VantageStationNetwork")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::~VantageStationNetwork() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::initializeNetwork() {
    //
    // See if the saved file exists
    //
    struct stat st;
    if (stat(networkFile.c_str(), &st) == 0)
        return initializeNetworkFromFile();
    else
        return initializeNetworkFromConsole();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::initializeNetworkFromFile() {
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::findRepeaters() {
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        if (sensorStationData[i].repeaterId != RepeaterId::NO_REPEATER) {
            RepeaterId repeaterId = sensorStationData[i].repeaterId;
            RepeaterNode node;
            if (repeaters.find(sensorStationData[i].repeaterId) == repeaters.end()) {
                node.repeaterId = repeaterId;
                node.endPoint = true;
                node.impliedExistance = false;
            }
            else
                node = repeaters[repeaterId];

            node.connectedStations.push_back(sensorStationData[i].stationId);

            repeaters[node.repeaterId] = node;
        }
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Found " << repeaters.size() << " repeaters in the weather station network" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::createSensorStationChains() {
    if (repeaters.size() == 0) {
        logger.log(VantageLogger::VANTAGE_INFO) << "No repeaters with which to make sensor station chains" << endl;
        return;
    }

    //
    // For each repeater, look for a repeater that precedes it in the repeater container.
    // If it does not exist, then add it to the chain.
    // That is, if we find Repeater C, and Repeater B does not exist, create it and
    // add it to the chain for Repeater C. Then continue the process, looking for
    // Repeater A. Note that a chain has a maximum of 4 repeaters in a chain. This is
    // due to communication/delay concerns.
    //
    for (RepeaterNodeMap::iterator it = repeaters.begin(); it != repeaters.end(); ++it) {
        SensorStationChain chain;
        chain.endPoint = it->first;
        chain.hasRepeater = true;
        chain.repeaters.push_back(it->first);

        RepeaterId previousRepeaterId = it->first;
        do {
            previousRepeaterId = repeaterIdEnum.previousValue(previousRepeaterId);
            if (previousRepeaterId != RepeaterId::NO_REPEATER && repeaters.find(previousRepeaterId) == repeaters.end()) {
                chain.repeaters.push_back(previousRepeaterId);
            }
        }
        while(previousRepeaterId != RepeaterId::NO_REPEATER && chain.repeaters.size() < MAX_REPEATERS_PER_CHAIN);

        chains[chain.endPoint] = chain;
    }

    //
    // Now create the repeater nodes that were implicitly identified
    //
    for (SensorStationChainMap::iterator it = chains.begin(); it != chains.end(); ++it) {
        for (int i = 1; i < it->second.repeaters.size(); i++) {
            RepeaterNode node;
            node.repeaterId = it->second.repeaters[i];
            node.endPoint = false;
            node.impliedExistance = true;
            repeaters[node.repeaterId] = node;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::initializeNetworkFromConsole() {
    //
    // Get the raw data from the console
    //
    if (!retrieveSensorStationInfo())
        return false;

    //
    // Create the network from the raw data
    //
    SensorStationChain noRepeaterChain;
    findRepeaters();
    createSensorStationChains();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveSensorStationInfo() {
    char buffer[EE_STATION_LIST_SIZE];

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving sensor station information" << endl;

    if (!station.eepromBinaryRead(EE_STATION_LIST_ADDRESS, EE_STATION_LIST_SIZE, buffer))
        return false;

    if (!station.eepromBinaryRead(EE_USED_TRANSMITTERS_ADDRESS, 1, &transmitterMask))
        return false;

    windSensorStationId = UNKNOWN_STATION_ID;
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        sensorStationData[i].stationId = i + 1;
        sensorStationData[i].repeaterId = static_cast<RepeaterId>(BitConverter::getUpperNibble(buffer[i * 2]));
        sensorStationData[i].stationType = static_cast<SensorStationType>(BitConverter::getLowerNibble(buffer[i * 2]));
        sensorStationData[i].extraTemperatureIndex = BitConverter::getLowerNibble(buffer[(i * 2) + 1]);
        sensorStationData[i].extraHumidityIndex = BitConverter::getLowerNibble(buffer[(i * 2) + 1]);

        if (sensorStationData[i].stationType == SensorStationType::ANEMOMETER_STATION)
            windSensorStationId = i + 1;
        else if (sensorStationData[i].stationType == SensorStationType::INTEGRATED_SENSOR_STATION && windSensorStationId == UNKNOWN_STATION_ID)
            windSensorStationId = i + 1;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::formatJSON() const {
    ostringstream oss;

    oss << "{ \"weatherStationNetwork\" : { "
        << " \"chains\" : [ ";

    for (SensorStationChainMap::const_iterator it = chains.begin(); it != chains.end(); ++it) {
        oss << " { ";

    }


    oss << " ] } }";

    return oss.str();
}

} /* namespace vws */
