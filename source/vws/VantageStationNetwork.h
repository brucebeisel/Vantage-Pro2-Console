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

#include "../vws/SensorStation.h"
#include "../vws/VantageLogger.h"
#include "../vws/VantageProtocolConstants.h"

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
 * Some repeater configuration information can be deduced, but other must be guess until the user provides the data.
 * For example, if a console can see Repeater B and Repeater D, the actual configuration could be:
 *      A --> B --> Console
 *      C --> D --> Console  OR
 *            B --> Console
 *            D --> Console
 *  It depends how the user configured the repeaters. This class will assume the maximum number of repeaters in the
 *  chain until provided the actual configuration is sent from the user. It will also assume that all sensor stations
 *  are communicating with the first repeater in the chain.
 */
class VantageStationNetwork {
public:
    VantageStationNetwork(VantageWeatherStation & station);
    virtual ~VantageStationNetwork();

    bool initialize();
    bool retrieveSensorStationInfo();

private:
    static constexpr int UNKNOWN_STATION_ID = 0;

    VantageLogger              logger;
    VantageWeatherStation &    station;
    // TODO determine if an array or a vector is the proper container. As a vector you will only have as many sensor station as the
    // console can see, but as an array you have to search the array to find which stations are actually there.
    SensorStation              sensorStations[ProtocolConstants::MAX_STATIONS];           // The sensor stations as reported by the console
    StationId                  windSensorStationId;
};

} /* namespace vws */

#endif
