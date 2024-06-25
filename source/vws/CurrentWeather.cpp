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

#include "CurrentWeather.h"

#include <time.h>
#include <vector>
#include <iomanip>
#include <sstream>

#include "ForecastRule.h"
#include "LoopPacket.h"
#include "Loop2Packet.h"
#include "Weather.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeather::CurrentWeather() : packetTime(0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeather::~CurrentWeather() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeather::setLoopData(const LoopPacket & loopPacket) {
    this->loopPacket = loopPacket;
    windSpeed = loopPacket.getWindSpeed();
    windDirection = loopPacket.getWindDirection();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeather::setLoop2Data(const Loop2Packet & loop2Packet) {
    this->loop2Packet = loop2Packet;
    windSpeed = loop2Packet.getWindSpeed();
    windDirection = loop2Packet.getWindDirection();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeather::setPacketTime(DateTime time) {
    packetTime = time;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeather::setDominantWindDirectionData(const vector<std::string> & dominantWindDirs) {
    dominantWindDirections.clear();
    dominantWindDirections.assign(dominantWindDirs.begin(), dominantWindDirs.end());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const LoopPacket &
CurrentWeather::getLoopPacket() const {
    return loopPacket;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Loop2Packet &
CurrentWeather::getLoop2Packet() const {
    return loop2Packet;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
CurrentWeather::getPacketTime() const {
    return packetTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
CurrentWeather::getWindSpeed() const {
    return windSpeed;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
CurrentWeather::getWindDirection() const {
    return windDirection;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeather::formatJSON(bool pretty) const {
    DateTime cwTime;

    if (packetTime == 0)
        cwTime = time(0);
    else
        cwTime = packetTime;

    int indentLevel = 0;
    if (pretty)
        indentLevel = 1;

    ostringstream ss;
    ss << "{"
       << "\"time\" : \"" << Weather::formatDateTime(cwTime) << "\""
       << loopPacket.getInsideTemperature().formatJSON("insideTemperature", indentLevel, true)
       << loopPacket.getInsideHumidity().formatJSON("insideHumidity", indentLevel, true)
       << loopPacket.getOutsideTemperature().formatJSON("outsideTemperature", indentLevel, true)
       << loopPacket.getOutsideHumidity().formatJSON("outsideHumidity", indentLevel, true)
       << loop2Packet.getDewPoint().formatJSON("dewPoint", indentLevel, true)
       << loop2Packet.getWindChill().formatJSON("windChill", indentLevel, true)
       << loop2Packet.getHeatIndex().formatJSON("heatIndex", indentLevel, true)
       << loop2Packet.getThsw().formatJSON("thsw", indentLevel, true)
       << windSpeed.formatJSON("windSpeed", indentLevel, true)
       << windDirection.formatJSON("windDirection", indentLevel, true)
       << loop2Packet.getWindGust10Minute().formatJSON("gustSpeed", indentLevel, true)
       << loop2Packet.getWindGustDirection10Minute().formatJSON("gustDirection", indentLevel, true)
       << loop2Packet.getWindSpeed10MinuteAverage().formatJSON("windSpeed10MinAvg", indentLevel, true)
       << loop2Packet.getWindSpeed2MinuteAverage().formatJSON("windSpeed2MinAvg", indentLevel, true);

    ss << ", \"dominantWindDirections\" : [";
    for (unsigned int i = 0; i < dominantWindDirections.size(); i++) {
        if (i != 0)
            ss << ",";

        ss << "\"" << dominantWindDirections[i] << "\"";
    }
    ss << "]";

    ss << loopPacket.getBarometricPressure().formatJSON("barometricPressure", indentLevel, true)
       << loop2Packet.getBarometricSensorRawReading().formatJSON("atmosphericPressure", indentLevel, true)
       << ", \"barometerTrend\" : \"" << loopPacket.getBarometerTrendString() << "\""
       << ", \"rainRate\" : " << loopPacket.getRainRate()
       << ", \"rainToday\" : " << loopPacket.getDayRain()
       //<< ", \"rainToday2\" : " << loop2Packet.getDayRain()
       << ", \"rain15Minute\" : " << loop2Packet.get15MinuteRain()
       << ", \"rainHour\" : " << loop2Packet.getHourRain()
       << ", \"rain24Hour\" : " << loop2Packet.get24HourRain()
       << ", \"rainMonth\" : " << loopPacket.getMonthRain()
       << ", \"rainWeatherYear\" : " << loopPacket.getYearRain()
       << loopPacket.getSolarRadiation().formatJSON("solarRadiation", indentLevel, true);

    if (loopPacket.getDayET() > 0.0)
        ss << ", \"dayET\" : " << loopPacket.getDayET();

    if (loopPacket.getMonthET() > 0.0)
        ss << ", \"monthET\" : " << loopPacket.getMonthET();

    if (loopPacket.getYearET() > 0.0)
        ss << ", \"yearET\" : " << loopPacket.getYearET();

    ss << loopPacket.getUvIndex().formatJSON("uvIndex", indentLevel, true);

    if (loopPacket.isStormOngoing())
        ss << ", \"stormStart\" : \"" << loopPacket.getStormStart().formatDate() << "\""
           << ", \"stormRain\" : " << loopPacket.getStormRain();


    ss << ", \"forecastRule\" : \"" << ForecastRule::forecastString(loopPacket.getForecastRuleIndex()) << "\""
        << ", \"forecast\" : \"" << loopPacket.getForecastIconString() << "\"";

    struct tm sunriseTm;
    struct tm sunsetTm;
    Weather::localtime(loopPacket.getSunriseTime(), sunriseTm);
    Weather::localtime(loopPacket.getSunsetTime(), sunsetTm);

    ss << setfill('0') << setw(2);
    ss << ", \"sunrise\" : \"" << setw(2) << sunriseTm.tm_hour << ":" << setw(2) << sunriseTm.tm_min << "\""
       << ", \"sunset\" : \"" << setw(2) << sunsetTm.tm_hour << ":" << setw(2) << sunsetTm.tm_min << "\"";

    bool firstValue = true;
    ss << ", \"extraTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        if (loopPacket.getExtraTemperature(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getExtraTemperature(i).getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"extraHumidities\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++) {
        if (loopPacket.getExtraHumidity(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getExtraHumidity(i).getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++) {
        if (loopPacket.getSoilTemperature(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getSoilMoisture(i).getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilMoistures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        if (loopPacket.getSoilMoisture(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getSoilMoisture(i).getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"leafTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        if (loopPacket.getLeafTemperature(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getLeafTemperature(i).getValue() << " }";
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"leafWetnesses\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        if (loopPacket.getLeafWetness(i).isValid()) {
            if (!firstValue) ss << ", "; else firstValue = false;
            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getLeafWetness(i).getValue() << " }";
        }
    }
    ss << " ]";

    ss << " }";

    return ss.str();
}
}
