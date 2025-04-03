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
#ifndef VANTAGE_STATION_NETWORK_H
#define VANTAGE_STATION_NETWORK_H

#include <time.h>
#include <vector>
#include <map>
#include <bitset>

#include "VantageProtocolConstants.h"
#include "VantageEepromConstants.h"
#include "VantageWeatherStation.h"
#include "ConsoleConnectionMonitor.h"
#include "LoopPacketListener.h"

namespace vws {
class VantageLogger;
class ArchiveManager;

/**
 * A Davis Instruments Vantage weather station is made up of many integrated devices. These
 * devices work together to form a weather station network. The Vantage product suite has
 * four different types of devices:
 *     1) Consoles - These devices either display data on a screen or send data to Internet sites
 *     2) Sensors - Devices that measure some weather related data, like temperature or soil moisture
 *     3) Sensor stations - These devices are used to receive data from sensors and transmit the data to consoles
 *     4) Repeaters - These devices are used to extend the geographic distances between the sensor stations and the consoles
 *
 * Here is the basic Vantage network topology.
 *     Sensor --> Sensor Station --> Repeater (optional) --> Console
 * Depending on the type, a sensor station can have multiple sensors attached. Also a repeater can be listening
 * to multiple sensor stations and there can be a string of repeaters in order to extend the distance from the sensor
 * stations to the console. Each repeater chain is a series of one or more repeaters. Theoretically a single weather
 * station network can have up to 4 repeater chain, each with 2 repeaters.
 *
 * Some repeater configuration information can be deduced, but other must be guessed until the user provides the data.
 * For example, if a console can see Repeater B and Repeater D, the actual configuration could be:
 *      A --> B --> Console
 *      C --> D --> Console  OR
 *            B --> Console
 *            D --> Console
 *  It depends how the user configured the repeaters. This class will assume the maximum number of repeaters in the
 *  chain until the actual configuration is sent from the user. It will also assume that all sensor stations
 *  are communicating with the first repeater in the chain.
 *
 *  The user should be able to edit the following data:
 *      1. The station list, which includes the type of station and what repeater is being used. The extra temperature
 *         and humidity fields should be built based on the station types and IDs.
 *      2. The repeater chain. A repeater chain can for example be A -> B -> C or B -> C and the data in the console
 *         cannot tell the difference. This software assumes the former, but the user can override the repeater chain to the
 *         latter.
 *      3. The repeaters to which a station is communicating. In the previous example this software assumes that all
 *         stations are communicating directly with Repeater A, but some might be communicating with Repeater B or
 *         Repeater C.
 *      4. The names of the sensors. Each sensor probably has a unique goal, such as monitoring the temperature in a
 *         wine cellar. The user can name this sensor "Wine Cellar Thermometer". This name could then be used to label
 *         a dataset in a graph.
 *
 * Other considerations:
 * The Vantage Serial Protocol document states that the extra temperatures and humidity should be numbered 0 - 6
 * for temperatures and 1 - 7 for humidities. It also states that the numbering should be allocated in increasing
 * order based on the station ID of the sensor station.
 * As an example, let's say that station ID 4 is a temperature/humidity sensor station. It would be allocated an
 * extra temperature index of 0 and an extra humidity index of 1. These extra values are stored as part of the archive
 * packets data store. In the future, this software may give the user the ability to name these values for readability
 * purposes. Now let's say that you add a temperature sensor station with ID of 2. Per the protocol document the
 * extra temperature index for this new station should be 0 and the extra temperature index for the old sensor station
 * should be changed to 1. This introduces an inconsistency in the archive data. Before station 2 was added
 * extraTemperature[0] represented the old temperature sensor, now that data is in extraTemperature[1] and
 * extraTemperature[0] has the values of the newly added thermometer.
 * Is the protocol document instruction a requirement or a guideline? If it is a guideline, then this is not an issue.
 * When the second sensor station is added it will get allocated the extra temperature index of 1. The archive will
 * then be consistent. If it is a requirement, then it makes archive queries more difficult as any archive packets after
 * the addition of the new sensor station will have to be modified on retrieval to keep the data consistent. This
 * software will have to keep a record of the exact time the new sensor station came online. Which is a pain. I am
 * curious how the Davis software handles this issue, if it does at all. Perhaps it just ignores it and lets the user
 * of the data perform any adjustments.
 */

using namespace EepromConstants;
using namespace ProtocolConstants;

class Repeater {
public:
    Repeater() : repeaterId(RepeaterId::NO_REPEATER), endPoint(false), impliedExistance(false) {}
    RepeaterId             repeaterId;
    bool                   endPoint;          // If true then this repeater talks to the console
    bool                   impliedExistance;  // If true there is no evidence this repeater exists,
                                              // but it might due to the configuration recommendations by Davis Instruments.
    std::vector<StationId> connectedStations; // The stations that are communicating directly with this repeater (user provided)
};

class RepeaterChain {
public:
    RepeaterChain() : hasRepeater(false), endPoint(RepeaterId::NO_REPEATER) {}
    std::string               name;           // The name of the chain. Defaults to the end point name
    bool                      hasRepeater;    // If true then the endPoint and repeaters are filled in
    RepeaterId                endPoint;       // The last repeater in the chain, if any
    std::vector<RepeaterId>   repeaters;      // The repeater IDs that are part of this chain (if any)
    std::vector<StationId>    chainStations;  // The stations can be linked with any repeater in the chain.
                                              // Only the user can provide the exact connection configuration.
};

class StationData {
public:
    static constexpr int NO_EXTRA_VALUE_INDEX = 0xF;

