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
#include "../vws/VantageStationNetwork.h"

#include <iostream>

#include "../vws/BitConverter.h"
#include "../vws/VantageDecoder.h"
#include "../vws/VantageEepromConstants.h"
#include "../vws/VantageEnums.h"
#include "../vws/VantageWeatherStation.h"
#include "../vws/WeatherTypes.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;
using namespace ProtocolConstants;


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
    char buffer[EE_STATION_LIST_SIZE];

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving sensor station information" << endl;

    if (!station.eepromBinaryRead(EE_STATION_LIST_ADDRESS, EE_STATION_LIST_SIZE, buffer))
        return false;

    VantageDecoder::SensorStationData sensorStationData[ProtocolConstants::MAX_STATIONS];
    VantageDecoder::decodeSensorStationList(buffer, 0, sensorStationData);

    //
    // Find the sensor station with the anemometer. An anemometer sensor station overrides an ISS
    //
    for (int i = 0; i < MAX_STATIONS; i++) {
        if (sensorStationData[i].stationType == SensorStationType::ANEMOMETER_STATION)
            windSensorStationId = i + 1;
        else if (sensorStationData[i].stationType == SensorStationType::INTEGRATED_SENSOR_STATION && windSensorStationId == UNKNOWN_STATION_ID)
            windSensorStationId = i + 1;
    }



    //
    // Assign the anemometer to the sensor station to which it is connected
    //
    for (int i = 0; i < ProtocolConstants::MAX_STATION_ID; i++) {
        if (sensorStationData[i].stationType != SensorStationType::NO_STATION) {
            bool hasAnemometer = (i + 1) == windSensorStationId;
            sensorStations[i].setData(sensorStationData[i].stationType, i + 1, sensorStationData[i].repeaterId, hasAnemometer);
        }
    }

    cout << "@@@@@@@@@@ Station Data:" << endl;
    cout << "@@@@@@@@@@ Wind Sensor Station ID: " << windSensorStationId << endl;
    for (int i = 0; i < ProtocolConstants::MAX_STATION_ID; i++) {
        cout << "@@@@@@@@@@ [" << i << "] Repeater ID: " << sensorStationData[i].repeaterId
             << " Station Type: " << sensorStationTypeEnum.valueToString(sensorStationData[i].stationType)
             << " Humidity Sensor: " << sensorStationData[i].extraHumidityIndex
             << " Temperature Sensor: " << sensorStationData[i].extraTemperatureIndex << endl;

    }

    for (int i = 0; i < MAX_STATION_ID; i++)
        logger.log(VantageLogger::VANTAGE_DEBUG1) << sensorStations[i] << endl;

    return true;
}

} /* namespace vws */
