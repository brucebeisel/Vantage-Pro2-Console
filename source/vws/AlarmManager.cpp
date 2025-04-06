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

#include "AlarmManager.h"

#include <sstream>
#include <fstream>
#include "VantageEepromConstants.h"
#include "VantageLogger.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

namespace vws {

using namespace EepromConstants;

const string AlarmManager::ALARM_FILENAME = "vws-alarms.log";
const string AlarmManager::ALARM_ACTIVE_STRING = "ACTIVE";
const string AlarmManager::ALARM_CLEAR_STRING = "CLEAR";

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
AlarmManager::AlarmManager(const string & logDirectory, VantageWeatherStation & station) : station(station),
                                                                                           alarmLogFile(logDirectory + "/" + ALARM_FILENAME),
                                                                                           rainCollectorSize(0.0),
                                                                                           logger(VantageLogger::getLogger("AlarmManager")) {
    int numProperties;
    const AlarmProperties * alarmProperties = AlarmProperties::getAlarmProperties(numProperties);
    for (int i = 0; i < numProperties; i++) {
        Alarm alarm(alarmProperties[i]);
        alarms.push_back(alarm);
    }

    loadAlarmStatesFromFile();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::processLoopPacket(const LoopPacket & packet) {
    currentWeather.setLoopData(packet);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::processLoop2Packet(const Loop2Packet & packet) {
    //
    // Process the alarm bits after a LOOP2 packet is received.
    //
    currentWeather.setLoop2Data(packet);
    setAlarmStates();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::processRainCollectorSizeChange(Rainfall bucketSize) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Received new rain bucket size of " << bucketSize << " inches" << endl;
    rainCollectorSize = bucketSize;

    for (auto & alarm : alarms) {
        alarm.setRainAlarmScale(bucketSize);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::consoleConnected() {
    retrieveThresholds();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::consoleDisconnected() {
    clearAllThresholds();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
AlarmManager::formatAlarmThresholdsJSON() {
    retrieveThresholds();
    ostringstream oss;

    oss << "{ \"alarmThresholds\" : [ ";
    bool first = true;
    for (const auto & alarm : alarms) {
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
    for (const auto & alarm : alarms) {
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
bool
AlarmManager::setAlarmState(const std::string & alarmName, bool triggered) {
    for (auto & alarm : alarms) {
        if (alarm.getAlarmName() == alarmName) {
            alarm.setTriggered(triggered);
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

    for (auto & alarm : alarms) {
        AlarmProperties props = alarm.getAlarmProperties();
        int offset = props.eepromThresholdByte;
        int thresholdValue = 0;

        if (props.eepromThresholdSize == 1)
            thresholdValue = BitConverter::toUint8(buffer, offset);
        else
            thresholdValue = BitConverter::toUint16(buffer, offset);

        alarm.setThreshold(thresholdValue);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::updateThresholds() {
    byte buffer[EE_ALARM_THRESHOLDS_SIZE];

    for (const auto & alarm : alarms) {
        const AlarmProperties props = alarm.getAlarmProperties();
        BitConverter::getBytes(alarm.getEepromThreshold(), buffer, props.eepromThresholdByte, props.eepromThresholdSize);
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
AlarmManager::setAlarmStates() {
    const LoopPacket::AlarmBitSet & alarmBits = currentWeather.getLoopPacket().getAlarmBits();
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Setting alarm states. Bitset=" << alarmBits << endl;

    bool firstTransition = true;
    json cwJsonObject;
    DateTimeFields now(time(0));
    for (auto & alarm : alarms) {
        bool currentState = alarm.isTriggered();
        AlarmProperties props = alarm.getAlarmProperties();
        if (props.alarmBit >= 0) {
            bool newState = alarmBits[props.alarmBit] == 1;
            alarm.setTriggered(newState);
             if (currentState != newState) {
                 //
                 // The first time we encounter a change to an alarm state build the current weather
                 // JSON string so the current weather value can be retrieved. If there are no transitions
                 // then we don't take up any cycles performing JSON processing.
                 //
                 if (firstTransition) {
                     string cwJsonString = cwJsonString = currentWeather.formatJSON();
                     cwJsonObject = json::parse(cwJsonString.begin(), cwJsonString.end());
                     firstTransition = false;
                 }

                 writeAlarmTransition(cwJsonObject, alarm, now);
             }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::loadAlarmStatesFromFile() {
    ifstream ifs(alarmLogFile);
    if (!ifs.is_open()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to open alarm log file '" << alarmLogFile << "' for reading" << endl;
        return;
    }

    //
    // TODO figure out a way to not go through the entire file to determine which alarms were active when the program was
    // stopped
    //
    string line;
    char date[100];
    char time[100];
    char state[100];
    char alarmName[100];
    char threshold[100];
    char valueAtTransition[100];
    while (getline(ifs, line)) {
        int tokens = sscanf(line.c_str(), "%s %s %s \"%[^\"]\" %s %s", date, time, state, alarmName, threshold, valueAtTransition);
        if (tokens != 6) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Skipping alarm log line due to missing tokens. Expected 6, got " << tokens << ": '" << line << "'" << endl;
            continue;
        }

        bool triggered = ALARM_ACTIVE_STRING == state;
        bool found = setAlarmState(alarmName, triggered);
        if (found)
            logger.log(VantageLogger::VANTAGE_DEBUG2) << "Set alarm state from log file of alarm: '" << alarmName << "' Triggered: " << boolalpha << triggered << endl;
        else
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to set alarm: '" << alarmName << "' from log file. Alarm name was not found." << endl;

    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
AlarmManager::writeAlarmTransition(const json & jsonObject, const Alarm & alarm, const DateTimeFields & transitionTime) {
    string state = ALARM_ACTIVE_STRING;

    if (!alarm.isTriggered())
        state = ALARM_CLEAR_STRING;

    ofstream ofs(alarmLogFile, ios::app);
    if (!ofs.is_open()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to open alarm log file '" << alarmLogFile << "' for writing" << endl;
        return;
    }

    double value;
    if (findWeatherValue(jsonObject, alarm.getAlarmCurrentWeatherFieldName(), value))
        ofs << transitionTime.formatDateTime(true) << " " << state << " \"" << alarm.getAlarmName() << "\" " << alarm.getActualThreshold() << " " << value << endl;
    else
        ofs << transitionTime.formatDateTime(true) << " " << state << " \"" << alarm.getAlarmName() << "\" " << alarm.getActualThreshold() << " ---" << endl;

    ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
AlarmManager::findWeatherValue(const json & jsonObject, const std::string & field, double & value) {
    try {
        double lookupValue = jsonObject.at(field);
        value = lookupValue;
        return true;
    }
    catch (const json::out_of_range & e) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to find current weather field '" << field << "'" << endl;
        return false;
    }

}
}