    /**
     * Constructor.
     */
    StationData();

    /**
     * Encode the station data into the provided buffer.
     *
     * @param buffer The buffer into which to write the station data
     * @param offset The offset into the buffer to which to write the station data
     */
    void encode(byte buffer[], int offset) const;

    /**
     * Decode the station data from the provided buffer.
     *
     * @param id     The station ID of the station being decoded
     * @param buffer The buffer from which the station data will be decoded
     * @param offset The offset within the buffer from which the station data will be decoded
     */
    void decode(StationId id, const byte buffer[], int offset);

    /**
     * Format the station data into JSON.
     *
     * @return The formatted JSON string
     */
    std::string formatJSON() const;

    /**
     * Output the station data to an ostream.
     *
     * @param os          The ostream
     * @param stationData The station data to output
     * @return The ostream passed in
     */
    friend std::ostream & operator<<(std::ostream & os, const StationData stationData);

    StationId   stationId;              // Valid IDs are 1 - 8
    RepeaterId  repeaterId;             // NO_REPEATER or repeater A - H
    StationType stationType;            // NO_STATION if this station is not being monitored
    int         extraHumidityIndex;     // Index 1 - 8
    int         extraTemperatureIndex;  // Index 0 - 7
};

/**
 * Class that represents the list of stations that this console can manage.
 */
class StationList {
public:
    /**
     * Constructor to initialize the stations to their default values.
     */
    StationList();

    /**
     * Destuctor.
     */
    ~StationList();

    /**
     * Get the data for the station at the provided index.
     *
     * @param index The index of the station
     * @return The data for the station at the index or an invalid station with an ID of 0
     */
    const StationData & getStationByIndex(int index) const;

    /**
     * Get the data for the station with the provided ID
     *
     * @param index The ID of the station
     * @return The data for the station with the provide ID or an invalid station with an ID of 0
     */
    const StationData & getStationById(StationId id) const;

    /**
     * Array index operator to retrieve data for a specified station index.
     *
     * @param index The index of the station
     * @return The data for the station at the index or an invalid station with an ID of 0
     */
    const StationData & operator[](int index);

    /**
     * Set the data for the station provided.
     *
     * @param stationData The station data to be set based on the ID within
     * @return True if the station ID is valid
     */
    bool setStation(const StationData & stationData);

    /**
     * Encode the station list data into the provided buffer.
     *
     * @param buffer     The buffer into which to write the station list data
     * @param bufferSize The size of the buffer pointed to by buffer
     * @return True is the buffer size is correct
     */
    bool encode(byte buffer[], size_t bufferSize) const;

    /**
     * Decode the station list data from the provided buffer.
     *
     * @param buffer     The buffer from which to read the station list data
     * @param bufferSize The size of the buffer pointed to by buffer
     * @return True is the buffer size is correct
     */
    bool decode(const byte buffer[], size_t bufferSize);

    /**
     * Format the station list into JSON.
     *
     * @return The JSON formatted string
     */
    std::string formatJSON() const;

private:
    StationData     stationData[MAX_STATIONS];
    StationData     invalidStation;            // An error return value for the get methods
    VantageLogger & logger;
};

enum SensorType {
    ANEMOMETER,
    BAROMETER,
    HYGROMETER,
    LEAF_WETNESS_SENSOR,
    LEAF_TEMPERATURE_PROBE,
    RAIN_COLLECTOR,
    SOLAR_RADIATION_SENSOR,
    SOIL_MOISTURE_SENSOR,
    SOIL_MOISTURE_COMPENSATION_THERMOMETER,
    THERMOMETER,
    ULTRAVIOLET_SENSOR
};

class Sensor {
public:
    std::string name;
    SensorType  sensorType;
    StationId   onStationId;
};

/**
 * A container class for a weather station device that has sensors connected.
 */
class SensorContainer {
public:
    std::vector<Sensor>     connectedSensors;
};

/**
 * Class representing a sensor station.
 * A sensor station is a transmitter that has sensors attached.
 * It reads the data from the sensors and transmits packets via radio to either
 * a console or a repeater.
 */
class Station : public SensorContainer {
public:
    std::string name;              // User editable
    StationData stationData;       // Read-only
    bool        isBatteryGood;     // Updated by console

};

class Console : public SensorContainer {
public:
    ProtocolConstants::ConsoleType consoleType;       //
    std::vector<StationId>         connectedStations; // Read-only
    StationId                      retransmitId;      // The ID on which the console is retransmitting (0 = not retransmitting)
    float                          batteryVoltage;    // Updated by console
};

typedef std::map<StationId,std::vector<Sensor>> stationSensors;

static const std::string NETWORK_CONFIG_FILE = "vantage-network-configuration.dat";
static const std::string NETWORK_STATUS_FILE = "vantage-network-status.dat";

class VantageStationNetwork : public LoopPacketListener, public ConsoleConnectionMonitor {
public:
    /**
     * Constructor.
     *
     * @param dataDirectory The directory into which the network status file and the network configuration file will be written.
     * @param station       The object used to communicate with the console
     * @param networkFile   The file to read/write the network configuration data
     * @param windStationId The station ID of the station with the anemometer, only used for testing
     */
    VantageStationNetwork(const std::string & dataDirectory, VantageWeatherStation & station, ArchiveManager & archiveManager, StationId windStationId = UNKNOWN_STATION_ID);

