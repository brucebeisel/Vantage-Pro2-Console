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

#include "AlarmManager.h"

#include <sstream>
#include "VantageEepromConstants.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

using namespace VantageEepromConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AlarmManager::AlarmManager(VantageWeatherStation & station) : station(station), logger(VantageLogger::getLogger("AlarmManager")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::initialize() {
    int numProperties;
    const AlarmProperties * alarmProperties = AlarmProperties::getAlarmProperties(numProperties);
    for (int i = 0; i < numProperties; i++) {
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
        if (alarm.getAlarmProperties().fieldValid) {
            if (!first) oss << ", "; else first = false;
            double minValue = alarm.calculateActualValue(alarm.getAlarmProperties().minimumValue);
            double maxValue = alarm.calculateActualValue(alarm.getAlarmProperties().maximumValue);

            oss << "{ \"name\" : \"" << alarm.getAlarmName() << "\", "
                << "\"field\" : \"" << alarm.getAlarmCurrentWeatherFieldName() << "\", "
                << "\"min\" : " << minValue << ", \"max\" : " << maxValue << ", "
                << "\"set\" : " << std::boolalpha << alarm.isThresholdSet();

            if (alarm.isThresholdSet())
                oss << ", \"threshold\" : " << alarm.getActualThreshold();

            oss << " }";
        }
    }

    oss << " ] }";
    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
AlarmManager::formatActiveAlarmsJSON() const {
    ostringstream oss;
    oss << "{ \"activeAlarms\" : [ ";
    bool first = true;
    for (auto alarm : alarms) {
        if (alarm.isTriggered()) {
            if (first) first = false; else oss << ", ";
            oss << "{ "
                << "\"name\" : \"" << alarm.getAlarmName() << "\", "
                << "\"field\" : \"" << alarm.getAlarmCurrentWeatherFieldName() << "\""
                << " }";
        }
    }

    oss << " ] }";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::setAlarmThreshold(const std::string & alarmName, double actualThreshold) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Setting threshold for alarm " << alarmName << " to " << actualThreshold << endl;
    for (auto & alarm : alarms) {
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
AlarmManager::setAlarmThresholds(const vector<Threshold> & thresholds) {
    clearAllThresholds();

    for (auto & threshold : thresholds) {
        if (!setAlarmThreshold(threshold.first, threshold.second)) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to set alarm threshold for alarm '" << threshold.first << "'" << endl;
            return false;
        }
    }

    return updateThresholds();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::clearAlarmThreshold(const std::string & alarmName) {
    for (auto & alarm : alarms) {
        if (alarm.getAlarmName() == alarmName) {
            alarm.clearThreshold();
            return true;
        }
    }
    return false;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::clearAllThresholds() {
    for (auto & alarm : alarms) {
        alarm.clearThreshold();
    }
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
        const AlarmProperties props = it->getAlarmProperties();
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
