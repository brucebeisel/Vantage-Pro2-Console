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

#include "HiLowPacket.h"

#include <sstream>
#include <iomanip>

#include "BitConverter.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "Weather.h"

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
    return todayExtremeValue.isValid() &&  monthExtremeValue.isValid() && yearExtremeValue.isValid();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::Values<T>::formatJSON(bool low) const {
    ostringstream ss;

    string which = low ? "low" : "high";

    ss << " \"" << which << "\" : {"
       << " \"today\" : { ";

    if (todayExtremeValue.isValid())
        ss << "\"value\" : " << todayExtremeValue.getValue() << ", \"time\"  : \"" << formatExtremeValueTime() << "\" } ";
    else
        ss << " }";

    if (monthExtremeValue.isValid())
        ss << ", \"month\" : " << monthExtremeValue;

    if (yearExtremeValue.isValid())
        ss << ", \"year\"  : " << yearExtremeValue;

    ss << " }";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
string
HiLowPacket::Values<T>::formatExtremeValueTime() const {

    ostringstream oss;
    if (todayExtremeValueTime != ProtocolConstants::INVALID_16BIT_TIME) {
        int hour = todayExtremeValueTime / 100;
        int minute = todayExtremeValueTime % 100;
        oss << hour << ":" << setw(2) << setfill('0') << minute;
    }
    else
        oss << "N/A";

    return oss.str();

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
HiLowPacket::HighLowValues<T>::formatJSON() const {
    string s = lows.formatJSON(true).append(",\n").append(highs.formatJSON(false));
    return s;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
HiLowPacket::formatJSON() const {
    ostringstream ss;
    ss << "{ "
       << " \"highLow\" : {"
       << " \"outsideTemperature\" : {" << outsideTemperature.formatJSON() << " },"
       << " \"outsideHumidity\" : {" << outsideHumidity.formatJSON() << " },"
       << " \"dewPoint\" : {" << dewPoint.formatJSON() << " },"
       << " \"heatIndex\" : {" << heatIndex.formatJSON(false) << " },"
       << " \"windChill\" : {" << windChill.formatJSON(true) << " },"
       << " \"thsw\" : {" << thsw.formatJSON(false) << " },"
       << " \"insideTemperature\" : {" << insideTemperature.formatJSON() << " },"
       << " \"insideHumidity\" : {" << insideHumidity.formatJSON() << " },"
       << " \"windSpeed\" : {" << wind.formatJSON(false) << " },"
       << " \"barometer\" : {" << barometer.formatJSON() << " },"
       << " \"uvIndex\" : {" << uvIndex.formatJSON(false) << " },"
       << " \"solarRadiation\" : {" << solarRadiation.formatJSON(false) << " },"
       << " \"rainRate\" : { \"high\" : {"
       << " \"today\" : { \"value\" : " << rainRate.todayExtremeValue << ", \"time\"  : \"" << rainRate.formatExtremeValueTime() << "\" },"
       << " \"hour\" : " << highHourRainRate << ", "
       << " \"month\" : " << rainRate.monthExtremeValue << ", \"year\"  : " << rainRate.yearExtremeValue
       << " } } } }";

    return ss.str();
    /*
    HighLowValues<Temperature>  extraTemperature[ProtocolConstants::MAX_EXTRA_TEMPERATURES];
    HighLowValues<Temperature>  soilTemperature[ProtocolConstants::MAX_SOIL_TEMPERATURES];
    HighLowValues<Temperature>  leafTemperature[ProtocolConstants::MAX_LEAF_TEMPERATURES];
    HighLowValues<Humidity>     extraHumidity[ProtocolConstants::MAX_EXTRA_HUMIDITIES];
    HighLowValues<SoilMoisture> soilMoisture[ProtocolConstants::MAX_SOIL_MOISTURES];
    HighLowValues<LeafWetness>  leafWetness[ProtocolConstants::MAX_LEAF_WETNESSES];
    */
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
HiLowPacket::decodeHiLowPacket(byte buffer[]) {

    //
    // Barometer section
    //
    bool valid;
    barometer.lows.todayExtremeValue      = VantageDecoder::decodeBarometricPressure(buffer, 0);
    barometer.highs.todayExtremeValue     = VantageDecoder::decodeBarometricPressure(buffer, 2);
    barometer.lows.monthExtremeValue    = VantageDecoder::decodeBarometricPressure(buffer, 4);
    barometer.highs.monthExtremeValue   = VantageDecoder::decodeBarometricPressure(buffer, 6);
    barometer.lows.yearExtremeValue     = VantageDecoder::decodeBarometricPressure(buffer, 8);
    barometer.highs.yearExtremeValue    = VantageDecoder::decodeBarometricPressure(buffer, 10);
    barometer.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 12);
    barometer.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 14);

    //
    // Wind section
    //
    wind.todayExtremeValue     = VantageDecoder::decodeWindSpeed(buffer, 16);
    wind.todayExtremeValueTime = BitConverter::toUint16(buffer, 17);
    wind.monthExtremeValue   = VantageDecoder::decodeWindSpeed(buffer, 19);
    wind.yearExtremeValue    = VantageDecoder::decodeWindSpeed(buffer, 20);

    //
    // Inside temperature section
    //
    insideTemperature.highs.todayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 21);
    insideTemperature.lows.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 23);
    insideTemperature.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 25);
    insideTemperature.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 27);
    insideTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 29);
    insideTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 31);
    insideTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 33);
    insideTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 35);

    //
    // Inside humidity section
    //
    insideHumidity.highs.todayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 37);
    insideHumidity.lows.todayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 38);
    insideHumidity.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 39);
    insideHumidity.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 41);
    insideHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 43);
    insideHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 44);
    insideHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 45);
    insideHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 46);

    //
    // Outside temperature section
    //
    outsideTemperature.lows.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 47);
    outsideTemperature.highs.todayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 49);
    outsideTemperature.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 51);
    outsideTemperature.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 53);
    outsideTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 55);
    outsideTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 57);
    outsideTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 59);
    outsideTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 61);

    //
    // Dew point section
    //
    dewPoint.lows.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 63, false);
    dewPoint.highs.todayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 65, false);
    dewPoint.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 67);
    dewPoint.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 69);
    dewPoint.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 71, false);
    dewPoint.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 73, false);
    dewPoint.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 75, false);
    dewPoint.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 77, false);

    //
    // Wind chill section
    //
    windChill.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 79, false);
    windChill.todayExtremeValueTime  = BitConverter::toUint16(buffer, 81);
    windChill.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 83, false);
    windChill.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 85, false);

    //
    // Heat index section
    //
    heatIndex.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 87, false);
    heatIndex.todayExtremeValueTime  = BitConverter::toUint16(buffer, 89);
    heatIndex.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 91, false);
    heatIndex.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 93, false);

    //
    // THSW index section
    //
    thsw.todayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 95, false);
    thsw.todayExtremeValueTime  = BitConverter::toUint16(buffer, 97);
    thsw.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 99, false);
    thsw.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 101, false);

    //
    // Solar radiation section
    //
    solarRadiation.todayExtremeValue      = VantageDecoder::decodeSolarRadiation(buffer, 103);
    solarRadiation.todayExtremeValueTime  = BitConverter::toUint16(buffer, 105);
    solarRadiation.monthExtremeValue    = VantageDecoder::decodeSolarRadiation(buffer, 107);
    solarRadiation.yearExtremeValue     = VantageDecoder::decodeSolarRadiation(buffer, 109);

    //
    // UV section
    //
    uvIndex.todayExtremeValue      = VantageDecoder::decodeUvIndex(buffer, 111);
    uvIndex.todayExtremeValueTime  = BitConverter::toUint16(buffer, 112);
    uvIndex.monthExtremeValue    = VantageDecoder::decodeUvIndex(buffer, 114);
    uvIndex.yearExtremeValue     = VantageDecoder::decodeUvIndex(buffer, 115);

    //
    // Rain rate section
    //
    rainRate.todayExtremeValue      = VantageDecoder::decodeRain(buffer, 116);
    rainRate.todayExtremeValueTime  = BitConverter::toUint16(buffer, 118);
    highHourRainRate              = VantageDecoder::decodeRain(buffer, 120);
    rainRate.monthExtremeValue    = VantageDecoder::decodeRain(buffer, 122);
    rainRate.yearExtremeValue     = VantageDecoder::decodeRain(buffer, 124);

    //
    // Extra temperatures section
    //
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_TEMPERATURES; i++) {
        extraTemperature[i].lows.todayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + i);
        extraTemperature[i].highs.todayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + i);
        extraTemperature[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 156 + (i * 2));
        extraTemperature[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 186 + (i * 2));
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
        soilTemperature[i].lows.todayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + offset + i);
        soilTemperature[i].highs.todayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + offset + i);
        soilTemperature[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 156 + (offset * 2) + (i * 2));
        soilTemperature[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 186 + (offset * 2) + (i * 2));
        soilTemperature[i].highs.monthExtremeValue   = VantageDecoder::decode8BitTemperature(buffer, 216 + offset + i);
        soilTemperature[i].lows.monthExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 231 + offset + i);
        soilTemperature[i].highs.yearExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 246 + offset + i);
        soilTemperature[i].lows.yearExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 261 + offset + i);
    }

    //
    // Leaf temperature
    offset = ProtocolConstants::MAX_EXTRA_TEMPERATURES + ProtocolConstants::MAX_SOIL_TEMPERATURES;
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_TEMPERATURES; i++) {
        leafTemperature[i].lows.todayExtremeValue      = VantageDecoder::decode8BitTemperature(buffer, 126 + offset + i);
        leafTemperature[i].highs.todayExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 141 + offset + i);
        leafTemperature[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 156 + (offset * 2) + (i * 2));
        leafTemperature[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 186 + (offset * 2) + (i * 2));
        leafTemperature[i].highs.monthExtremeValue   = VantageDecoder::decode8BitTemperature(buffer, 216 + offset + i);
        leafTemperature[i].lows.monthExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 231 + offset + i);
        leafTemperature[i].highs.yearExtremeValue    = VantageDecoder::decode8BitTemperature(buffer, 246 + offset + i);
        leafTemperature[i].lows.yearExtremeValue     = VantageDecoder::decode8BitTemperature(buffer, 261 + offset + i);
    }

    //
    // Outside humidity section
    //
    outsideHumidity.lows.todayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 276);
    outsideHumidity.highs.todayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 284);
    outsideHumidity.lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 308);
    outsideHumidity.highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 292);
    outsideHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 324);
    outsideHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 332);
    outsideHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 340);
    outsideHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 348);

    //
    // Extra humidity section
    //
    for (int i = 0; i < ProtocolConstants::MAX_EXTRA_HUMIDITIES; i++) {
        extraHumidity[i].lows.todayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 277 + i);
        extraHumidity[i].highs.todayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 285 + i);
        extraHumidity[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 309 + i);
        extraHumidity[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 293 + i);
        extraHumidity[i].highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 325 + i);
        extraHumidity[i].lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 333 + i);
        extraHumidity[i].highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 341 + 1);
        extraHumidity[i].lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 349 + i);
    }

    //
    // Soil moisture section
    //
    for (int i = 0; i < ProtocolConstants::MAX_SOIL_MOISTURES; i++) {
        soilMoisture[i].highs.todayExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 356 + i);
        soilMoisture[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 360 + i);
        soilMoisture[i].lows.todayExtremeValue      = VantageDecoder::decodeSoilMoisture(buffer, 368 + i);
        soilMoisture[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 372 + i);
        soilMoisture[i].lows.monthExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 380 + i);
        soilMoisture[i].highs.monthExtremeValue   = VantageDecoder::decodeSoilMoisture(buffer, 384 + i);
        soilMoisture[i].lows.yearExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 388 + i);
        soilMoisture[i].highs.yearExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 392 + 1);
    }

    //
    // Leaf wetness section
    //
    for (int i = 0; i < ProtocolConstants::MAX_LEAF_WETNESSES; i++) {
        leafWetness[i].highs.todayExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 396 + i);
        leafWetness[i].highs.todayExtremeValueTime = BitConverter::toUint16(buffer, 400 + i);
        leafWetness[i].lows.todayExtremeValue      = VantageDecoder::decodeLeafWetness(buffer, 408 + i);
        leafWetness[i].lows.todayExtremeValueTime  = BitConverter::toUint16(buffer, 412 + i);
        leafWetness[i].lows.monthExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 420 + i);
        leafWetness[i].highs.monthExtremeValue   = VantageDecoder::decodeLeafWetness(buffer, 424 + i);
        leafWetness[i].lows.yearExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 428 + i);
        leafWetness[i].highs.yearExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 432 + 1);
    }
}

}
