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
#include <cmath>
#include "BitConverter.h"
#include "VantageConfiguration.h"
#include "VantageDecoder.h"
#include "VantageConstants.h"
#include "VantageProtocolConstants.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::VantageConfiguration(VantageWeatherStation & station) : station(station), logger(VantageLogger::getLogger("VantageConfiguration")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::~VantageConfiguration() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updatePosition(double latitude, double longitude, int elevation) {
    bool success = false;
    char buffer[4];

    int value = std::lround(latitude * LAT_LON_SCALE);
    BitConverter::getBytes(value, buffer, 0, 2);

    value = std::lround(longitude * LAT_LON_SCALE);
    BitConverter::getBytes(value, buffer, 2, 2);

    if (station.eepromBinaryWrite(VantageConstants::EE_LATITUDE_ADDRESS, buffer, 4)) {
        if (station.updateElevationAndBarometerOffset(elevation, 0.0)) {
            success = true;
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrievePosition(double & latitude, double & longitude, int & consoleElevation) {
    bool success = false;

    byte positionData[6];

    if (station.eepromBinaryRead(VantageConstants::EE_LATITUDE_ADDRESS, 6, positionData)) {
        latitude = static_cast<double>(BitConverter::toInt16(positionData, 0)) / LAT_LON_SCALE;  // TBD Does BitConverter::toInt16 handle signed values?
        longitude = static_cast<double>(BitConverter::toInt16(positionData, 2)) / LAT_LON_SCALE;
        consoleElevation = BitConverter::toInt16(positionData, 4);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateTimeSettings(const TimeSettings & timeSettings) {
    byte buffer[6];

    buffer[0] = '0' + timeSettings.timezoneIndex;
    buffer[1] = timeSettings.manualDaylightSavingsTime ? 1 : 0;
    buffer[2] = timeSettings.manualDaylightSavingsTimeOn ? 1 : 0;
    buffer[5] = timeSettings.useGmtOffset ? 1 : 0;

    int value = (timeSettings.gmtOffsetMinutes / 60 * 100) + (timeSettings.gmtOffsetMinutes % 60);
    BitConverter::getBytes(value, buffer, 3, 2);

    return station.eepromBinaryWrite(VantageConstants::EE_TIME_FIELDS_START_ADDRESS, buffer, 6);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveTimeSettings(TimeSettings & timeSettings) {
    bool success = false;
    byte buffer[6];

    if (station.eepromBinaryRead(VantageConstants::EE_TIME_FIELDS_START_ADDRESS, 6, buffer)) {
        timeSettings.timezoneIndex = buffer[0] - '0';
        timeSettings.manualDaylightSavingsTime = BitConverter::toInt8(buffer, 1) == 1;
        timeSettings.manualDaylightSavingsTimeOn = BitConverter::toInt8(buffer, 2) == 1;

        int value16 = BitConverter::toInt16(buffer, 3);
        timeSettings.gmtOffsetMinutes = ((value16 / 100) * 60) + (value16 % 100);
        timeSettings.useGmtOffset = BitConverter::toInt8(buffer, 5) == 1;
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateUnitsSettings(BarometerUnits baroUnits,
                                          TemperatureUnits temperatureUnits,
                                          ElevationUnits elevationUnits,
                                          RainUnits rainUnits,
                                          WindUnits windUnits) {
    bool success = false;
    byte buffer;
    buffer = static_cast<int>(baroUnits) & 0x3;
    buffer |= (static_cast<int>(temperatureUnits) & 0x3) << 2;
    buffer |= (static_cast<int>(elevationUnits) & 0x1) << 4;
    buffer |= (static_cast<int>(rainUnits) & 0x1) << 5;
    buffer |= (static_cast<int>(windUnits) & 0x3) << 6;

    byte invertedBuffer = ~buffer;
    if (station.eepromBinaryWrite(VantageConstants::EE_UNIT_BITS_ADDRESS, &buffer, 1) &&
        station.eepromBinaryWrite(VantageConstants::EE_UNIT_BITS_ADDRESS + 1, &invertedBuffer, 1)) {
            success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveUnitsSettings(BarometerUnits & baroUnits,
                                            TemperatureUnits & temperatureUnits,
                                            ElevationUnits & elevationUnits,
                                            RainUnits & rainUnits,
                                            WindUnits & windUnits) {
    bool success = false;
    byte buffer;
    if (station.eepromBinaryRead(VantageConstants::EE_UNIT_BITS_ADDRESS, 1, &buffer)) {
        baroUnits = static_cast<BarometerUnits>(buffer & 0x3);
        temperatureUnits = static_cast<TemperatureUnits>((buffer >> 2) & 0x3);
        elevationUnits = static_cast<ElevationUnits>((buffer >> 4) & 0x1);
        rainUnits = static_cast<RainUnits>((buffer >> 5) & 0x1);
        windUnits = static_cast<WindUnits>((buffer >> 6) & 0x3);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateSetupBits(const SetupBits & setupBits) {
    bool success = false;
    byte buffer;

    buffer = setupBits.is24HourMode ?  0x1 : 0;
    buffer |= setupBits.isAMMode ? 0x2 : 0;
    buffer |= setupBits.isDayMonthDisplay ? 0x4 : 0;
    buffer |= setupBits.isWindCupLarge ? 0x8 : 0;
    buffer |= setupBits.isNorthLatitude ? 0x40 : 0;
    buffer |= setupBits.isEastLongitude ? 0x80 : 0;
    buffer |= (static_cast<int>(setupBits.rainCollectorSizeType) & 0x3) << 4;
    if (station.eepromBinaryWrite(VantageConstants::EE_SETUP_BITS_ADDRESS, &buffer, 1)) {
        saveRainCollectorSize(setupBits.rainCollectorSizeType);

        //
        // Per the serial protocol documentation, when the setup bits byte is changed, the
        // console must be reinitialized.
        success = station.initializeSetup();
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveSetupBits(SetupBits & setupBits) {
    bool success = false;
    char buffer;

    if (station.eepromBinaryRead(VantageConstants::EE_SETUP_BITS_ADDRESS, 1, &buffer)) {
        setupBits.is24HourMode = (buffer & 0x1) == 0;
        setupBits.isAMMode = (buffer & 0x2) == 1;
        setupBits.isDayMonthDisplay = (buffer & 0x4) == 1;
        setupBits.isWindCupLarge = (buffer & 0x8) == 1;
        setupBits.isNorthLatitude = (buffer & 0x40) == 1;
        setupBits.isEastLongitude = (buffer & 0x80) == 1;
        setupBits.rainCollectorSizeType = static_cast<RainCupSizeType>((buffer & 0x30) >> 4);
        success = true;
        saveRainCollectorSize(setupBits.rainCollectorSizeType);
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::saveRainCollectorSize(RainCupSizeType rainCupType) {
    Rainfall rainCollectorSize = 0.01;

    switch (rainCupType) {
        case RainCupSizeType::POINT_01_INCH:
            rainCollectorSize = ProtocolConstants::POINT_01_INCH_SIZE;
            break;
        case ProtocolConstants::RainCupSizeType::POINT_2_MM:
            rainCollectorSize = ProtocolConstants::POINT_2_MM_SIZE;
            break;
        case RainCupSizeType::POINT_1_MM:
            rainCollectorSize = ProtocolConstants::POINT_1_MM_SIZE;
            break;
        default:
            logger.log(VantageLogger::VANTAGE_WARNING) << "Rain collector size type not valid. Using .01 inches as default" << endl;
            break;
    }

    VantageDecoder::setRainCollectorSize(rainCollectorSize);
}

}
