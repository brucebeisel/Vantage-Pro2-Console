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
#ifndef SENSOR_STATION_H
#define SENSOR_STATION_H

#include <iostream>
#include <vector>
#include <string>
#include "Weather.h"

namespace vws {
/**
 * Class that represents a sensor station that reports data to the console.
 */
class SensorStation {
public:
    /**
     * Sensor station types supported by the Vantage weather station, the enum integer values are from the Vantage serial protocol.
     */
    enum SensorStationType {
        INTEGRATED_SENSOR_STATION = 0,
        TEMPERATURE_ONLY = 1,
        HUMIDITY_ONLY = 2,
        TEMPERATURE_HUMIDITY = 3,
        ANEMOMETER = 4,
        RAIN = 5,
        LEAF = 6,
        SOIL = 7,
        SOIL_LEAF = 8,
        NO_STATION = 10,
        UNKNOWN = 99       // The sensor station has been heard, but not identified
    };

    enum VantageVueSensorStationType {
        VUE_INTEGRATED_SENSOR_STATION = 0,
        VUE_ANEMOMETER = 4,
        VP2_INTEGRATED_SENSOR_STATION = 5,
        VUE_NO_STATION = 10,
        VUE_UNKNOWN = 99       // The sensor station has been heard, but not identified
    };

    enum RepeaterId {
        NO_REPEATER = 0,
        REPEATER_A = 8,
        REPEATER_B = 9,
        REPEATER_C = 10,
        REPEATER_D = 11,
        REPEATER_E = 12,
        REPEATER_F = 13,
        REPEATER_G = 14,
        REPEATER_H = 15
    };

    static constexpr int NO_LINK_QUALITY = 999;

    /**
     * Constructor.
     * 
     * @param type                     The type of sensor station
     * @param sensorTransmitterChannel The channel on which the station is transmitting.  The channel is usually determined by
     *                                 DIP switches within the hardware. Note that sensor stations may be heard, but are not
     *                                 part of this Vantage network.
     * @param repeaterId               The repeater through which this sensor station is transmitting
     * @param hasAnemometer            Whether this station has an anemometer which will determine if link quality is calculated
     */
    SensorStation(SensorStationType type, int sensorTransmitterChannel, RepeaterId repeaterId = NO_REPEATER, bool hasAnemometer = false);

    /**
     * Destructor.
     */
    virtual ~SensorStation() {}

    /**
     * Get the sensor station type.
     * 
     * @return The sensor station type
     */
    SensorStationType getSensorStationType() const;

    /**
     * Get the sensor station index.
     * 
     * @return The index of the sensor station
     */
    int getSensorTransmitterChannel() const;

    /**
     * Get the repeater ID of this sensor station.
     *
     * @return The ID of the repeater, or NONE
     */
    RepeaterId getRepeaterId() const;

    /**
     * Get the battery status of the sensor station. These stations are typically wireless and the battery will
     * need to be replaced at varying intervals. It has been my observation that by the time the station reports a
     * bad battery, it will need to be replaced in less than a day.
     * 
     * @return True if the battery is good
     */
    bool isBatteryGood() const;

    /**
     * Set the battery status.
     * 
     * @param value True if the battery is good
     */
    void setBatteryStatus(bool value);

    /**
     * Get the link quality of this station. Not all sensor stations are monitored for link quality. The Vantage only
     * monitors the link quality of the sensor station with the anemometer.
     * 
     * @return The quality of the link between 0 and 100
     */
    int getLinkQuality() const;

    /**
     * Set the link quality for this sensor station. If this station does not have an anemometer, this call has no effect.
     * 
     * @param value The link quality between 0 and 100
     */
    void setLinkQuality(int value);

    void setTemperatureHumidityIndicies(int temperatureIndex, int HumidityIndex);
    void setTemperatureIndex(int temperatureIndex);
    void setHumidityIndex(int HumidityIndex);

    int getTemperatureIndex() const;
    int getHumidityIndex() const;

    static const std::string & sensorStationTypeToString(SensorStationType sensorStationType);
    static bool lookupSensorStationType(const std::string & sensorStationName, SensorStationType & sensorStationType);

    /**
     * Build a message to send to the collector that reports which sensor stations are connected (wired or wireless) to the console.
     * 
     * @param list The list of sensor stations
     * 
     * @return The message
     */
    static std::string formatSensorStationMessage(const std::vector<SensorStation> & list);

    /**
     * Build a message to send to the collector that reports the status of the sensor stations.
     * 
     * @param list The list of sensor stations
     * @param time The time that the data was collected
     * @return The message
     */
    static std::string formatSensorStationStatusMessage(const std::vector<SensorStation> & list, DateTime time);

    /**
     * ostream operator.
     *
     * @param os The output stream
     * @param ss The sensor station for which the text will be generated
     * @return The output stream passed in
     */
    friend std::ostream & operator<<(std::ostream & os, const SensorStation & ss);

private:
    SensorStationType type;                     // The type of this sensor station
    RepeaterId        repeaterId;               // ID of the repeater, if any
    int               sensorTransmitterChannel; // Sensor index, 1-8
    bool              isAnemometerConnected;    // True if this sensor station has the anemometer
    int               humiditySensorIndex;      // The index into the "extra humidities" that this station's values are reported (1 - 8)
    int               temperatureSensorIndex;   // The index into the "extra temperatures" that this station's values are reported (0 - 7)
    bool              batteryStatus;            // Battery status
    int               linkQuality;              // Only reported if this is an ISS or an Anemometer station

};
}

#endif /* SENSOR_STATION_H */
