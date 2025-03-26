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
#ifndef VANTAGE_EEPROM_CONSTANTS_H
#define VANTAGE_EEPROM_CONSTANTS_H
#include <string>

#include "WeatherTypes.h"

namespace vws {
/**
 * This class contains constants that are specifically for the EEPROM memory that is documented in the
 * Vantage Pro, Vantage Pro2, Vantage Vue Serial Communications Reference Manual
 * Version 2.6.1
 * Date: 3/29/2013
 */
namespace EepromConstants {
    /**
     * Sensor station types supported by the Vantage weather station, the enum integer values are from the Vantage serial protocol.
     */
    enum StationType {
        INTEGRATED_SENSOR_STATION = 0,
        TEMPERATURE_ONLY_STATION = 1,
        HUMIDITY_ONLY_STATION = 2,
        TEMPERATURE_HUMIDITY_STATION = 3,
        ANEMOMETER_STATION = 4,
        RAIN_STATION = 5,
        LEAF_STATION = 6,
        SOIL_STATION = 7,
        SOIL_LEAF_STATION = 8,
        NO_STATION = 10,
        UNKNOWN_STATION = 99       // The sensor station has been heard, but not identified
    };

    enum class VantageVueStationType {
        VUE_INTEGRATED_SENSOR_STATION = 0,
        VUE_ANEMOMETER_STATION = 4,
        VP2_INTEGRATED_SENSOR_STATION = 5,
        VUE_NO_STATION = 10,
        VUE_UNKNOWN_STATION = 99       // The sensor station has been heard, but not identified
    };

    enum class RepeaterId {
        NO_REPEATER = 0,
        REPEATER_A = 8,
        REPEATER_B = 9,
        REPEATER_C = 10,
        REPEATER_D = 11,
        REPEATER_E = 12,
        REPEATER_F = 13,
        REPEATER_G = 14,
        REPEATER_H = 15
    };

    enum class SecondaryWindCupSize {
        UNDEFINED = 0,
        SMALL = 1,
        LARGE = 2,
        OTHER = 3
    };

    //
    // EEPROM sizes
    //
    static constexpr int      EE_DATA_BLOCK_SIZE = 4096;
    static constexpr int      EE_ALARM_THRESHOLDS_SIZE = 94;
    static constexpr int      EE_GRAPH_POINTERS_SIZE = 8;
    static constexpr int      EE_STATION_LIST_SIZE = 16;
    static constexpr int      EE_CALIBRATION_DATA_SIZE = 29;

    static constexpr int      EE_NON_GRAPH_DATA_SIZE = 176;
    static constexpr int      EE_GRAPH_DATA_SIZE = 3730 + 24;

    //
    // EEPROM Addresses and sizes
    //
    static constexpr unsigned EE_BAR_GAIN_ADDRESS =                   0x01;
    static constexpr unsigned EE_BAR_OFFSET_ADDRESS =                 0x03;
    static constexpr unsigned EE_BAR_CAL_ADDRESS =                    0x05;
    static constexpr unsigned EE_HUM33_ADDRESS =                      0x07;
    static constexpr unsigned EE_HUM80_ADDRESS =                      0x09;
    static constexpr unsigned EE_LATITUDE_ADDRESS =                   0x0B;
    static constexpr unsigned EE_LONGITUDE_ADDRESS =                  0x0D;
    static constexpr unsigned EE_ELEVATION_ADDRESS =                  0x0F;
    static constexpr unsigned EE_TIME_FIELDS_START_ADDRESS =          0x11;
    static constexpr unsigned EE_TIMEZONE_ADDRESS =                   0x11;
    static constexpr unsigned EE_MANUAL_OR_AUTO_DST_ADDRESS =         0x12;
    static constexpr unsigned EE_DST_ON_ADDRESS =                     0x13;
    static constexpr unsigned EE_GMT_OFFSET_ADDRESS =                 0x14;
    static constexpr unsigned EE_GMT_OR_TIMEZONE_ADDRESS =            0x16;
    static constexpr unsigned EE_USED_TRANSMITTERS_ADDRESS =          0x17;
    static constexpr unsigned EE_RETRANSMIT_ID_ADDRESS =              0x18;
    static constexpr unsigned EE_STATION_LIST_ADDRESS =               0x19;
    static constexpr unsigned EE_UNIT_BITS_ADDRESS =                  0x29;
    static constexpr unsigned EE_SETUP_BITS_ADDRESS =                 0x2B;
    static constexpr unsigned EE_RAIN_SEASON_START_ADDRESS =          0x2C;
    static constexpr unsigned EE_ARCHIVE_PERIOD_ADDRESS =             0x2D;
    static constexpr unsigned EE_INSIDE_TEMP_CAL_ADDRESS =            0x32;
    static constexpr unsigned EE_INSIDE_TEMP_COMP_ADDRESS =           0x33;
    static constexpr unsigned EE_OUTSIDE_TEMP_CAL_ADDRESS =           0x34;
    static constexpr unsigned EE_EXTRA_TEMPS_CAL_ADDRESS =            0x35;
    static constexpr unsigned EE_INSIDE_HUMID_CAL_ADDRESS =           0x44;
    static constexpr unsigned EE_HUMID_CAL_ADDRESS =                  0x45;
    static constexpr unsigned EE_WIND_DIR_CAL_ADDRESS =               0x4D;
    static constexpr unsigned EE_BARO_GRAPH_TIME_SPAN_ADDRESS =       0x4F;
    static constexpr unsigned EE_RAIN_GRAPH_TIME_SPAN_ADDRESS =       0x50;
    static constexpr unsigned EE_WIND_SPEED_GRAPH_TIME_SPAN_ADDRESS = 0x51;
    static constexpr unsigned EE_ALARM_THRESHOLDS_ADDRESS =           0x52;
    static constexpr unsigned EE_WIND_CUP_SIZE_ADDRESS =              195;  // Decimal because the protocol document uses decimal for graph data addresses
    static constexpr unsigned EE_LOG_AVG_TEMP_ADDRESS =               0xFFC;
    static constexpr unsigned EE_PASSWORD_CRC_ADDRESS =               0xFFE;

