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
#ifndef VANTAGE_CONSTANTS_H
#define VANTAGE_CONSTANTS_H
#include <string>
#include "WeatherTypes.h"

namespace vws {
/**
 * This class contains all of the constants from the
 * Vantage Pro, Vantage Pro2, Vantage Vue Serial Communications Reference Manual
 * Version 2.6.1
 * Date: 3/29/2013
 */
namespace VantageConstants {
    //
    // EEPROM Addresses and sizes
    //
    static constexpr unsigned EE_LATITUDE_ADDRESS = 0x0B;
    static constexpr unsigned EE_LONGITUDE_ADDRESS = 0x0D;
    static constexpr unsigned EE_ELEVATION_ADDRESS = 0x0F;
    static constexpr unsigned EE_TIMEZONE_ADDRESS = 0x11;
    static constexpr unsigned EE_TIME_FIELDS_START_ADDRESS = 0x11;
    static constexpr unsigned EE_STATION_LIST_ADDRESS = 0x19;
    static constexpr unsigned EE_UNIT_BITS_ADDRESS = 0x29;
    static constexpr unsigned EE_SETUP_BITS_ADDRESS = 0x2B;
    static constexpr unsigned EE_RAIN_SEASON_START_ADDRESS = 0x2C;
    static constexpr unsigned EE_ARCHIVE_PERIOD_ADDRESS = 0x2D;
    static constexpr unsigned EE_ALARM_THRESHOLDS_ADDRESS = 0x52;
    static constexpr unsigned EE_GRAPH_DATA_ADDRESS = 0x145;

    static constexpr int      EE_DATA_BLOCK_SIZE = 4096;
    static constexpr int      EE_NON_GRAPH_DATA_SIZE = 176;
    static constexpr int      EE_GRAPH_DATA_SIZE = EE_DATA_BLOCK_SIZE - EE_GRAPH_DATA_ADDRESS;
    static constexpr int      EE_ALARM_THRESHOLDS_SIZE = 94;
};
}

#endif /* VANTAGE_CONSTANTS_H */
