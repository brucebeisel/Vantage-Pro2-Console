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
#include "BitConverter.h"
#include "UnitConverter.h"
#include "ArchivePacket.h"
#include "VantageConstants.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"



using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::ArchivePacket() : packetTime(0), windSampleCount(0), buffer(""), log(&VantageLogger::getLogger("ArchivePacket")) {
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::ArchivePacket(const byte buffer[], int offset) : log(&VantageLogger::getLogger("ArchivePacket")) {
    updateArchiveData(buffer, offset);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::~ArchivePacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchivePacket::updateArchiveData(const byte buffer[], int offset) {
    //
    // Copy the packet from the passed in buffer to the buffer member
    //
    for (int i = 0; i < BYTES_PER_PACKET; i++) {
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
bool ArchivePacket::isEmptyPacket() const {
    return packetTime == EMPTY_ARCHIVE_PACKET_TIME;
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
ArchivePacket::formatMessage() const {
    ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    ss << "<historicalRecord>";
    DateTime archiveTime = extractArchiveDate();
    ss << "<time>" << Weather::formatDateTime(archiveTime) << "</time>";

    Measurement<Temperature> temperature;
    VantageDecoder::decode16BitTemperature(buffer, OUTSIDE_TEMPERATURE_OFFSET, temperature);
    ss << temperature.formatXML("avgOutdoorTemperature");

    VantageDecoder::decode16BitTemperature(buffer, HIGH_OUTSIDE_TEMPERATURE_OFFSET, temperature);
    ss << temperature.formatXML("highOutdoorTemperature");

    VantageDecoder::decode16BitTemperature(buffer, LOW_OUTSIDE_TEMPERATURE_OFFSET, temperature);
    ss << temperature.formatXML("lowOutdoorTemperature");

    Rainfall r = VantageDecoder::decodeRain(buffer, RAINFALL_OFFSET);
    ss << "<rainfall>" << r << "</rainfall>";

    r = VantageDecoder::decodeRain(buffer, HIGH_RAIN_RATE_OFFSET);
    ss << "<highRainfallRate>" << r << "</highRainfallRate>";

    Measurement<Pressure> baroPressure;
    VantageDecoder::decodeBarometricPressure(buffer, BAROMETER_OFFSET, baroPressure);
    ss << baroPressure.formatXML("baroPressure");

    Measurement<SolarRadiation> solarRadiation;
    VantageDecoder::decodeSolarRadiation(buffer, SOLAR_RADIATION_OFFSET, solarRadiation);
    ss << solarRadiation.formatXML("avgSolarRadiation");
  
    VantageDecoder::decode16BitTemperature(buffer, INSIDE_TEMPERATURE_OFFSET, temperature);
    ss << temperature.formatXML("indoorTemperature");

    Measurement<Humidity> humidity;
    VantageDecoder::decodeHumidity(buffer, INSIDE_HUMIDITY_OFFSET, humidity);
    ss << humidity.formatXML("indoorHumidity");

    VantageDecoder::decodeHumidity(buffer, OUTSIDE_HUMIDITY_OFFSET, humidity);
    ss << humidity.formatXML("outdoorHumidity");

    //
    // Both wind speed and direction must be valid to generate the XML
    //
    Measurement<Speed> windSpeed = VantageDecoder::decodeWindSpeed(buffer, AVG_WIND_SPEED_OFFSET);
    Measurement<Heading> windDir = VantageDecoder::decodeWindDirectionSlice(buffer, PREVAILING_WIND_DIRECTION_OFFSET);

    if (windSpeed.isValid() && windDir.isValid()) {
        ss << "<avgWind><speed>" << windSpeed << "</speed>"
           << "<direction>" << windDir << "</direction>"
           << "</avgWind>";
    }

    windSpeed = VantageDecoder::decodeWindSpeed(buffer, HIGH_WIND_SPEED_OFFSET);
    windDir = VantageDecoder::decodeWindDirectionSlice(buffer, DIR_OF_HIGH_WIND_SPEED_OFFSET);

    if (windSpeed.isValid() && windDir.isValid()) {
        ss << "<highWind><speed>" << windSpeed << "</speed>"
           << "<direction>" << windDir << "</direction>"
           << "</highWind>";
    }

    Measurement<UvIndex> uvIndex;
    VantageDecoder::decodeUvIndex(buffer, AVG_UV_INDEX_OFFSET, uvIndex);
    ss << uvIndex.formatXML("avgUvIndex");

    Measurement<Evapotranspiration> et = VantageDecoder::decodeDayET(buffer, ET_OFFSET);
    ss <<  et.formatXML("evapotranspiration");

    VantageDecoder::decodeSolarRadiation(buffer, HIGH_SOLAR_RADIATION_OFFSET, solarRadiation);
    ss << solarRadiation.formatXML("highSolarRadiation");

    VantageDecoder::decodeUvIndex(buffer, HIGH_UV_INDEX_OFFSET, uvIndex);
    ss << uvIndex.formatXML("highUvIndex");

    ss << "<extraHumidities>";
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        VantageDecoder::decodeHumidity(buffer, EXTRA_HUMIDITIES_BASE_OFFSET + i, humidity);
        if (humidity.isValid()) {
            ss << "<humidity><index>" << i << "</index><value>" << humidity.getValue() << "</value></humidity>";
        }
    }
    ss << "</extraHumidities>";

    ss << "<extraTemperatures>";
    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        VantageDecoder::decode8BitTemperature(buffer, EXTRA_TEMPERATURES_BASE_OFFSET + i, temperature);
        if (temperature.isValid()) {
            ss << "<temperature><index>" << i << "</index><value>" << temperature.getValue() << "</value></temperature>";
        }
    }
    ss << "</extraTemperatures>";

/*
    for (int i = 0; i < VantageConstants::APB_MAX_LEAF_TEMPERATURES; i++) {
        value8 = buffer[LEAF_TEMPERATURE_BASE_OFFSET + i];
        if (value8 != VantageConstants::APB_INVALID_LEAF_TEMPERATURE)
            ss << ""; //leaf_temperature[{0}]={1};", i, value8 - EXTRA_TEMPERATURE_OFFSET);
    }

    ss << "<leafWetnessSensorEntries>";
    for (int i = 0; i < VantageConstants::APB_MAX_LEAF_WETNESSES; i++) {
        int leafWetness = BitConverter::toInt8(buffer, LEAF_WETNESS_BASE_OFFSET + i);
        if (leafWetness != VantageConstants::APB_INVALID_LEAF_WETNESS) {
            ss << "<entry><key>" << (500 + i) << "</key><value><sensorId>" << (500 + i) << "</sensorId><sensorType>LEAF_WETNESS</sensorType>";
            ss << "<measurement xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"leafWetness\">";
            ss << leafWetness << "</measurement></value></entry>";
        }
    }
    ss << "</leafWetnessSensorEntries>";

    for (int i = 0; i < VantageConstants::APB_MAX_SOIL_TEMPERATURES; i++) {
        value8 = buffer[SOIL_TEMPERATURE_BASE_OFFSET + i];
        if (value8 != VantageConstants::APB_INVALID_SOIL_TEMPERATURE)
            ss << "";//soil_temperature[{0}]={1};", i, value8 - EXTRA_TEMPERATURE_OFFSET);
    }

    ss << "<soilMoistureSensorEntries>";
    for (int i = 0; i < VantageConstants::APB_MAX_SOIL_MOISTURES; i++) {
        int soilMoisture = BitConverter::toInt8(buffer, SOIL_MOISTURES_BASE_OFFSET + i);
        if (soilMoisture != VantageConstants::APB_INVALID_SOIL_MOISTURE) {
            ss << "<entry><key>" << (600 + i) << "</key><value><sensorId>" << (600 + i) << "</sensorId><sensorType>SOIL_MOISTURE</sensorType>";
            ss << "<measurement xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"soilMoisture\">";
            ss << soilMoisture << "</measurement></value></entry>";
        }
    }
    ss << "</soilMoistureSensorEntries>";
*/
    ss << "</historicalRecord>";
    return ss.str();
}
}
