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
#include <sstream>
#include "BitConverter.h"
#include "Weather.h"
#include "HiLowPacket.h"
#include "VantageDecoder.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
HiLowPacket::HiLowPacket() : highHourRainRate(0.0), logger(VantageLogger::getLogger("HiLowPacket")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
HiLowPacket::~HiLowPacket() {
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
bool
HiLowPacket::Values<T>::isValid() const {
    return dayExtremeValue.isValid() &&  monthExtremeValue.isValid() && yearExtremeValue.isValid();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::Values<T>::formatXML(bool low) const {
    ostringstream ss;

    if (!isValid())
        return "";

    string which = low ? "low" : "high";

    ss << "    <" << which << ">" << endl
       << "        <day>" << endl
       << "            " << dayExtremeValue.formatXML("value") << endl
       << "            <time>" << Weather::formatDateTime(dayExtremeValueTime) << "</time>" << endl
       << "        </day>" << endl
       << "        " << monthExtremeValue.formatXML("month") << endl
       << "        " << yearExtremeValue.formatXML("year") << endl
       << "    </" << which << ">" << endl;

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::Values<T>::formatJSON(bool low) const {
    ostringstream ss;

    if (!isValid())
        return "";

    string which = low ? "low" : "high";

    ss << "    \"" << which << "\" : " << endl
       << "        { \"day\" : { \"value\" : " << dayExtremeValue.getValue() << ", \"time\"  : \"" << Weather::formatDateTime(dayExtremeValueTime) << "\" }," << endl
       << "          \"month\" : " << monthExtremeValue << ", \"year\"  : " << yearExtremeValue << " }";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
bool
HiLowPacket::HighLowValues<T>::isValid() const {
    return lows.isValid() && highs.isValid();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::HighLowValues<T>::formatXML() const {
    string s = lows.formatXML(true).append(highs.formatXML(false));
    return s;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::HighLowValues<T>::formatJSON() const {
    string s = lows.formatJSON(true).append(",\n").append(highs.formatJSON(false));
    return s;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
HiLowPacket::formatXML() const {
    ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << endl;
    ss << "<hiLowPacket>" << endl;
    ss << "<barometer>" << endl;
    ss << barometer.formatXML();
    ss << "</barometer>" << endl;
    ss << "<wind>" << endl;
    ss << wind.formatXML(false) << endl;
    ss << "</wind>" << endl;
    ss << "<indoorTemperature>" << endl;
    ss << indoorTemperature.formatXML();
    ss << "</indoorTemperature>" << endl;
    ss << "<outdoorTemperature>" << endl;
    ss << outdoorTemperature.formatXML();
    ss << "</outdoorTemperature>" << endl;
    ss << "<indoorHumidity>" << endl;
    ss << indoorHumidity.formatXML();
    ss << "</indoorHumidity>" << endl;
    ss << "<outdoorHumidity>" << endl;
    ss << outdoorHumidity.formatXML();
    ss << "</outdoorHumidity>" << endl;
    ss << "<dewPoint>" << endl;
    ss << dewPoint.formatXML();
    ss << "</dewPoint>" << endl;
    ss << "<windChill>" << endl;
    ss << windChill.formatXML(true);
    ss << "</windChill>" << endl;
    ss << "<heatIndex>" << endl;
    ss << heatIndex.formatXML(false);
    ss << "</heatIndex>" << endl;
    ss << "<thsw>" << endl;
    ss << thsw.formatXML(false);
    ss << "</thsw>" << endl;
    ss << "<solarRadiation>" << endl;
    ss << solarRadiation.formatXML(false);
    ss << "</solarRadiation>" << endl;
    ss << "<uvIndex>" << endl;
    ss << uvIndex.formatXML(false);
    ss << "</uvIndex>" << endl;
    ss << "<rainfallRate>" << endl;
    ss << "    <hour>" << highHourRainRate <<"</hour>" << endl;
    ss << rainRate.formatXML(false);
    ss << "</rainfallRate>" << endl;
    ss << "<extraTemperatures>" << endl;
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        if (extraTemperature[i].isValid()) {
            ss << "<extraTemperature>";
            ss << "    <index>" << i << "</index>" << endl;
            ss << "    <values>" << endl;
            ss << extraTemperature[i].formatXML();
            ss << "    </values>" << endl;
            ss << "</extraTemperature>";
        }
    }
    ss << "</extraTemperatures>" << endl;
    ss << "</hiLowPacket>";

   return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
HiLowPacket::formatJSON() const {
    ostringstream ss;
    ss << "{ \"high-low\" : " << endl
       << "    { \"outdoorTemperature\" : " << outdoorTemperature.formatJSON() << " }," << endl
       << "    { \"indoorTemperature\" : " << indoorTemperature.formatJSON() << " }," << endl
       << "    { \"barometer\" : " << barometer.formatJSON() << " }," << endl
       << "    { \"rainRate\" : " << rainRate.formatJSON(false) << " }" << endl
       << "}" << endl;

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
HiLowPacket::decodeHiLowPacket(byte buffer[]) {

    //
    // Barometer section
    //
    bool valid;
    barometer.lows.dayExtremeValue      = VantageDecoder::decodeBarometricPressure(buffer, 0);
    barometer.highs.dayExtremeValue     = VantageDecoder::decodeBarometricPressure(buffer, 2);
    barometer.lows.monthExtremeValue    = VantageDecoder::decodeBarometricPressure(buffer, 4);
    barometer.highs.monthExtremeValue   = VantageDecoder::decodeBarometricPressure(buffer, 6);
    barometer.lows.yearExtremeValue     = VantageDecoder::decodeBarometricPressure(buffer, 8);
    barometer.highs.yearExtremeValue    = VantageDecoder::decodeBarometricPressure(buffer, 10);
    barometer.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 12);
    barometer.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 14);

    //
    // Wind section
    //
    wind.dayExtremeValue     = VantageDecoder::decodeWindSpeed(buffer, 16);
    wind.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 17);
    wind.monthExtremeValue   = VantageDecoder::decodeWindSpeed(buffer, 19);
    wind.yearExtremeValue    = VantageDecoder::decodeWindSpeed(buffer, 20);

    //
    // Indoor temperature section
    //
    indoorTemperature.highs.dayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 21);
    indoorTemperature.lows.dayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 23);
    indoorTemperature.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 25);
    indoorTemperature.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 27);
    indoorTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 29);
    indoorTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 31);
    indoorTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 33);
    indoorTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 35);

    //
    // Indoor humidity section
    //
    indoorHumidity.highs.dayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 37);
    indoorHumidity.lows.dayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 38);
    indoorHumidity.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 39);
    indoorHumidity.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 41);
    indoorHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 43);
    indoorHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 44);
    indoorHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 45);
    indoorHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 46);

    //
    // Outdoor temperature section
    //
    outdoorTemperature.lows.dayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 47);
    outdoorTemperature.highs.dayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 49);
    outdoorTemperature.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 51);
    outdoorTemperature.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 53);
    outdoorTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 55);
    outdoorTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 57);
    outdoorTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 59);
    outdoorTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 61);

    //
    // Dew point section
    //
    dewPoint.lows.dayExtremeValue      = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 63);
    dewPoint.highs.dayExtremeValue     = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 65);
    dewPoint.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 67);
    dewPoint.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 69);
    dewPoint.highs.monthExtremeValue   = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 71);
    dewPoint.lows.monthExtremeValue    = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 73);
    dewPoint.highs.yearExtremeValue    = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 75);
    dewPoint.lows.yearExtremeValue     = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 77);

    //
    // Wind chill section
    //
    windChill.dayExtremeValue      = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 79);
    windChill.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 81);
    windChill.monthExtremeValue    = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 83);
    windChill.yearExtremeValue     = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 85);

    //
    // Heat index section
    //
    heatIndex.dayExtremeValue      = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 87);
    heatIndex.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 89);
    heatIndex.monthExtremeValue    = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 91);
    heatIndex.yearExtremeValue     = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 93);

    //
    // THSW index section
    //
    thsw.dayExtremeValue      = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 95);
    thsw.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 97);
    thsw.monthExtremeValue    = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 99);
    thsw.yearExtremeValue     = VantageDecoder::decodeNonScaled16BitTemperature(buffer, 101);

    //
    // Solar radiation section
    //
    solarRadiation.dayExtremeValue      = VantageDecoder::decodeSolarRadiation(buffer, 103);
    solarRadiation.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 105);
    solarRadiation.monthExtremeValue    = VantageDecoder::decodeSolarRadiation(buffer, 107);
    solarRadiation.yearExtremeValue     = VantageDecoder::decodeSolarRadiation(buffer, 109);

    //
    // UV section
    //
    uvIndex.dayExtremeValue      = VantageDecoder::decodeUvIndex(buffer, 111);
    uvIndex.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 112);
    uvIndex.monthExtremeValue    = VantageDecoder::decodeUvIndex(buffer, 114);
    uvIndex.yearExtremeValue     = VantageDecoder::decodeUvIndex(buffer, 115);

    //
    // Rain rate section
    //
    rainRate.dayExtremeValue      = VantageDecoder::decodeRain(buffer, 116);
    rainRate.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 118);
    highHourRainRate              = VantageDecoder::decodeRain(buffer, 120);
    rainRate.monthExtremeValue    = VantageDecoder::decodeRain(buffer, 122);
    rainRate.yearExtremeValue     = VantageDecoder::decodeRain(buffer, 124);

    //
    // Extra temperatures section
    //
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        extraTemperature[i].lows.dayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + i);
        extraTemperature[i].highs.dayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + i);
        extraTemperature[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 156 + (i * 2));
        extraTemperature[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 186 + (i * 2));
        extraTemperature[i].highs.monthExtremeValue   = VantageDecoder::decode8BitTemperature(buffer, 216 + i);
        extraTemperature[i].lows.monthExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 231 + i);
        extraTemperature[i].highs.yearExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 246 + i);
        extraTemperature[i].lows.yearExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 261 + i);
    }

    //
    // Soil temperatures section
    //
    int offset = ProtocolConstants::MAX_EXTRA_TEMPERATURES;
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_TEMPERATURES; i++) {
        soilTemperature[i].lows.dayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + offset + i);
        soilTemperature[i].highs.dayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + offset + i);
        soilTemperature[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 156 + (offset * 2) + (i * 2));
        soilTemperature[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 186 + (offset * 2) + (i * 2));
        soilTemperature[i].highs.monthExtremeValue   = VantageDecoder::decode8BitTemperature(buffer, 216 + offset + i);
        soilTemperature[i].lows.monthExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 231 + offset + i);
        soilTemperature[i].highs.yearExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 246 + offset + i);
        soilTemperature[i].lows.yearExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 261 + offset + i);
    }

    //
    // Leaf temperature
    offset = ProtocolConstants::MAX_EXTRA_TEMPERATURES + ProtocolConstants::MAX_SOIL_TEMPERATURES;
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        leafTemperature[i].lows.dayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + offset + i);
        leafTemperature[i].highs.dayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + offset + i);
        leafTemperature[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 156 + (offset * 2) + (i * 2));
        leafTemperature[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 186 + (offset * 2) + (i * 2));
        leafTemperature[i].highs.monthExtremeValue   = VantageDecoder::decode8BitTemperature(buffer, 216 + offset + i);
        leafTemperature[i].lows.monthExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 231 + offset + i);
        leafTemperature[i].highs.yearExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 246 + offset + i);
        leafTemperature[i].lows.yearExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 261 + offset + i);
    }

    //
    // Outdoor humidity section
    //
    outdoorHumidity.lows.dayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 276);
    outdoorHumidity.highs.dayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 284);
    outdoorHumidity.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 308);
    outdoorHumidity.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 292);
    outdoorHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 324);
    outdoorHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 332);
    outdoorHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 340);
    outdoorHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 348);

    //
    // Extra humidity section
    //
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++) {
        extraHumidity[i].lows.dayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 277 + i);
        extraHumidity[i].highs.dayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 285 + i);
        extraHumidity[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 309 + i);
        extraHumidity[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 293 + i);
        extraHumidity[i].highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 325 + i);
        extraHumidity[i].lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 333 + i);
        extraHumidity[i].highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 341 + 1);
        extraHumidity[i].lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 349 + i);
    }

    //
    // Soil moisture section
    //
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        soilMoisture[i].highs.dayExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 356 + i);
        soilMoisture[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 360 + i);
        soilMoisture[i].lows.dayExtremeValue      = VantageDecoder::decodeSoilMoisture(buffer, 368 + i);
        soilMoisture[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 372 + i);
        soilMoisture[i].lows.monthExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 380 + i);
        soilMoisture[i].highs.monthExtremeValue   = VantageDecoder::decodeSoilMoisture(buffer, 384 + i);
        soilMoisture[i].lows.yearExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 388 + i);
        soilMoisture[i].highs.yearExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 392 + 1);
    }

    //
    // Leaf wetness section
    //
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        leafWetness[i].highs.dayExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 396 + i);
        leafWetness[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 400 + i);
        leafWetness[i].lows.dayExtremeValue      = VantageDecoder::decodeLeafWetness(buffer, 408 + i);
        leafWetness[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 412 + i);
        leafWetness[i].lows.monthExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 420 + i);
        leafWetness[i].highs.monthExtremeValue   = VantageDecoder::decodeLeafWetness(buffer, 424 + i);
        leafWetness[i].lows.yearExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 428 + i);
        leafWetness[i].highs.yearExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 432 + 1);
    }

    return true;
}

}
