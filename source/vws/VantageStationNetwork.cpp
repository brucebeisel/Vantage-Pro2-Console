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

#include "VantageStationNetwork.h"

#include <sys/stat.h>
#include <time.h>
#include <iostream>
#include <fstream>

#include "BitConverter.h"
#include "LoopPacket.h"
#include "VantageDecoder.h"
#include "VantageEepromConstants.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"
#include "ArchiveManager.h"
#include "WeatherTypes.h"
#include "Weather.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;
using namespace ProtocolConstants;

static const char *SENSOR_NAMES[] = {
    "Anemometer",
    "Barometer",
    "Hygrometer",
    "Leaf Wetness Sensor",
    "Leaf Temperature Sensor",
    "Rain Gauge",
    "Solar Radiation Sensor",
    "Soil Moisture Sensor",
    "Soil Temperature Sensor",
    "Thermometer",
    "Ultraviolet Sensor"
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::VantageStationNetwork(const string & dataDirectory, VantageWeatherStation & station, ArchiveManager & am) : station(station),
                                                                                                                          archiveManager(am),
                                                                                                                          monitoredStationMask(0),
                                                                                                                          networkConfigFile(dataDirectory + "/" + NETWORK_CONFIG_FILE),
                                                                                                                          networkStatusFile(dataDirectory + "/" + NETWORK_STATUS_FILE),
                                                                                                                          windStationLinkQuality(0),
                                                                                                                          windStationId(UNKNOWN_STATION_ID),
                                                                                                                          firstLoopPacketReceived(false),
                                                                                                                          linkQualityCalculationMday(0),
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
    if (stat(networkConfigFile.c_str(), &st) == 0)
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
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "============== NETWORK ================" << endl << formatJSON() << endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::processLoop2Packet(const Loop2Packet & packet) {
    //
    // Nothing in the LOOP2 packet is of interest to this class.
    // Using it as a pseudo timer.
    //
    calculateStationReceptionPercentage();
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

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "++++++++ STATION DATA +++++++" << endl
                                              << "Monitored Station Mask: " << monitoredStationMask << endl;
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "ID: "  << stationData[i].stationId
                                                  << " Repeater ID: " << stationData[i].repeaterId
                                                  << " StationType: " << stationData[i].stationType
                                                  << " Extra Humidity: " << stationData[i].extraHumidityIndex
                                                  << " Extra Temperature: " << stationData[i].extraTemperatureIndex << endl;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::calculateStationReceptionPercentage() {

    DateTime now = time(0);
    struct tm tm;
    Weather::localtime(now, tm);

    //
    // Only calculate the link quality once a day
    //
    if (linkQualityCalculationMday == tm.tm_mday)
        return;

    linkQualityCalculationMday = tm.tm_mday;

    ArchivePeriod archivePeriod;
    station.retrieveArchivePeriod(archivePeriod);

    int archivePeriodMinutes = static_cast<int>(archivePeriod);

    float archivePeriodSeconds = archivePeriodMinutes * 60.0F;
    float stationIndex = windStationId - 1.0F;

    int maxWindSamplesPerArchivePacket = static_cast<int>(archivePeriodSeconds / ((41.0F + stationIndex) / 16.0F));

    vector<ArchivePacket> list;

    now -= 86400;
    Weather::localtime(now, tm);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;

    DateTime startTime = mktime(&tm);

    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;

    DateTime endTime = mktime(&tm);

    archiveManager.queryArchiveRecords(startTime, endTime, list);

    int windSamplesForDay = 0;
    for (ArchivePacket & packet : list)
        windSamplesForDay += packet.getWindSampleCount();

    int maxWindSamplesForDay = maxWindSamplesPerArchivePacket * list.size();

    windStationLinkQuality = (windSamplesForDay * 100) / maxWindSamplesForDay;
    if (windStationLinkQuality > MAX_STATION_RECEPTION)
        windStationLinkQuality = MAX_STATION_RECEPTION;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Link quality calculation parameters. Archive packets: " << list.size() << " Wind Samples: " << windSamplesForDay << " Max Wind Samples for Day: " << maxWindSamplesForDay << endl;
    logger.log(VantageLogger::VANTAGE_INFO) << "Link quality for the wind sensor station at ID " << windStationId << " for the date " << (tm.tm_mon + 1) << "/" << tm.tm_mday << " is " << windStationLinkQuality << endl;

    writeStatusFile(tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::writeStatusFile(struct tm & tm) {
    ofstream ofs(networkStatusFile.c_str(), ofstream::out | ios::app);

    if (!ofs.is_open()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not open Network Status file for writing: " << networkStatusFile << endl;
        return;
    }
    char timeBuffer[100];

    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d", &tm);

    ofs << "{ \"networkStatus\" : { \"date\" : \"" << timeBuffer << "\" : \"consoleVoltage\" : " << console.batteryVoltage  << ", "
        << "\"windStationLinkQuality\" : " << windStationLinkQuality << ", \"stationsBatteryStatus\" : [";

    bool first = true;
    for (auto entry : stations) {
        if (!first) ofs << ", "; else first = false;
        ofs << std::boolalpha << entry.second.isBatteryGood;
    }

    ofs << " }" << endl;

    ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::formatJSON() const {
    ostringstream oss;

    oss << "{ \"weatherStationNetwork\" : { ";

    bool first = true;
    oss << " \"monitoredStationIds\" : [";
    for (int i = 0; i < MAX_STATIONS; i++) {
        if ((monitoredStationMask & (1 << i)) != 0) {
            if (!first) oss << ", "; else first = false;
            oss << (i + 1);
        }
    }
    oss << "], ";

    oss << " \"console\" : { \"type\" : \"" << consoleTypeEnum.valueToString(console.consoleType) << "\", ";

    oss << "\"stations\" : [";

    first = true;
    for (auto stationId : console.connectedStations) {
        if (!first) oss << ", "; else first = false;
        oss << stationId;
    }
    oss << " ], ";

    oss << "\"sensors\" : [";

    first = true;
    for (auto sensor : console.connectedSensors) {
        if (!first) oss << ", "; else first = false;
        oss << "{ \"sensor\" : \"" << sensor.name << "\", \"type\" : \"" << SENSOR_NAMES[sensor.sensorType] << "\" }";
    }

    oss << "] }, ";

    oss << " \"chains\" : [ ";

    bool firstChain = true;
    for (auto chain : chains) {
        if (!firstChain) oss << ", "; else firstChain = false;

        oss << " { \"name\" : \"" << chain.second.name << "\", ";

        oss << "\"repeaters\" : [ ";

        first = true;
        for (auto id : chain.second.repeaters) {
            if (!first) oss << ", "; else first = false;
            oss << "\"" << repeaterIdEnum.valueToString(id) << "\"";
        }

        oss << "], ";

        oss << "\"stations\" : [ ";
        first = true;
        for (auto id : chain.second.chainStations) {
            if (!first) oss << ", "; else first = false;
            oss << id;
        }

        oss << "] } ";
    }

    oss << "], ";

    oss << " \"repeaters\" : [ ";

    bool firstRepeater = true;
    for (auto repeater : repeaters) {
        if (!firstRepeater) oss << ", "; else firstRepeater = false;

        oss << " { \"repeater\" : \"" << repeaterIdEnum.valueToString(repeater.second.repeaterId) << "\", \"stations\" : [";
        first = true;
        for (auto stationId : repeater.second.connectedStations) {
            if (!first) oss << ", "; else first = false;
            oss << stationId;
        }
        oss << " } ";
    }

    oss << " ], ";

    oss << " \"stations\" : [ ";
    bool firstStation = true;
    for (auto station : stations) {
        if (!firstStation) oss << ", "; else firstStation = false;

        oss << " { \"station\" : \"" << station.second.name
            << "\", \"type\" : \"" << stationTypeEnum.valueToString(station.second.stationData.stationType) << "\", "
            << " \"sensors\" : [ ";

        first = true;
        for (auto sensor : station.second.connectedSensors) {
            if (!first) oss << ", "; else first = false;
            oss << "{ \"sensor\" : \"" << sensor.name << "\", \"type\" : \"" << SENSOR_NAMES[sensor.sensorType] << "\" }";
        }

        oss << "] } ";
    }
    oss << " ] ";

    oss << " } }";

    return oss.str();
}

} /* namespace vws */
