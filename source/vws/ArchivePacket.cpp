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

#include "ArchivePacket.h"

#include <time.h>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

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
ArchivePacket::ArchivePacket() : packetTime(0), windSampleCount(0), buffer(""), logger(VantageLogger::getLogger("ArchivePacket")) {
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::ArchivePacket(const byte buffer[], int offset) : logger(VantageLogger::getLogger("ArchivePacket")) {
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

    windSampleCount = BitConverter::toUint16(this->buffer, NUM_WIND_SAMPLES_OFFSET);
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
bool
ArchivePacket::archivePacketContainsData(const byte * buffer, int offset) {
    bool containsData = false;

    //
    // Any bytes that is not equal to 0xFF means that there is data
    //
    for (int i = 0; i < BYTES_PER_ARCHIVE_PACKET; i++) {
        if (BitConverter::toUint8(buffer, offset + i) != PACKET_NO_VALUE) {
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
    PacketTimeFields fields;
    fields = decodePacketDateTime();

    struct tm tm{};
    tm.tm_year = fields.year - Weather::TIME_STRUCT_YEAR_OFFSET;
    tm.tm_mon = fields.month - 1;
    tm.tm_mday = fields.monthDay;
    tm.tm_hour = fields.hour;
    tm.tm_min = fields.minute;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return mktime(&tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ArchivePacket::getPacketDateTimeString() const {
    PacketTimeFields fields;
    fields = decodePacketDateTime();

    ostringstream oss;
    oss << setfill('0') << fields.year << "-" << setw(2) << fields.month << "-" << setw(2) << fields.monthDay << " " << setw(2) << fields.hour << ":" << setw(2) << fields.minute;

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchivePacket::PacketTimeFields
ArchivePacket::decodePacketDateTime() const {
    int date = BitConverter::toUint16(buffer, DATE_STAMP_OFFSET);
    int time = BitConverter::toUint16(buffer, TIME_STAMP_OFFSET);
    PacketTimeFields fields;
    fields.year = ((date >> 9) & 0x3F) + YEAR_OFFSET;
    fields.month = (date >> 5) & 0xF;
    fields.monthDay = date & 0x1F;
    fields.hour = time / 100;
    fields.minute = time % 100;

    return fields;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getAverageOutsideTemperature() const {
    return VantageDecoder::decode16BitTemperature(buffer, OUTSIDE_TEMPERATURE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getHighOutsideTemperature() const {
    return VantageDecoder::decode16BitTemperature(buffer, HIGH_OUTSIDE_TEMPERATURE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getLowOutsideTemperature() const {
    return VantageDecoder::decode16BitTemperature(buffer, LOW_OUTSIDE_TEMPERATURE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Rainfall>
ArchivePacket::getRainfall() const {
    return VantageDecoder::decodeRain(buffer, RAINFALL_OFFSET);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Rainfall>
ArchivePacket::getHighRainfallRate() const {
    return VantageDecoder::decodeRain(buffer, HIGH_RAIN_RATE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Pressure>
ArchivePacket::getBarometricPressure() const {
    return VantageDecoder::decodeBarometricPressure(buffer, BAROMETER_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SolarRadiation>
ArchivePacket::getAverageSolarRadiation() const {
    return VantageDecoder::decodeSolarRadiation(buffer, SOLAR_RADIATION_OFFSET);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getInsideTemperature() const {
    return VantageDecoder::decode16BitTemperature(buffer, INSIDE_TEMPERATURE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getInsideHumidity() const {
    return VantageDecoder::decodeHumidity(buffer, INSIDE_HUMIDITY_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getOutsideHumidity() const {
    return VantageDecoder::decodeHumidity(buffer, OUTSIDE_HUMIDITY_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
ArchivePacket::getAverageWindSpeed() const {
    return VantageDecoder::decodeWindSpeed(buffer, AVG_WIND_SPEED_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<HeadingIndex>
ArchivePacket::getPrevailingWindHeadingIndex() const {
    return VantageDecoder::decodeWindDirectionIndex(buffer, PREVAILING_WIND_DIRECTION_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Speed>
ArchivePacket::getHighWindSpeed() const {
    return VantageDecoder::decodeWindSpeed(buffer, HIGH_WIND_SPEED_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<HeadingIndex>
ArchivePacket::getHighWindHeadingIndex() const {
    return VantageDecoder::decodeWindDirectionIndex(buffer, DIR_OF_HIGH_WIND_SPEED_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<UvIndex>
ArchivePacket::getAverageUvIndex() const {
    return VantageDecoder::decodeUvIndex(buffer, AVG_UV_INDEX_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Evapotranspiration>
ArchivePacket::getEvapotranspiration() const {
    return VantageDecoder::decodeArchiveET(buffer, ET_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SolarRadiation>
ArchivePacket::getHighSolarRadiation() const {
    return VantageDecoder::decodeSolarRadiation(buffer, HIGH_SOLAR_RADIATION_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<UvIndex>
ArchivePacket::getHighUvIndex() const {
    return VantageDecoder::decodeUvIndex(buffer, HIGH_UV_INDEX_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
ArchivePacket::getForecastRule() const {
    return BitConverter::toUint8(buffer, FORECAST_RULE_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Humidity>
ArchivePacket::getExtraHumidity(int index) const {
    Measurement<Humidity> humidity;
    if (index >= 0 && index < MAX_EXTRA_HUMIDITIES)
        humidity = VantageDecoder::decodeHumidity(buffer, EXTRA_HUMIDITIES_BASE_OFFSET + index);

    return humidity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getExtraTemperature(int index) const {
    Measurement<Temperature> temperature;
    if (index >= 0 && index < MAX_EXTRA_TEMPERATURES)
        temperature = VantageDecoder::decode8BitTemperature(buffer, EXTRA_TEMPERATURES_BASE_OFFSET + index);

    return temperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getLeafTemperature(int index) const {
    Measurement<Temperature> temperature;
    if (index >= 0 && index < MAX_LEAF_TEMPERATURES)
        temperature = VantageDecoder::decode8BitTemperature(buffer, LEAF_TEMPERATURE_BASE_OFFSET + index);

    return temperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<LeafWetness>
ArchivePacket::getLeafWetness(int index) const {
    Measurement<LeafWetness> leafWetness;
    if (index >= 0 && index < MAX_LEAF_WETNESSES)
        leafWetness = VantageDecoder::decodeLeafWetness(buffer, LEAF_WETNESS_BASE_OFFSET + index);

    return leafWetness;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<Temperature>
ArchivePacket::getSoilTemperature(int index) const {
    Measurement<Temperature> temperature;
    if (index >= 0 && index < MAX_SOIL_TEMPERATURES)
        temperature = VantageDecoder::decode8BitTemperature(buffer, SOIL_TEMPERATURE_BASE_OFFSET + index);

    return temperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Measurement<SoilMoisture>
ArchivePacket::getSoilMoisture(int index) const {
    Measurement<SoilMoisture> soilMoisture;
    if (index >= 0 && index < MAX_SOIL_MOISTURES)
        soilMoisture = VantageDecoder::decodeSoilMoisture(buffer, SOIL_MOISTURES_BASE_OFFSET + index);

    return soilMoisture;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ArchivePacket::formatJSON() const {
    ostringstream ss;
    DateTime archiveTime = extractArchiveDate();
    ss << "{ \"time\" : \"" << Weather::formatDateTime(archiveTime) << "\"";

    Measurement<Temperature> temperature = getAverageOutsideTemperature();
    ss << temperature.formatJSON("avgOutsideTemperature", true);

    temperature = getHighOutsideTemperature();
    ss << temperature.formatJSON("highOutsideTemperature", true);

    temperature = getLowOutsideTemperature();
    ss << temperature.formatJSON("lowOutsideTemperature", true);

    Rainfall r = getRainfall();
    ss << ", \"rainfall\" : " << r;

    r = getHighRainfallRate();
    ss << ", \"highRainfallRate\" : " << r;

    Measurement<Pressure> baroPressure = getBarometricPressure();
    ss << baroPressure.formatJSON("barometricPressure", true);

    Measurement<SolarRadiation> solarRadiation = getAverageSolarRadiation();
    ss << solarRadiation.formatJSON("avgSolarRadiation", true);

    temperature = getInsideTemperature();
    ss << temperature.formatJSON("insideTemperature", true);

    Measurement<Humidity> humidity = getInsideHumidity();
    ss << humidity.formatJSON("insideHumidity", true);

    humidity = getOutsideHumidity();
    ss << humidity.formatJSON("outsideHumidity", true);

    //
    // Both wind speed and direction must be valid to generate the JSON
    //
    Measurement<Speed> windSpeed = getAverageWindSpeed();
    ss << windSpeed.formatJSON("avgWindSpeed", true);

    Measurement<HeadingIndex> windDir = getPrevailingWindHeadingIndex();
    ss << windDir.formatJSON("avgWindDirection", true);

    windSpeed = getHighWindSpeed();
    ss << windSpeed.formatJSON("highWindSpeed", true);

    windDir = getHighWindHeadingIndex();
    ss << windDir.formatJSON("highWindDirection", true);

    Measurement<UvIndex> uvIndex = getAverageUvIndex();
    ss << uvIndex.formatJSON("avgUvIndex", true);

    Measurement<Evapotranspiration> et = getEvapotranspiration();
    if (et.isValid())
        ss <<  et.formatJSON("evapotranspiration", true);

    solarRadiation = getHighSolarRadiation();
    ss << solarRadiation.formatJSON("highSolarRadiation", true);

    uvIndex = getHighUvIndex();
    ss << uvIndex.formatJSON("highUvIndex", true);

    int forecastRule = getForecastRule();
    ss << ", \"forcastRule\" : " << forecastRule;

    bool firstValue = true;
    ss << ", \"extraHumidities\" : [";
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        humidity = getExtraHumidity(i);
        if (humidity.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << humidity.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"extraTemperatures\" : [ ";
    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        temperature = getExtraTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ] ";

    firstValue = true;
    ss << ", \"leafTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        temperature = getLeafTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"leafWetnesses\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        Measurement<LeafWetness> leafWetness = getLeafWetness(i);
        if (leafWetness.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << leafWetness.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++) {
        temperature = getSoilTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << temperature.getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilMoistures\" : [";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        Measurement<SoilMoisture> soilMoisture = getSoilMoisture(i);
        if (soilMoisture.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << soilMoisture.getValue() << " }";
        }
    }
    ss << " ] ";

    ss << "}";

    return ss.str();
}
}
