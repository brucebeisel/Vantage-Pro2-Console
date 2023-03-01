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
         0,                       // Triggered bit within LOOP packet alarms
         1, 255                   // Minimum/Maximum values of the alarm threshold
    },
    {
        "Barometer Rising",
         0,    1,
         0, 1000,
         0,
         1,
         1, 255
    },
    {
        "Low Inside Temperature",
          6,    1,
         90,    1,
        255,
          2,
          0, 254
    },
    {
        "High Inside Temperature",
          7,    1,
         90,    1,
        255,
          3,
          0, 254
    },
    {
        "Low Inside Humidity",
         40,   1,
          0,   1,
        255,
          4,
          0, 100
    },
    {
        "High Inside Humidity",
         41,   1,
          0,   1,
        255,
          5,
          0, 100
    },
    {
        "Time Alarm",
         2,     2,
         0,     1,
        -1,
         6,
         0,   255
    },
    {
        "Time Alarm 2s-Compliment",
         4,     2,
         0,     1,
         0,
        -1,
         0,   255
    },
    {
        "Low Outside Temperature",
          8,    1,
         90,    1,
        255,
         16,
          0,  254
    },
    {
        "High Outside Temperature",
          9,   1,
         90,   1,
        255,
         17,
          0, 254
    },
    {
        "Low Extra Temperature 1",
         10,    1,
         90,    1,
        255,
         40,
          0, 254
    },
    {
        "Low Extra Temperature 2",
         11,    1,
         90,    1,
        255,
         48,
          0, 254
    },
    {
        "Low Extra Temperature 3",
         12,    1,
         90,    1,
        255,
         56,
          0, 254
    },
    {
        "Low Extra Temperature 4",
         13,    1,
         90,    1,
        255,
         64,
          0, 254
    },
    {
        "Low Extra Temperature 5",
         14,    1,
         90,    1,
        255,
         72,
          0, 254
    },
    {
        "Low Extra Temperature 6",
         15,    1,
         90,    1,
        255,
         80,
          0, 254
    },
    {
        "Low Extra Temperature 7",
         16,    1,
         90,    1,
        255,
         88,
          0, 254
    },
    {
        "Low Soil Temperature 1",
         17,    1,
         90,    1,
        255,
        102,
          0, 254
    },
    {
        "Low Soil Temperature 2",
         18,    1,
         90,    1,
        255,
        (13 * 8) + 6,
          0, 254
    },
    {
        "Low Soil Temperature 3",
         19,    1,
         90,    1,
        255,
         (14 * 8) + 6,
          0, 254
    },
    {
        "Low Soil Temperature 4",
         20,    1,
         90,    1,
        255,
         (15 * 8) + 6,
          0, 254
    },
    {
        "Low Leaf Temperature 1",
         21,    1,
         90,    1,
        255,
         (12 * 8) + 4,
         0,   254
    },
    {
        "Low Leaf Temperature 2",
         22,    1,
         90,    1,
        255,
         (13 * 8) + 4,
          0,  254
    },
    {
        "Low Leaf Temperature 3",
         23,    1,
         90,    1,
        255,
         (14 * 8) + 4,
          0,  254
    },
    {
        "Low Leaf Temperature 4",
         24,   1,
         90,   1,
        255,
         (15 * 8) + 4,
          0,  254
    },
    {
        "High Extra Temperature 1",
         25,    1,
         90,    1,
        255,
          (5 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 2",
         26,    1,
         90,    1,
        255,
          (6 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 3",
         27,    1,
         90,    1,
        255,
          (7 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 4",
         28,    1,
         90,    1,
        255,
          (8 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 5",
         29,    1,
         90,    1,
        255,
          (9 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 6",
         30,    1,
         90,    1,
        255,
         (10 * 8) + 1,
          0,  254
    },
    {
        "High Extra Temperature 7",
         31,    1,
         90,    1,
        255,
         (11 * 8) + 1,
          0,  254
    },
    {
        "High Soil Temperature 1",
         32,    1,
         90,    1,
        255,
         (12 * 8) + 7,
          0,  254
    },
    {
        "High Soil Temperature 2",
         33,    1,
         90,    1,
        255,
         (13 * 8) + 7,
          0,  254
    },
    {
        "High Soil Temperature 3",
         34,    1,
         90,    1,
        255,
         (14 * 8) + 7,
          0,  254
    },
    {
        "High Soil Temperature 4",
         35,    1,
         90,    1,
        255,
         (15 * 8) + 7,
          0,  254
    },
    {
        "High Leaf Temperature 1",
         36,    1,
         90,    1,
        255,
         (12 * 8) + 5,
          0,  254
    },
    {
        "High Leaf Temperature 2",
         37,    1,
         90,    1,
        255,
         (13 * 8) + 5,
          0,  254
    },
    {
        "High Leaf Temperature 3",
         38,    1,
         90,    1,
        255,
         (14 * 8) + 5,
          0,  254
    },
    {
        "High Leaf Temperature 4",
         39,   1,
         90,   1,
        255,
         (15 * 8) + 5,
          0,  254
    },
    {
        "Low Outside Humidity",
         42,   1,
          0,   1,
        255,
          (4 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 1",
         43,   1,
          0,   1,
        255,
          (5 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 2",
         44,   1,
          0,   1,
        255,
          (6 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 3",
         45,   1,
          0,   1,
        255,
          (7 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 4",
         46,   1,
          0,   1,
        255,
          (8 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 5",
         47,   1,
          0,   1,
        255,
          (9 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 6",
         48,   1,
          0,   1,
        255,
          (0 * 8) + 2,
          0,  100
    },
    {
        "Low Extra Humidity 7",
         49,   1,
          0,   1,
        255,
          (1 * 8) + 2,
          0,  100
    },
    {
        "High Outside Humidity",
         50,   1,
          0,   1,
        255,
          (4 * 8) + 3,
          0,  100
    },
    {
        "High Extra Humidity 1",
         51,   1,
          0,   1,
        255,
          (5 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 2",
         52,   1,
          0,   1,
        255,
          (6 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 3",
         43,   1,
          0,   1,
        255,
          (7 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 4",
         54,   1,
          0,   1,
        255,
          (8 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 5",
         55,   1,
          0,   1,
        255,
          (9 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 6",
         56,   1,
          0,   1,
        255,
         (10 * 8) + 3,
          0, 100
    },
    {
        "High Extra Humidity 7",
         57,   1,
          0,   1,
        255,
         (11 * 8) + 3,
          0, 100
    },
    {
        "Low Dew Point",
         58,   1,
        120,   1,
        255,
          (2 * 8) + 4,
          0, 100
    },
    {
        "High Dew Point",
         59,   1,
        120,   1,
        255,
          (2 * 8) + 5,
          0, 100
    },
    {
        "Low Wind Chill",
         60,   1,
        120,   1,
        255,
          (2 * 8) + 7,
          0, 100
    },
    {
        "High Heat Index",
         61,   1,
         90,   1,
        255,
          (2 * 8) + 6,
          0, 254
    },
    {
        "High THSW",
         62,   1,
         90,   1,
        255,
          (3 * 8) + 0,
          0, 254
    },
    {
        "Wind Speed",
         63,   1,
          0,   1,
        255,
          (2 * 8) + 2,
          0, 254
    },
    {
        "10 Minute Average Wind Speed",
         64,   1,
          0,   1,
        255,
          (2 * 8) + 3,
          0, 254
    },
    {
        "High UV",
         65,   1,
          0,  10,
        255,
          (3 * 8) + 2,
          0, 254
    },
    {
        "UNAVAILABLE",
         66,   1,
          0,   1,
          0,
         -1,
          0,   0
    },
    {
        "Low Soil Moisture 1",
         67,   1,
          0,   1,
        255,
         (12 * 8) + 2,
          0, 254
    },
    {
        "Low Soil Moisture 2",
         68,   1,
          0,   1,
        255,
         (13 * 8) + 2,
         0,  254
    },
    {
        "Low Soil Moisture 3",
         69,   1,
          0,   1,
        255,
         (14 * 8) + 2,
         0,  254
    },
    {
        "Low Soil Moisture 4",
         70,   1,
          0,   1,
        255,
         (15 * 8) + 2,
         0,  254
    },
    {
        "High Soil Moisture 1",
         71,   1,
          0,   1,
        255,
         (12 * 8) + 3,
         0,  254
    },
    {
        "High Soil Moisture 2",
         72,   1,
          0,   1,
        255,
         (13 * 8) + 3,
          0, 254
    },
    {
        "High Soil Moisture 3",
         73,   1,
          0,   1,
        255,
         (14 * 8) + 3,
          0, 254
    },
    {
        "High Soil Moisture 4",
         74,   1,
          0,   1,
        255,
         (15 * 8) + 3,
          0, 254
    },
    {
        "Low Leaf Wetness 1",
         75,   1,
          0,   1,
        255,
         (12 * 8) + 0,
          0,  15
    },
    {
        "Low Leaf Wetness 2",
         76,   1,
          0,   1,
        255,
         (13 * 8) + 0,
          0,  15
    },
    {
        "Low Leaf Wetness 3",
         77,   1,
          0,   1,
        255,
         (14 * 8) + 0,
          0,  15
    },
    {
        "Low Leaf Wetness 4",
         78,   1,
          0,   1,
        255,
         (15 * 8) + 0,
          0,  15
    },
    {
        "High Leaf Wetness 1",
         79,   1,
          0,   1,
        255,
         (12 * 8) + 1,
          0,  15
    },
    {
        "High Leaf Wetness 2",
         80,   1,
          0,   1,
        255,
         (13 * 8) + 1,
          0,  15
    },
    {
        "High Leaf Wetness 3",
         81,   1,
          0,   1,
        255,
         (14 * 8) + 1,
          0,  15
    },
    {
        "High Leaf Wetness 4",
         82,   1,
          0,   1,
        255,
         (15 * 8) + 1,
          0,  15
    },
    {
        "High Solar Radiation",
         83,   2,
          0,   1,
      32767,
          (3 * 8) + 1,
          1, 1800
    },
    { // TBD, rate alarm need rain collector size
        "High Rain Rate",
         85,   2,
          0,   1,
      65535,
          (1 * 8) + 0
    },
    { // TBD, rate alarm need rain collector size
        "15 Minute Rain",
         87,   2,
          0,   1,
      65535,
          (1 * 8) + 1,
          1,  10000,
    },
    { // TBD, rate alarm need rain collector size
        "24 Hour Rain",
         89,   2,
          0,   1,
      65535,
          (1 * 8) + 1,
          1, 10000
    },
    { // TBD, rate alarm need rain collector size
        "Storm Total Rain",
         91,   2,
          0,   1,
      65535,
          (1 * 8) + 3,
          1, 10000
    },
    { 
        "Daily ET",
         93,   1,
          0,1000,
        255,
          (1 * 8) + 4,
          1, 254
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double
Alarm::fromEepromToActualThreshold(int eepromValue, int offset, double scale) {
    double actualThreshold = static_cast<double>(eepromValue - offset) / scale;
    return actualThreshold;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Alarm::fromActualToEepromThreshold(double actualValue, int offset, int scale) {
    int eepromThreshold = static_cast<int>(actualValue * scale) + offset;
    return eepromThreshold;
}

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
        clearThreshold();
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Setting threshold for alarm " << properties.alarmName << " to " << actualThreshold;
        alarmThresholdSet = true;
        alarmTriggered = false;
        actualThreshold = fromEepromToActualThreshold(eepromThreshold, properties.eepromThresholdOffset, properties.eepromThresholdScale);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setThreshold(double actualThreshold) {
    this->actualThreshold = actualThreshold;
    int eepromThreshold = fromActualToEepromThreshold(actualThreshold, properties.eepromThresholdOffset, properties.eepromThresholdScale);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::clearThreshold() {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Clearing threshold for alarm " + properties.alarmName;
    eepromThreshold = properties.eepromNotSetThreshold;
    alarmThresholdSet = false;
    alarmTriggered = false;
    actualThreshold = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double
Alarm::getActualThreshold() const {
    return actualThreshold;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
Alarm::getEepromThreshold() const {
    return eepromThreshold;
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
int
Alarm::calculateEepromValue(double actualValue) const {
    return Alarm::fromActualToEepromThreshold(actualValue, properties.eepromThresholdOffset, properties.eepromThresholdScale);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double
Alarm::calculateActualValue(int eepromValue) const {
    return Alarm::fromEepromToActualThreshold(eepromValue, properties.eepromThresholdOffset, properties.eepromThresholdScale);
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

    return retrieveThresholds();
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
    retrieveThresholds();
    ostringstream oss;

    oss << "{ \"alarmThresholds\" : [ ";
    bool first = true;
    for (auto alarm : alarms) {
        if (!first) oss << ", "; else first = false;
        double minValue = alarm.calculateActualValue(alarm.getAlarmProperties().minimumValue);
        double maxValue = alarm.calculateActualValue(alarm.getAlarmProperties().maximumValue);

        oss << "{ \"alarmName\" : \"" << alarm.getAlarmName() << "\", "
            << "\"minValue\" : " << minValue << ", \"maxValue\" : " << maxValue << ", "
            << "\"isThresholdSet\" : " << std::boolalpha << alarm.isThresholdSet();

        if (alarm.isThresholdSet())
            oss << ", \"threshold\" : " << alarm.getActualThreshold();

        oss << " }";
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
AlarmManager::setAlarmThreshold(const std::string & alarmName, double actualThreshold) {
    for (auto alarm : alarms) {
        if (alarm.getAlarmName() == alarmName) {
            alarm.setThreshold(actualThreshold);
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::clearAlarmThreshold(const std::string & alarmName) {
    for (auto alarm : alarms) {
        if (alarm.getAlarmName() == alarmName) {
            alarm.clearThreshold();
            return true;
        }
    }
    return false;

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::retrieveThresholds() {
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
            thresholdValue = BitConverter::toUint16(buffer, offset);

        it->setThreshold(thresholdValue);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::updateThresholds() {
    byte buffer[EE_ALARM_THRESHOLDS_SIZE];

    for (vector<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        AlarmProperties props = it->getAlarmProperties();
        BitConverter::getBytes(it->getEepromThreshold(), buffer, props.eepromThresholdByte, props.eepromThresholdSize);
    }

    if (!station.eepromBinaryWrite(EE_ALARM_THRESHOLDS_ADDRESS, buffer, EE_ALARM_THRESHOLDS_SIZE)) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to write alarm threshold data to EEPROM" << endl;
        return false;
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