    /**
     * Destructor.
     */
    virtual ~VantageStationNetwork();

    /**
     * Retrieve the monitored station list from the console after clearing the input vector.
     *
     * @param [out] monitoredStations The vector into which the monitored stations will be added. Note the vector will
     *                                be cleared even in the case of an error.
     * @return True if monitored station list was retrieved
     */
    bool retrieveMonitoredStations(std::vector<StationId> & monitoredStations);

    bool updateMonitoredStations(const std::vector<StationId> & monitoredStations);

    bool retrieveRetransmitId(StationId & retransmitId);

    bool updateRetransmitId(StationId retransmitId);

    bool retrieveStationList(StationList & stationList);

    bool updateStationList(const StationList & stationList);

    /**
     * Format the JSON message containing the network configuration data.
     *
     * @return The JSON message
     */
    std::string formatConfigurationJSON() const;

    /**
     * Update the network configuration from the received JSON.
     */
    bool updateNetworkConfiguration(const std::string & networkConfigJson);

    /**
     * Format the JSON message containing the network status data in the given date range.
     *
     * @return The JSON message
     */
    std::string formatStatusJSON(const DateTimeFields & startDate, const DateTimeFields & endDate) const;

    /**
     * Format the JSON message containing the network status data for today.
     *
     * @return The JSON message
     */
    std::string todayNetworkStatusJSON() const;

    /**
     * Process a LOOP packet as part of the LoopPacketListener interface.
     *
     * @param packet The LOOP packet
     * @return True if the LOOP packets loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & packet);

    /**
     * Process a LOOP2 packet as part of the LoopPacketListener interface.
     *
     * @param packet The LOOP2 packet
     * @return True if the LOOP packets loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & packet);

    /**
     * Called when the console is connected.
     */
    virtual void consoleConnected();

    /**
     * Called when the console is disconnected.
     */
    virtual void consoleDisconnected();

    VantageWeatherStation::LinkQuality calculateLinkQuality(const std::vector<ArchivePacket> & list) const;
    VantageWeatherStation::LinkQuality calculateLinkQuality(const ArchivePacket & packet) const;
    VantageWeatherStation::LinkQuality calculateLinkQualityForDay(DateTime day) const;
private:
    //
    // The console ID is set to 16 to avoid clashes with Station ID range of 0 (invalid) to 8 and the
    // Repeater ID of 8 through 15.
    //
    static constexpr StationId   CONSOLE_STATION_ID = 16;
    static constexpr StationId   UNKNOWN_STATION_ID = 0;
    static constexpr int         MAX_REPEATERS_PER_CHAIN = 4;
    static constexpr StationId   NO_RETRANSMIT_STATION_ID = 0;


    bool retrieveStationInfo();
    bool initializeNetworkFromFile();
    bool initializeNetworkFromConsole();
    void findRepeaters();
    void createRepeaterChains();
    void decodeStationData();
    void detectSensors(const LoopPacket & packet);

    std::string formatNetworkStatusJSON(struct tm & tm) const;
    void writeStatusFile(struct tm & tm);
    void calculateDailyNetworkStatus();

    typedef std::map<RepeaterId,RepeaterChain> RepeaterChainMap;
    typedef std::map<RepeaterId,Repeater> RepeaterMap;
    typedef std::map<StationId,Station> StationMap;

    VantageLogger &         logger;
    VantageWeatherStation & station;
    ArchiveManager &        archiveManager;
    std::string             networkStatusFile;               // The file in which the network status is stored.
    byte                    monitoredStationMask;            // The mask of station IDs that the console is monitoring
    std::vector<StationId>  monitoredStations;               // The list of monitored stations extracted from the monitored station mask

    RepeaterChainMap        chains;                          // The chains of repeaters and their sensor stations in the network
    RepeaterMap             repeaters;                       // The repeaters in the network
    StationList             stationList;                     // The sensor station data as reported by the console
    StationMap              stations;                        // The sensor stations being monitored
    Console                 console;                         // The console with which this software is communicating

    StationId               windStationId;                   // The station ID of the sensor station that has the wind sensor
    VantageWeatherStation::LinkQuality             windStationLinkQuality;          // The current link quality of the sensor station that has the wind sensor
    int                     linkQualityCalculationMday;      // The day of the month for which the last link quality was calculated
    bool                    firstLoopPacketReceived;         // Whether the first LOOP packet has been received
};

} /* namespace vws */

#endif
