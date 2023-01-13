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
#include <cstring>
#include <iostream>
#include "BitConverter.h"
#include "UnitConverter.h"
#include "LoopPacket.h"
#include "VantageConstants.h"
#include "VantageCRC.h"
#include "VantageDecoder.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
LoopPacket::LoopPacket(void) : logger(VantageLogger::getLogger("LoopPacket")),
                               packetType(-1),
                               rainRate(0.0),
                               stormRain(0.0),
                               dayRain(0.0),
                               monthRain(0.0),
                               yearRain(0.0),
                               consoleBatteryVoltage(0.0),
                               forecastRuleIndex(0),
                               stormStart(0),
                               sunriseTime(0),
                               sunsetTime(0),
                               transmitterBatteryStatus(0),
                               nextRecord(-1),
                               baroTrend(STEADY),
                               forecastIcon(SUNNY) {
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
    //
    // Perform a number of validation on the Loop packet before decoding all of the values
    //
    if (packetData[0] != 'L' || packetData[1] != 'O' || packetData[2] != 'O') {
        logger.log(VantageLogger::VANTAGE_ERROR) << "LOOP packet data does not begin with LOO: "
                                      << "[0] = " << packetData[0] << " [1] = " << packetData[1] << " [2] = " << packetData[2] << endl;
        return false;
    }

    if (!VantageCRC::checkCRC(packetData, 97)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "LOOP packet failed CRC check" << endl;
        return false;
    }

    packetType = BitConverter::toInt8(packetData, 4);
    if (packetType != LOOP_PACKET_TYPE) {
        logger.log(VantageLogger::VANTAGE_ERROR)<< "Invalid packet type for LOOP packet. Expected: "
                                     << LOOP_PACKET_TYPE << " Received: " << packetType << endl;
        return false;
    }

    if (packetData[95] != VantageConstants::LINE_FEED || packetData[96] != VantageConstants::CARRIAGE_RETURN) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "<LF><CR> not found" << endl;
        return false;
    }


    if (packetData[3] != 'P') {
        int baroTrendValue = BitConverter::toInt8(packetData, 3);

        switch (baroTrendValue) {
            case UNKNOWN:
            case FALLING_RAPIDLY:
            case FALLING_SLOWLY:
            case STEADY:
            case RISING_SLOWLY:
            case RISING_RAPIDLY:
                baroTrend = static_cast<BaroTrend>(baroTrendValue);
                break;
            default:
                logger.log(VantageLogger::VANTAGE_ERROR) << "Invalid barometer trend 0x" << hex << (int)packetData[3] << dec << endl;
                baroTrend = UNKNOWN;
                return false;
        }
    }
    else
        baroTrend = UNKNOWN;

    nextRecord = BitConverter::toInt16(packetData,5);

    VantageDecoder::decodeBarometricPressure(packetData, 7, barometricPressure);
    VantageDecoder::decode16BitTemperature(packetData, 9, insideTemperature);
    VantageDecoder::decodeHumidity(packetData, 11, insideHumidity);
    VantageDecoder::decode16BitTemperature(packetData, 12, outsideTemperature);

    windSpeed = VantageDecoder::decodeWindSpeed(packetData, 14);
    windSpeed10MinuteAverage = VantageDecoder::decodeWindSpeed(packetData, 15);
    windDirection = VantageDecoder::decodeWindDirection(packetData, 16);

    for (int i = 0; i < VantageConstants::MAX_EXTRA_TEMPERATURES; i++)
        VantageDecoder::decode8BitTemperature(packetData, 18 + i, extraTemperature[i]);

    for (int i = 0; i < VantageConstants::MAX_SOIL_TEMPERATURES; i++)
        VantageDecoder::decode8BitTemperature(packetData, 25 + i, soilTemperature[i]);

    for (int i = 0; i < VantageConstants::MAX_LEAF_TEMPERATURES; i++)
        VantageDecoder::decode8BitTemperature(packetData, 29 + i, leafTemperature[i]);

    VantageDecoder::decodeHumidity(packetData, 33, outsideHumidity);

    for (int i = 0; i < VantageConstants::MAX_EXTRA_HUMIDITIES; i++)
        VantageDecoder::decodeHumidity(packetData, 34 + i, extraHumidity[i]);

    rainRate = VantageDecoder::decodeRain(packetData, 41);

    VantageDecoder::decodeUvIndex(packetData, 43, uvIndex);
    VantageDecoder::decodeSolarRadiation(packetData, 44, solarRadiation);

    stormRain = VantageDecoder::decodeStormRain(packetData, 46);
    stormStart = VantageDecoder::decodeStormStartDate(packetData, 48);

    dayRain = VantageDecoder::decodeRain(packetData, 50);
    monthRain = VantageDecoder::decodeRain(packetData, 52);
    yearRain = VantageDecoder::decodeRain(packetData, 54);

    dayET = VantageDecoder::decodeDayET(packetData, 56);
    monthET = VantageDecoder::decodeMonthYearET(packetData, 58);
    yearET = VantageDecoder::decodeMonthYearET(packetData, 60);

    for (int i = 0; i < VantageConstants::MAX_SOIL_MOISTURES; i++)
        VantageDecoder::decodeSoilMoisture(packetData, 62 + i, soilMoisture[i]);

    for (int i = 0; i < VantageConstants::MAX_LEAF_WETNESSES; i++)
        VantageDecoder::decodeLeafWetness(packetData, 66 + i, leafWetness[i]);

    for (int i = 0; i < 16; i++) {
        int alarms = BitConverter::toInt8(packetData, 70 + i);
        for (int j = 0; j < 8; j++) {
            int bit = (i * 8) + j;
            alarmBits[bit] = (alarms & (1 << j)) == 0 ? 0 : 1;
        }
    }

    transmitterBatteryStatus = BitConverter::toInt8(packetData, 86);
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Transmitter Battery Status: " << transmitterBatteryStatus << endl;

    consoleBatteryVoltage = VantageDecoder::decodeConsoleBatteryVoltage(packetData, 87);
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Console Battery Voltage: " << consoleBatteryVoltage << endl;

    forecastIcon = static_cast<Forecast>(BitConverter::toInt8(packetData, 89));
    forecastRuleIndex = BitConverter::toInt8(packetData, 90);

    sunriseTime = VantageDecoder::decodeTime(packetData, 91);
    sunsetTime = VantageDecoder::decodeTime(packetData, 93);

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
LoopPacket::BaroTrend
LoopPacket::getBaroTrend() const {
    return baroTrend;
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
Rainfall
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
DateTime
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
LoopPacket::Forecast
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
LoopPacket::isTransmitterBatteryGood(int index) const {
    return (transmitterBatteryStatus & (1 << (index - 1))) == 0;
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
    return stormStart != 0 && stormRain > 0.0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
LoopPacket::getBaroTrendString() const {
    switch(baroTrend) {
        case FALLING_RAPIDLY:
            return "Falling Rapidly";

        case FALLING_SLOWLY:
            return "Falling Slowly";

        case STEADY:
            return "Steady";

        case RISING_SLOWLY:
            return "Rising Slowly";

        case RISING_RAPIDLY:
            return "Rising Rapidly";

        default:
            return "Steady";
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
LoopPacket::getForecastIconString() const {
    switch (forecastIcon) {
        case SUNNY:
            return "Sunny";

        case PARTLY_CLOUDY:
            return "Partly Cloudy";

        case MOSTLY_CLOUDY:
            return "Mostly Cloudy";

        case MOSTLY_CLOUDY_WITH_RAIN:
            return "Mostly Cloudy With Rain";

        case MOSTLY_CLOUDY_WITH_SNOW:
            return "Mostly Cloudy With Snow";

        case MOSTLY_CLOUDY_WITH_RAIN_OR_SNOW:
            return "Mostly Cloudy With Rain or Snow";

        case PARTLY_CLOUDY_WITH_RAIN_LATER:
            return "Partly Cloudy With Rain Later";

        case PARTLY_CLOUDY_WITH_SNOW_LATER:
            return "Partly Cloudy With Snow Later";

        case PARTLY_CLOUDY_WITH_RAIN_OR_SNOW_LATER:
            return "Partly Cloudy With Snow Later";

        default:
            return "Sunny";
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const std::bitset<LoopPacket::ALARM_BITS> &
LoopPacket::getAlarmBits() const {
    return alarmBits;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
LoopPacket::getSunriseTime() const {
    return sunriseTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
LoopPacket::getSunsetTime() const {
    return sunsetTime;
}

}
