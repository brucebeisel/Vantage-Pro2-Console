#include <sstream>
#include "BitConverter.h"
#include "Weather.h"
#include "HiLowPacket.h"
#include "VantageDecoder.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
HiLowPacket::HiLowPacket() : highHourRainRate(0.0), log(VantageLogger::getLogger("HiLowPacket")) {
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

    ss << "    { \"" << which << "\" : " << endl
       << "        { \"day\" : " << endl
       << "            {\"value\" : " << dayExtremeValue.getValue() << "}," << endl
       << "            {\"time\"  : \"" << Weather::formatDateTime(dayExtremeValueTime) << "\" }" << endl
       << "        }," << endl
       << "        { \"month\" : " << monthExtremeValue << "}," << endl
       << "        { \"year\"  : " << yearExtremeValue <<"}" << endl
       << "    }" << endl;

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
    string s = lows.formatJSON(true).append(highs.formatJSON(false));
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
    ss << "<insideTemperature>" << endl;
    ss << insideTemperature.formatXML();
    ss << "</insideTemperature>" << endl;
    ss << "<outsideTemperature>" << endl;
    ss << outsideTemperature.formatXML();
    ss << "</outsideTemperature>" << endl;
    ss << "<insideHumidity>" << endl;
    ss << insideHumidity.formatXML();
    ss << "</insideHumidity>" << endl;
    ss << "<outsideHumidity>" << endl;
    ss << outsideHumidity.formatXML();
    ss << "</outsideHumidity>" << endl;
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
    for (int i = 0; i < VantageConstants::MAX_EXTRA_TEMPERATURES; i++) {
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
    ss << "{" << endl
       << "    {\"high-low\" : " << endl
       << "        { \"barometer\" : " << endl
       << barometer.formatJSON() << endl
       << "        }" << endl
       << "    }" << endl
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
    // Inside temperature section
    //
    insideTemperature.highs.dayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 21);
    insideTemperature.lows.dayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 23);
    insideTemperature.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 25);
    insideTemperature.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 27);
    insideTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 29);
    insideTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 31);
    insideTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 33);
    insideTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 35);

    //
    // Indoor humidity section
    //
    insideHumidity.highs.dayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 37);
    insideHumidity.lows.dayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 38);
    insideHumidity.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 39);
    insideHumidity.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 41);
    insideHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 43);
    insideHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 44);
    insideHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 45);
    insideHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 46);

    //
    // Outside temperature section
    //
    outsideTemperature.lows.dayExtremeValue      = VantageDecoder::decode16BitTemperature(buffer, 47);
    outsideTemperature.highs.dayExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 49);
    outsideTemperature.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 51);
    outsideTemperature.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 53);
    outsideTemperature.highs.monthExtremeValue   = VantageDecoder::decode16BitTemperature(buffer, 55);
    outsideTemperature.lows.monthExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 57);
    outsideTemperature.highs.yearExtremeValue    = VantageDecoder::decode16BitTemperature(buffer, 59);
    outsideTemperature.lows.yearExtremeValue     = VantageDecoder::decode16BitTemperature(buffer, 61);

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
    for (int i = 0; i < VantageConstants::MAX_EXTRA_TEMPERATURES; i++) {
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
    int offset = VantageConstants::MAX_EXTRA_TEMPERATURES;
    for (int i = 0; i < VantageConstants::MAX_SOIL_TEMPERATURES; i++) {
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
    offset = VantageConstants::MAX_EXTRA_TEMPERATURES + VantageConstants::MAX_SOIL_TEMPERATURES;
    for (int i = 0; i < VantageConstants::MAX_LEAF_TEMPERATURES; i++) {
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
    outsideHumidity.lows.dayExtremeValue      = VantageDecoder::decodeHumidity(buffer, 276);
    outsideHumidity.highs.dayExtremeValue     = VantageDecoder::decodeHumidity(buffer, 284);
    outsideHumidity.lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 308);
    outsideHumidity.highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 292);
    outsideHumidity.highs.monthExtremeValue   = VantageDecoder::decodeHumidity(buffer, 324);
    outsideHumidity.lows.monthExtremeValue    = VantageDecoder::decodeHumidity(buffer, 332);
    outsideHumidity.highs.yearExtremeValue    = VantageDecoder::decodeHumidity(buffer, 340);
    outsideHumidity.lows.yearExtremeValue     = VantageDecoder::decodeHumidity(buffer, 348);

    //
    // Extra humidity section
    //
    for (int i = 0; i < VantageConstants::MAX_EXTRA_HUMIDITIES; i++) {
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
    for (int i = 0; i < VantageConstants::MAX_SOIL_MOISTURES; i++) {
        soilMoisture[i].highs.dayExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 356 +i);
        soilMoisture[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 360 + i);
        soilMoisture[i].lows.dayExtremeValue      = VantageDecoder::decodeSoilMoisture(buffer, 368 +i);
        soilMoisture[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 372 + i);
        soilMoisture[i].lows.monthExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 380 + i);
        soilMoisture[i].highs.monthExtremeValue   = VantageDecoder::decodeSoilMoisture(buffer, 384 + i);
        soilMoisture[i].lows.yearExtremeValue     = VantageDecoder::decodeSoilMoisture(buffer, 388 + i);
        soilMoisture[i].highs.yearExtremeValue    = VantageDecoder::decodeSoilMoisture(buffer, 392 + 1);
    }

    //
    // Leaf wetness section
    //
    for (int i = 0; i < VantageConstants::MAX_SOIL_MOISTURES; i++) {
        leafWetness[i].highs.dayExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 396 +i);
        leafWetness[i].highs.dayExtremeValueTime = VantageDecoder::decodeTime(buffer, 400 + i);
        leafWetness[i].lows.dayExtremeValue      = VantageDecoder::decodeLeafWetness(buffer, 408 +i);
        leafWetness[i].lows.dayExtremeValueTime  = VantageDecoder::decodeTime(buffer, 412 + i);
        leafWetness[i].lows.monthExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 420 + i);
        leafWetness[i].highs.monthExtremeValue   = VantageDecoder::decodeLeafWetness(buffer, 424 + i);
        leafWetness[i].lows.yearExtremeValue     = VantageDecoder::decodeLeafWetness(buffer, 428 + i);
        leafWetness[i].highs.yearExtremeValue    = VantageDecoder::decodeLeafWetness(buffer, 432 + 1);
    }

    return true;
}

}
