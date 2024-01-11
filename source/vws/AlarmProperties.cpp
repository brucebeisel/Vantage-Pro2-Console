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
#include "AlarmProperties.h"

namespace vws {

/**
 * An array of alarm properties, one for each entry in the Alarm Threshold block in the EEPROM.
 */
static const AlarmProperties alarmProperties[] = {
    {
        "Barometer Falling",
        "barometricPressure",     // Current weather field
         1,    1,                 // EEPROM threshold byte, threshold size
         0, 1000,                 // Value offset, value scale
         0,                       // Not set value
         0,                       // Triggered bit within LOOP packet alarms
         1, 255,                  // Minimum/Maximum values of the alarm threshold
         true
    },
    {
        "Barometer Rising",
        "barometricPressure",
         0,    1,
         0, 1000,
         0,
         1,
         1, 255,
         true
    },
    {
        "Low Inside Temperature",
        "insideTemperature",
          6,    1,
         90,    1,
        255,
          2,
          0, 254,
          true
    },
    {
        "High Inside Temperature",
        "insideTemperature",
          7,    1,
         90,    1,
        255,
          3,
          0, 254,
          true
    },
    {
        "Low Inside Humidity",
        "insideHumidity",
         40,   1,
          0,   1,
        255,
          4,
          0, 100,
          true
    },
    {
        "High Inside Humidity",
        "insideHumidity",
         41,   1,
          0,   1,
        255,
          5,
          0, 100,
          true
    },
    {
        "Time Alarm",
        "",
         2,     2,
         0,     1,
        -1,
         6,
         0,   255,
         false
    },
    {
        "Time Alarm 2s-Compliment",
        "",
         4,     2,
         0,     1,
         0,
        -1,
         0,   255,
         false
    },
    {
        "Low Outside Temperature",
        "outsideTemperature",
          8,    1,
         90,    1,
        255,
         16,
          0,  254,
          true
    },
    {
        "High Outside Temperature",
        "outsideTemperature",
          9,   1,
         90,   1,
        255,
         17,
          0, 254,
          true
    },
    {
        "Low Extra Temperature 1",
        "extraTemperature[0]",
         10,    1,
         90,    1,
        255,
         40,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 2",
        "extraTemperature[1]",
         11,    1,
         90,    1,
        255,
         48,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 3",
        "extraTemperature[2]",
         12,    1,
         90,    1,
        255,
         56,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 4",
        "extraTemperature[3]",
         13,    1,
         90,    1,
        255,
         64,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 5",
        "extraTemperature[4]",
         14,    1,
         90,    1,
        255,
         72,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 6",
        "extraTemperature[5]",
         15,    1,
         90,    1,
        255,
         80,
          0, 254,
          false
    },
    {
        "Low Extra Temperature 7",
        "extraTemperature[6]",
         16,    1,
         90,    1,
        255,
         88,
          0, 254,
          false
    },
    {
        "Low Soil Temperature 1",
        "soilTemperature[0]",
         17,    1,
         90,    1,
        255,
        102,
          0, 254,
          false
    },
    {
        "Low Soil Temperature 2",
        "soilTemperature[1]",
         18,    1,
         90,    1,
        255,
        (13 * 8) + 6,
          0, 254,
          false
    },
    {
        "Low Soil Temperature 3",
        "soilTemperature[2]",
         19,    1,
         90,    1,
        255,
         (14 * 8) + 6,
          0, 254,
          false
    },
    {
        "Low Soil Temperature 4",
        "soilTemperature[3]",
         20,    1,
         90,    1,
        255,
         (15 * 8) + 6,
          0, 254,
          false
    },
    {
        "Low Leaf Temperature 1",
        "leafTemperature[0]",
         21,    1,
         90,    1,
        255,
         (12 * 8) + 4,
         0,   254,
         false
    },
    {
        "Low Leaf Temperature 2",
        "leafTemperature[1]",
         22,    1,
         90,    1,
        255,
         (13 * 8) + 4,
          0,  254,
          false
    },
    {
        "Low Leaf Temperature 3",
        "leafTemperature[2]",
         23,    1,
         90,    1,
        255,
         (14 * 8) + 4,
          0,  254,
          false
    },
    {
        "Low Leaf Temperature 4",
        "leafTemperature[3]",
         24,   1,
         90,   1,
        255,
         (15 * 8) + 4,
          0,  254,
          false
    },
    {
        "High Extra Temperature 1",
        "extraTemperature[0]",
         25,    1,
         90,    1,
        255,
          (5 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 2",
        "extraTemperature[1]",
         26,    1,
         90,    1,
        255,
          (6 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 3",
        "extraTemperature[2]",
         27,    1,
         90,    1,
        255,
          (7 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 4",
        "extraTemperature[3]",
         28,    1,
         90,    1,
        255,
          (8 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 5",
        "extraTemperature[4]",
         29,    1,
         90,    1,
        255,
          (9 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 6",
        "extraTemperature[5]",
         30,    1,
         90,    1,
        255,
         (10 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Extra Temperature 7",
        "extraTemperature[6]",
         31,    1,
         90,    1,
        255,
         (11 * 8) + 1,
          0,  254,
          false
    },
    {
        "High Soil Temperature 1",
        "soilTemperature[0]",
         32,    1,
         90,    1,
        255,
         (12 * 8) + 7,
          0,  254,
          false
    },
    {
        "High Soil Temperature 2",
        "soilTemperature[1]",
         33,    1,
         90,    1,
        255,
         (13 * 8) + 7,
          0,  254,
          false
    },
    {
        "High Soil Temperature 3",
        "soilTemperature[2]",
         34,    1,
         90,    1,
        255,
         (14 * 8) + 7,
          0,  254,
          false
    },
    {
        "High Soil Temperature 4",
        "soilTemperature[3]",
         35,    1,
         90,    1,
        255,
         (15 * 8) + 7,
          0,  254,
          false
    },
    {
        "High Leaf Temperature 1",
        "leafTemperature[0]",
         36,    1,
         90,    1,
        255,
         (12 * 8) + 5,
          0,  254,
          false
    },
    {
        "High Leaf Temperature 2",
        "leafTemperature[1]",
         37,    1,
         90,    1,
        255,
         (13 * 8) + 5,
          0,  254,
          false
    },
    {
        "High Leaf Temperature 3",
        "leafTemperature[2]",
         38,    1,
         90,    1,
        255,
         (14 * 8) + 5,
          0,  254,
          false
    },
    {
        "High Leaf Temperature 4",
        "leafTemperature[3]",
         39,   1,
         90,   1,
        255,
         (15 * 8) + 5,
          0,  254,
          false
    },
    {
        "Low Outside Humidity",
        "outsideHumidity",
         42,   1,
          0,   1,
        255,
          (4 * 8) + 2,
          0,  100,
          true
    },
    {
        "Low Extra Humidity 1",
        "extraHumidity[0]",
         43,   1,
          0,   1,
        255,
          (5 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 2",
        "extraHumidity[1]",
         44,   1,
          0,   1,
        255,
          (6 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 3",
        "extraHumidity[2]",
         45,   1,
          0,   1,
        255,
          (7 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 4",
        "extraHumidity[3]",
         46,   1,
          0,   1,
        255,
          (8 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 5",
        "extraHumidity[4]",
         47,   1,
          0,   1,
        255,
          (9 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 6",
        "extraHumidity[5]",
         48,   1,
          0,   1,
        255,
          (0 * 8) + 2,
          0,  100,
          false
    },
    {
        "Low Extra Humidity 7",
        "extraHumidity[6]",
         49,   1,
          0,   1,
        255,
          (1 * 8) + 2,
          0,  100,
          false
    },
    {
        "High Outside Humidity",
        "outsideHumidity",
         50,   1,
          0,   1,
        255,
          (4 * 8) + 3,
          0,  100,
          false
    },
    {
        "High Extra Humidity 1",
        "extraHumidity[0]",
         51,   1,
          0,   1,
        255,
          (5 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 2",
        "extraHumidity[1]",
         52,   1,
          0,   1,
        255,
          (6 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 3",
        "extraHumidity[2]",
         43,   1,
          0,   1,
        255,
          (7 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 4",
        "extraHumidity[3]",
         54,   1,
          0,   1,
        255,
          (8 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 5",
        "extraHumidity[4]",
         55,   1,
          0,   1,
        255,
          (9 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 6",
        "extraHumidity[5]",
         56,   1,
          0,   1,
        255,
         (10 * 8) + 3,
          0, 100,
          false
    },
    {
        "High Extra Humidity 7",
        "extraHumidity[6]",
         57,   1,
          0,   1,
        255,
         (11 * 8) + 3,
          0, 100,
          false
    },
    {
        "Low Dew Point",
        "dewPoint",
         58,   1,
        120,   1,
        255,
          (2 * 8) + 4,
          0, 100,
          true
    },
    {
        "High Dew Point",
        "dewPoint",
         59,   1,
        120,   1,
        255,
          (2 * 8) + 5,
          0, 100,
          true
    },
    {
        "Low Wind Chill",
        "windChill",
         60,   1,
        120,   1,
        255,
          (2 * 8) + 7,
          0, 100,
          true
    },
    {
        "High Heat Index",
        "heatIndex",
         61,   1,
         90,   1,
        255,
          (2 * 8) + 6,
          0, 254,
          true
    },
    {
        "High THSW",
        "thsw",
         62,   1,
         90,   1,
        255,
          (3 * 8) + 0,
          0, 254,
          true
    },
    {
        "Wind Speed",
        "windSpeed",
         63,   1,
          0,   1,
        255,
          (2 * 8) + 2,
          0, 254,
          true
    },
    {
        "10 Minute Average Wind Speed",
        "windSpeed10MinAvg",
         64,   1,
          0,   1,
        255,
          (2 * 8) + 3,
          0, 254,
          true
    },
    {
        "High UV",
        "uvIndex",
         65,   1,
          0,  10,
        255,
          (3 * 8) + 2,
          0, 254,
          true
    },
    {
        "UV Dose", // This alarm is a bit strange according to the serial protocol manual
        "uvIndex",
         66,   1,
          0,   1,
        255,
          (3 * 8) + 3,
          0,   254,
          false
    },
    {
        "Low Soil Moisture 1",
        "soilMoisture[0]",
         67,   1,
          0,   1,
        255,
         (12 * 8) + 2,
          0, 254,
          false
    },
    {
        "Low Soil Moisture 2",
        "soilMoisture[1]",
         68,   1,
          0,   1,
        255,
         (13 * 8) + 2,
         0,  254,
         false
    },
    {
        "Low Soil Moisture 3",
        "soilMoisture[2]",
         69,   1,
          0,   1,
        255,
         (14 * 8) + 2,
         0,  254,
         false
    },
    {
        "Low Soil Moisture 4",
        "soilMoisture[3]",
         70,   1,
          0,   1,
        255,
         (15 * 8) + 2,
         0,  254,
         false
    },
    {
        "High Soil Moisture 1",
        "soilMoisture[0]",
         71,   1,
          0,   1,
        255,
         (12 * 8) + 3,
         0,  254,
         false
    },
    {
        "High Soil Moisture 2",
        "soilMoisture[1]",
         72,   1,
          0,   1,
        255,
         (13 * 8) + 3,
          0, 254,
          false
    },
    {
        "High Soil Moisture 3",
        "soilMoisture[2]",
         73,   1,
          0,   1,
        255,
         (14 * 8) + 3,
          0, 254,
          false
    },
    {
        "High Soil Moisture 4",
        "soilMoisture[3]",
         74,   1,
          0,   1,
        255,
         (15 * 8) + 3,
          0, 254,
          false
    },
    {
        "Low Leaf Wetness 1",
        "leafWetness[0]",
         75,   1,
          0,   1,
        255,
         (12 * 8) + 0,
          0,  15,
          false
    },
    {
        "Low Leaf Wetness 2",
        "leafWetness[1]",
         76,   1,
          0,   1,
        255,
         (13 * 8) + 0,
          0,  15,
          false
    },
    {
        "Low Leaf Wetness 3",
        "leafWetness[2]",
         77,   1,
          0,   1,
        255,
         (14 * 8) + 0,
          0,  15,
          false
    },
    {
        "Low Leaf Wetness 4",
        "leafWetness[3]",
         78,   1,
          0,   1,
        255,
         (15 * 8) + 0,
          0,  15,
          false
    },
    {
        "High Leaf Wetness 1",
        "leafWetness[0]",
         79,   1,
          0,   1,
        255,
         (12 * 8) + 1,
          0,  15,
          false
    },
    {
        "High Leaf Wetness 2",
        "leafWetness[1]",
         80,   1,
          0,   1,
        255,
         (13 * 8) + 1,
          0,  15,
          false
    },
    {
        "High Leaf Wetness 3",
        "leafWetness[2]",
         81,   1,
          0,   1,
        255,
         (14 * 8) + 1,
          0,  15,
          false
    },
    {
        "High Leaf Wetness 4",
        "leafWetness[3]",
         82,   1,
          0,   1,
        255,
         (15 * 8) + 1,
          0,  15,
          false
    },
    {
        "High Solar Radiation",
        "solarRadiation",
         83,   2,
          0,   1,
      32767,
          (3 * 8) + 1,
          1, 1800,
          true
    },
    { // TBD, rate alarm need rain collector size
        "High Rain Rate",
        "rainRate",
         85,   2,
          0,   1,
      65535,
          (1 * 8) + 0,
          1, 60000,
          true
    },
    { // TBD, rate alarm need rain collector size
        "15 Minute Rain",
        "rain15Minute",
         87,   2,
          0,   1,
      65535,
          (1 * 8) + 1,
          1,  10000,
          true
    },
    { // TBD, rate alarm need rain collector size
        "24 Hour Rain",
        "rain24Hour",
         89,   2,
          0,   1,
      65535,
          (1 * 8) + 1,
          1, 10000,
          true
    },
    { // TBD, rate alarm need rain collector size
        "Storm Total Rain",
        "stormRain",
         91,   2,
          0,   1,
      65535,
          (1 * 8) + 3,
          1, 10000,
          true
    },
    {
        "Daily ET",
        "dayET",
         93,   1,
          0,1000,
        255,
          (1 * 8) + 4,
          1, 254,
          true
    }
};

static const int numProperties = sizeof(alarmProperties) / sizeof(alarmProperties[0]);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
AlarmProperties::getAlarmPropertyCount() {
    return numProperties;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const AlarmProperties *
AlarmProperties::getAlarmProperties(int & count) {
    count = numProperties;
    return alarmProperties;
}

}
