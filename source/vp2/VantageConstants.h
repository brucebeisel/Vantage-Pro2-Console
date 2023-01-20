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
    // Various values used generically in various command protocols
    //
/*
    static constexpr char NULLCHAR = '\0';
    static constexpr char LINE_FEED = '\n';
    static constexpr char CARRIAGE_RETURN = '\r';
    static constexpr char ACK = 0x6;
    static constexpr char NACK = 0x21; // Not an ASCII NACK, but it is what is used
    static constexpr char CANCEL = 0x18;
    static constexpr char CRC_FAILURE = CANCEL;
    static constexpr char ESCAPE = 0x15;
    */


    static constexpr Rainfall POINT_01_INCH_SIZE = 0.01;       // Inches
    static constexpr Rainfall POINT_2_MM_SIZE   = 0.2 / 25.4; // Inches
    static constexpr Rainfall POINT_1_MM_SIZE   = 0.1 / 25.4; // Inches

    //
    // LOOP packet forecast icons
    //
    static const short RAIN_BIT = 0x1;
    static const short CLOUDY_BIT = 0x2;
    static const short PARTLY_CLOUDY_BIT = 0x4;
    static const short SUNNY_BIT = 0x8;
    static const short SNOW_BIT = 0x10;

    static const int MOSTLY_CLEAR_FORECAST = SUNNY_BIT;
    static const int PARTLY_CLOUDY_FORECAST = PARTLY_CLOUDY_BIT | CLOUDY_BIT;
    static const int MOSTLY_CLOUDY_FORECAST = CLOUDY_BIT;
    static const int MOSTLY_CLOUDY_CHANCE_OF_RAIN_FORECAST = CLOUDY_BIT | RAIN_BIT;
    static const int MOSTLY_CLOUDY_CHANCE_OF_SNOW_FORECAST = CLOUDY_BIT | SNOW_BIT;
    static const int PARTLY_CLOUDY_CHANCE_OF_RAIN_FORECAST = PARTLY_CLOUDY_BIT | RAIN_BIT;
    static const int PARTLY_CLOUDY_CHANCE_OF_SNOW_FORECAST = PARTLY_CLOUDY_BIT | SNOW_BIT;
    static const int PARTLY_CLOUDY_CHANCE_OF_RAIN_OR_SNOW_FORECAST = PARTLY_CLOUDY_BIT | RAIN_BIT | SNOW_BIT;

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


    static const int NUM_ARCHIVE_RECORDS = 2560;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Everything below has been vetted. Everything above might be moved

    //
    // Maximum counts
    //
    static constexpr int MAX_STATION_ID = 8;
    static constexpr int MAX_INTEGRATED_SENSOR_STATIONS = 1;
    static constexpr int MAX_ANEMOMETER_STATIONS = 1;
    //
    // Per the serial protocol ICD, you cannot have two stations with both leaf and soil moisture sensors.
    // One can have both, but if there are two stations, one must be leaf, the other soil.
    // It is not clear how the console knows this and how it would handle the unsupported configuration.
    //
    static constexpr int MAX_LEAF_SOIL_MOISTURE_TEMPERATURE_STATIONS = 2;
    static constexpr int MAX_TEMPERATURE_STATIONS = 8;
    static constexpr int MAX_TEMPERATURE_HUMIDITY_STATIONS = 8;

    static constexpr int MAX_SENSOR_STATIONS = 8;
    static constexpr int MAX_EXTRA_TEMPERATURES = 7;
    static constexpr int MAX_EXTRA_HUMIDITIES = 7;
    static constexpr int MAX_SOIL_TEMPERATURES = 4;
    static constexpr int MAX_SOIL_MOISTURES = 4;
    static constexpr int MAX_LEAF_TEMPERATURES = 4;
    static constexpr int MAX_LEAF_WETNESSES = 4;

    //
    // Common constants
    //
    static constexpr Temperature        TEMPERATURE_16BIT_SCALE = 10.0;
    static constexpr int                TEMPERATURE_16BIT_INVALID_VALUE = 32767;
    static constexpr Temperature        TEMPERATURE_8BIT_OFFSET = 90.0;
    static constexpr int                TEMPERATURE_8BIT_INVALID_VALUE = 255;
    static constexpr Pressure           BAROMETER_SCALE = 1000.0;
    static constexpr Speed              AVG_WIND_SPEED_SCALE = 10.0;
    static constexpr int                YEAR_OFFSET = 2000;
    static constexpr UvIndex            UV_INDEX_SCALE = 10.0;
    static constexpr Evapotranspiration DAY_ET_SCALE = 1000.0;
    static constexpr Evapotranspiration MONTH_YEAR_ET_SCALE = 100.0;
    static constexpr Rainfall           STORM_RAIN_SCALE = 100.0;
    static constexpr double             LAT_LON_SCALE = 10.0;

    static const int NORTH_HEADING_VALUE = 360;

    static constexpr int INVALID_16BIT_TEMPERATURE = 32767;
    static constexpr int INVALID_16BIT_HIGH_TEMPERATURE = -32768;
    static constexpr int INVALID_8BIT_TEMPERATURE = 255;
    static constexpr int INVALID_HUMIDITY = 255;
    static constexpr int INVALID_WIND_DIRECTION_SLICE = 255;
    static constexpr int INVALID_WIND_DIRECTION = 0;
    static constexpr int INVALID_WIND_SPEED = 255;
    static constexpr int INVALID_16BIT_AVG_WIND_SPEED = 32767;
    static constexpr int INVALID_UV_INDEX = 255;
    static constexpr int INVALID_LEAF_WETNESS = 255;
    static constexpr int INVALID_LEAF_TEMPERATURE = 255;
    static constexpr int INVALID_SOIL_TEMPERATURE = 255;
    static constexpr int INVALID_SOIL_MOISTURE = 255;
    static constexpr int INVALID_BAROMETER = 0;
    static constexpr int INVALID_SOLAR_RADIATION = 32767;
    static constexpr int INVALID_THSW = 32767;
    static constexpr int INVALID_ET = 0;
    static constexpr int NO_STORM_ACTIVE_DATE = -1;
    static constexpr int MIN_LEAF_WETNESS = 0;
    static constexpr int MAX_LEAF_WETNESS = 15;

    //
    // A wind slice is a segment of wind direction that is centered on a compass direction such
    // as N, NNE, NE, etc.
    //
    static constexpr int NUM_WIND_DIR_SLICES = 16;
    static constexpr double DEGREES_PER_SLICE = 360.0 / NUM_WIND_DIR_SLICES;
};
}

#endif /* VANTAGE_CONSTANTS_H */
