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

#include "UnitsSettings.h"

#include <sstream>
#include "VantageEnums.h"
#include "JsonUtils.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
UnitsSettings::UnitsSettings() : baroUnits(BarometerUnits::IN_HG),
                                 temperatureUnits(TemperatureUnits::FAHRENHEIT),
                                 elevationUnits(ElevationUnits::FEET),
                                 rainUnits(RainUnits::INCHES),
                                 windUnits(WindUnits::MPH) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
UnitsSettings::UnitsSettings(BarometerUnits baroUnits, ElevationUnits elevationUnits, RainUnits rainUnits, TemperatureUnits temperatureUnits, WindUnits windUnits) {
    setUnits(baroUnits, elevationUnits, rainUnits, temperatureUnits, windUnits);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setUnits(BarometerUnits baro, ElevationUnits elevation, RainUnits rain, TemperatureUnits temperature, WindUnits wind) {
    baroUnits = baro;
    temperatureUnits = temperature;
    elevationUnits = elevation;
    rainUnits = rain;
    windUnits = wind;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setBarometerUnits(BarometerUnits units) {
    baroUnits = units;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setElevationUnits(ElevationUnits units) {
    elevationUnits = units;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setRainUnits(RainUnits units) {
    rainUnits = units;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setTemperatureUnits(TemperatureUnits units) {
    temperatureUnits = units;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::setWindUnits(WindUnits units) {
    windUnits = units;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::decode(byte settings) {
    baroUnits = static_cast<BarometerUnits>(settings & 0x3);
    temperatureUnits = static_cast<TemperatureUnits>((settings >> 2) & 0x3);
    elevationUnits = static_cast<ElevationUnits>((settings >> 4) & 0x1);
    rainUnits = static_cast<RainUnits>((settings >> 5) & 0x1);
    windUnits = static_cast<WindUnits>((settings >> 6) & 0x3);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
UnitsSettings::encode(byte & settings) const {
    settings = static_cast<int>(baroUnits) & 0x3;
    settings |= (static_cast<int>(temperatureUnits) & 0x3) << 2;
    settings |= (static_cast<int>(elevationUnits) & 0x1) << 4;
    settings |= (static_cast<int>(rainUnits) & 0x1) << 5;
    settings |= (static_cast<int>(windUnits) & 0x3) << 6;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
UnitsSettings::formatJSON() const {
    ostringstream oss;

    oss << "\"units\" : {"
        << " \"baroUnits\" : \"" << barometerUnitsEnum.valueToString(baroUnits) << "\""
        << ", \"elevationUnits\" : \"" << elevationUnitsEnum.valueToString(elevationUnits) << "\""
        << ", \"rainUnits\" : \"" << rainUnitsEnum.valueToString(rainUnits) << "\""
        << ", \"temperatureUnits\" : \"" << temperatureUnitsEnum.valueToString(temperatureUnits) << "\""
        << ", \"windUnits\" : \"" << windUnitsEnum.valueToString(windUnits) << "\""
        << " }";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
UnitsSettings::parseJson(const nlohmann::json & node) {
    string enumString;

    nlohmann::json units = node.at("units");

    if (!JsonUtils::findJsonValue(units, "baroUnits", enumString))
        return false;

    baroUnits = barometerUnitsEnum.stringToValue(enumString);

    if (!JsonUtils::findJsonValue(units, "elevationUnits", enumString))
        return false;

    elevationUnits = elevationUnitsEnum.stringToValue(enumString);

    if (!JsonUtils::findJsonValue(units, "rainUnits", enumString))
        return false;

    rainUnits = rainUnitsEnum.stringToValue(enumString);

    if (!JsonUtils::findJsonValue(units, "temperatureUnits", enumString))
        return false;

    temperatureUnits = temperatureUnitsEnum.stringToValue(enumString);

    if (!JsonUtils::findJsonValue(units, "windUnits", enumString))
        return false;

    windUnits = windUnitsEnum.stringToValue(enumString);

    return true;
}
}
