/* 
 * Copyright (C) 2025 Bruce Beisel
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

#include "LoopPacket.h"

#include <time.h>
#include <cstring>
#include <iostream>
#include <iomanip>

#include "BitConverter.h"
#include "VantageCRC.h"
#include "VantageDecoder.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
LoopPacket::LoopPacket(void) : logger(&VantageLogger::getLogger("LoopPacket")),
                               packetType(-1),
                               rainRate(0.0),
                               stormRain(0.0),
                               dayRain(0.0),
                               monthRain(0.0),
                               yearRain(0.0),
                               consoleBatteryVoltage(0.0),
                               forecastRuleIndex(0),
                               transmitterBatteryStatus(0),
                               nextRecord(-1),
                               barometerTrend(BarometerTrend::STEADY),
                               forecastIcon(Forecast::SUNNY) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
LoopPacket::~LoopPacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const byte *
LoopPacket::getPacketData() const {
    return packetData;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
LoopPacket::decodeLoopPacket(byte buffer[]) {
    memcpy(packetData, buffer, LOOP_PACKET_SIZE);
    stormStart.resetDateTimeFields();

    //
    // Perform a number of validation on the Loop packet before decoding all of the values
    //
    if (packetData[L_OFFSET] != 'L' || packetData[FIRST_O_OFFSET] != 'O' || packetData[SECOND_O_OFFSET] != 'O') {
        logger->log(VantageLogger::VANTAGE_ERROR) << "LOOP packet data does not begin with LOO:"
                                                 << " [0] = " << packetData[L_OFFSET]
                                                 << " [1] = " << packetData[FIRST_O_OFFSET]
                                                 << " [2] = " << packetData[SECOND_O_OFFSET] << endl;
        return false;
    }

    if (!VantageCRC::checkCRC(packetData, CRC_OFFSET)) {
        logger->log(VantageLogger::VANTAGE_ERROR) << "LOOP packet failed CRC check" << endl;
        return false;
    }

    packetType = BitConverter::toUint8(packetData, PACKET_TYPE_OFFSET);

    if (packetType != LOOP_PACKET_TYPE) {
        logger->log(VantageLogger::VANTAGE_ERROR)<< "Invalid packet type for LOOP packet. Expected: "
                                     << LOOP_PACKET_TYPE << " Received: " << packetType << endl;
        return false;
    }

    if (packetData[LINE_FEED_OFFSET] != ProtocolConstants::LINE_FEED || packetData[CARRIAGE_RETURN_OFFSET] != ProtocolConstants::CARRIAGE_RETURN) {
        logger->log(VantageLogger::VANTAGE_ERROR) << "<LF><CR> not found" << endl;
        return false;
    }


    if (packetData[BAROMETER_TREND_OFFSET] != 'P') {
        BarometerTrend baroTrendValue = static_cast<BarometerTrend>(BitConverter::toUint8(packetData, BAROMETER_TREND_OFFSET));

        switch (baroTrendValue) {
            case BarometerTrend::UNKNOWN:
            case BarometerTrend::FALLING_RAPIDLY:
            case BarometerTrend::FALLING_SLOWLY:
            case BarometerTrend::STEADY:
            case BarometerTrend::RISING_SLOWLY:
            case BarometerTrend::RISING_RAPIDLY:
                barometerTrend = static_cast<BarometerTrend>(baroTrendValue);
                break;
            default:
                logger->log(VantageLogger::VANTAGE_ERROR) << "Invalid barometer trend 0x" << hex << (int)packetData[BAROMETER_TREND_OFFSET] << dec << endl;
                barometerTrend = BarometerTrend::UNKNOWN;
                return false;
        }
    }
    else
        barometerTrend = BarometerTrend::UNKNOWN;

    nextRecord = BitConverter::toInt16(packetData, NEXT_RECORD_OFFSET);

    barometricPressure = VantageDecoder::decodeBarometricPressure(packetData, BAROMETER_OFFSET);
    insideTemperature = VantageDecoder::decode16BitTemperature(packetData, INSIDE_TEMPERATURE_OFFSET);
    insideHumidity = VantageDecoder::decodeHumidity(packetData, INSIDE_HUMIDITY_OFFSET);
    outsideTemperature = VantageDecoder::decode16BitTemperature(packetData, OUTSIDE_TEMPERATURE_OFFSET);

    windSpeed = VantageDecoder::decodeWindSpeed(packetData, WIND_SPEED_OFFSET);
    windSpeed10MinuteAverage = VantageDecoder::decodeWindSpeed(packetData, TEN_MINUTE_AVG_WIND_SPEED_OFFSET);
    windDirection = VantageDecoder::decodeWindDirection(packetData, WIND_DIRECTION_OFFSET);

    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++)
        extraTemperature[i] = VantageDecoder::decode8BitTemperature(packetData, EXTRA_TEMPERATURES_OFFSET + i);

    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++)
        soilTemperature[i] = VantageDecoder::decode8BitTemperature(packetData, SOIL_TEMPERATURES_OFFSET + i);

    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++)
        leafTemperature[i] = VantageDecoder::decode8BitTemperature(packetData, LEAF_TEMPERATURES_OFFSET + i);

    outsideHumidity = VantageDecoder::decodeHumidity(packetData, OUTSIDE_HUMIDITY_OFFSET);

    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++)
        extraHumidity[i] = VantageDecoder::decodeHumidity(packetData, EXTRA_HUMIDITIES_OFFSET + i);

    rainRate = VantageDecoder::decodeRain(packetData, RAIN_RATE_OFFSET);

    uvIndex = VantageDecoder::decodeUvIndex(packetData, UV_INDEX_OFFSET);
    solarRadiation = VantageDecoder::decodeSolarRadiation(packetData, SOLAR_RADIATION_OFFSET);

    stormRain = VantageDecoder::decodeStormRain(packetData, STORM_RAIN_OFFSET);
    stormStart = VantageDecoder::decodeStormDate(packetData, STORM_START_DATE_OFFSET);

    dayRain = VantageDecoder::decodeRain(packetData, DAY_RAIN_OFFSET);
    monthRain = VantageDecoder::decodeRain(packetData, MONTH_RAIN_OFFSET);
    yearRain = VantageDecoder::decodeRain(packetData, YEAR_RAIN_OFFSET);

    dayET = VantageDecoder::decodeDayET(packetData, DAY_ET_OFFSET);
    monthET = VantageDecoder::decodeMonthYearET(packetData, MONTH_ET_OFFSET);
    yearET = VantageDecoder::decodeMonthYearET(packetData, YEAR_ET_OFFSET);

    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++)
        soilMoisture[i] = VantageDecoder::decodeSoilMoisture(packetData, SOIL_MOISTURES_OFFSET + i);

    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++)
        leafWetness[i] = VantageDecoder::decodeLeafWetness(packetData, LEAF_WETNESSES_OFFSET + i);

    for (int i = 0; i < ALARM_BYTES; i++) {
        uint8 alarms = BitConverter::toUint8(packetData, ALARMS_OFFSET + i);
        for (int j = 0; j < 8; j++) {
            int bit = (i * 8) + j;
            alarmBits[bit] = (alarms & (1 << j)) == 0 ? false : true;
            if (alarmBits[bit])
                logger->log(VantageLogger::VANTAGE_DEBUG2) << "Alarm byte " << i << " bit " << j << " is set. Set bitset[" << bit << "] to true" << endl;
        }
    }

    transmitterBatteryStatus = BitConverter::toUint8(packetData, TRANSMITTER_BATTERY_STATUS_OFFSET);
    logger->log(VantageLogger::VANTAGE_DEBUG2) << "Transmitter Battery Status: " << transmitterBatteryStatus << endl;

    consoleBatteryVoltage = VantageDecoder::decodeConsoleBatteryVoltage(packetData, CONSOLE_BATTERY_VOLTAGE_OFFSET);
    logger->log(VantageLogger::VANTAGE_DEBUG2) << "Console Battery Voltage: " << consoleBatteryVoltage << endl;

    forecastIcon = static_cast<Forecast>(BitConverter::toUint8(packetData, FORECAST_ICONS_OFFSET));
    forecastRuleIndex = BitConverter::toUint8(packetData, FORECAST_RULE_NUMBER_OFFSET);

    sunriseTime = VantageDecoder::decodeTime(packetData, SUNRISE_TIME_OFFSET);
    sunsetTime = VantageDecoder::decodeTime(packetData, SUNSET_TIME_OFFSET);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
LoopPacket::getNextRecord() const {
    return nextRecord;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ProtocolConstants::BarometerTrend
LoopPacket::getBarometerTrend() const {
    return barometerTrend;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
LoopPacket::getPacketType() const {
    return packetType;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
LoopPacket::getOutsideTemperature() const {
    return outsideTemperature;
}
        
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
LoopPacket::getInsideTemperature() const {
    return insideTemperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
LoopPacket::getOutsideHumidity() const {
    return outsideHumidity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
LoopPacket::getInsideHumidity() const {
    return insideHumidity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
LoopPacket::getWindSpeed() const {
    return windSpeed;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
LoopPacket::getWindSpeed10MinuteAverage() const {
    return windSpeed10MinuteAverage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
LoopPacket::getWindDirection() const {
    return windDirection;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
LoopPacket::getBarometricPressure() const {
    return barometricPressure;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RainfallRate
LoopPacket::getRainRate() const {
    return rainRate;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
LoopPacket::getStormRain() const {
    return stormRain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const DateTimeFields &
LoopPacket::getStormStart() const {
    return stormStart;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
LoopPacket::getDayRain() const {
    return dayRain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
LoopPacket::getMonthRain() const {
    return monthRain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
LoopPacket::getYearRain() const {
    return yearRain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<UvIndex> &
LoopPacket::getUvIndex() const {
    return uvIndex;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
LoopPacket::getYearET() const {
    return yearET;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
LoopPacket::getMonthET() const {
    return monthET;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
LoopPacket::getDayET() const {
    return dayET;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<SolarRadiation> &
LoopPacket::getSolarRadiation() const {
    return solarRadiation;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
float
LoopPacket::getConsoleBatteryVoltage() const {
    return consoleBatteryVoltage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ProtocolConstants::Forecast
LoopPacket::getForecastIcon() const {
    return forecastIcon;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
LoopPacket::getForecastRuleIndex() const {
    return forecastRuleIndex;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
LoopPacket::isTransmitterBatteryGood(StationId stationId) const {
    return (transmitterBatteryStatus & (1 << (stationId - 1))) == 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
LoopPacket::getExtraTemperature(int index) const {
    return extraTemperature[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
LoopPacket::getExtraHumidity(int index) const {
    return extraHumidity[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<LeafWetness> &
LoopPacket::getLeafWetness(int index) const {
    return leafWetness[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<SoilMoisture> &
LoopPacket::getSoilMoisture(int index) const {
    return soilMoisture[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
LoopPacket::getSoilTemperature(int index) const {
    return soilTemperature[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
LoopPacket::getLeafTemperature(int index) const {
    return leafTemperature[index];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
LoopPacket::isStormOngoing() const {
    //
    // The last couple of LOOP packets that have a valid storm start will
    // report a storm rain total of 0.0 inches. This may be an indicator that the storm has stopped,
    // but we are not using that at this point in time. By definition the storm rain has to be > 0, so
    // we will stop reporting an ongoing storm if the storm rain is 0.0
    //
    return stormStart.isDateTimeValid() && stormRain > 0.0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
LoopPacket::getBarometerTrendString() const {
    return barometerTrendEnum.valueToString(barometerTrend);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
LoopPacket::getForecastIconString() const {
    return forecastEnum.valueToString(forecastIcon);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const std::bitset<LoopPacket::ALARM_BITS> &
LoopPacket::getAlarmBits() const {
    return alarmBits;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
LoopPacket::getSunriseTime() const {
    return sunriseTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
LoopPacket::getSunsetTime() const {
    return sunsetTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::ostream &
operator<<(std::ostream & os, const LoopPacket & packet) {
    os << "---------- LOOP ----------" << endl
       << "Packet Byte 0:   " << static_cast<char>(packet.packetData[0]) << endl
       << "Packet Byte 1:   " << static_cast<char>(packet.packetData[1]) << endl
       << "Packet Byte 2:   " << static_cast<char>(packet.packetData[2]) << endl
       << "Barometer Trend: " << packet.getBarometerTrend() << endl
       << "Packet Type:     " << packet.getPacketType() << endl
       << "Next Record:     " << packet.getNextRecord() << endl
       << "Barometer:       " << packet.getBarometricPressure() << endl
       << "Inside Temperature: " << packet.getInsideTemperature() << endl
       << "Inside Humidity: " << packet.getInsideHumidity() << endl
       << "Outside Temperature: " << packet.getOutsideTemperature() << endl
       << "Wind Speed: " << packet.getWindSpeed() << endl
       << "10 Minute Avg Wind Speed: " << packet.getWindSpeed10MinuteAverage() << endl
       << "Wind Direction: " << packet.getWindDirection() << endl
       << "Extra Temperatures: " << endl
       << "    0: " << packet.getExtraTemperature(0) << endl
       << "    1: " << packet.getExtraTemperature(1) << endl
       << "    2: " << packet.getExtraTemperature(2) << endl
       << "    3: " << packet.getExtraTemperature(3) << endl
       << "    4: " << packet.getExtraTemperature(4) << endl
       << "    5: " << packet.getExtraTemperature(5) << endl
       << "    6: " << packet.getExtraTemperature(6) << endl
       << "Soil Temperatures:" << endl
       << "    1: " << packet.getSoilTemperature(0) << endl
       << "    2: " << packet.getSoilTemperature(1) << endl
       << "    3: " << packet.getSoilTemperature(2) << endl
       << "    4: " << packet.getSoilTemperature(3) << endl
       << "Leaf Temperatures:" << endl
       << "    1: " << packet.getLeafTemperature(0) << endl
       << "    2: " << packet.getLeafTemperature(1) << endl
       << "Outside Humidity: " << packet.getOutsideHumidity() << endl
       << "Extra Humidities: " << endl
       << "    0: " << packet.getExtraHumidity(0) << endl
       << "    1: " << packet.getExtraHumidity(1) << endl
       << "    2: " << packet.getExtraHumidity(2) << endl
       << "    3: " << packet.getExtraHumidity(3) << endl
       << "    4: " << packet.getExtraHumidity(4) << endl
       << "    5: " << packet.getExtraHumidity(5) << endl
       << "    6: " << packet.getExtraHumidity(6) << endl
       << "Rain Rate: " << packet.getRainRate() << endl
       << "UV Index: " << packet.getUvIndex() << endl
       << "Solar Radiation: " << packet.getSolarRadiation() << endl
       << "Storm Rain: " << packet.getStormRain() << endl
       << "Storm Start Date: " << packet.getStormStart() << endl
       << "Day Rain: " << packet.getDayRain() << endl
       << "Month Rain: " << packet.getMonthRain() << endl
       << "Year Rain: " << packet.getYearRain() << endl
       << "Day ET: " << packet.getDayET() << endl
       << "Month ET: " << packet.getMonthET() << endl
       << "Year ET: " << packet.getYearET() << endl
       << "Soil Moistures:" << endl
       << "    0: " << packet.getSoilMoisture(0) << endl
       << "    1: " << packet.getSoilMoisture(1) << endl
       << "    2: " << packet.getSoilMoisture(2) << endl
       << "    3: " << packet.getSoilMoisture(3) << endl
       << "Leaf Wetnesses:" << endl
       << "    0: " << packet.getLeafWetness(0) << endl
       << "    1: " << packet.getLeafWetness(1) << endl
       << "    2: " << packet.getLeafWetness(2) << endl
       << "    3: " << packet.getLeafWetness(3) << endl
       << "Alarms: TBD" << endl
       << "Transmitter Battery Status: " << endl << boolalpha
       << "    0: " <<  packet.isTransmitterBatteryGood(0) << endl
       << "    1: " <<  packet.isTransmitterBatteryGood(1) << endl
       << "    2: " <<  packet.isTransmitterBatteryGood(2) << endl
       << "    3: " <<  packet.isTransmitterBatteryGood(3) << endl
       << "    4: " <<  packet.isTransmitterBatteryGood(4) << endl
       << "    5: " <<  packet.isTransmitterBatteryGood(5) << endl
       << "    6: " <<  packet.isTransmitterBatteryGood(6) << endl
       << "    7: " <<  packet.isTransmitterBatteryGood(7) << endl
       << "Console Battery Voltage: " << packet.getConsoleBatteryVoltage() << endl
       << "Forecast Icons: " << packet.getForecastIcon() << endl
       << "Forecast Rule Number: " << packet.getForecastRuleIndex() << endl
       << "Sunrise Time: " << packet.getSunriseTime().formatTime() << endl
       << "Sunset Time: " << packet.getSunsetTime().formatTime() << endl
       << "Terminator 1: 0x" << hex << setw(2) << setfill('0') << static_cast<int>(packet.packetData[95]) << dec << endl
       << "Terminator 2: 0x" << hex << setw(2) << setfill('0') << static_cast<int>(packet.packetData[96]) << dec << endl;

    return os;

}
}
