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

#include <vector>

#include "SensorStation.h"
#include "VantageLogger.h"
#include "VantageDecoder.h"
#include "VantageProtocolConstants.h"

namespace vws {

class VantageWeatherStation;

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

class RepeaterNode {
public:
    RepeaterId             repeaterId;
    bool                   endPoint;          // If true then this repeater talks to the console
    bool                   impliedExistance;  // If true there is no evidence this repeater exists,
                                              // but it might due to the configuration recommendations by Davis Instruments.
    std::vector<StationId> connectedStations; // The stations that are communicating directly with this node (user provided)
};

class SensorStationChain {
public:
    bool                      hasRepeater;    // If true then the endPoint and repeaters are filled in
    RepeaterId                endPoint;       // The last repeater in the chain, if any
    std::vector<RepeaterId>   repeaters;      // The repeater IDs that are part of this chain (if any)
    std::vector<StationId>    chainStations;  // The stations can be linked with any repeater node in the chain.
                                              // Only the user can provide the exact connection configuration.
};

class VantageStationNetwork {
public:
    VantageStationNetwork(VantageWeatherStation & station, const std::string & networkFile);
    virtual ~VantageStationNetwork();

    bool initializeNetwork();

    std::string formatJSON() const;

private:
    static constexpr int UNKNOWN_STATION_ID = 0;
    static constexpr int MAX_REPEATERS_PER_CHAIN = 4;

    struct SensorStationData {
        StationId         stationId;
        RepeaterId        repeaterId;
        SensorStationType stationType;
        int               extraHumidityIndex;     // Index 1 - 8
        int               extraTemperatureIndex;  // Index 0 - 7
    };


    bool retrieveSensorStationInfo();
    bool initializeNetworkFromFile();
    bool initializeNetworkFromConsole();
    void findRepeaters();
    void createSensorStationChains();
    void decodeSensorStationData();

    typedef std::map<RepeaterId,SensorStationChain> SensorStationChainMap;
    typedef std::map<RepeaterId,RepeaterNode> RepeaterNodeMap;

    VantageLogger &         logger;
    VantageWeatherStation & station;
    byte                    transmitterMask;
    SensorStationData       sensorStationData[MAX_STATIONS]; // The sensor station data as reported by the console
    RepeaterNodeMap         repeaters;
    SensorStationChainMap   chains;
    std::string             networkFile;  // The file in which the network configuration is stored. This includes user inputs.
    StationId               windSensorStationId;

    // TODO determine if an array or a vector is the proper container. As a vector you will only have as many sensor station as the
    // console can see, but as an array you have to search the array to find which stations are actually there.
    //SensorStation              console;


    //SensorStation              sensorStations[MAX_STATIONS]; // The sensor stations as reported by the console
};

} /* namespace vws */

#endif
