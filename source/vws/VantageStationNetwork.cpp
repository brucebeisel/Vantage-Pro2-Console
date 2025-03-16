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

#include "VantageStationNetwork.h"

#include <time.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "json.hpp"

#include "JsonUtils.h"
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
using json = nlohmann::json;

namespace vws {
using namespace EepromConstants;
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
StationData::StationData() : stationId(0),
                             repeaterId(RepeaterId::NO_REPEATER),
                             stationType(StationType::NO_STATION),
                             extraHumidityIndex(StationData::NO_EXTRA_VALUE_INDEX),
                             extraTemperatureIndex(StationData::NO_EXTRA_VALUE_INDEX) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
StationData::encode(byte buffer[], int offset) const {
    buffer[offset] = ((static_cast<int>(repeaterId) << 4) & 0xF0) | (stationType & 0xF);
    buffer[offset + 1] = ((extraTemperatureIndex << 4) & 0xF0) | (extraTemperatureIndex & 0xF);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
StationData::decode(StationId id, const byte buffer[], int offset) {
    stationId = id;
    repeaterId = static_cast<RepeaterId>(BitConverter::getUpperNibble(buffer[offset]));
    stationType = static_cast<StationType>(BitConverter::getLowerNibble(buffer[offset]));
    extraTemperatureIndex = BitConverter::getLowerNibble(buffer[offset + 1]);
    extraHumidityIndex = BitConverter::getUpperNibble(buffer[offset + 1]);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::VantageStationNetwork(const string & dataDirectory, VantageWeatherStation & station, ArchiveManager & am) : station(station),
                                                                                                                          archiveManager(am),
                                                                                                                          monitoredStationMask(0),
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
void
VantageStationNetwork::consoleConnected() {
    initializeNetworkFromConsole();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::consoleDisconnected() {
    // TODO Do we need to initial any members on disconnect?

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveMonitoredStations(std::vector<StationId> & monitoredStations) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving EEPROM data for monitored station mask" << endl;

    monitoredStations.clear();

    if (!station.eepromBinaryRead(EE_USED_TRANSMITTERS_ADDRESS, 1, &monitoredStationMask))
        return false;

    for (int i = 0; i < MAX_STATIONS; i++) {
        if ((monitoredStationMask & (1 << i)) != 0) {
            StationId id = i + 1;
            monitoredStations.push_back(id);
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::updateMonitoredStations(const vector<StationId> & monitoredStations) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating EEPROM data for monitored station mask" << endl;

    byte monitoredStationMask = 0;
    for (auto stationId : monitoredStations) {
        monitoredStationMask |= (1 << (stationId - 1));
    }

    return station.eepromWriteByte(EE_USED_TRANSMITTERS_ADDRESS, monitoredStationMask);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveRetransmitId(StationId & retransmitId) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving EEPROM data for retransmit ID" << endl;
    byte retransmitValue;
    if (!station.eepromBinaryRead(EE_RETRANSMIT_ID_ADDRESS, 1, &retransmitValue))
        return false;

    retransmitId = static_cast<StationId>(retransmitValue);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::updateRetransmitId(StationId retransmitId) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating EEPROM data for retransmit ID" << endl;

    byte retransmitIdValue = static_cast<byte>(retransmitId);

    return station.eepromWriteByte(EE_RETRANSMIT_ID_ADDRESS, retransmitIdValue);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveStationList(StationData stationList[MAX_STATIONS]) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving EEPROM data for station list" << endl;

    char buffer[EE_STATION_LIST_SIZE];
    if (!station.eepromBinaryRead(EE_STATION_LIST_ADDRESS, EE_STATION_LIST_SIZE, buffer))
        return false;

    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++)
        stationList[i].decode(i + 1, buffer, i * 2);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::updateStationList(const StationData stationList[MAX_STATIONS]) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating EEPROM data for station list" << endl;

    char buffer[EE_STATION_LIST_SIZE];
    for (int i = 0; i < MAX_STATIONS; i++)
        stationList[i].encode(buffer, i * 2);

    //return station.eepromBinaryWrite(EE_STATION_LIST_ADDRESS, buffer, EE_STATION_LIST_SIZE);

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
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "============== NETWORK ================" << endl << formatConfigurationJSON() << endl;
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
    calculateDailyNetworkStatus();
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
        sensor.onStationId = CONSOLE_STATION_ID;
        console.connectedSensors.push_back(sensor);
    }

    if (packet.getInsideHumidity().isValid()) {
        sensor.name = "Inside Humidity";
        sensor.sensorType = SensorType::HYGROMETER;
        sensor.onStationId = CONSOLE_STATION_ID;
        console.connectedSensors.push_back(sensor);
    }

    if (packet.getBarometricPressure().isValid()) {
        sensor.name = "Barometric Pressure";
        sensor.sensorType = SensorType::BAROMETER;
        sensor.onStationId = CONSOLE_STATION_ID;
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
    // TODO Now find the sensors on the leaf/soil sensor stations.
    // Not sure how the values map to the sensor stations.
    //

    //
    // TODO Now find the sensors on the temperature/humidity sensor stations
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
    for (auto repeater : repeaters) {
        RepeaterChain chain;
        chain.name = repeaterIdEnum.valueToString(repeater.first);
        chain.endPoint = repeater.first;
        chain.hasRepeater = true;
        chain.repeaters.push_back(repeater.first);

        RepeaterId previousRepeaterId = repeater.first;
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
    for (auto chain : chains) {
        for (int i = 1; i < chain.second.repeaters.size(); i++) {
            Repeater repeater;
            repeater.repeaterId = chain.second.repeaters[i];
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
            else
                station.name = "Station " + (i + 1);

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving sensor station information" << endl;

    if (!retrieveRetransmitId(console.retransmitId))
        return false;

    if (!retrieveMonitoredStations(monitoredStations))
        return false;

    if (!retrieveStationList(stationData))
        return false;

    windStationId = UNKNOWN_STATION_ID;
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        //
        // The wind station is THE anemometer station or the first ISS
        //
        if (stationData[i].stationType == StationType::ANEMOMETER_STATION)
            windStationId = i + 1;
        else if (stationData[i].stationType == StationType::INTEGRATED_SENSOR_STATION && windStationId == UNKNOWN_STATION_ID)
            windStationId = i + 1;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "++++++++ STATION DATA +++++++" << endl
                                              << "Monitored Station Mask: " << static_cast<int>(monitoredStationMask) << endl;
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
int
VantageStationNetwork::calculateLinkQuality(int stationId, int windSamples, int archivePeriod, int archiveRecords) const {

    float archivePeriodSeconds = static_cast<float>(archivePeriod) * 60.0F;
    float stationIndex = static_cast<float>(stationId - 1);

    int maxWindSamples = static_cast<int>(archivePeriodSeconds / ((41.0F + stationIndex) / 16.0F));

    int linkQuality = (windSamples * 100) / (maxWindSamples * archiveRecords);
    if (linkQuality > MAX_STATION_RECEPTION)
        linkQuality = MAX_STATION_RECEPTION;

    return linkQuality;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
VantageStationNetwork::calculateLinkQualityFromArchiveRecords(const std::vector<ArchivePacket> & list) const {
    int totalWindSamples = 0;
    for (const ArchivePacket & packet : list)
        totalWindSamples += packet.getWindSampleCount();

    return calculateLinkQuality(windStationId, totalWindSamples, station.getArchivePeriod(), list.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
VantageStationNetwork::calculateLinkQualityFromArchiveRecord(const ArchivePacket & packet) const {
    vector<ArchivePacket> list;
    list.push_back(packet);
    return calculateLinkQualityFromArchiveRecords(list);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::queryArchiveForDay(DateTime day, vector<ArchivePacket> & list) const {
    struct tm tm;
    Weather::localtime(day, tm);

    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    DateTimeFields startTime;
    startTime.setDateTime(tm);

    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    DateTimeFields endTime;
    endTime.setDateTime(tm);

    archiveManager.queryArchiveRecords(startTime, endTime, list);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
VantageStationNetwork::calculateLinkQualityForDay(DateTime day) const {
    vector<ArchivePacket> list;
    queryArchiveForDay(day, list);
    return calculateLinkQualityFromArchiveRecords(list);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageStationNetwork::calculateDailyNetworkStatus() {
    DateTime now = time(0);
    struct tm tm0;
    Weather::localtime(now, tm0);

    //
    // Subtract enough seconds to move the time into the previous day.
    //
    now -= Weather::SECONDS_PER_DAY;

    struct tm tm;
    Weather::localtime(now, tm);

    //
    // Prevent this algorithm from jumping back 2 days due to DST or
    // not jumping back any days also due to DST
    //
    if (tm.tm_hour != tm0.tm_hour) {
        logger.log(VantageLogger::VANTAGE_INFO) << "calculateDailyNetworkStatus() skipped check due to DST starting or ending" << endl;
        return;
    }

    //
    // Only calculate the link quality once a day
    //
    if (linkQualityCalculationMday == tm.tm_mday)
        return;

    windStationLinkQuality = calculateLinkQualityForDay(now);
    linkQualityCalculationMday = tm.tm_mday;

    writeStatusFile(tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::formatNetworkStatusJSON(struct tm & tm) const {
    ostringstream oss;
    char timeBuffer[100];

    //
    // Note that this calculates the link quality for the previous day, but using the most recent
    // console voltage and station battery status, which may be a few seconds newer. They could also be
    // much newer if the vws process has not been running for a while.
    //
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d", &tm);

    oss << "{ \"date\" : \"" << timeBuffer << "\", \"consoleVoltage\" : " << console.batteryVoltage  << ", "
        << "\"windStationLinkQuality\" : " << windStationLinkQuality << ", \"stationsBatteryStatus\" : [";

    bool first = true;
    for (auto entry : stations) {
        if (!first) oss << ", "; else first = false;
        oss << std::boolalpha << " { \"id\" : \"" << entry.second.stationData.stationId << "\", \"batteryGood\" : " << entry.second.isBatteryGood << " }";
    }

    oss << " ] }" << endl;

    return oss.str();
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

    ofs << formatNetworkStatusJSON(tm);
    ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::formatConfigurationJSON() const {
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

    oss << "\"retransmitEnabled\" : " << std::boolalpha << (console.retransmitId != NO_RETRANSMIT_STATION_ID) << ", ";

    if (console.retransmitId != 0)
        oss << "\"retransmitId\" : " << console.retransmitId << ", ";

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

        oss << " { \"station\" : \"" << station.second.name << "\""
            << ", \"stationId\" : " << station.second.stationData.stationId
            << ", \"type\" : \"" << stationTypeEnum.valueToString(station.second.stationData.stationType) << "\""
            << ", \"sensors\" : [ ";

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::updateNetworkConfiguration(const std::string & networkConfigJson) {
    json networkConfig = json::parse(networkConfigJson.begin(), networkConfigJson.end());

    //
    // For now we are only dealing with writing to the EEPROM and not saving the network data
    // to a file. There are two EEPROM areas, the monitored station list and the 16 byte station
    // list table that contains the station types, repeaters and extra temperature/humidity indices.
    //
    vector<StationId> monitoredStationIds;

    if (!JsonUtils::findJsonVector(networkConfig, "monitoredStationIds", monitoredStationIds))
        return false;

    updateMonitoredStations(monitoredStationIds);

    StationData stationData[MAX_STATIONS];
    auto stations = networkConfig.at("stations");
    for (auto station : stations) {
        string name = station.at("station");
        int stationId = station.at("stationId");
        string typeString = station.at("type");
        stationData[stationId - 1].stationId = stationId;
        stationData[stationId - 1].repeaterId = RepeaterId::NO_REPEATER;
        stationData[stationId - 1].stationType = stationTypeEnum.stringToValue(typeString);
        stationData[stationId - 1].extraTemperatureIndex = 0xF;
        stationData[stationId - 1].extraHumidityIndex = 0xF;
    }

    //
    // Calculate the extra temperature and humidity indexes.
    // This algorithm assigns the indexes, but I am not sure that this is the best method.
    // If station ID 5 is a temperature station, then it uses extra temperature index of 0.
    // If a new temperature station is added using ID 3, then it will use index 0, which may
    // cause the archive data to be inconsistent, as the temperatures from ID 5 used to be in extra temperatures[0]
    // and after adding station ID 3, they are in temperatures[1]. The best approach would be to preserve the
    // extra temperature index no matter if the station changes IDs or a new station is added. That could be
    // accomplished using unique and permanent names. This approach would require the date/time of the change
    // to be preserved so the mapping from index to name could be done.
    //
    int nextExtraTemperatureIndex = 0;
    int nextExtraHumidityIndex = 1;
    for (int i = 0; i < MAX_STATIONS; i++) {
        if (stationData[i].stationType == StationType::TEMPERATURE_ONLY_STATION ||
            stationData[i].stationType == StationType::TEMPERATURE_HUMIDITY_STATION) {
            stationData[i].extraTemperatureIndex = nextExtraTemperatureIndex++;
        }

        if (stationData[i].stationType == StationType::HUMIDITY_ONLY_STATION ||
            stationData[i].stationType == StationType::TEMPERATURE_HUMIDITY_STATION) {
            stationData[i].extraHumidityIndex = nextExtraHumidityIndex++;
        }
    }

    return updateStationList(stationData);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::formatStatusJSON(const DateTimeFields & startDate, const DateTimeFields & endDate) const {
    ostringstream oss;

    oss << "{ \"networkStatus\" : [ ";
    ifstream ifs(networkStatusFile.c_str());

    if (ifs.is_open()) {
        string line;
        bool first = true;
        while (std::getline(ifs, line)) {
            int pos = line.find(':');
            pos = line.find('"', pos);
            string date = line.substr(pos + 1, 10);
            DateTimeFields recordTime;
            recordTime.parseDate(date);

            if (recordTime > endDate)
                break;

            if (recordTime >= startDate) {
                if (!first) oss << ", "; else first = false;
                oss << line;
            }
        }
        ifs.close();
    }
    else
        logger.log(VantageLogger::VANTAGE_ERROR) << "Could not open Network Status file for writing: " << networkStatusFile << endl;

    oss << " ] }";
    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageStationNetwork::todayNetworkStatusJSON() const {
    ostringstream oss;

    oss << "{ \"todayNetworkStatus\" : { \"consoleVoltage\" : " << console.batteryVoltage
        << ", \"stationsBatteryStatus\" : [";

    bool first = true;
    for (auto entry : stations) {
        if (!first) oss << ", "; else first = false;
        oss << std::boolalpha << " { \"id\" : " << entry.second.stationData.stationId << ", \"batteryGood\" : " << entry.second.isBatteryGood << " }";
    }
    oss << " ], ";
    oss << "\"linkQuality\" : ";

    vector<ArchivePacket> list;
    queryArchiveForDay(time(0), list);
    int linkQuality = calculateLinkQualityFromArchiveRecords(list);
    oss << " { \"overall\" : " << linkQuality << ", \"individual\" : [ ";

    first = true;
    for (const ArchivePacket & packet : list) {
        if (!first) oss << ", "; else first = false;
        linkQuality = calculateLinkQualityFromArchiveRecord(packet);
        oss << " { \"time\" : \""  << packet.getPacketDateTimeString() << "\", "
            << " \"linkQuality\" : " << linkQuality << " }";
    }

    oss << " ] } } }";
    return oss.str();
}

} /* namespace vws */
