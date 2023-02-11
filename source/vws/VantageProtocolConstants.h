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

#include "WeatherTypes.h"

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
// Wake up command/response
//
static const std::string WAKEUP_COMMAND = std::string(1, LINE_FEED);
static const std::string WAKEUP_RESPONSE = std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);

//
// Testing Commands
//
static const std::string TEST_CMD = "TEST";                          // Sends the string "TEST\n" back
static constexpr char WRD_BYTE1 = 0x12;
static constexpr char WRD_BYTE2 = 0x4D;
static constexpr char WRD_BUFFER[] = {'W', 'R', 'D', WRD_BYTE1, WRD_BYTE2};
static const std::string STATION_TYPE_CMD(WRD_BUFFER);               // Responds with the weather station type. This command is backward compatible with earlier Davis weather products.
static const std::string RECEIVE_CHECK_CMD = "RXCHECK";              // Console diagnostics report
static const std::string RXTEST_CMD = "RXTEST";                      // Move console to main current conditions screen
static const std::string FIRMWARE_DATE_CMD = "VER";                  // Firmware date
static const std::string RECEIVER_LIST_CMD = "RECEIVERS";            // Get the list of receivers as a bitmap, bit 0 represents station ID 1
static const std::string FIRMWARE_VERSION_CMD = "NVER";              // Get the firmware version

//
// Current Data Commands
//
static const std::string LOOP_CMD = "LOOP";                          // Get the current data values, alarms, battery status, etc. through the LOOP packet. Note that the LPS command renders this superfluous.
static const std::string LPS_CMD = "LPS 3";                          // Get the current values through both the LOOP and LOOP2 packets
static const std::string HIGH_LOW_CMD = "HILOWS";                    // Get the high and low that includes daily, monthly and yearly
static const std::string PUT_YEARLY_RAIN_CMD = "PUTRAIN";            // Set the yearly rainfall
static const std::string PUT_YEARLY_ET_CMD = "PUTET";                // Set the yearly ET

//
// Download Commands
//
static const std::string DUMP_ARCHIVE_CMD = "DMP";                   // Dump the entire archive
static const std::string DUMP_AFTER_CMD = "DMPAFT";                  // Dump the archive after a given date/time

//
// EEPROM Commands
//
static const std::string DUMP_EEPROM_CMD = "GETEE";                  // Read the entire EEPROM data block
static const std::string WRITE_EEPROM_CMD = "EEWR";                  // Write a single byte to EEPROM as hex strings
static const std::string READ_EEPROM_CMD = "EERD";                   // Read EEPROM address as hex strings
static const std::string WRITE_EEPROM_AS_BINARY_CMD = "EEBWR";       // Write to EEPROM as binary
static const std::string READ_EEPROM_AS_BINARY_CMD = "EEBRD";        // Read EEPROM address as binary

//
// Calibration Commands
//
static const std::string CALIBRATE_TEMPERATURE_HUMIDITY = "CALED";   // Send temperature and humidity calibration values
static const std::string CALIBRATE_TEMPERATURE_HUMIDITY2 = "CALFIX"; // Updates the display when calibration numbers have changed
static const std::string SET_BAROMETRIC_DATA_CMD = "BAR=";            // Sets barometric offset using local reading and/or elevation
static const std::string SET_BAROMETRIC_CAL_DATA_CMD = "BARDATA";    // Get the current barometer calibration parameters

//
// Clearing Commands
//
static const std::string CLEAR_ARCHIVE_CMD = "CLRLOG";               // Clear the archived data
static const std::string CLEAR_ALARM_THRESHOLDS_CMD = "CLRALM";      // Clear the alarm thresholds
static const std::string CLEAR_TEMP_HUMID_CAL_CMD = "CLRCAL";        // Set temperature and humidity calibration offsets to zero
static const std::string CLEAR_GRAPH_POINTS_CMD = "CLRGRA";          // Clear the graph points
static const std::string CLEAR_CUMULATIVE_VALUE_CMD = "CLRVAR";      // Clear the specified cumulative value
static const std::string CLEAR_HIGH_VALUES_CMD = "CLRHIGHS";         // Clear the daily, monthly or yearly high values
static const std::string CLEAR_LOW_VALUES_CMD = "CLRLOWS";           // Clear the daily, monthly or yearly low values
static const std::string CLEAR_ACTIVE_ALARMS_CMD = "CLRBITS";        // Clear active alarms
static const std::string CLEAR_CURRENT_DATA_VALUES_CMD = "CLRDATA";  // Clear all current data values which includes setting the rain and ET cumulative values to 0!

