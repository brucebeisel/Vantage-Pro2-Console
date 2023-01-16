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
#ifndef VANTAGE_PROTOCOL_CONSTANTS
#define VANTAGE_PROTOCOL_CONSTANTS
#include <string>
#include <Weather.h>

namespace vws::ProtocolConstants {

//
// Various values used generically in various command protocols
//
static constexpr char NULLCHAR = '\0';
static constexpr char LINE_FEED = '\n';
static constexpr char CARRIAGE_RETURN = '\r';
static constexpr char ACK = 0x6;
static constexpr char NACK = 0x21; // Not an ASCII NACK, but it is what is used
static constexpr char CANCEL = 0x18;
static constexpr char CRC_FAILURE = CANCEL;
static constexpr char ESCAPE = 0x15;

//
// Wakeup command/response
//
static constexpr std::string WAKEUP_COMMAND = std::string(1, LINE_FEED);
static constexpr std::string WAKEUP_RESPONSE = std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);

//
// Testing Commands
//
static const std::string TEST_CMD = "TEST";                          // Sends the string "TEST\n" back
static constexpr char WRD_BYTE1 = 0x12;
static constexpr char WRD_BYTE2 = 0x4D;
static constexpr char WRD_BUFFER[] = {'W', 'R', 'D', WRD_BYTE1, WRD_BYTE2};
static constexpr std::string STATION_TYPE_CMD(WRD_BUFFER);               // Responds with the weather station type. This command is backward compatible with earlier Davis weather products.
static constexpr std::string RECEIVE_CHECK_CMD = "RXCHECK";              // Console diagnostics report
static constexpr std::string RXTEST_CMD = "RXTEST";                      // Move console to main current conditions screen
static constexpr std::string FIRMWARE_DATE_CMD = "VER";                  // Firmware date
static constexpr std::string RECEIVER_LIST_CMD = "RECEIVERS";            // Get the list of receivers as a bitmap, bit 0 represents station ID 1
static constexpr std::string FIRMWARE_VERSION_CMD = "NVER";              // Get the firmware version

//
// Current Data Commands
//
static constexpr std::string LOOP_CMD = "LOOP";                          // Get the current data values, alarms, battery status, etc. through the LOOP packet. Note that the LPS command renders this superfluous.
static constexpr std::string LPS_CMD = "LPS 3";                          // Get the current values through both the LOOP and LOOP2 packets
static constexpr std::string HIGH_LOW_CMD = "HILOWS";                    // Get the high and low that includes daily, monthly and yearly
static constexpr std::string PUT_YEARLY_RAIN_CMD = "PUTRAIN";            // Set the yearly rainfall
static constexpr std::string PUT_YEARLY_ET_CMD = "PUTET";                // Set the yearly ET

//
// Download Commands
//
static constexpr std::string DUMP_ARCHIVE_CMD = "DMP";                   // Dump the entire archive
static constexpr std::string DUMP_AFTER_CMD = "DMPAFT";                  // Dump the archive after a given date/time

//
// EEPROM Commands
//
static constexpr std::string DUMP_EEPROM_CMD = "GETEE";                  // Read the entire EEPROM data block
static constexpr std::string WRITE_EEPROM_CMD = "EEWR";                  // Write a single byte to EEPROM as hex strings
static constexpr std::string READ_EEPROM_CMD = "EERD";                   // Read EEPROM address as hex strings
static constexpr std::string WRITE_EEPROM_AS_BINARY_CMD = "EEBWR";       // Write to EEPROM as binary
static constexpr std::string READ_EEPROM_AS_BINARY_CMD = "EEBRD";        // Read EEPROM address as binary

//
// Calibration Commands
//
static constexpr std::string CALIBRATE_TEMPERATURE_HUMIDITY = "CALED";   // Send temperature and humidity calibration values
static constexpr std::string CALIBRATE_TEMPERATURE_HUMIDITY2 = "CALFIX"; // Updates the display when calibration numbers have changed
static constexpr std::string SET_BAROMETRIC_DATA_CMD = "BAR=";            // Sets barometric offset using local reading and/or elevation
static constexpr std::string SET_BAROMETRIC_CAL_DATA_CMD = "BARDATA";    // Get the current barometer calibration parameters

