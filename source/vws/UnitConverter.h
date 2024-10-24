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
#ifndef UNIT_CONVERTER_H
#define UNIT_CONVERTER_H

#include "WeatherTypes.h"

namespace vws {

/**
 * Class that contains utility routines to convert from Vantage weather console units to other units.
 */
class UnitConverter {
public:
 
    /**
     * Convert from Fahrenheit to Celsius
     * 
     * @param temperature The temperature in Fahrenheit
     * @return The temperature in Celsius
     */
    static Temperature toCelsius(Temperature temperature);

    /**
     * Convert from inches to millimeters.
     * 
     * @param rain The amount of rain in inches
     * @return The amount of rain in millimeters
     */
    static Rainfall toMillimeter(Rainfall rain);

    /**
     * Convert from inHG to millibars
     * 
     * @param pressure The pressure in inches of mercury
     * @return The pressure in millibars
     */
    static Pressure toMillibars(Pressure pressure);

    /**
     * Convert from MPH to meters per second.
     *
     * @param speed The wind speed in MPH
     * @return The wind speed in meters per second
     */
    static Speed toMetersPerSecond(Speed speed);

    /**
     * Convert from MPH to KPH
     *
     * @param speed The wind speed in MPH
     * @return The wind speed in KPH
     */
    static Speed toKilometersPerHour(Speed speed);

    /**
     * Convert from MPH to Knots
     *
     * @param speed The wind speed in MPH
     * @return The wind speed in Knots
     */
    static Speed toKnots(Speed speed);

    /**
     * Convert feet to millimeters
     *
     * @param elevation An elevation in feet
     */
    static Elevation feetToMillimeters(Elevation elevation);

private:
    //
    // All of these factors are for converting from the Vantage console units to the units in the name of the constant.
    //
    static constexpr Temperature CELSIUS_OFFSET = 32.0;
    static constexpr Temperature CELSIUS_FACTOR = 1.8;
    static constexpr Rainfall    MILLIMETER_FACTOR = 25.4;
    static constexpr Pressure    MILLIBAR_FACTOR = 33.86386;
    static constexpr Speed       METERS_PER_SECOND_FACTOR = 0.44704;
    static constexpr Speed       KILOMETERS_PER_HOUR_FACTOR = 1.609344;
    static constexpr Speed       KNOTS_FACTOR = 0.868976;
    static constexpr Elevation   MILLIMETERS_FACTOR = 304.8;
    /**
     * Private constructor to prevent object from being created.
     */
    UnitConverter() = delete;
    UnitConverter(const UnitConverter &) = delete;
    ~UnitConverter() = delete;
};
}
#endif
