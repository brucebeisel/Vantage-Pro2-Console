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
#include "VantageEepromConstants.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;

static const AlarmProperties alarmProperties[] = {
    {
        "Barometer Falling",
         1,    1,                 // EEPROM threshold byte, threshold size
         0, 1000,                 // Value offset, value scale
         0,                       // Not set value
         0                        // Triggered bit within LOOP packet alarms
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
        -1,
         6
    },
    {
        "Time Alarm 2s-Compliment",
         4,     2,
         0,     1,
         0,
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
        "Low Extra Temperature 1",
         10,    1,
         90,    1,
        255,
         40,
    },
    {
        "Low Extra Temperature 2",
         11,    1,
         90,    1,
        255,
         48,
    },
    {
        "Low Extra Temperature 3",
         12,    1,
         90,    1,
        255,
         56,
    },
    {
        "Low Extra Temperature 4",
         13,    1,
         90,    1,
        255,
         64,
    },
    {
        "Low Extra Temperature 5",
         14,    1,
         90,    1,
        255,
         72,
    },
    {
        "Low Extra Temperature 6",
         15,    1,
         90,    1,
        255,
         80,
    },
    {
        "Low Extra Temperature 7",
         16,    1,
         90,    1,
        255,
         88,
    },
    {
        "Low Soil Temperature 1",
         17,    1,
         90,    1,
        255,
        102,
    },
    {
        "Low Soil Temperature 2",
         18,    1,
         90,    1,
        255,
        (13 * 8) + 6,
    },
    {
        "Low Soil Temperature 3",
         19,    1,
         90,    1,
        255,
         (14 * 8) + 6
    },
    {
        "Low Soil Temperature 4",
         20,    1,
         90,    1,
        255,
         (15 * 8) + 6
    },
    {
        "Low Leaf Temperature 1",
         21,    1,
         90,    1,
        255,
         (12 * 8) + 4
    },
    {
        "Low Leaf Temperature 2",
         22,    1,
         90,    1,
        255,
         (13 * 8) + 4
    },
    {
        "Low Leaf Temperature 3",
         23,    1,
         90,    1,
        255,
         (14 * 8) + 4
    },
    {
        "Low Leaf Temperature 4",
         24,   1,
         90,   1,
        255,
         (15 * 8) + 4
    },
    {
        "High Extra Temperature 1",
         25,    1,
         90,    1,
        255,
          (5 * 8) + 1
    },
    {
        "High Extra Temperature 2",
         26,    1,
         90,    1,
        255,
          (6 * 8) + 1
    },
    {
        "High Extra Temperature 3",
         27,    1,
         90,    1,
        255,
          (7 * 8) + 1
    },
    {
        "High Extra Temperature 4",
         28,    1,
         90,    1,
        255,
          (8 * 8) + 1
    },
    {
        "High Extra Temperature 5",
         29,    1,
         90,    1,
        255,
          (9 * 8) + 1
    },
    {
        "High Extra Temperature 6",
         30,    1,
         90,    1,
        255,
         (10 * 8) + 1
    },
    {
        "High Extra Temperature 7",
         31,    1,
         90,    1,
        255,
         (11 * 8) + 1
    },
    {
        "High Soil Temperature 1",
         32,    1,
         90,    1,
        255,
         (12 * 8) + 7
    },
    {
        "High Soil Temperature 2",
         33,    1,
         90,    1,
        255,
         (13 * 8) + 7
    },
    {
        "High Soil Temperature 3",
         34,    1,
         90,    1,
        255,
         (14 * 8) + 7
    },
    {
        "High Soil Temperature 4",
         35,    1,
         90,    1,
        255,
         (15 * 8) + 7
    },
    {
        "High Leaf Temperature 1",
         36,    1,
         90,    1,
        255,
         (12 * 8) + 5
    },
    {
        "High Leaf Temperature 2",
         37,    1,
         90,    1,
        255,
         (13 * 8) + 5
    },
    {
        "High Leaf Temperature 3",
         38,    1,
         90,    1,
        255,
         (14 * 8) + 5
    },
    {
        "High Leaf Temperature 4",
         39,   1,
         90,   1,
        255,
         (15 * 8) + 5
    },
    {
        "Low Outside Humidity",
         42,   1,
          0,   1,
        255,
          (4 * 8) + 2
    },
    {
        "Low Extra Humidity 1",
         43,   1,
          0,   1,
        255,
          (5 * 8) + 2
    },
    {
        "Low Extra Humidity 2",
         44,   1,
          0,   1,
        255,
          (6 * 8) + 2
    },
    {
        "Low Extra Humidity 3",
         45,   1,
          0,   1,
        255,
          (7 * 8) + 2
    },
    {
        "Low Extra Humidity 4",
         46,   1,
          0,   1,
        255,
          (8 * 8) + 2
    },
    {
        "Low Extra Humidity 5",
         47,   1,
          0,   1,
        255,
          (9 * 8) + 2
    },
    {
        "Low Extra Humidity 6",
         48,   1,
          0,   1,
        255,
          (0 * 8) + 2
    },
    {
        "Low Extra Humidity 7",
         49,   1,
          0,   1,
        255,
          (1 * 8) + 2
    },
    {
        "High Outside Humidity",
         50,   1,
          0,   1,
        255,
          (4 * 8) + 3
    },
    {
        "High Extra Humidity 1",
         51,   1,
          0,   1,
        255,
          (5 * 8) + 3
    },
    {
        "High Extra Humidity 2",
         52,   1,
          0,   1,
        255,
          (6 * 8) + 3
    },
    {
        "High Extra Humidity 3",
         43,   1,
          0,   1,
        255,
          (7 * 8) + 3
    },
    {
        "High Extra Humidity 4",
         54,   1,
          0,   1,
        255,
          (8 * 8) + 3
    },
    {
        "High Extra Humidity 5",
         55,   1,
          0,   1,
        255,
          (9 * 8) + 3
    },
    {
        "High Extra Humidity 6",
         56,   1,
          0,   1,
        255,
         (10 * 8) + 3
    },
    {
        "High Extra Humidity 7",
         57,   1,
          0,   1,
        255,
         (11 * 8) + 3
    },
    {
        "Low Dew Point",
         58,   1,
        120,   1,
        255,
          (2 * 8) + 4
    },
    {
        "High Dew Point",
         59,   1,
        120,   1,
        255,
          (2 * 8) + 5
    },
    {
        "Low Wind Chill",
         60,   1,
        120,   1,
        255,
          (2 * 8) + 7
    },
    {
        "High Heat Index",
         61,   1,
         90,   1,
        255,
          (2 * 8) + 6
    },
    {
        "High THSW",
         62,   1,
         90,   1,
        255,
          (3 * 8) + 0
    },
    {
        "Wind Speed",
         63,   1,
          0,   1,
        255,
          (2 * 8) + 2
    },
    {
        "10 Minute Average Wind Speed",
         64,   1,
          0,   1,
        255,
          (2 * 8) + 3
    },
    {
        "High UV",
         65,   1,
          0,  10,
        255,
          (3 * 8) + 2
    },
    {
        "UNAVAILABLE",
         66,   1,
          0,   1,
          0,
         -1
    },
    {
        "Low Soil Moisture 1",
         67,   1,
          0,   1,
        255,
         (12 * 8) + 2
    },
    {
        "Low Soil Moisture 2",
         68,   1,
          0,   1,
        255,
         (13 * 8) + 2
    },
    {
        "Low Soil Moisture 3",
         69,   1,
          0,   1,
        255,
         (14 * 8) + 2
    },
    {
        "Low Soil Moisture 4",
         70,   1,
          0,   1,
        255,
         (15 * 8) + 2
    },
    {
        "High Soil Moisture 1",
         71,   1,
          0,   1,
        255,
         (12 * 8) + 3
    },
    {
        "High Soil Moisture 2",
         72,   1,
          0,   1,
        255,
         (13 * 8) + 3
    },
    {
        "High Soil Moisture 3",
         73,   1,
          0,   1,
        255,
         (14 * 8) + 3
    },
    {
        "High Soil Moisture 4",
         74,   1,
          0,   1,
        255,
         (15 * 8) + 3
    },
    {
        "Low Leaf Wetness 1",
         75,   1,
          0,   1,
        255,
         (12 * 8) + 0
    },
    {
        "Low Leaf Wetness 2",
         76,   1,
          0,   1,
        255,
         (13 * 8) + 0
    },
    {
        "Low Leaf Wetness 3",
         77,   1,
          0,   1,
        255,
         (14 * 8) + 0
    },
    {
        "Low Leaf Wetness 4",
         78,   1,
          0,   1,
        255,
         (15 * 8) + 0
    },
    {
        "High Leaf Wetness 1",
         79,   1,
          0,   1,
        255,
         (12 * 8) + 1
    },
    {
        "High Leaf Wetness 2",
         80,   1,
          0,   1,
        255,
         (13 * 8) + 1
    },
    {
        "High Leaf Wetness 3",
         81,   1,
          0,   1,
        255,
         (14 * 8) + 1
    },
    {
        "High Leaf Wetness 4",
         82,   1,
          0,   1,
        255,
         (15 * 8) + 1
    },
    {
        "High Solar Radiation",
         83,   2,
          0,   1,
      32767,
          (3 * 8) + 1
    },
    { // TBD, rate alarm need rain collector size
        "High Rain Rate",
         85,   2,
          0,   1,
         -1,
          (1 * 8) + 0
    },
    { // TBD, rate alarm need rain collector size
        "15 Minute Rain",
         87,   2,
          0,   1,
         -1,
          (1 * 8) + 1
    },
    { // TBD, rate alarm need rain collector size
        "24 Hour Rain",
         89,   2,
          0,   1,
         -1,
          (1 * 8) + 1
    },
    { // TBD, rate alarm need rain collector size
        "Storm Total Rain",
         91,   2,
          0,   1,
         -1,
          (1 * 8) + 3
    },
    { 
        "Daily ET",
         93,   1,
          0,1000,
        255,
          (1 * 8) + 4
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Alarm::Alarm(const AlarmProperties & properties) : properties(properties),
                                                   eepromThreshold(properties.eepromNotSetThreshold),
                                                   actualThreshold(0.0),
                                                   alarmThresholdSet(false),
                                                   alarmTriggered(false),
                                                   logger(VantageLogger::getLogger("Alarm")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setThreshold(int eepromThreshold) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "########## Setting threshold for alarm " + properties.alarmName + " Not set value = " << properties.eepromNotSetThreshold;
    this->eepromThreshold = eepromThreshold;
    if (this->eepromThreshold == properties.eepromNotSetThreshold) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Clearing threshold for alarm " + properties.alarmName;
        alarmThresholdSet = false;
        alarmTriggered = false;
        actualThreshold = 0.0;
    }
    else {
        alarmThresholdSet = true;
        alarmTriggered = false;
        actualThreshold = static_cast<float>(eepromThreshold - properties.eepromThresholdOffset) / properties.eepromThresholdScale;
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Setting threshold for alarm " << properties.alarmName << " to " << actualThreshold;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
float
fromEepromToActualThreshold(int eepromValue, int offset, float scale) {
    float actualThreshold = static_cast<float>(eepromValue - offset) / scale;
    return actualThreshold;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
fromActualToEepromThreshold(float actualValue, int offset, int scale) {
    int eepromThreshold = static_cast<int>(actualValue * scale) + offset;
    return eepromThreshold;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double
Alarm::getThreshold() const {
    return actualThreshold;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
Alarm::isThresholdSet() const {
    return alarmThresholdSet;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setTriggered(bool triggered) {
    alarmTriggered = triggered;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
Alarm::isTriggered() const {
    return alarmTriggered;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AlarmProperties
Alarm::getAlarmProperties() const {
    return properties;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
Alarm::getAlarmName() const {
    return properties.alarmName;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AlarmManager::AlarmManager(VantageWeatherStation & station) : station(station), logger(VantageLogger::getLogger("AlarmManager")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::initialize() {
    for (int i = 0; i < sizeof(alarmProperties) / sizeof(alarmProperties[0]); i++) {
        Alarm alarm(alarmProperties[i]);
        alarms.push_back(alarm);
    }

    return loadThresholds();
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
    // LOOP2 packet is of no concern to this class
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
AlarmManager::formatAlarmThresholdsJSON() {
    loadThresholds();
    ostringstream oss;

            //<< "\"minValue\" : " << alarm.getAlarmProperties().

    oss << "{ \"alarmThresholds\" : [ ";
    bool first = true;
    for (auto alarm : alarms) {
        if (!first) oss << ", ";
        oss << "{ \"alarmName\" : \"" << alarm.getAlarmName() << "\", "
            << "\"isThresholdSet\" : " << std::boolalpha << alarm.isThresholdSet();

        if (alarm.isThresholdSet())
            oss << ", \"threshold\" : " << alarm.getThreshold();

        oss << " }";
        first = false;
    }

    oss << " ] }";
    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
AlarmManager::formatActiveAlarmsJSON() {
    ostringstream oss;
    oss << "{ \"activeAlarms\" : [ ";
    bool first = true;
    for (auto alarm : alarms) {
        if (alarm.isTriggered()) {
            if (first) first = false; else oss << ", ";
            oss << alarm.getAlarmName();
        }
    }

    oss << " ] }";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::loadThresholds() {
    byte buffer[EE_ALARM_THRESHOLDS_SIZE];

    if (!station.eepromBinaryRead(EE_ALARM_THRESHOLDS_ADDRESS, EE_ALARM_THRESHOLDS_SIZE, buffer)) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed reading alarm threshold data from EEPROM" << endl;
        return false;
    }

    for (vector<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        AlarmProperties props = it->getAlarmProperties();
        int offset = props.eepromThresholdByte;
        int thresholdValue = 0;

        if (props.eepromThresholdSize == 1) 
            thresholdValue = BitConverter::toUint8(buffer, offset);
        else
            thresholdValue = BitConverter::toInt16(buffer, offset);

        it->setThreshold(thresholdValue);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::setAlarmStates(const LoopPacket::AlarmBitSet & alarmBits) {
    for (vector<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        AlarmProperties props = it->getAlarmProperties();
        if (props.alarmBit >= 0)
            it->setTriggered(alarmBits[props.alarmBit] == 1);
    }
}

}
