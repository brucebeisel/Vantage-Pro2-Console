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
#include "Alarm.h"
#include "BitConverter.h"

using namespace std;

namespace vws {

static const AlarmProperties alarmProperties[] = {
    {
        "Barometer Falling",
         1,    1,
         0, 1000,
         0,
         0
    },
    {
        "Barometer Rising",
         0,    1,
         0, 1000,
         0,
         1
    },
    {
        "Low Inside Temperature",
          6,    1,
         90,    1,
        255,
          2
    },
    {
        "High Inside Temperature",
          7,    1,
         90,    1,
        255,
          3
    },
    {
        "Low Inside Humidity",
         40,   1,
          0,   1,
        255,
          4
    },
    {
        "High Inside Humidity",
         41,   1,
          0,   1,
        255,
          5
    },
    {
        "Time Alarm",
         2,     2,
         0,     1,
         0xffff,
         6
    },
    {
        "Time Alarm 2s-Compliment",
         4,     2,
         0,     1,
         0xffff,
         -1
    },
    {
        "Low Outside Temperature",
          8,    1,
         90,    1,
        255,
         16
    },
    {
        "High Outside Temperature",
          9,   1,
         90,   1,
        255,
         17,
    },
    {
        "Extra Temperature/Humidity 1 - Low Temperature",
         10,    1,
         90,    1,
        255,
         40,
    },
    {
        "Extra Temperature/Humidity 2 - Low Temperature",
         11,    1,
         90,    1,
        255,
         48,
    },
    {
        "Extra Temperature/Humidity 3 - Low Temperature",
         12,    1,
         90,    1,
        255,
         56,
    },
    {
        "Extra Temperature/Humidity 4 - Low Temperature",
         13,    1,
         90,    1,
        255,
         64,
    },
    {
        "Extra Temperature/Humidity 5 - Low Temperature",
         14,    1,
         90,    1,
        255,
         72,
    },
    {
        "Extra Temperature/Humidity 6 - Low Temperature",
         15,    1,
         90,    1,
        255,
         80,
    },
    {
        "Extra Temperature/Humidity 7 - Low Temperature",
         16,    1,
         90,    1,
        255,
         88,
    },
    {
        "Soil/Leaf 1 - Low Soil Temperature",
         17,    1,
         90,    1,
        255,
        102,
    },
    {
        "Soil/Leaf 2 - Low Soil Temperature",
         18,    1,
         90,    1,
        255,
        (13 * 8) + 6,
    },
    {
        "Soil/Leaf 3 - Low Soil Temperature",
         19,    1,
         90,    1,
        255,
         (14 * 8) + 6
    },
    {
        "Soil/Leaf 4 - Low Soil Temperature",
         20,    1,
         90,    1,
        255,
         (15 * 8) + 6
    },
    {
        "Soil/Leaf 1 - Low Leaf Temperature",
         21,    1,
         90,    1,
        255,
         (12 * 8) + 4
    },
    {
        "Soil/Leaf 2 - Low Leaf Temperature",
         22,    1,
         90,    1,
        255,
         (13 * 8) + 4
    },
    {
        "Soil/Leaf 3 - Low Leaf Temperature",
         23,    1,
         90,    1,
        255,
         (14 * 8) +    4
    },
    {
        "Soil/Leaf 4 - Low Leaf Temperature",
         24,   1,
         90,   1,
        255,
         (15 * 8) +   4
    },
    {
        "Extra Temperature/Humidity 1 - High Temperature",
         25,    1,
         90,    1,
        255,
          (5 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 2 - High Temperature",
         26,    1,
         90,    1,
        255,
          (6 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 3 - High Temperature",
         27,    1,
         90,    1,
        255,
          (7 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 4 - High Temperature",
         28,    1,
         90,    1,
        255,
          (8 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 5 - High Temperature",
         29,    1,
         90,    1,
        255,
          (9 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 6 - High Temperature",
         30,    1,
         90,    1,
        255,
         (10 * 8) +    1
    },
    {
        "Extra Temperature/Humidity 7 - High Temperature",
         31,    1,
         90,    1,
        255,
         (11 * 8) +    1
    },
    {
        "Soil/Leaf 1 - High Soil Temperature",
         32,    1,
         90,    1,
        255,
         (12 * 8) +    7
    },
    {
        "Soil/Leaf 2 - High Soil Temperature",
         33,    1,
         90,    1,
        255,
         (13 * 8) +    7
    },
    {
        "Soil/Leaf 3 - High Soil Temperature",
         34,    1,
         90,    1,
        255,
         (14 * 8) +    7
    },
    {
        "Soil/Leaf 4 - High Soil Temperature",
         35,    1,
         90,    1,
        255,
         (15 * 8) +    7
    },
    {
        "Soil/Leaf 1 - High Leaf Temperature",
         36,    1,
         90,    1,
        255,
         (12 * 8) +    5
    },
    {
        "Soil/Leaf 2 - High Leaf Temperature",
         37,    1,
         90,    1,
        255,
         (13 * 8) +    5
    },
    {
        "Soil/Leaf 3 - High Leaf Temperature",
         38,    1,
         90,    1,
        255,
         (14 * 8) +    5
    },
    {
        "Soil/Leaf 4 - High Leaf Temperature",
         39,   1,
         90,   1,
        255,
         (15 * 8) +   5
    },
    {
        "Low Outside Humidity",
         42,   1,
          0,   1,
        255,
          (4 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 1 - Low Humidity",
         43,   1,
          0,   1,
        255,
          (5 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 2 - Low Humidity",
         44,   1,
          0,   1,
        255,
          (6 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 3 - Low Humidity",
         45,   1,
          0,   1,
        255,
          (7 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 4 - Low Humidity",
         46,   1,
          0,   1,
        255,
          (8 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 5 - Low Humidity",
         47,   1,
          0,   1,
        255,
          (9 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 6 - Low Humidity",
         48,   1,
          0,   1,
        255,
          (0 * 8) +   2
    },
    {
        "Extra Temperature/Humidity 7 - Low Humidity",
         49,   1,
          0,   1,
        255,
          (1 * 8) +   2
    },
    {
        "High Outside Humidity",
         50,   1,
          0,   1,
        255,
          (4 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 1 - High Humidity",
         51,   1,
          0,   1,
        255,
          (5 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 2 - High Humidity",
         52,   1,
          0,   1,
        255,
          (6 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 3 - High Humidity",
         43,   1,
          0,   1,
        255,
          (7 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 4 - High Humidity",
         54,   1,
          0,   1,
        255,
          (8 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 5 - High Humidity",
         55,   1,
          0,   1,
        255,
          (9 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 6 - High Humidity",
         56,   1,
          0,   1,
        255,
         (10 * 8) +   3
    },
    {
        "Extra Temperature/Humidity 7 - High Humidity",
         57,   1,
          0,   1,
        255,
         (11 * 8) +   3
    },
    {
        "Low Dew Point",
         58,   1,
        120,   1,
        255,
          (2 * 8) +   4
    },
    {
        "High Dew Point",
         59,   1,
        120,   1,
        255,
          (2 * 8) +   5
    },
    {
        "Low Wind Chill",
         60,   1,
        120,   1,
        255,
          (2 * 8) +   7
    },
    {
        "High Heat Index",
         61,   1,
         90,   1,
        255,
          (2 * 8) +   6
    },
    {
        "High THSW",
         62,   1,
         90,   1,
        255,
          (3 * 8) +   0
    },
    {
        "Wind Speed",
         63,   1,
          0,   1,
        255,
          (2 * 8) +   2
    },
    {
        "10 Minute Average Wind Speed",
         64,   1,
          0,   1,
        255,
          (2 * 8) +   3
    },
    {
        "High UV",
         65,   1,
          0,  10,
        255,
          (3 * 8) +   2
    },
    {
        "UNAVAILABLE",
         66,   1,
          0,   1,
          0,
         -1
    },
    {
        "Soil/Leaf 1 - Low Soil Moisture",
         67,   1,
          0,   1,
        255,
         (12 * 8) +   2
    },
    {
        "Soil/Leaf 2 - Low Soil Moisture",
         68,   1,
          0,   1,
        255,
         (13 * 8) +   2
    },
    {
        "Soil/Leaf 3 - Low Soil Moisture",
         69,   1,
          0,   1,
        255,
         (14 * 8) +   2
    },
    {
        "Soil/Leaf 4 - Low Soil Moisture",
         70,   1,
          0,   1,
        255,
         (15 * 8) +   2
    },
    {
        "Soil/Leaf 1 - High Soil Moisture",
         71,   1,
          0,   1,
        255,
         (12 * 8) +   3
    },
    {
        "Soil/Leaf 2 - High Soil Moisture",
         72,   1,
          0,   1,
        255,
         (13 * 8) +   3
    },
    {
        "Soil/Leaf 3 - High Soil Moisture",
         73,   1,
          0,   1,
        255,
         (14 * 8) +   3
    },
    {
        "Soil/Leaf 4 - High Soil Moisture",
         74,   1,
          0,   1,
        255,
         (15 * 8) +   3
    },
    {
        "Soil/Leaf 1 - Low Leaf Wetness",
         75,   1,
          0,   1,
        255,
         (12 * 8) +   0
    },
    {
        "Soil/Leaf 2 - Low Leaf Wetness",
         76,   1,
          0,   1,
        255,
         (13 * 8) +   0
    },
    {
        "Soil/Leaf 3 - Low Leaf Wetness",
         77,   1,
          0,   1,
        255,
         (14 * 8) +   0
    },
    {
        "Soil/Leaf 4 - Low Leaf Wetness",
         78,   1,
          0,   1,
        255,
         (15 * 8) +   0
    },
    {
        "Soil/Leaf 1 - High Leaf Wetness",
         79,   1,
          0,   1,
        255,
         (12 * 8) +   1
    },
    {
        "Soil/Leaf 2 - High Leaf Wetness",
         80,   1,
          0,   1,
        255,
         (13 * 8) +   1
    },
    {
        "Soil/Leaf 3 - High Leaf Wetness",
         81,   1,
          0,   1,
        255,
         (14 * 8) +   1
    },
    {
        "Soil/Leaf 4 - High Leaf Wetness",
         82,   1,
          0,   1,
        255,
         (15 * 8) +   1
    },
    {
        "High Solar Radiation",
         83,   2,
          0,   1,
      65535,
          (3 * 8) +   1
    },
    { // TBD, rate alarm need rain collector size
        "High Rain Rate",
         85,   2,
          0,   1,
      65535,
          (1 * 8) +   0
    },
    { // TBD, rate alarm need rain collector size
        "15 Minute Rain",
         87,   2,
          0,   1,
      65535,
          (1 * 8) +   1
    },
    { // TBD, rate alarm need rain collector size
        "24 Hour Rain",
         89,   2,
          0,   1,
      65535,
          (1 * 8) +   1
    },
    { // TBD, rate alarm need rain collector size
        "Storm Total Rain",
         91,   2,
          0,   1,
      65535,
          (1 * 8) +   3
    },
    { 
        "Daily ET",
         93,   2,
          0,1000,
        255,
          (1 * 8) +   4
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Alarm::Alarm(const AlarmProperties & properties) : properties(properties),
                                                   eepromThreshold(properties.eepromNotSetThreshold),
                                                   actualThreshold(0.0),
                                                   alarmThresholdSet(false),
                                                   alarmTriggered(false) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setThreshold(int eepromThreshold) {
    this->eepromThreshold = eepromThreshold;
    if (this->eepromThreshold == properties.eepromNotSetThreshold) {
        alarmThresholdSet = false;
        alarmTriggered = false;
        actualThreshold = 0.0;
    }
    else {
        alarmThresholdSet = true;
        alarmTriggered = false;
        actualThreshold = static_cast<float>(eepromThreshold - properties.eepromThresholdOffset) / properties.eepromThresholdScale;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setTriggered(bool triggered) {
    alarmTriggered = triggered;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AlarmProperties
Alarm::getAlarmProperties() const {
    return properties;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::initialize() {
    for (int i = 0; i < sizeof(alarmProperties) / sizeof(alarmProperties[0]); i++) {
        Alarm alarm(alarmProperties[i]);
        alarms.push_back(alarm);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::loadThresholds(const byte buffer[]) {
    for (vector<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        AlarmProperties props = it->getAlarmProperties();
        int offset = props.eepromThresholdByte;
        int thresholdValue = 0;

        if (props.eepromThresholdSize == 1) 
            thresholdValue = BitConverter::toInt8(buffer, offset);
        else
            thresholdValue = BitConverter::toInt16(buffer, offset);

        it->setThreshold(thresholdValue);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::setAlarmStates(const LoopPacket::AlarmBitSet & alarmBits) {
    for (vector<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        AlarmProperties props = it->getAlarmProperties();
        it->setTriggered(alarmBits[props.alarmBit] == 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::processLoopPacket(const LoopPacket & packet) {
    setAlarmStates(packet.getAlarmBits());
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::processLoop2Packet(const Loop2Packet & packet) {
    return true;
}
}
