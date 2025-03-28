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
#ifndef WEATHER_TYPES_H
#define WEATHER_TYPES_H

#include <time.h>
#include <ostream>
#include <iomanip>

namespace vws {
//
// Typedefs for the various weather types
//
using Speed = double;
using Rainfall = double;
using RainfallRate = double;
using Temperature = double;
using Pressure = double;
using Humidity = double;
using SolarRadiation = double;
using UvIndex = float;
using LeafWetness = int;
using SoilMoisture = int;
using Heading = float;
using HeadingIndex = int;
using Elevation = int;
using byte = char;
using DateTime = time_t;
using Evapotranspiration = double;
using StationId = unsigned;
using int8 = byte;
using uint8 = unsigned char;
using int16 = short;
using uint16 = unsigned short;
using int32 = int;
using uint32 = unsigned int;


}

#endif