//
// Clearing Commands
//
static constexpr std::string CLEAR_ARCHIVE_CMD = "CLRLOG";               // Clear the archived data
static constexpr std::string CLEAR_ALARM_THRESHOLDS_CMD = "CLRALM";      // Clear the alarm thresholds
static constexpr std::string CLEAR_TEMP_HUMID_CAL_CMD = "CLRCAL";        // Set temperature and humidity calibration offsets to zero
static constexpr std::string CLEAR_GRAPH_POINTS_CMD = "CLRGRA";          // Clear the graph points
static constexpr std::string CLEAR_CUMULATIVE_VALUE_CMD = "CLRVAR";      // Clear the specified cumulative value
static constexpr std::string CLEAR_HIGH_VALUES_CMD = "CLRHIGHS";         // Clear the daily, monthly or yearly high values
static constexpr std::string CLEAR_LOW_VALUES_CMD = "CLRLOWS";           // Clear the daily, monthly or yearly low values
static constexpr std::string CLEAR_ACTIVE_ALARMS_CMD = "CLRBITS";        // Clear active alarms
static constexpr std::string CLEAR_CURRENT_DATA_VALUES_CMD = "CLRDATA";  // Clear all current data values

//
// Configuration Commands
//
static constexpr std::string SET_BAUD_RATE_CMD = "BAUD";                 // Sets the console to a new baud rate. Valid values are 1200, 2400, 4800, 9600, 14400 and 19200
static constexpr std::string SET_TIME_CMD = "SETTIME";                   // Sets the time and date on the console
static constexpr std::string GET_TIME_CMD = "GETTIME";                   // Retrieves the current time and date on the Vantage console. Data is sent in binary format
static constexpr std::string SET_ARCHIVE_PERIOD_CMD = "SETPER";          // Set how often the console saves an archive record
static constexpr std::string STOP_ARCHIVING_CMD = "STOP";                // Disables the creation of archive records
static constexpr std::string START_ARCHIVING_CMD = "START";              // Enables the create of archive records
static constexpr std::string REINITIALIZE_CMD = "NEWSETUP";              // Reinitialize the console after making any significant changes to the console's configuration
static constexpr std::string CONTROL_LAMP_CMD = "LAMPS";                 // Turn on/off the console's light

//
// Dump/Dump After responses
//
static constexpr std::string DMP_SEND_NEXT_PAGE = std::string(1, ACK);
static constexpr std::string DMP_CANCEL_DOWNLOAD = std::string(1, ESCAPE);
static constexpr std::string DMP_RESEND_PAGE = std::string(1, NACK);

//
// Generic strings for various command protocols
//
static constexpr std::string COMMAND_TERMINATOR = std::string(1, LINE_FEED);
static constexpr std::string RESPONSE_FRAME = std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);
static constexpr std::string COMMAND_RECOGNIZED_RESPONSE = RESPONSE_FRAME + "OK" + RESPONSE_FRAME;
static constexpr std::string DONE_RESPONSE = "DONE" + std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);
static constexpr std::string TEST_RESPONSE = "TEST" + std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);

//
// Types values that are used for command values or response data
//
//
// Cumulative Values that can be cleared using CLRVAR
//
enum CumulativeValue {
    DAILY_RAIN_CUM = 13,
    STORM_RAIN_CUM = 14,
    MONTH_RAIN_CUM = 16,
    YEAR_RAIN_CUM = 17,
    DAY_ET_CUM = 26,
    MONTH_ET_CUM = 25,
    YEAR_ET_CUM = 27
};

//
// High/Low Clear Types
//
enum ExtremePeriod : int {
    DAILY_EXTREMES = 0,
    MONTHLY_EXTREMES = 1,
    YEARLY_EXTREMES = 2
};

enum ArchivePeriod : int {
    ONE_MINUTE = 1,
    FIVE_MINUTES = 5,
    TEN_MINUTES = 10,
    FIFTEEN_MINUTES = 15,
    THIRTY_MINUTES = 30,
    ONE_HOUR = 60,
    TWO_HOURS = 120,
};

