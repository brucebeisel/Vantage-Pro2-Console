/*
 * Copyright (C) 2024 Bruce Beisel
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

#include "VantageDecoder.h"

#include <iostream>

#include "BitConverter.h"
#include "VantageEepromConstants.h"
#include "VantageProtocolConstants.h"
#include "Weather.h"

namespace vws {
using namespace ProtocolConstants;
using namespace VantageEepromConstants;

Rainfall VantageDecoder::rainCollectorSizeInches = static_cast<Rainfall>(0.01);
bool VantageDecoder::rainCollectorSizeSet = false;
VantageLogger * logger = nullptr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decode16BitTemperature(const byte buffer[], int offset, bool scaleValue) {
    Measurement<Temperature> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);
    Temperature scale = 1.0;

    if (scaleValue)
        scale = TEMPERATURE_16BIT_SCALE;

    if (value16 != INVALID_16BIT_TEMPERATURE && value16 != INVALID_16BIT_TEMPERATURE_NEGATIVE)
        measurement.setValue(static_cast<Temperature>(value16) / scale);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decode8BitTemperature(const byte buffer[], int offset) {
    Measurement<Temperature> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_8BIT_TEMPERATURE)
        measurement.setValue(static_cast<Temperature>(value8 - TEMPERATURE_8BIT_OFFSET));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Pressure>
VantageDecoder::decodeBarometricPressure(const byte buffer[], int offset) {
    Measurement<Pressure> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != 0)
        measurement.setValue(static_cast<Pressure>(value16) / BAROMETER_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Humidity>
VantageDecoder::decodeHumidity(const byte buffer[], int offset) {
    Measurement<Humidity> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_HUMIDITY)
        measurement.setValue(static_cast<Humidity>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<UvIndex>
VantageDecoder::decodeUvIndex(const byte buffer[], int offset) {
    Measurement<UvIndex> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_UV_INDEX)
        measurement.setValue(static_cast<UvIndex>(value8) / UV_INDEX_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeArchiveET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value8) / DAY_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeDayET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value16) / DAY_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeMonthYearET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value16) / MONTH_YEAR_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SolarRadiation>
VantageDecoder::decodeSolarRadiation(const byte buffer[], int offset) {
    Measurement<SolarRadiation> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_SOLAR_RADIATION)
        measurement.setValue(static_cast<SolarRadiation>(value16));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decodeWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_WIND_SPEED)
        measurement.setValue(static_cast<Speed>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decode16BitWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    measurement.setValue(static_cast<Speed>(value16));

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decodeAverageWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_16BIT_AVG_WIND_SPEED)
        measurement.setValue(static_cast<Speed>(value16) / AVG_WIND_SPEED_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<HeadingIndex>
VantageDecoder::decodeWindDirectionIndex(const byte buffer[], int offset) {
    Measurement<HeadingIndex> measurement;
    HeadingIndex index = BitConverter::toUint8(buffer, offset);

    if (index != INVALID_WIND_DIRECTION_INDEX)
        measurement.setValue(index);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Heading>
VantageDecoder::decodeWindDirection(const byte buffer[], int offset) {
    Measurement<Heading> measurement;
    int16 value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_WIND_DIRECTION) {
        Heading heading;
        if (value16 == NORTH_HEADING_VALUE)
            heading = 0.0;
        else
            heading = static_cast<Heading>(value16);

        measurement.setValue(heading);
    }
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
VantageDecoder::decodeStormRain(const byte buffer[], int offset) {
    if (logger == nullptr)
        logger = &VantageLogger::getLogger("VantageDecoder");

    if (!rainCollectorSizeSet)
        logger->log(VantageLogger::VANTAGE_WARNING) << "Decoding rain value before rain collector size has been set. Using .01 inches" << std::endl;

    // The LOOP packet has storm rain as 1/100th of an inch, while the LOOP2 packet
    // has it as rain clicks. This method assumes rain clicks as that is how every
    // rain value is decoded.
    //
    // Which is correct? Unfortunately, we cannot tell due to the fact that my rain
    // bucket reports in 1/100th of an inch, which yields that same value.
    //
    int16 value16 = BitConverter::toInt16(buffer, offset);
    Rainfall rain = static_cast<Rainfall>(value16) * rainCollectorSizeInches;

    return rain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDecoder::setRainCollectorSize(Rainfall collectorSize) {
    rainCollectorSizeInches = collectorSize;
    rainCollectorSizeSet = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
VantageDecoder::decodeRain(const byte buffer[], int offset) {

    if (logger == nullptr)
        logger = &VantageLogger::getLogger("VantageDecoder");

    if (!rainCollectorSizeSet)
        logger->log(VantageLogger::VANTAGE_WARNING) << "Decoding rain value before rain collector size has been set. Using .01 inches" << std::endl;
    
    int16 value16 = BitConverter::toInt16(buffer, offset);
    Rainfall rain = static_cast<Rainfall>(value16) * rainCollectorSizeInches;

    return rain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
VantageDecoder::decodeStormDate(const byte buffer[], int offset) {
    DateTimeFields stormDate;
    uint16 value16 = BitConverter::toUint16(buffer, offset);

    if (value16 != NO_STORM_ACTIVE_DATE) {
        int year = (value16 & 0x3F) + YEAR_OFFSET;
        int day = (value16 >> 7) & 0x1F;
        int month = (value16 >> 12) & 0xF;

        stormDate.setDate(year, month, day);
    }

    return stormDate;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
float
VantageDecoder::decodeConsoleBatteryVoltage(const byte buffer[], int offset) {
    int16 value16 = BitConverter::toInt16(buffer, offset);
    float consoleBatteryVoltage = static_cast<float>(value16 * 300) / 512.0F / 100.0F;
    return consoleBatteryVoltage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<LeafWetness>
VantageDecoder::decodeLeafWetness(const byte buffer[], int offset) {
    Measurement<LeafWetness> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_LEAF_WETNESS && value8 >= MIN_LEAF_WETNESS_VALUE && value8 <= MAX_LEAF_WETNESS_VALUE)
        measurement.setValue(static_cast<LeafWetness>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SoilMoisture>
VantageDecoder::decodeSoilMoisture(const byte buffer[], int offset) {
    Measurement<SoilMoisture> measurement;
    uint8 value8 = BitConverter::toUint8(buffer, offset);

    if (value8 != INVALID_SOIL_MOISTURE)
        measurement.setValue(static_cast<SoilMoisture>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
VantageDecoder::decodeTime(const byte buffer[], int offset) {
    int16 value16 = BitConverter::toInt16(buffer, offset);
    int minute = value16 % 100;
    int hour = value16 / 100;

    //
    // Use the UNIX time to set the date to today's date
    //
    time_t now = time(0);
    DateTimeFields dateTime(now);
    dateTime.setTime(hour, minute, 0);
    return dateTime;
}
}
