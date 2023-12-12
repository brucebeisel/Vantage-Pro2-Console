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

#include "Loop2Packet.h"

#include <iostream>
#include <cstring>

#include "BitConverter.h"
#include "VantageProtocolConstants.h"
#include "VantageCRC.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "VantageEnums.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Loop2Packet::Loop2Packet() : logger(&VantageLogger::getLogger("Loop2Packet")),
                             barometerTrend(BarometerTrend::STEADY),
                             packetType(-1),
                             rainRate(0.0),
                             stormRain(0.0),
                             stormStart(0),
                             rainDay(0.0),
                             rain15Minute(0.0),
                             rainHour(0.0),
                             rain24Hour(0.0),
                             barometricReductionMethod(2),
                             next10MinuteWindSpeedGraphPointer(0),
                             next15MinuteWindSpeedGraphPointer(0),
                             nextHourlyWindSpeedGraphPointer(0),
                             nextDailyWindSpeedGraphPointer(0),
                             nextMinuteRainGraphPointer(0),
                             nextRainStormGraphPointer(0),
                             nextMonthlyRainGraphPointer(0),
                             nextYearlyRainGraphPointer(0),
                             nextSeasonalRainGraphPointer(0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Loop2Packet::~Loop2Packet() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const byte *
Loop2Packet::getPacketData() const {
    return packetData;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ProtocolConstants::BarometerTrend
Loop2Packet::getBarometerTrend() const {
    return barometerTrend;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
Loop2Packet::getBarometerTrendString() const {
    return barometerTrendEnum.valueToString(barometerTrend);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getPacketType() const {
    return packetType;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getBarometricPressure() const {
    return barometricPressure;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getInsideTemperature() const {
    return insideTemperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
Loop2Packet::getInsideHumidity() const {
    return insideHumidity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getOutsideTemperature() const {
    return outsideTemperature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed() const {
    return windSpeed;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
Loop2Packet::getWindDirection() const {
    return windDirection;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed10MinuteAverage() const {
    return windSpeed10MinuteAverage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed2MinuteAverage() const {
    return windSpeed2MinuteAverage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindGust10Minute() const {
    return windGust10Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
Loop2Packet::getWindGustDirection10Minute() const {
    return windGustDirection10Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getDewPoint() const {
    return dewPoint;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Humidity> &
Loop2Packet::getOutsideHumidity() const {
    return insideHumidity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getHeatIndex() const {
    return heatIndex;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getWindChill() const {
    return windChill;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getThsw() const {
    return thsw;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getRainRate() const {
    return rainRate;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<UvIndex> &
Loop2Packet::getUvIndex() const {
    return uvIndex;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<SolarRadiation> &
Loop2Packet::getSolarRadiation() const {
    return solarRadiation;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getStormRain() const {
    return stormRain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
Loop2Packet::getStormStart() const {
    return stormStart;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getDayRain() const {
    return rainDay;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::get15MinuteRain() const {
    return rain15Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getHourRain() const {
    return rainHour;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Evapotranspiration> &
Loop2Packet::getDayET() const {
    return dayET;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::get24HourRain() const {
    return rain24Hour;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getBarometricReductionMethod() const {
    return barometricReductionMethod;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getUserEnteredBarometricOffset() const {
    return userEnteredBarometricOffset;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getBarometricCalibrationNumber() const {
    return barometricCalibrationNumber;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getBarometricSensorRawReading() const {
    return barometricSensorRawReading;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getAbsoluteBarometricPressure() const {
    return absoluteBarometricPressure;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getAltimeterBarometerOffset() const {
    return altimeterSetting;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNext10MinuteWindSpeedGraphPointer() const {
    return next10MinuteWindSpeedGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNext15MinuteWindSpeedGraphPointer() const {
    return next15MinuteWindSpeedGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextHourlyWindSpeedGraphPointer() const {
    return nextHourlyWindSpeedGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextDailyWindSpeedGraphPointer() const {
    return nextDailyWindSpeedGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextMinuteRainGraphPointer() const {
    return nextMinuteRainGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextRainStormGraphPointer() const {
    return nextRainStormGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getIndexToTheMinuteWithinAnHour() const {
    return indexToTheMinuteWithinAnHour;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextMonthlyRainGraphPointer() const {
    return nextMonthlyRainGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextYearlyRainGraphPointer() const {
    return nextYearlyRainGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Loop2Packet::getNextSeasonalRainGraphPointer() const {
    return nextSeasonalRainGraphPointer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
Loop2Packet::decodeLoop2Packet(const byte buffer[]) {

    memcpy(packetData, buffer, LOOP2_PACKET_SIZE);
    //
    // Perform packet validation before decoding the actual data
    //
    if (!VantageCRC::checkCRC(packetData, CRC_OFFSET)) {
        logger->log(VantageLogger::VANTAGE_ERROR) << "LOOP2 packet failed CRC check" << endl;
        return false;
    }

    if (packetData[L_OFFSET] != 'L' || packetData[FIRST_O_OFFSET] != 'O' || packetData[SECOND_O_OFFSET] != 'O') {
        logger->log(VantageLogger::VANTAGE_ERROR) << "LOOP2 packet data does not begin with LOO: "
                                      << "[0] = " << packetData[L_OFFSET] << " [1] = " << packetData[FIRST_O_OFFSET] << " [2] = " << packetData[SECOND_O_OFFSET] << endl;
        return false;
    }

    packetType = BitConverter::toUint8(packetData, PACKET_TYPE_OFFSET);

    if (packetType != LOOP2_PACKET_TYPE) {
        logger->log(VantageLogger::VANTAGE_ERROR) << "Invalid packet type for LOOP2 packet. "
                                      << "Expected " << LOOP2_PACKET_TYPE << " Received: " << packetType << endl;
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

    barometricPressure = VantageDecoder::decodeBarometricPressure(packetData, BAROMETER_OFFSET);
    insideTemperature = VantageDecoder::decode16BitTemperature(packetData, INSIDE_TEMPERATURE_OFFSET);
    insideHumidity = VantageDecoder::decodeHumidity(packetData, INSIDE_HUMIDITY_OFFSET);
    outsideTemperature = VantageDecoder::decode16BitTemperature(packetData, OUTSIDE_TEMPERATURE_OFFSET);

    windSpeed = VantageDecoder::decodeWindSpeed(packetData, WIND_SPEED_OFFSET);
    windDirection = VantageDecoder::decodeWindDirection(packetData, WIND_DIRECTION_OFFSET);
    windSpeed10MinuteAverage = VantageDecoder::decodeAverageWindSpeed(packetData, TEN_MINUTE_AVG_WIND_SPEED_OFFSET);
    windSpeed2MinuteAverage = VantageDecoder::decodeAverageWindSpeed(packetData, TWO_MINUTE_AVG_WIND_SPEED_OFFSET);
    windGust10Minute = VantageDecoder::decode16BitWindSpeed(packetData, TEN_MINUTE_WIND_GUST_OFFSET);
    windGustDirection10Minute = VantageDecoder::decodeWindDirection(packetData, TEN_MINUTE_WIND_GUST_DIRECTION_OFFSET);

    dewPoint = VantageDecoder::decode16BitTemperature(packetData, DEW_POINT_OFFSET, false);
    outsideHumidity = VantageDecoder::decodeHumidity(packetData, OUTSIDE_HUMIDITY_OFFSET);
    heatIndex = VantageDecoder::decode16BitTemperature(packetData, HEAT_INDEX_OFFSET, false);
    windChill = VantageDecoder::decode16BitTemperature(packetData, WIND_CHILL_OFFSET, false);
    thsw = VantageDecoder::decode16BitTemperature(packetData, THSW_OFFSET, false);

    rainRate = VantageDecoder::decodeRain(packetData, RAIN_RATE_OFFSET);

    uvIndex = VantageDecoder::decodeUvIndex(packetData, UV_INDEX_OFFSET);
    solarRadiation = VantageDecoder::decodeSolarRadiation(packetData, SOLAR_RADIATION_OFFSET);

    stormRain = VantageDecoder::decodeStormRain(packetData, STORM_RAIN_OFFSET);
    stormStart = VantageDecoder::decodeStormDate(packetData, STORM_START_DATE_OFFSET);

    rainDay = VantageDecoder::decodeRain(packetData, DAY_RAIN_OFFSET);
    rain15Minute = VantageDecoder::decodeRain(packetData, FIFTEEN_MINUTE_RAIN_OFFSET);
    rainHour = VantageDecoder::decodeRain(packetData, HOUR_RAIN_OFFSET);
    dayET = VantageDecoder::decodeDayET(packetData, DAY_ET_OFFSET);
    rain24Hour = VantageDecoder::decodeRain(packetData, TWENTY_FOUR_HOUR_RAIN_OFFSET);

    barometricReductionMethod = BitConverter::toUint8(packetData, BAROMETRIC_REDUCTION_METHOD_OFFSET);
    userEnteredBarometricOffset = VantageDecoder::decodeBarometricPressure(packetData, USER_ENTERED_BAROMETRIC_OFFSET_OFFSET);
    barometricCalibrationNumber = VantageDecoder::decodeBarometricPressure(packetData, BAROMETRIC_CALIBRATION_NUMBER_OFFSET);
    barometricSensorRawReading = VantageDecoder::decodeBarometricPressure(packetData, BAROMETRIC_SENSOR_RAW_READING_OFFSET);
    absoluteBarometricPressure = VantageDecoder::decodeBarometricPressure(packetData, ABSOLUTE_BAROMETRIC_PRESSURE_OFFSET);
    altimeterSetting = VantageDecoder::decodeBarometricPressure(packetData, ALTIMETER_SETTING_OFFSET);

    next10MinuteWindSpeedGraphPointer = BitConverter::toUint8(buffer, NEXT_10_MINUTE_WIND_SPEED_GRAPH_POINTER_OFFSET);
    next15MinuteWindSpeedGraphPointer = BitConverter::toUint8(buffer, NEXT_15_MINUTE_WIND_SPEED_GRAPH_POINTER_OFFSET);
    nextHourlyWindSpeedGraphPointer = BitConverter::toUint8(buffer, NEXT_HOURLY_WIND_SPEED_GRAPH_POINTER_OFFSET);
    nextDailyWindSpeedGraphPointer = BitConverter::toUint8(buffer, NEXT_DAILY_WIND_SPEED_GRAPH_POINTER_OFFSET);
    nextMinuteRainGraphPointer = BitConverter::toUint8(buffer, NEXT_MINUTE_RAIN_GRAPH_POINTER_OFFSET);
    nextRainStormGraphPointer = BitConverter::toUint8(buffer, NEXT_RAIN_STORM_GRAPH_POINTER_OFFSET);
    nextMonthlyRainGraphPointer = BitConverter::toUint8(buffer, NEXT_MONTHLY_RAIN_GRAPH_POINTER_OFFSET);
    nextYearlyRainGraphPointer = BitConverter::toUint8(buffer, NEXT_YEARLY_RAIN_GRAPH_POINTER_OFFSET);
    nextSeasonalRainGraphPointer = BitConverter::toUint8(buffer, NEXT_SEASONAL_RAIN_GRAPH_POINTER_OFFSET);

    return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::ostream &
operator<<(std::ostream & os, const Loop2Packet & packet) {
    os << "++++++++++ LOOP2 ++++++++++" << endl
       << "Packet Byte 0:   " << static_cast<char>(packet.packetData[0]) << endl
       << "Packet Byte 1:   " << static_cast<char>(packet.packetData[1]) << endl
       << "Packet Byte 2:   " << static_cast<char>(packet.packetData[2]) << endl
       << "Barometer Trend: " << packet.getBarometerTrend() << endl
       << "Packet Type:     " << packet.getPacketType() << endl
       << "Barometer:       " << packet.getBarometricPressure() << endl
       << "Inside Temperature: " << packet.getInsideTemperature() << endl
       << "Inside Humidity: " << packet.getInsideHumidity() << endl
       << "Outside Temperature: " << packet.getOutsideTemperature() << endl
       << "Wind Speed: " << packet.getWindSpeed() << endl
       << "10 Minute Avg Wind Speed: " << packet.getWindSpeed10MinuteAverage() << endl
       << "2 Minute Avg Wind Speed: " << packet.getWindSpeed2MinuteAverage() << endl
       << "10 Minute Wind Gust: " << packet.getWindGust10Minute() << endl
       << "10 Minute Wind Gust Direction: " << packet.getWindGustDirection10Minute() << endl
       << "Dew Point: " << packet.getDewPoint() << endl
       << "Outside Humidity: " << packet.getOutsideHumidity() << endl
       << "Dew Point: " << packet.getDewPoint() << endl
       << "Outside Humidity: " << packet.getOutsideHumidity() << endl
       << "Heat Index: " << packet.getHeatIndex() << endl
       << "Wind Chill: " << packet.getWindChill() << endl
       << "THSW: " << packet.getThsw() << endl
       << "Rain Rate: " << packet.getRainRate() << endl
       << "UV Index: " << packet.getUvIndex() << endl
       << "Solar Radiation: " << packet.getSolarRadiation() << endl
       << "Storm Rain: " << packet.getStormRain() << endl
       << "Storm Start Date: " << packet.getStormStart() << endl
       << "Day Rain: " << packet.getDayRain() << endl
       << "15 Minute Rain: " << packet.get15MinuteRain() << endl
       << "Hour Rain: " << packet.getHourRain() << endl
       << "Day ET: " << packet.getDayET() << endl
       << "24 Hour Rain: " << packet.get24HourRain() << endl
       << "Barometric Reduction Method: " << packet.getBarometricReductionMethod() << endl
       << "User Entered Barometric Offset: " << packet.getUserEnteredBarometricOffset() << endl
       << "Barometric Calibration Number: " << packet.getBarometricCalibrationNumber() << endl
       << "Barometric Sensor Raw Reading: " << packet.getBarometricSensorRawReading() << endl
       << "Absolute Barometric Pressure: " << packet.getAbsoluteBarometricPressure() << endl
       << "Altimeter Barometric Offset: " << packet.getAltimeterBarometerOffset() << endl
       << "Next 10 Minute Wind Speed Graph Pointer: " << packet.getNext10MinuteWindSpeedGraphPointer() << endl
       << "Next 15 Minute Wind Speed Graph Pointer: " << packet.getNext15MinuteWindSpeedGraphPointer() << endl
       << "Next Hourly Wind Speed Graph Pointer: " << packet.getNextHourlyWindSpeedGraphPointer() << endl
       << "Next Daily Wind Speed Graph Pointer: " << packet.getNextDailyWindSpeedGraphPointer() << endl
       << "Next Minute Rain Graph Pointer: " << packet.getNextMinuteRainGraphPointer() << endl
       << "Next Rain Storm Graph Pointer: " << packet.getNextRainStormGraphPointer() << endl
       << "Next Monthly Rain Graph Pointer: " << packet.getNextMonthlyRainGraphPointer() << endl
       << "Next Yearly Rain Graph Pointer: " << packet.getNextYearlyRainGraphPointer() << endl
       << "Next Seasonal Rain Graph Pointer: " << packet.getNextSeasonalRainGraphPointer() << endl
       << "Terminator 1" << "0x" << hex << static_cast<int>(packet.packetData[95]) << dec << endl
       << "Terminator 2" << "0x" << hex << static_cast<int>(packet.packetData[96]) << dec << endl;

    return os;

}
}