//
// Configuration Commands
//
static const std::string SET_BAUD_RATE_CMD("BAUD");                 // Sets the console to a new baud rate. Valid values are 1200, 2400, 4800, 9600, 14400 and 19200
static const std::string SET_TIME_CMD = "SETTIME";                   // Sets the time and date on the console
static const std::string GET_TIME_CMD = "GETTIME";                   // Retrieves the current time and date on the Vantage console. Data is sent in binary format
static const std::string SET_ARCHIVE_PERIOD_CMD = "SETPER";          // Set how often the console saves an archive record
static const std::string STOP_ARCHIVING_CMD = "STOP";                // Disables the creation of archive records
static const std::string START_ARCHIVING_CMD = "START";              // Enables the create of archive records
static const std::string REINITIALIZE_CMD = "NEWSETUP";              // Reinitialize the console after making any significant changes to the console's configuration
static const std::string CONTROL_LAMP_CMD = "LAMPS";                 // Turn on/off the console's light

//
// Dump/Dump After responses
//
static const std::string DMP_SEND_NEXT_PAGE = std::string(1, ACK);
static const std::string DMP_CANCEL_DOWNLOAD = std::string(1, ESCAPE);
static const std::string DMP_RESEND_PAGE = std::string(1, NACK);

//
// Generic strings for various command protocols
//
static const std::string COMMAND_TERMINATOR = std::string(1, LINE_FEED);
static const std::string RESPONSE_FRAME = std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);
static const std::string COMMAND_RECOGNIZED_RESPONSE = RESPONSE_FRAME + "OK" + RESPONSE_FRAME;
static const std::string DONE_RESPONSE = "DONE" + std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);
static const std::string TEST_RESPONSE = "TEST" + std::string(1, LINE_FEED) + std::string(1, CARRIAGE_RETURN);

