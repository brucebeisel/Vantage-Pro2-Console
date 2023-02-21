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
#include "LoopPacket.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;
using namespace ProtocolConstants;

static const char *SENSOR_NAMES[] = {
    "ANEMOMETER",
    "BAROMETER",
    "HYGROMETER",
    "LEAF_WETNESS",
    "LEAF_TEMPERATURE",
    "RAIN_COLLECTOR",
    "SOLAR_RADIATION",
    "SOIL_MOISTURE",
    "SOIL_TEMPERATURE",
    "THERMOMETER",
    "ULTRAVIOLET"
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::VantageStationNetwork(VantageWeatherStation & station, const string & file) : station(station),
                                                                                                     monitoredStationMask(0),
                                                                                                     networkFile(file),
                                                                                                     windStationLinkQuality(0),
                                                                                                     windStationId(UNKNOWN_STATION_ID),
                                                                                                     firstLoopPacketReceived(false),
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
    station.addLoopPacketListener(*this);

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
bool
VantageStationNetwork::processLoopPacket(const LoopPacket & packet) {
    for (auto station : stations) {
        station.second.isBatteryGood = packet.isTransmitterBatteryGood(station.second.stationData.stationId - 1);
    }

    console.batteryVoltage = packet.getConsoleBatteryVoltage();

    //
    // Deduce the sensor existence and locations based on the loop packet data
    //
    if (!firstLoopPacketReceived) {
        firstLoopPacketReceived = true;
        detectSensors(packet);
        console.consoleType = station.getConsoleType();
        cout << "============== NETWORK ================" << endl << formatJSON() << endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::processLoop2Packet(const Loop2Packet & packet) {
    //
    // Nothing in the LOOP2 packet is of interest to this class
    //
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::detectSensors(const LoopPacket & packet) {

    StationId issId = UNKNOWN_STATION_ID;

    for (int i = 0; i < MAX_STATION_ID; i++) {
        if (stationData[i].stationType == StationType::INTEGRATED_SENSOR_STATION)
            issId = stationData[i].stationId;
    }

    Sensor sensor;

    //
    // Add the sensors that are attached to the ISS
    //
    sensor.name = "Rain Collector";
    sensor.sensorType = SensorType::RAIN_COLLECTOR;
    sensor.onStationId = issId;
    stations[issId].connectedSensors.push_back(sensor);

    if (packet.getOutsideTemperature().isValid()) {
        sensor.name = "Outside Temperature";
        sensor.sensorType = SensorType::THERMOMETER;
        sensor.onStationId = issId;
        stations[issId].connectedSensors.push_back(sensor);
    }

    if (packet.getOutsideHumidity().isValid()) {
        sensor.name = "Outside Humidity";
        sensor.sensorType = SensorType::HYGROMETER;
        sensor.onStationId = issId;
        stations[issId].connectedSensors.push_back(sensor);
    }

    if (packet.getSolarRadiation().isValid()) {
        sensor.name = "Solar Radiation";
        sensor.sensorType = SensorType::SOLAR_RADIATION_SENSOR;
        sensor.onStationId = issId;
        stations[issId].connectedSensors.push_back(sensor);
    }

    if (packet.getUvIndex().isValid()) {
        sensor.name = "UV Index";
        sensor.sensorType = SensorType::ULTRAVIOLET_SENSOR;
        sensor.onStationId = issId;
        stations[issId].connectedSensors.push_back(sensor);
    }

    //
    // See what sensors the console supports
    //
    if (packet.getInsideTemperature().isValid()) {
        sensor.name = "Inside Temperature";
        sensor.sensorType = SensorType::THERMOMETER;
        sensor.onStationId = 0;       // TODO determine the station ID of the console
        console.connectedSensors.push_back(sensor);
    }

    if (packet.getInsideHumidity().isValid()) {
        sensor.name = "Inside Humidity";
        sensor.sensorType = SensorType::HYGROMETER;
        sensor.onStationId = 0;       // TODO determine the station ID of the console
        console.connectedSensors.push_back(sensor);
    }

    if (packet.getBarometricPressure().isValid()) {
        sensor.name = "Barometric Pressure";
        sensor.sensorType = SensorType::BAROMETER;
        sensor.onStationId = 0;       // TODO determine the station ID of the console
        console.connectedSensors.push_back(sensor);
    }

    //
    // Add the anemometer to the proper sensor station
    //
    sensor.name = "Wind";
    sensor.sensorType = SensorType::ANEMOMETER;
    if (windStationId == issId)
        sensor.onStationId = issId;
    else
        sensor.onStationId = windStationId;

    stations[sensor.onStationId].connectedSensors.push_back(sensor);

    //
    // Now find the sensors on the leaf/soil sensor stations.
    // Not sure how the values map to the sensor stations.
    //

    //
    // Now find the sensors on the temperature/humidity sensor stations
    //
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::findRepeaters() {
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        if (stationData[i].repeaterId != RepeaterId::NO_REPEATER) {
            RepeaterId repeaterId = stationData[i].repeaterId;
            Repeater node;
            if (repeaters.find(stationData[i].repeaterId) == repeaters.end()) {
                node.repeaterId = repeaterId;
                node.endPoint = true;
                node.impliedExistance = false;
            }
            else
                node = repeaters[repeaterId];

            node.connectedStations.push_back(stationData[i].stationId);

            repeaters[node.repeaterId] = node;
        }
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Found " << repeaters.size() << " repeaters in the weather station network" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::createRepeaterChains() {
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
    for (RepeaterMap::iterator it = repeaters.begin(); it != repeaters.end(); ++it) {
        RepeaterChain chain;
        chain.name = repeaterIdEnum.valueToString(it->first);
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
    for (RepeaterChainMap::iterator it = chains.begin(); it != chains.end(); ++it) {
        for (int i = 1; i < it->second.repeaters.size(); i++) {
            Repeater repeater;
            repeater.repeaterId = it->second.repeaters[i];
            repeater.endPoint = false;
            repeater.impliedExistance = true;
            repeaters[repeater.repeaterId] = repeater;
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
    if (!retrieveStationInfo())
        return false;

    //
    // Build the sensor station map
    //
    Station station;
    for (int i = 0; i < MAX_STATIONS; i++) {
        if (stationData[i].stationType != StationType::NO_STATION) {
            station.stationData = stationData[i];
            station.isBatteryGood = true;
            if (stationData[i].stationType == StationType::INTEGRATED_SENSOR_STATION)
                station.name = "ISS";

            stations[stationData[i].stationId] = station;
        }
    }

    //
    // Create the network from the raw data
    //
    findRepeaters();
    createRepeaterChains();

    //
    // Add the special console chain that contains no repeaters
    //
    RepeaterChain chain;
    chain.name = "Console";
    chain.hasRepeater = false;
    chain.endPoint = RepeaterId::NO_REPEATER;

    //
    // Add the stations that directly communicate with the console to the
    // Console object and to the chain without any repeaters.
    //
    for (auto station : stations) {
        if (station.second.stationData.repeaterId == RepeaterId::NO_REPEATER) {
            chain.chainStations.push_back(station.second.stationData.stationId);
            console.connectedStations.push_back(station.second.stationData.stationId);
        }
    }

    chains[RepeaterId::NO_REPEATER] = chain;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveStationInfo() {
    char buffer[EE_STATION_LIST_SIZE];

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving sensor station information" << endl;

    if (!station.eepromBinaryRead(EE_STATION_LIST_ADDRESS, EE_STATION_LIST_SIZE, buffer))
        return false;

    if (!station.eepromBinaryRead(EE_USED_TRANSMITTERS_ADDRESS, 1, &monitoredStationMask))
        return false;

    windStationId = UNKNOWN_STATION_ID;
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        stationData[i].stationId = i + 1;
        stationData[i].repeaterId = static_cast<RepeaterId>(BitConverter::getUpperNibble(buffer[i * 2]));
        stationData[i].stationType = static_cast<StationType>(BitConverter::getLowerNibble(buffer[i * 2]));
        stationData[i].extraTemperatureIndex = BitConverter::getLowerNibble(buffer[(i * 2) + 1]);
        stationData[i].extraHumidityIndex = BitConverter::getLowerNibble(buffer[(i * 2) + 1]);

        if (stationData[i].stationType == StationType::ANEMOMETER_STATION)
            windStationId = i + 1;
        else if (stationData[i].stationType == StationType::INTEGRATED_SENSOR_STATION && windStationId == UNKNOWN_STATION_ID)
            windStationId = i + 1;
    }

    cout << "++++++++ STATION DATA +++++++" << endl;
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        cout << "ID: "  << stationData[i].stationId
             << " Repeater ID: " << stationData[i].repeaterId
             << " StationType: " << stationData[i].stationType
             << " Extra Humidity: " << stationData[i].extraHumidityIndex
             << " Extra Temperature: " << stationData[i].extraTemperatureIndex << endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
        { "weatherStationNetwork" :
            {
                "monitoredStationIds" : [ 1, 2, 3, 4, 8],  // Theoretically each of the stations listed here must be in the network data
                "console" : {
                    "type" : "Vantage Pro2", "stations" : [ 1, 2, 3 ]
                },
                "chains" : [
                    { "name" : "Repeater C", "repeaters" : [ "Repeater A", "Repeater B" : "Repeater C" ], "stations" : [ 1, 2, 8 ] }, // The default name is the last node
                    { "name" : "Repeater D", "repeaters" : [ "Repeater D" ], "stations" : [ 3, 4 ] },
                ],
                "repeaters" : [
                    { "repeater" : "Repeater A", "stations" : [1, 2] },
                    { "repeater" : "Repeater B", "stations" : [8] },
                    { "repeater" : "Repeater C", "stations" : [] },
                    { "repeater" : "Repeater D", "stations" : [3, 4] }
                ],
                "stations" : [
                    { "station" : "ISS", "type" : "ISS", "id" : 1,                       // The name for the ISS can be hard-coded as there can only be one
                        "sensors" : [
                            { "sensor" : "Outside Temperature", "type" : "Thermometer" },
                            { "sensor" : "Outside Humidity", "type" : "Hygrometer" },
                            { "sensor" : "Solar Radiation", "type" : "Solar Radiation" },
                            { "sensor" : "UV Index", "type" : "Ultra-violet" },
                            { "sensor" : "Rain", "type" : "Rain" },
                            { "sensor" : "Wind", "type" : "Anemometer" },
                            { "sensor" : "Barometer", "type" : "Barometer" },
                            { "sensor" : "Inside Temperature", "type" : "Thermometer" },
                            { "sensor" : "Inside Humidity", "type" : "Hygrometer" }
                        ]
                    }
                    { "station" : "Pool", "type" : "Temperature", "id" : 2, "sensors" : [] }
                ]
            }
        }
*/
std::string
VantageStationNetwork::formatJSON() const {
    ostringstream oss;

    oss << "{ \"weatherStationNetwork\" : { ";

    bool first = true;
    oss << " \"monitoredStationIds\" : [";
    for (int i = 0; i < MAX_STATIONS; i++) {
        if (monitoredStationMask & (1 << i) != 0) {
            if (!first)
                oss << ", ";

            oss << (i + 1);
            first = false;
        }
    }
    oss << "], ";

    oss << " \"console\" : { \"type\" : \"" << consoleTypeEnum.valueToString(console.consoleType) << "\", ";

    oss << "\"stations\" : [";

    first = true;
    for (auto stationId : console.connectedStations) {
        if (!first)
            oss << ", ";

        oss << stationId;
        first = false;
    }
    oss << " ], ";

    oss << "\"sensors\" : [";

    first = true;
    for (auto sensor : console.connectedSensors) {
        if (!first)
            oss << ", ";

        oss << "{ \"sensor\" : \"" << sensor.name << "\", \"type\" : \"" << SENSOR_NAMES[sensor.sensorType] << "\" }";
        first = false;
    }

    oss << "] }, ";

    oss << " \"chains\" : [ ";

    bool firstChain = true;
    for (auto chain : chains) {
        if (!firstChain)
            oss << ", ";

        oss << " { \"name\" : \"" << chain.second.name << "\", ";

        oss << "\"repeaters\" : [ ";

        first = true;
        for (auto id : chain.second.repeaters) {
            if (!first)
                oss << ", ";

            oss << "\"" << repeaterIdEnum.valueToString(id) << "\"";
            first = false;
        }

        oss << "], ";

        oss << "\"stations\" : [ ";
        first = true;
        for (auto id : chain.second.chainStations) {
            if (!first)
                oss << ", ";

            oss << id;
        }

        oss << "] } ";
        firstChain = false;
    }

    oss << "], ";

    oss << " \"repeaters\" : [ ";

    bool firstRepeater = true;
    for (auto repeater : repeaters) {
        if (!firstRepeater)
            oss << ", ";

        oss << " { \"repeater\" : \"" << repeaterIdEnum.valueToString(repeater.second.repeaterId) << "\", \"stations\" : [";
        first = true;
        for (auto stationId : repeater.second.connectedStations) {
            if (!first)
                oss << ", ";

            oss << stationId;
            first = false;
        }
        oss << " } ";

        firstRepeater = false;
    }

    oss << " ], ";

    oss << " \"stations\" : [ ";
    bool firstStation = true;
    for (auto station : stations) {
        if (!firstStation)
            oss << ", ";

        oss << " { \"station\" : \"" << station.second.name
            << "\", \"type\" : \"" << stationTypeEnum.valueToString(station.second.stationData.stationType) << "\", "
            << " \"sensors\" : [ ";

        first = true;
        for (auto sensor : station.second.connectedSensors) {
            if (!first)
                oss << ", ";

            oss << "{ \"sensor\" : \"" << sensor.name << "\", \"type\" : \"" << SENSOR_NAMES[sensor.sensorType] << "\" }";
            first = false;
        }

        oss << "] } ";

        firstStation = false;
    }
    oss << " ] ";

    oss << " } }";

    return oss.str();
}

} /* namespace vws */