    //
    // Graph data addresses
    //
    static constexpr unsigned EE_GRAPH_POINTERS_ADDRESS =             177;  // Decimal because the protocol document uses decimal for graph data addresses
    static constexpr unsigned EE_NEXT_10MIN_PTR_ADDRESS =             EE_GRAPH_POINTERS_ADDRESS;
    static constexpr unsigned EE_NEXT_15MIN_PTR_ADDRESS =             EE_GRAPH_POINTERS_ADDRESS + 1;
    static constexpr unsigned EE_NEXT_HOUR_PTR_ADDRESS =              EE_GRAPH_POINTERS_ADDRESS + 2;
    static constexpr unsigned EE_NEXT_DAY_PTR_ADDRESS =               EE_GRAPH_POINTERS_ADDRESS + 3;
    static constexpr unsigned EE_NEXT_MONTH_PTR_ADDRESS =             EE_GRAPH_POINTERS_ADDRESS + 4;
    static constexpr unsigned EE_NEXT_YEAR_PTR_ADDRESS =              EE_GRAPH_POINTERS_ADDRESS + 5;
    static constexpr unsigned EE_NEXT_RAIN_STORM_PTR_ADDRESS =        EE_GRAPH_POINTERS_ADDRESS + 6;
    static constexpr unsigned EE_NEXT_RAIN_YEAR_PTR_ADDRESS =         EE_GRAPH_POINTERS_ADDRESS + 7;

    //
    // Graph Data
    //
    static constexpr unsigned EE_GRAPH_DATA_ADDRESS =                 325;
    static constexpr unsigned EE_INSIDE_TEMP_HOUR_ADDRESS =           EE_GRAPH_DATA_ADDRESS + 0;
    static constexpr unsigned EE_INSIDE_TEMP_DAY_HIGHS_ADDRESS =      EE_GRAPH_DATA_ADDRESS + 24;
    static constexpr unsigned EE_INSIDE_TEMP_DAY_HIGH_TIMES_ADDRESS = EE_GRAPH_DATA_ADDRESS + 48;
    static constexpr unsigned EE_INSIDE_TEMP_DAY_LOWS_ADDRESS =       EE_GRAPH_DATA_ADDRESS + 96;
    static constexpr unsigned EE_INSIDE_TEMP_DAY_LOW_TIMES_ADDRESS =  EE_GRAPH_DATA_ADDRESS + 120;
    static constexpr unsigned EE_RAIN_STORM_DATA_ADDRESS =            EE_GRAPH_DATA_ADDRESS + 2642;
    static constexpr unsigned EE_RX_PERCENTAGE_ADDRESS =              EE_GRAPH_DATA_ADDRESS + 3730;

};

}

#endif
