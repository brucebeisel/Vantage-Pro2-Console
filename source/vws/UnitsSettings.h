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
#ifndef UNITS_SETTINGS_H_
#define UNITS_SETTINGS_H_

#include "json.hpp"
#include "VantageProtocolConstants.h"

namespace vws {

/**
 * Structure to hold the units that are displayed by the console
 */
class UnitsSettings {
public:
    /**
     * Default constructor.
     */
    UnitsSettings();

    /**
     * Constructor to set all of the units values.
     *
     * @param baroUnits        The units for the barometric values
     * @param elevationUnits   The units for elevation values
     * @param rainUnits        The units for rain values
     * @param temperatureUnits The units for temperature values
     * @param windUnits        The units for wind speed values
     */
    UnitsSettings(ProtocolConstants::BarometerUnits baroUnits, ProtocolConstants::ElevationUnits elevationUnits, ProtocolConstants::RainUnits rainUnits, ProtocolConstants::TemperatureUnits temperatureUnits, ProtocolConstants::WindUnits windUnits);

    /**
     * Setter for all of the units values.
     *
     * @param baroUnits        The units for the barometric values
     * @param elevationUnits   The units for elevation values
     * @param rainUnits        The units for rain values
     * @param temperatureUnits The units for temperature values
     * @param windUnits        The units for wind speed values
     */
    void setUnits(ProtocolConstants::BarometerUnits baroUnits, ProtocolConstants::ElevationUnits elevationUnits, ProtocolConstants::RainUnits rainUnits, ProtocolConstants::TemperatureUnits temperatureUnits, ProtocolConstants::WindUnits windUnits);

    void setBarometerUnits(ProtocolConstants::BarometerUnits units);
    void setElevationUnits(ProtocolConstants::ElevationUnits units);
    void setRainUnits(ProtocolConstants::RainUnits units);
    void setTemperatureUnits(ProtocolConstants::TemperatureUnits units);
    void setWindUnits(ProtocolConstants::WindUnits units);

    /**
     * Decode the units from a single byte.
     *
     * @param settings The units setting
     */
    void decode(byte settings);

    /**
     * Encode the units settings into the provided byte.
     *
     * @param settings The location where the units settings will be written
     */
    void encode(byte & settings) const;

    /**
     * Parse a JSON node and load the values.
     *
     * @param node The root node of the settings JSON
     * @return true if the JSON is valid
     */
    bool parseJson(const nlohmann::json & node);

    /**
     * Format the units settings into JSON.
     *
     * @return The units settings as a JSON string
     */
    std::string formatJSON() const;

private:
    ProtocolConstants::BarometerUnits   baroUnits;
    ProtocolConstants::ElevationUnits   elevationUnits;
    ProtocolConstants::RainUnits        rainUnits;
    ProtocolConstants::TemperatureUnits temperatureUnits;
    ProtocolConstants::WindUnits        windUnits;
};

}

#endif
