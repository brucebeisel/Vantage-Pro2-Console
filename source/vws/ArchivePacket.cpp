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

#include <time.h>
#include <string>
#include <ctime>
#include <sstream>

#include "ArchivePacket.h"
#include "BitConverter.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "Weather.h"
#include "WeatherTypes.h"



using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::ArchivePacket() : packetTime(0), windSampleCount(0), buffer(""), logger(&VantageLogger::getLogger("ArchivePacket")) {
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::ArchivePacket(const byte buffer[], int offset) : logger(&VantageLogger::getLogger("ArchivePacket")) {
    updateArchivePacketData(buffer, offset);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::~ArchivePacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchivePacket::updateArchivePacketData(const byte buffer[], int offset) {
    //
    // Copy the packet from the passed in buffer to the buffer member
    //
    for (int i = 0; i < BYTES_PER_ARCHIVE_PACKET; i++) {
        this->buffer[i] = buffer[offset + i];
    }

    windSampleCount = BitConverter::toInt16(this->buffer, NUM_WIND_SAMPLES_OFFSET);
    packetTime = extractArchiveDate();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const byte *
ArchivePacket::getBuffer() const {
    return buffer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
ArchivePacket::getWindSampleCount() const {
    return windSampleCount;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
ArchivePacket::getDateTime() const {
    return packetTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getOutsideTemperature() const {
    Measurement<Temperature> temperature = VantageDecoder::decode16BitTemperature(buffer, OUTSIDE_TEMPERATURE_OFFSET);
    return temperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool ArchivePacket::isEmptyPacket() const {
    return packetTime == EMPTY_ARCHIVE_PACKET_TIME;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchivePacket::archivePacketContainsData(const byte * buffer, int offset) {
    bool containsData = false;

    //
    // Any bytes that is not equal to 0xFF means that there is data
    //
    for (int i = 0; i < BYTES_PER_ARCHIVE_PACKET; i++) {
        if (BitConverter::toInt8(buffer, offset + i) != PACKET_NO_VALUE) {
            containsData = true;
            break;
        }
    }

    return containsData;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
ArchivePacket::extractArchiveDate() const {
    int date = BitConverter::toInt16(buffer, DATE_STAMP_OFFSET);
    int time = BitConverter::toInt16(buffer, TIME_STAMP_OFFSET);
    int year = ((date >> 9) & 0x3F) + 2000;
    int month = (date >> 5) & 0xF;
    int day = date & 0x1F;
    int hour = time / 100;
    int minute = time % 100;

    time_t now = ::time(0);
    struct tm tm;
    Weather::localtime(now, tm);
    tm.tm_year = year - TIME_STRUCT_YEAR_OFFSET;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    return mktime(&tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ArchivePacket::formatJSON() const {
    ostringstream ss;
    DateTime archiveTime = extractArchiveDate();
    ss << "{ \"time\" : \"" << Weather::formatDateTime(archiveTime) << "\"";

    Measurement<Temperature> temperature = VantageDecoder::decode16BitTemperature(buffer, OUTSIDE_TEMPERATURE_OFFSET);
    ss << temperature.formatJSON("avgOutsideTemperature", true);

    temperature = VantageDecoder::decode16BitTemperature(buffer, HIGH_OUTSIDE_TEMPERATURE_OFFSET);
    ss << temperature.formatJSON("highOutsideTemperature", true);

    temperature = VantageDecoder::decode16BitTemperature(buffer, LOW_OUTSIDE_TEMPERATURE_OFFSET);
    ss << temperature.formatJSON("lowOutsideTemperature", true);

    Rainfall r = VantageDecoder::decodeRain(buffer, RAINFALL_OFFSET);
    ss << ", \"rainfall\" : " << r;

    r = VantageDecoder::decodeRain(buffer, HIGH_RAIN_RATE_OFFSET);
    ss << ", \"highRainfallRate\" : " << r;

    Measurement<Pressure> baroPressure = VantageDecoder::decodeBarometricPressure(buffer, BAROMETER_OFFSET);
    ss << baroPressure.formatJSON("barometricPressure", true);

    Measurement<SolarRadiation> solarRadiation = VantageDecoder::decodeSolarRadiation(buffer, SOLAR_RADIATION_OFFSET);
    ss << solarRadiation.formatJSON("avgSolarRadiation", true);

    temperature = VantageDecoder::decode16BitTemperature(buffer, INSIDE_TEMPERATURE_OFFSET);
    ss << temperature.formatJSON("insideTemperature", true);

    Measurement<Humidity> humidity = VantageDecoder::decodeHumidity(buffer, INSIDE_HUMIDITY_OFFSET);
    ss << humidity.formatJSON("insideHumidity", true);

    humidity = VantageDecoder::decodeHumidity(buffer, OUTSIDE_HUMIDITY_OFFSET);
    ss << humidity.formatJSON("outsideHumidity", true);

    //
    // Both wind speed and direction must be valid to generate the JSON
    //
    Measurement<Speed> windSpeed = VantageDecoder::decodeWindSpeed(buffer, AVG_WIND_SPEED_OFFSET);
    Measurement<Heading> windDir = VantageDecoder::decodeWindDirectionSlice(buffer, PREVAILING_WIND_DIRECTION_OFFSET);

    if (windSpeed.isValid() && windDir.isValid()) {
        ss << ", \"avgWindSpeed\" : " << windSpeed << ", "
           << "\"avgWindDirection\" : " << windDir;
    }

    windSpeed = VantageDecoder::decodeWindSpeed(buffer, HIGH_WIND_SPEED_OFFSET);
    windDir = VantageDecoder::decodeWindDirectionSlice(buffer, DIR_OF_HIGH_WIND_SPEED_OFFSET);

    if (windSpeed.isValid() && windDir.isValid()) {
        ss << ", \"highWindSpeed\" : " << windSpeed << ", "
           << "\"highWindDirection\" : " << windDir;
    }

    Measurement<UvIndex> uvIndex = VantageDecoder::decodeUvIndex(buffer, AVG_UV_INDEX_OFFSET);
    ss << uvIndex.formatJSON("avgUvIndex", true);

    Measurement<Evapotranspiration> et = VantageDecoder::decodeArchiveET(buffer, ET_OFFSET);
    if (et.isValid())
        ss <<  et.formatJSON("evapotranspiration", true);

    solarRadiation = VantageDecoder::decodeSolarRadiation(buffer, HIGH_SOLAR_RADIATION_OFFSET);
    ss << solarRadiation.formatJSON("highSolarRadiation", true);

    uvIndex = VantageDecoder::decodeUvIndex(buffer, HIGH_UV_INDEX_OFFSET);
    ss << uvIndex.formatJSON("highUvIndex", true);

    int forecastRule = BitConverter::toInt8(buffer, FORECAST_RULE_OFFSET);
    ss << ", \"forcastRule\" : " << forecastRule;

    bool firstValue = true;
    ss << ", \"extraHumidities\" : [";
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        humidity = VantageDecoder::decodeHumidity(buffer, EXTRA_HUMIDITIES_BASE_OFFSET + i);
        if (humidity.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << humidity.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"extraTemperatures\" : [ ";
    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        temperature = VantageDecoder::decode8BitTemperature(buffer, EXTRA_TEMPERATURES_BASE_OFFSET + i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ] ";

    firstValue = true;
    ss << ", \"leafTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        temperature = VantageDecoder::decode8BitTemperature(buffer, LEAF_TEMPERATURE_BASE_OFFSET + i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"leafWetnesses\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        int leafWetness = BitConverter::toUint8(buffer, LEAF_WETNESS_BASE_OFFSET + i);
        if (leafWetness != ProtocolConstants::INVALID_LEAF_WETNESS) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << leafWetness << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++) {
        temperature = VantageDecoder::decode8BitTemperature(buffer, SOIL_TEMPERATURE_BASE_OFFSET + i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilMoistures\" : [";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        int soilMoisture = BitConverter::toUint8(buffer, SOIL_MOISTURES_BASE_OFFSET + i);
        if (soilMoisture != ProtocolConstants::INVALID_SOIL_MOISTURE) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << soilMoisture << " }";
        }
    }
    ss << " ] ";

    ss << "}";

    return ss.str();
}
}
