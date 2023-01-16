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
#include <iostream>
#include "VantageStationNetwork.h"
#include "VantageWeatherStation.h"
#include "VantageConstants.h"
#include "BitConverter.h"

using namespace std;

namespace vws {

struct SensorStationData {
    SensorStation::RepeaterId        repeaterId;
    SensorStation::SensorStationType stationType;
    byte                             humiditySensorNumber;
    byte                             temperatureSensorNumber;
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageStationNetwork::VantageStationNetwork(VantageWeatherStation & station) : station(station),
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
VantageStationNetwork::initialize() {
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageStationNetwork::retrieveSensorStationInfo() {
    char buffer[STATION_DATA_SIZE];

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving sensor station information" << endl;

    if (!station.eepromBinaryRead(VantageConstants::EE_STATION_LIST_ADDRESS, STATION_DATA_SIZE))
        return false;

    SensorStationData data[VantageConstants::MAX_STATION_ID];
    for (int i = 0; i < VantageConstants::MAX_STATION_ID; i++) {
        data[i].repeaterId = static_cast<SensorStation::RepeaterId>(BitConverter::getUpperNibble(buffer[i * 2]));
        data[i].stationType = static_cast<SensorStation::SensorStationType>(BitConverter::getLowerNibble(buffer[i * 2]));
        data[i].temperatureSensorNumber = BitConverter::getLowerNibble(buffer[(i * 2) + 1]);

        //
        // If there is an anemometer sensor station, it by definition is the sensor station that measures
        // the wind. The ISS cannot measure the wind if an anemometer station exists.
        //
        if (data[i].stationType == SensorStation::ANEMOMETER)
            windSensorStationId = i + 1;
        else if (data[i].stationType == SensorStation::INTEGRATED_SENSOR_STATION && windSensorStationId == 0)
            windSensorStationId = i + 1;

    }

    //
    // Assign the anemometer to the sensor station to which it is connected
    //
    for (int i = 0; i < VantageConstants::MAX_STATION_ID; i++) {
        if (data[i].stationType != SensorStation::NO_STATION) {
            bool hasAnemometer = (i + 1) == windSensorStationId;
            sensorStations.push_back(SensorStation(data[i].stationType, i + 1, data[i].repeaterId, hasAnemometer));
        }
    }

    cout << "@@@@@@@@@@ Station Data:" << endl;
    cout << "@@@@@@@@@@ Wind Sensor Station ID: " << windSensorStationId << endl;
    for (int i = 0; i < VantageConstants::MAX_STATION_ID; i++) {
        cout << "@@@@@@@@@@ [" << i << "] Repeater ID: " << data[i].repeaterId
             << " Station Type: " << data[i].stationType
             << " Humidity Sensor: " << data[i].humiditySensorNumber
             << " Temperature Sensor: " << data[i].temperatureSensorNumber << endl;

    }

    for (vector<SensorStation>::iterator it = sensorStations.begin(); it != sensorStations.end(); ++it)
        logger.log(VantageLogger::VANTAGE_DEBUG1) << *it << endl;

    return true;
}

} /* namespace vws */
