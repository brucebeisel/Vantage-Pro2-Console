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
#ifndef VANTAGE_STATION_NETWORK_H
#define VANTAGE_STATION_NETWORK_H

#include <time.h>
#include <vector>
#include <bitset>

#include "VantageDecoder.h"
#include "VantageProtocolConstants.h"
#include "VantageWeatherStation.h"

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
 */

using namespace VantageEepromConstants;
using namespace ProtocolConstants;

class Repeater {
public:
    RepeaterId             repeaterId;
    bool                   endPoint;          // If true then this repeater talks to the console
    bool                   impliedExistance;  // If true there is no evidence this repeater exists,
                                              // but it might due to the configuration recommendations by Davis Instruments.
    std::vector<StationId> connectedStations; // The stations that are communicating directly with this repeater (user provided)
};

class RepeaterChain {
public:
    std::string               name;           // The name of the chain. Defaults to the end point name
    bool                      hasRepeater;    // If true then the endPoint and repeaters are filled in
    RepeaterId                endPoint;       // The last repeater in the chain, if any
    std::vector<RepeaterId>   repeaters;      // The repeater IDs that are part of this chain (if any)
    std::vector<StationId>    chainStations;  // The stations can be linked with any repeater in the chain.
                                              // Only the user can provide the exact connection configuration.
};

class StationData {
public:
    StationId   stationId;
    RepeaterId  repeaterId;
    StationType stationType;
    int         extraHumidityIndex;     // Index 1 - 8
    int         extraTemperatureIndex;  // Index 0 - 7
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
    std::string name;
    StationData stationData;
    bool        isBatteryGood;

};

class Console : public SensorContainer {
public:
    ProtocolConstants::ConsoleType consoleType;
    std::vector<StationId>         connectedStations;
    float                          batteryVoltage;
};

typedef std::map<StationId,std::vector<Sensor>> stationSensors;

static const std::string NETWORK_CONFIG_FILE = "vantage-network-configuration.dat";
static const std::string NETWORK_STATUS_FILE = "vantage-network-status.dat";

class VantageStationNetwork : public VantageWeatherStation::LoopPacketListener {
public:
    /**
     * Constructor.
     *
     * @param dataDirectory The directory into which the network status file and the network configuration file will be written.
     * @param station       The object used to communicate with the console
     * @param networkFile   The file to read/write the network configuration data
     */
    VantageStationNetwork(const std::string & dataDirectory, VantageWeatherStation & station, ArchiveManager & archiveManager);

    /**
     * Destructor.
     */
    virtual ~VantageStationNetwork();

    /**
     * Initialize the network data from the file or the console.
     *
     * @return True if successful
     */
    bool initializeNetwork();

    /**
     * Format the JSON message containing the network data.
     *
     * @param string The JSON message
     */
    std::string formatJSON() const;

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

private:
    static constexpr int UNKNOWN_STATION_ID = 0;
    static constexpr int MAX_REPEATERS_PER_CHAIN = 4;
    static const int MAX_STATION_RECEPTION = 100;


    bool retrieveStationInfo();
    bool initializeNetworkFromFile();
    bool initializeNetworkFromConsole();
    void findRepeaters();
    void createRepeaterChains();
    void decodeStationData();
    void detectSensors(const LoopPacket & packet);
    void calculateStationReceptionPercentage();
    void writeStatusFile(struct tm & tm);

    typedef std::map<RepeaterId,RepeaterChain> RepeaterChainMap;
    typedef std::map<RepeaterId,Repeater> RepeaterMap;
    typedef std::map<StationId,Station> StationMap;

    VantageLogger &         logger;
    VantageWeatherStation & station;
    ArchiveManager &        archiveManager;
    std::string             networkConfigFile;               // The file in which the network configuration is stored. This includes user inputs.
    std::string             networkStatusFile;               // The file in which the network status is stored.
    byte                    monitoredStationMask;            // The mask of station IDs that the console is monitoring

    RepeaterChainMap        chains;                          // The chains of repeaters and their sensor stations in the network
    RepeaterMap             repeaters;                       // The repeaters in the network
    StationData             stationData[MAX_STATIONS];       // The sensor station data as reported by the console
    StationMap              stations;                        // The sensor stations being monitored
    Console                 console;                         // The console with which this software is communicating

    StationId               windStationId;                   // The station ID of the sensor station that has the wind sensor
    int                     windStationLinkQuality;          // The current link quality of the sensor station that has the wind sensor
    int                     linkQualityCalculationMday;      // The day of the month for which the last link quality was calculated
    bool                    firstLoopPacketReceived;         // Whether the first LOOP packet has been received
};

} /* namespace vws */

#endif