//
// Types values that are used for command values or response data
//
//
// Cumulative Values that can be cleared using CLRVAR
//
enum class CumulativeValue {
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
enum class ExtremePeriod {
    DAILY = 0,
    MONTHLY = 1,
    YEARLY = 2
};

enum class ArchivePeriod {
    ONE_MINUTE = 1,
    FIVE_MINUTES = 5,
    TEN_MINUTES = 10,
    FIFTEEN_MINUTES = 15,
    THIRTY_MINUTES = 30,
    ONE_HOUR = 60,
    TWO_HOURS = 120,
};

enum class BaudRate {
    BAUDRATE_1200 = 1200,
    BAUDRATE_2400 = 2400,
    BAUDRATE_4800 = 4800,
    BAUDRATE_9600 = 9600,
    BAUDRATE_14400 = 14400,
    BAUDRATE_19200 = 19200
};

enum class RainCupSizeType {
    POINT_01_INCH = 0,
    POINT_2_MM = 1,
    POINT_1_MM = 2
};

static constexpr double POINT_01_INCH_SIZE = 0.01;       // Inches
static constexpr double POINT_2_MM_SIZE   = 0.2 / 25.4; // Inches
static constexpr double POINT_1_MM_SIZE   = 0.1 / 25.4; // Inches

enum class BarometerUnits {
    IN_HG = 0,
    MILLIMETER = 1,
    HPA = 2,
    MILLIBAR = 3
};

enum class TemperatureUnits {
    FAHRENHEIT = 0,
    TENTH_FAHRENHEIT = 1,
    CELSIUS = 2,
    TENTH_CELSIUS = 3
};

enum class ElevationUnits {
    FEET = 0,
    METERS = 1
};

enum class RainUnits {
    INCHES = 0,
    MILLIMETERS = 1
};

enum class WindUnits {
    MPH = 0,
    MPS = 1,
    KPH = 2,
    KTS = 3
};

enum class Month {
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
/**
 * The trend of the barometer as reported in the LOOP packet
 */
enum class BarometerTrend {
    STEADY =            0,
    RISING_SLOWLY =    20,
    RISING_RAPIDLY =   60,
    FALLING_RAPIDLY = 196,
    FALLING_SLOWLY =  236,
    UNKNOWN =         255
};


//
// LOOP packet forecast icons
//
static constexpr short RAIN_BIT = 0x1;
static constexpr short CLOUD_BIT = 0x2;
static constexpr short PARTLY_CLOUDY_BIT = 0x4;
static constexpr short SUN_BIT = 0x8;
static constexpr short SNOW_BIT = 0x10;

enum class Forecast {
    SUNNY =                                 SUN_BIT,
    PARTLY_CLOUDY =                         CLOUD_BIT | PARTLY_CLOUDY_BIT,
    MOSTLY_CLOUDY =                         CLOUD_BIT,
    MOSTLY_CLOUDY_WITH_RAIN =               CLOUD_BIT | RAIN_BIT,
    MOSTLY_CLOUDY_WITH_SNOW =               CLOUD_BIT | SNOW_BIT,
    MOSTLY_CLOUDY_WITH_RAIN_OR_SNOW =       CLOUD_BIT | RAIN_BIT | SNOW_BIT,
    PARTLY_CLOUDY_WITH_RAIN_LATER =         PARTLY_CLOUDY_BIT | RAIN_BIT,
    PARTLY_CLOUDY_WITH_SNOW_LATER =         PARTLY_CLOUDY_BIT | SNOW_BIT,
    PARTLY_CLOUDY_WITH_RAIN_OR_SNOW_LATER = PARTLY_CLOUDY_BIT | RAIN_BIT | SNOW_BIT
};

//
// Maximum counts
//
static constexpr int NUM_ARCHIVE_RECORDS = 2560;
static constexpr int MAX_STATION_ID = 8;
static constexpr int MAX_STATIONS = 8;
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
//
// The protocol document suggests there are 4 possible leaf wetness values, but the Leaf/Soil
// sensor station manual states that there can only be two leaf wetness sensors installed.
// In addition, the 4th or [3] leaf wetness in the LOOP packet does not contain 0xFF, but
// has the value of 0, which implies it is a real value, not unknown.
//
static constexpr int MAX_LEAF_WETNESSES = 4;

//
// Scales and offsets
//
static const vws::Temperature        TEMPERATURE_16BIT_SCALE = 10.0;
static const vws::Temperature        TEMPERATURE_8BIT_OFFSET = 90.0;
static const vws::Pressure           BAROMETER_SCALE = 1000.0;
static const vws::Speed              AVG_WIND_SPEED_SCALE = 10.0;
static const int                     YEAR_OFFSET = 2000;
static const vws::UvIndex            UV_INDEX_SCALE = 10.0;
static const vws::Evapotranspiration DAY_ET_SCALE = 1000.0;
static const vws::Evapotranspiration MONTH_YEAR_ET_SCALE = 100.0;
static const vws::Rainfall           STORM_RAIN_SCALE = 100.0;
static const double                  LAT_LON_SCALE = 10.0;

//
// Invalid values, that is the value that is reported when the console has no value
//
static constexpr int INVALID_16BIT_TEMPERATURE = 32767;
static constexpr int INVALID_16BIT_TEMPERATURE_NEGATIVE = -32768; // Sometimes the invalid temperature is negative
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
static constexpr int INVALID_THSW = 255;
static constexpr int INVALID_ET = 0;
static constexpr int INVALID_16BIT_TIME = 65535;

//
// Limits
//
static constexpr int NORTH_HEADING_VALUE = 360;
static constexpr int NO_STORM_ACTIVE_DATE = -1;
static constexpr int MIN_LEAF_WETNESS_VALUE = 0;
static constexpr int MAX_LEAF_WETNESS_VALUE = 15;


//
// Used to translate archive wind direction index of 0 - 15 to a heading.
//
static constexpr int NUM_WIND_DIR_SLICES = 16;
static constexpr double DEGREES_PER_SLICE = 360.0 / NUM_WIND_DIR_SLICES;
}

#endif