enum class BaudRate : int {
    BAUD_1200 = 1200,
    BAUD_2400 = 2400,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,
    BAUD_14400 = 14400,
    BAUD_19200 = 19200
};

enum class RainCupSizeType : int {
    POINT_01_INCH = 0,
    POINT_2_MM = 1,
    POINT_1_MM = 2
};

static constexpr double POINT_01_INCH_SIZE = 0.01;       // Inches
static constexpr double POINT_2_MM_SIZE   = 0.2 / 25.4; // Inches
static constexpr double POINT_1_MM_SIZE   = 0.1 / 25.4; // Inches

enum BarometerUnits : int {
    BARO_UNITS_01_INCHES = 0,
    BARO_UNITS_TENTH_MM = 1,
    BARO_UNITS_TENTH_HPA = 2,
    BARO_UNITS_TENTH_MILLIBAR = 3
};

enum TemperatureUnits : int {
    TEMPERATURE_UNITS_DEGREES = 0,
    TEMPERATURE_UNITS_TENTH_DEGREES = 1,
    TEMPERATURE_UNITS_CELSIUS = 2,
    TEMPERATURE_UNITS_TENTH_CELSIUS = 3
};

enum ElevationUnits : int {
    ELEVATION_UNITS_FEET = 0,
    ELEVATION_UNITS_METERS = 1
};

enum RainUnits : int {
    RAIN_UNITS_INCHES = 0,
    RAIN_UNITS_MILLIMETERS = 1
};

enum WindUnits : int {
    WIND_UNITS_MPH = 0,
    WIND_UNITS_MPS = 1,
    WIND_UNITS_KPH = 2,
    WIND_UNITS_KTS = 3
};

enum Month : int {
    JANUARY = 1,
    FEBRUARY = 2,
    MARCH = 3,
    APRIL = 4,
    MAY = 5,
    JUNE = 6,
    JULY = 7,
    AUGUST = 8,
    SEPTEMBER = 9,
    OCTOBER = 10,
    NOVEMBER = 11,
    DECEMBER = 12
};

//
// LOOP packet forecast icons
//
static constexpr short RAIN_BIT = 0x1;
static constexpr short CLOUDY_BIT = 0x2;
static constexpr short PARTLY_CLOUDY_BIT = 0x4;
static constexpr short SUNNY_BIT = 0x8;
static constexpr short SNOW_BIT = 0x10;

static constexpr int MOSTLY_CLEAR_FORECAST = SUNNY_BIT;
static constexpr int PARTLY_CLOUDY_FORECAST = PARTLY_CLOUDY_BIT | CLOUDY_BIT;
static constexpr int MOSTLY_CLOUDY_FORECAST = CLOUDY_BIT;
static constexpr int MOSTLY_CLOUDY_CHANCE_OF_RAIN_FORECAST = CLOUDY_BIT | RAIN_BIT;
static constexpr int MOSTLY_CLOUDY_CHANCE_OF_SNOW_FORECAST = CLOUDY_BIT | SNOW_BIT;
static constexpr int PARTLY_CLOUDY_CHANCE_OF_RAIN_FORECAST = PARTLY_CLOUDY_BIT | RAIN_BIT;
static constexpr int PARTLY_CLOUDY_CHANCE_OF_SNOW_FORECAST = PARTLY_CLOUDY_BIT | SNOW_BIT;
static constexpr int PARTLY_CLOUDY_CHANCE_OF_RAIN_OR_SNOW_FORECAST = PARTLY_CLOUDY_BIT | RAIN_BIT | SNOW_BIT;

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

//
// Scales
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

static constexpr int NORTH_HEADING_VALUE = 360;
static constexpr int NO_STORM_ACTIVE_DATE = -1;
static constexpr int MIN_LEAF_WETNESS = 0;
static constexpr int MAX_LEAF_WETNESS = 15;

//
// Invalid values, that is the value that is reported when the console has no value
//
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


}

#endif
