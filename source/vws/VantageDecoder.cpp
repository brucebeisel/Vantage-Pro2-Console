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

#include "VantageDecoder.h"
#include "BitConverter.h"
#include "VantageEepromConstants.h"
#include "VantageProtocolConstants.h"
#include "Weather.h"

namespace vws {
using namespace ProtocolConstants;
using namespace VantageEepromConstants;

Rainfall VantageDecoder::rainCollectorSizeInches = static_cast<Rainfall>(0.0);
bool VantageDecoder::rainCollectorSizeSet = false;
VantageLogger * log = nullptr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
VantageDecoder::decode16BitTemperature(const byte buffer[], int offset, Measurement<Temperature> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_16BIT_TEMPERATURE)
        measurement.setValue(static_cast<Temperature>(value16) / TEMPERATURE_16BIT_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decode16BitTemperature(const byte buffer[], int offset) {
    Measurement<Temperature> measurement;
    return decode16BitTemperature(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
VantageDecoder::decode16BitHighTemperature(const byte buffer[], int offset, Measurement<Temperature> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_16BIT_HIGH_TEMPERATURE)
        measurement.setValue(static_cast<Temperature>(value16) / TEMPERATURE_16BIT_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decode16BitHighTemperature(const byte buffer[], int offset) {
    Measurement<Temperature> measurement;
    return decode16BitHighTemperature(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
VantageDecoder::decodeNonScaled16BitTemperature(const byte buffer[], int offset, Measurement<Temperature> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_16BIT_TEMPERATURE)
        measurement.setValue(static_cast<Temperature>(value16));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decodeNonScaled16BitTemperature(const byte buffer[], int offset) {
    Measurement<Temperature> measurement;
    return decodeNonScaled16BitTemperature(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
VantageDecoder::decode8BitTemperature(const byte buffer[], int offset, Measurement<Temperature> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_8BIT_TEMPERATURE)
        measurement.setValue(static_cast<Temperature>(value8 - TEMPERATURE_8BIT_OFFSET));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
VantageDecoder::decode8BitTemperature(const byte buffer[], int offset) {
    Measurement<Temperature> measurement;
    return decode8BitTemperature(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
VantageDecoder::decodeBarometricPressure(const byte buffer[], int offset, Measurement<Pressure> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    measurement = static_cast<Pressure>(value16) / BAROMETER_SCALE;

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Pressure>
VantageDecoder::decodeBarometricPressure(const byte buffer[], int offset) {
    Measurement<Pressure> measurement;
    return decodeBarometricPressure(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
VantageDecoder::decodeHumidity(const byte buffer[], int offset, Measurement<Humidity> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_HUMIDITY)
        measurement.setValue(static_cast<Humidity>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Humidity>
VantageDecoder::decodeHumidity(const byte buffer[], int offset) {
    Measurement<Humidity> measurement;
    return decodeHumidity(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<UvIndex> &
VantageDecoder::decodeUvIndex(const byte buffer[], int offset, Measurement<UvIndex> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_UV_INDEX)
        measurement.setValue(static_cast<UvIndex>(value8) / UV_INDEX_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<UvIndex>
VantageDecoder::decodeUvIndex(const byte buffer[], int offset) {
    Measurement<UvIndex> measurement;
    return decodeUvIndex(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
VantageDecoder::decodeArchiveET(const byte buffer[], int offset, Measurement<Evapotranspiration> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value8) / DAY_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeArchiveET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    return decodeArchiveET(buffer, offset, measurement);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
VantageDecoder::decodeDayET(const byte buffer[], int offset, Measurement<Evapotranspiration> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value16) / DAY_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeDayET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    return decodeDayET(buffer, offset, measurement);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
VantageDecoder::decodeMonthYearET(const byte buffer[], int offset, Measurement<Evapotranspiration> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_ET)
        measurement.setValue(static_cast<Evapotranspiration>(value16) / MONTH_YEAR_ET_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
VantageDecoder::decodeMonthYearET(const byte buffer[], int offset) {
    Measurement<Evapotranspiration> measurement;
    return decodeMonthYearET(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<SolarRadiation> &
VantageDecoder::decodeSolarRadiation(const byte buffer[], int offset, Measurement<SolarRadiation> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_SOLAR_RADIATION)
        measurement.setValue(static_cast<SolarRadiation>(value16));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SolarRadiation>
VantageDecoder::decodeSolarRadiation(const byte buffer[], int offset) {
    Measurement<SolarRadiation> measurement;
    return decodeSolarRadiation(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
VantageDecoder::decodeWindSpeed(const byte buffer[], int offset, Measurement<Speed> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_WIND_SPEED)
        measurement.setValue(static_cast<Speed>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decodeWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    return decodeWindSpeed(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
VantageDecoder::decode16BitWindSpeed(const byte buffer[], int offset, Measurement<Speed> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    measurement.setValue(static_cast<Speed>(value16));

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decode16BitWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    return decode16BitWindSpeed(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
VantageDecoder::decodeAverageWindSpeed(const byte buffer[], int offset, Measurement<Speed> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != INVALID_16BIT_AVG_WIND_SPEED)
        measurement.setValue(static_cast<Speed>(value16) / AVG_WIND_SPEED_SCALE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
VantageDecoder::decodeAverageWindSpeed(const byte buffer[], int offset) {
    Measurement<Speed> measurement;
    return decodeAverageWindSpeed(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
VantageDecoder::decodeWindDirectionSlice(const byte buffer[], int offset, Measurement<Heading> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_WIND_DIRECTION_SLICE)
        measurement.setValue(static_cast<Heading>(value8) * DEGREES_PER_SLICE);
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Heading>
VantageDecoder::decodeWindDirectionSlice(const byte buffer[], int offset) {
    Measurement<Heading> measurement;
    return decodeWindDirectionSlice(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
VantageDecoder::decodeWindDirection(const byte buffer[], int offset, Measurement<Heading> & measurement) {
    int value16 = BitConverter::toInt16(buffer, offset);

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
Measurement<Heading>
VantageDecoder::decodeWindDirection(const byte buffer[], int offset) {
    Measurement<Heading> measurement;
    return decodeWindDirection(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
VantageDecoder::decodeStormRain(const byte buffer[], int offset) {
    int value16 = BitConverter::toInt16(buffer, offset);
    Rainfall rain = static_cast<Rainfall>(value16) / STORM_RAIN_SCALE;

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

    if (log == nullptr)
        log = &VantageLogger::getLogger("VantageDecoder");

    if (!rainCollectorSizeSet)
        log->log(VantageLogger::VANTAGE_WARNING) << "Decoding rain value before rain collector size has been set. Using .01 inches" << std::endl;
    
    int value16 = BitConverter::toInt16(buffer, offset);
    Rainfall rain = static_cast<Rainfall>(value16) * rainCollectorSizeInches;

    return rain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
VantageDecoder::decodeStormStartDate(const byte buffer[], int offset) {
    DateTime stormStart = 0;
    int value16 = BitConverter::toInt16(buffer, offset);

    if (value16 != NO_STORM_ACTIVE_DATE) {
        int year = (value16 & 0x3F) + YEAR_OFFSET;
        int day = (value16 >> 7) & 0x1F;
        int month = (value16 >> 12) & 0xF;

        struct tm tm;
        tm.tm_year = year - TIME_STRUCT_YEAR_OFFSET;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        stormStart = mktime(&tm);
    }

    return stormStart;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
float
VantageDecoder::decodeConsoleBatteryVoltage(const byte buffer[], int offset) {
    int value16 = BitConverter::toInt16(buffer, offset);
    float consoleBatteryVoltage = static_cast<float>(value16 * 300) / 512.0F / 100.0F;
    return consoleBatteryVoltage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<LeafWetness> &
VantageDecoder::decodeLeafWetness(const byte buffer[], int offset, Measurement<LeafWetness> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_LEAF_WETNESS && value8 >= MIN_LEAF_WETNESS_VALUE && value8 <= MAX_LEAF_WETNESS_VALUE)
        measurement.setValue(static_cast<LeafWetness>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<LeafWetness>
VantageDecoder::decodeLeafWetness(const byte buffer[], int offset) {
    Measurement<LeafWetness> measurement;
    return decodeLeafWetness(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<SoilMoisture> &
VantageDecoder::decodeSoilMoisture(const byte buffer[], int offset, Measurement<SoilMoisture> & measurement) {
    int value8 = BitConverter::toInt8(buffer, offset);

    if (value8 != INVALID_SOIL_MOISTURE)
        measurement.setValue(static_cast<SoilMoisture>(value8));
    else
        measurement.invalidate();

    return measurement;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SoilMoisture>
VantageDecoder::decodeSoilMoisture(const byte buffer[], int offset) {
    Measurement<SoilMoisture> measurement;
    return decodeSoilMoisture(buffer, offset, measurement);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
VantageDecoder::decodeTime(const byte buffer[], int offset) {
    int value16 = BitConverter::toInt16(buffer, offset);
    int minute = value16 % 100;
    int hour = value16 / 100;

    time_t now = time(0);
    struct tm tm;
    Weather::localtime(now, tm);
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    DateTime t = mktime(&tm);

    return t;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDecoder::decodeSensorStationList(const byte buffer[], int offset, SensorStationData stationList[ProtocolConstants::MAX_STATIONS]) {
    for (int i = 0; i < ProtocolConstants::MAX_STATIONS; i++) {
        stationList[i].repeaterId = static_cast<RepeaterId>(BitConverter::getUpperNibble(buffer[offset + (i * 2)]));
        stationList[i].stationType = static_cast<SensorStationType>(BitConverter::getLowerNibble(buffer[offset + (i * 2)]));
        stationList[i].extraTemperatureIndex = BitConverter::getLowerNibble(buffer[offset + (i * 2) + 1]);
        stationList[i].extraHumidityIndex = BitConverter::getLowerNibble(buffer[offset + (i * 2) + 1]);
    }
}
}
