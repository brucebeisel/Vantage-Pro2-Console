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
#include <cstring>
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
ArchivePacket::ArchivePacket() : packetEpochDateTime(0), windSampleCount(0), buffer(""), logger(&VantageLogger::getLogger("ArchivePacket")) {
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

    windSampleCount = BitConverter::toUint16(this->buffer, NUM_WIND_SAMPLES_OFFSET);
    decodeDateTimeValues();

    int archiveType = getArchiveRecordType();
    if (archiveType != ARCHIVE_PACKET_REV_B)
        logger->log(VantageLogger::VANTAGE_WARNING) << "The archive type is not Rev B. Value is: " << archiveType << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchivePacket::clearArchivePacketData() {
    for (int i = 0; i < BYTES_PER_ARCHIVE_PACKET; i++)
        this->buffer[i] = PACKET_NO_VALUE;

    packetEpochDateTime = 0;
    windSampleCount = 0;
    packetDateTimeFields.resetDateTimeFields();
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
ArchivePacket::getEpochDateTime() const {
    return packetEpochDateTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool ArchivePacket::isEmptyPacket() const {
    return packetEpochDateTime == EMPTY_ARCHIVE_PACKET_TIME;
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
void
ArchivePacket::decodeDateTimeValues() {
    int date = BitConverter::toUint16(buffer, DATE_STAMP_OFFSET);
    int time = BitConverter::toUint16(buffer, TIME_STAMP_OFFSET);

    int year = ((date >> 9) & 0x3F) + YEAR_OFFSET;
    int month = (date >> 5) & 0xF;
    int monthDay = date & 0x1F;
    int hour = time / 100;
    int minute = time % 100;

    packetDateTimeFields.setDateTime(year, month, monthDay, hour, minute, 0);

    //
    // Note that this technique works in general, but because the Vantage Console does not report
    // weather DST is on or off, the conversion to the UNIX epoch is platform dependent. The epoch-based
    // date time should only be used for relative comparisons, not actual values.
    //
    packetEpochDateTime =  packetDateTimeFields.getEpochDateTime();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const DateTimeFields &
ArchivePacket::getDateTimeFields() const {
    return packetDateTimeFields;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ArchivePacket::getPacketDateTimeString() const {
    ostringstream oss;
    oss << packetDateTimeFields.formatDateTime();
    //oss << setfill('0')
    //    << packetDateTimeFields.year << "-" << setw(2) << packetDateTimeFields.month << "-" << setw(2) << packetDateTimeFields.monthDay
    //    << " " << setw(2) << packetDateTimeFields.hour << ":" << setw(2) << packetDateTimeFields.minute;

    return oss.str();
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
    //
    // The protocol document states that the high solar radiation invalid value is 0.
    // This does not make much sense, since 0 is a valid value.
    // This method instead tests the average solar radiation for validity and
    // passes that onward to the high solar radiation.
    // There are three possible scenarios that need to be validated with the actual console that
    // does not have a solar radiation sensor.
    //     1) The document is correct and a zero value is invalid
    //     2) The document is incorrect and the invalid value is actually 32767 as is the case for the average solar radiation
    //     3) This approach is correct and the protocol document just assumes that if the average solar radiation
    //        is invalid, then so is the high solar radiation.
    //
    Measurement<SolarRadiation> invalidSolarRadiation;

    if (getAverageSolarRadiation().isValid())
        return VantageDecoder::decodeSolarRadiation(buffer, HIGH_SOLAR_RADIATION_OFFSET);
    else
        return invalidSolarRadiation;
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
int
ArchivePacket::getArchiveRecordType() const {
    int recordType = BitConverter::toUint8(this->buffer, RECORD_TYPE_OFFSET);

    return recordType & 0xff;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchivePacket::operator==(const ArchivePacket & other) {

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchivePacket::operator<(const ArchivePacket & other) {

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ArchivePacket::formatJSON(bool pretty) const {
    ostringstream ss;
    ss << "{";

    if (pretty) ss << endl << "    ";

    ss << "\"time\" : \"" << packetDateTimeFields << "\"";

    Measurement<Temperature> temperature = getAverageOutsideTemperature();
    ss << temperature.formatJSON("avgOutsideTemperature", pretty ? 1 : 0, true);

    temperature = getHighOutsideTemperature();
    ss << temperature.formatJSON("highOutsideTemperature", pretty ? 1 : 0, true);

    temperature = getLowOutsideTemperature();
    ss << temperature.formatJSON("lowOutsideTemperature", pretty ? 1 : 0, true);

    Rainfall r = getRainfall();
    ss << ", ";
    if (pretty) ss << endl << "    ";
    ss << "\"rainfall\" : " << r;

    r = getHighRainfallRate();
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"highRainfallRate\" : " << r;

    Measurement<Pressure> baroPressure = getBarometricPressure();
    ss << baroPressure.formatJSON("barometricPressure", pretty ? 1 : 0, true);

    Measurement<SolarRadiation> solarRadiation = getAverageSolarRadiation();
    ss << solarRadiation.formatJSON("avgSolarRadiation", pretty ? 1 : 0, true);

    temperature = getInsideTemperature();
    ss << temperature.formatJSON("insideTemperature", pretty ? 1 : 0, true);

    Measurement<Humidity> humidity = getInsideHumidity();
    ss << humidity.formatJSON("insideHumidity", pretty ? 1 : 0, true);

    humidity = getOutsideHumidity();
    ss << humidity.formatJSON("outsideHumidity", pretty ? 1 : 0, true);

    //
    // Both wind speed and direction must be valid to generate the JSON
    //
    Measurement<Speed> windSpeed = getAverageWindSpeed();
    ss << windSpeed.formatJSON("avgWindSpeed", pretty ? 1 : 0, true);

    Measurement<HeadingIndex> windDir = getPrevailingWindHeadingIndex();
    ss << windDir.formatJSON("avgWindDirection", pretty ? 1 : 0, true);

    windSpeed = getHighWindSpeed();
    ss << windSpeed.formatJSON("highWindSpeed", pretty ? 1 : 0, true);

    windDir = getHighWindHeadingIndex();
    ss << windDir.formatJSON("highWindDirection", pretty ? 1 : 0, true);

    Measurement<UvIndex> uvIndex = getAverageUvIndex();
    ss << uvIndex.formatJSON("avgUvIndex", pretty ? 1 : 0, true);

    Measurement<Evapotranspiration> et = getEvapotranspiration();
    if (et.isValid())
        ss <<  et.formatJSON("evapotranspiration", pretty ? 1 : 0, true);

    solarRadiation = getHighSolarRadiation();
    ss << solarRadiation.formatJSON("highSolarRadiation", pretty ? 1 : 0, true);

    uvIndex = getHighUvIndex();
    ss << uvIndex.formatJSON("highUvIndex", pretty ? 1 : 0, true);

    int forecastRule = getForecastRule();
    ss << ", ";
    if (pretty) ss << endl << "    ";
    ss <<"\"forcastRule\" : " << forecastRule;

    bool firstValue = true;
    ss << ", ";
    if (pretty) ss << endl << "    ";
    ss << "\"extraHumidities\" : [";
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        humidity = getExtraHumidity(i);
        if (humidity.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{ ";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ", ";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << humidity.getValue();
            if (pretty) ss << endl << "        ";
            ss << "}";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "]";

    firstValue = true;
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"extraTemperatures\" : [ ";
    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        temperature = getExtraTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{ ";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ", ";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << temperature.getValue();
            if (pretty) ss << endl << "        ";
            ss << "}";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "] ";

    firstValue = true;
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"leafTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        temperature = getLeafTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ",";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << temperature.getValue();
            if (pretty) ss << endl << "        ";
            ss << "}";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "]";

    firstValue = true;
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"leafWetnesses\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        Measurement<LeafWetness> leafWetness = getLeafWetness(i);
        if (leafWetness.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ",";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << leafWetness.getValue();
            if (pretty) ss << endl << "        ";
            ss << "}";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "]";

    firstValue = true;
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"soilTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++) {
        temperature = getSoilTemperature(i);
        if (temperature.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ",";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << temperature.getValue();
            if (pretty) ss << endl << "        ";
            ss << " }";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "]";

    firstValue = true;
    ss << ",";
    if (pretty) ss << endl << "    ";
    ss << "\"soilMoistures\" : [";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        Measurement<SoilMoisture> soilMoisture = getSoilMoisture(i);
        if (soilMoisture.isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            if (pretty) ss << endl << "        ";
            ss << "{";
            if (pretty) ss << endl << "            ";
            ss << "\"index\" : " << i << ",";
            if (pretty) ss << endl << "            ";
            ss << "\"value\" : " << soilMoisture.getValue();
            if (pretty) ss << endl << "        ";
            ss << " }";
        }
    }
    if (pretty) ss << endl << "    ";
    ss << "] ";

    if (pretty) ss << endl;
    ss << "}";

    return ss.str();
}
}
