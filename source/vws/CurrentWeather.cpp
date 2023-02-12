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
#include <vector>
#include <iomanip>
#include <sstream>

#include "CurrentWeather.h"
#include "ForecastRule.h"
#include "Weather.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeather::CurrentWeather() {
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
    windSpeed10MinuteAverage = loopPacket.getWindSpeed10MinuteAverage();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeather::setLoop2Data(const Loop2Packet & loop2Packet) {
    this->loop2Packet = loop2Packet;
    windSpeed = loop2Packet.getWindSpeed();
    windDirection = loop2Packet.getWindDirection();
    windSpeed10MinuteAverage = loop2Packet.getWindSpeed10MinuteAverage();
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
int
CurrentWeather::getNextPacket() const {
    return loopPacket.getNextRecord();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeather::formatXML() const {
    ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
       << "<currentWeather>"
       << "<time>" << Weather::formatDateTime(time(0)) << "</time>"
       << loopPacket.getInsideTemperature().formatXML("insideTemperature")
       << loopPacket.getInsideHumidity().formatXML("insideHumidity")
       << loopPacket.getOutsideTemperature().formatXML("outsideTemperature")
       << loopPacket.getOutsideHumidity().formatXML("outsideHumidity")
       << loop2Packet.getDewPoint().formatXML("dewPoint")
       << loop2Packet.getWindChill().formatXML("windChill")
       << loop2Packet.getHeatIndex().formatXML("heatIndex")
       << loop2Packet.getThsw().formatXML("thsw");

    ss << "<wind><speed>" << windSpeed << "</speed><direction>" << windDirection << "</direction></wind>"
       << "<windGust><speed>" << loop2Packet.getWindGust10Minute() << "</speed><direction>" << loop2Packet.getWindGustDirection10Minute() << "</direction></windGust>"
       << "<windSpeed10MinAvg>" << windSpeed10MinuteAverage << "</windSpeed10MinAvg>"
       << "<windSpeed2MinAvg>" << loop2Packet.getWindSpeed2MinuteAverage() << "</windSpeed2MinAvg>";

    for (unsigned int i = 0; i < dominantWindDirections.size(); i++) {
        int windDirNumber = i + 1;
        ss << "<domWindDir" << windDirNumber << ">" << dominantWindDirections.at(i) << "</domWindDir" << windDirNumber << ">";
    }
   
    ss << loopPacket.getBarometricPressure().formatXML("baroPressure")
       << loop2Packet.getAtmPressure().formatXML("atmPressure")
       << "<baroTrend>" << loopPacket.getBarometerTrendString() << "</baroTrend>"
       << "<rainRate>" << loopPacket.getRainRate() << "</rainRate>"
       << "<rainToday>" << loopPacket.getDayRain() << "</rainToday>"
       << "<rain15Minute>" << loop2Packet.getRain15Minute() << "</rain15Minute>"
       << "<rainHour>" << loop2Packet.getRainHour() << "</rainHour>"
       << "<rain24Hour>" << loop2Packet.getRain24Hour() << "</rain24Hour>"
       << "<rainMonth>" << loopPacket.getMonthRain() << "</rainMonth>" 
       << "<rainWeatherYear>" << loopPacket.getYearRain() << "</rainWeatherYear>"
       << loopPacket.getSolarRadiation().formatXML("solarRadiation");

    if (loopPacket.getDayET() > 0.0)
        ss << "<dayET>" << loopPacket.getDayET() << "</dayET>";

    if (loopPacket.getMonthET() > 0.0)
        ss << "<monthET>" << loopPacket.getMonthET() << "</monthET>";

    if (loopPacket.getYearET() > 0.0)
        ss << "<yearET>" << loopPacket.getYearET() << "</yearET>";

    ss << loopPacket.getUvIndex().formatXML("uvIndex");

    if (loopPacket.isStormOngoing())
        ss << "<stormStart>" << Weather::formatDate(loopPacket.getStormStart()) << "</stormStart><stormRain>" << loopPacket.getStormRain() << "</stormRain>";


     ss << "<forecastRule>" << ForecastRule::forecastString(loopPacket.getForecastRuleIndex()) << "</forecastRule>"
        << "<forecast>" << loopPacket.getForecastIconString() << "</forecast>";

     ss << "<extraTemperatures>";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        if (loopPacket.getExtraTemperature(i).isValid()) {
            ss << "<temperature><index>" << i << "</index><value>" << loopPacket.getExtraTemperature(i).getValue() << "</value></temperature>";
        }
    }
    ss << "</extraTemperatures>";


    ss << "<extraHumidities>";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++) {
        if (loopPacket.getExtraHumidity(i).isValid())
            ss << "<humidity><index>" << i << "</index><value>" << loopPacket.getExtraHumidity(i).getValue() << "</value></humidity>";
    }
    ss << "</extraHumidities>";

    ss << "<soilMoistures>";
    /*
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        if (loopPacket.isSoilMoistureValid(i)) {
            ss << "<entry><key>" << 600 + i << "</key><value><sensorId>" << 600 + i << "</sensorId><sensorType>SOIL_MOISTURE</sensorType>"
               << "<measurement xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"soilMoisture\">"
               << loopPacket.getSoilMoisture(i) << "</measurement></value></entry>";
        }
    }
    */
    ss << "</soilMoistures>";

    ss << "<leafWetnesses>";
    /*
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        if (loopPacket.isLeafWetnessValid(i)) {
            ss << "<entry><key>" << 500 + i << "</key><value><sensorId>" << 500 + i << "</sensorId><sensorType>LEAF_WETNESS</sensorType>"
               << "<measurement xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"leafWetness\">"
               << loopPacket.getLeafWetness(i) << "</measurement></value></entry>";
        }
    }
    */
    ss << "</leafWetnesses>";

    ss << "</currentWeather>";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeather::formatJSON() const {
    ostringstream ss;
    ss << "{"
       << "\"time\" : \"" << Weather::formatDateTime(time(0)) << "\""
       << loopPacket.getInsideTemperature().formatJSON("insideTemperature", true)
       << loopPacket.getInsideHumidity().formatJSON("insideHumidity", true)
       << loopPacket.getOutsideTemperature().formatJSON("outsideTemperature", true)
       << loopPacket.getOutsideHumidity().formatJSON("outsideHumidity", true)
       << loop2Packet.getDewPoint().formatJSON("dewPoint", true)
       << loop2Packet.getWindChill().formatJSON("windChill", true)
       << loop2Packet.getHeatIndex().formatJSON("heatIndex", true)
       << loop2Packet.getThsw().formatJSON("thsw", true)
       << ", \"wind\" : { \"speed\" : " << windSpeed << ", \"direction\" : " <<  windDirection << " }"
       << ", \"windGust\" : { \"speed\" : " <<  loop2Packet.getWindGust10Minute()
       << ", \"direction\" : " <<  loop2Packet.getWindGustDirection10Minute() << " }"
       << ", \"windSpeed10MinAvg\" : " <<  windSpeed10MinuteAverage
       << ", \"windSpeed2MinAvg\" : " << loop2Packet.getWindSpeed2MinuteAverage();

    ss << ", \"dominantWindDirections\" : [";
    for (unsigned int i = 0; i < dominantWindDirections.size(); i++) {
        if (i != 0)
            ss << ",";

        ss << "\"" << dominantWindDirections[i] << "\"";
    }
    ss << "]";

    ss << loopPacket.getBarometricPressure().formatJSON("barometricPressure", true)
       << loop2Packet.getAtmPressure().formatJSON("atmosphericPressure", true)
       << ", \"barometerTrend\" : \"" << loopPacket.getBarometerTrendString() << "\""
       << ", \"rainRate\" : " << loopPacket.getRainRate()
       << ", \"rainToday\" : " << loopPacket.getDayRain()
       << ", \"rain15Minute\" : " << loop2Packet.getRain15Minute()
       << ", \"rainHour\" : " << loop2Packet.getRainHour()
       << ", \"rain24Hour\" : " << loop2Packet.getRain24Hour()
       << ", \"rainMonth\" : " << loopPacket.getMonthRain()
       << ", \"rainWeatherYear\" : " << loopPacket.getYearRain()
       << loopPacket.getSolarRadiation().formatJSON("solarRadiation", true);

    if (loopPacket.getDayET() > 0.0)
        ss << ", \"dayET\" : " << loopPacket.getDayET();

    if (loopPacket.getMonthET() > 0.0)
        ss << ", \"monthET\" : " << loopPacket.getMonthET();

    if (loopPacket.getYearET() > 0.0)
        ss << ", \"yearET\" : " << loopPacket.getYearET();

    ss << loopPacket.getUvIndex().formatJSON("uvIndex", true);

    if (loopPacket.isStormOngoing())
        ss << ", \"stormStart\" : \"" << Weather::formatDate(loopPacket.getStormStart()) << "\""
           << ", \"stormRain\" : " << loopPacket.getStormRain();


    ss << ", \"forecastRule\" : \"" << ForecastRule::forecastString(loopPacket.getForecastRuleIndex()) << "\""
        << ", \"forecast\" : \"" << loopPacket.getForecastIconString() << "\"";

    bool firstValue = true;
    ss << ", \"extraTemperatures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        if (loopPacket.getExtraTemperature(i).isValid()) {
            if (!firstValue)
                ss << ", ";

            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getExtraTemperature(i).getValue() << " }";
            firstValue = false;
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"extraHumidities\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++) {
        if (loopPacket.getExtraHumidity(i).isValid()) {
            if (!firstValue)
                ss << ", ";

            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getExtraHumidity(i).getValue() << " }";
            firstValue = false;
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"soilMoistures\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        if (loopPacket.getSoilMoisture(i).isValid()) {
            if (!firstValue)
                ss << ", ";

            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getSoilMoisture(i).getValue() << " }";
            firstValue = false;
        }
    }
    ss << " ]";

    firstValue = true;
    ss << ", \"leafWetnesses\" : [ ";
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        if (loopPacket.getLeafWetness(i).isValid()) {
            if (!firstValue)
                ss << ", ";

            ss << "{ \"index\" : " << i << ", \"value\" : " << loopPacket.getLeafWetness(i).getValue() << " }";
            firstValue = false;
        }
    }
    ss << " ]";

    ss << " }";

    return ss.str();
}
}
