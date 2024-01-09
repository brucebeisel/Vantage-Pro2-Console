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
#include "AlarmProperties.h"
#include "BitConverter.h"
#include "VantageEepromConstants.h"
#include "VantageLogger.h"
#include "VantageWeatherStation.h"

using namespace std;

namespace vws {
using namespace VantageEepromConstants;

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
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "########## Setting threshold for alarm " + properties.alarmName + " Not set value = " << properties.eepromNotSetThreshold << endl;
    this->eepromThreshold = eepromThreshold;
    if (this->eepromThreshold == properties.eepromNotSetThreshold) {
        clearThreshold();
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Setting threshold for alarm " << properties.alarmName << " to " << actualThreshold << endl;
        alarmThresholdSet = true;
        alarmTriggered = false;
        actualThreshold = fromEepromToActualThreshold(eepromThreshold, properties.eepromThresholdOffset, properties.eepromThresholdScale);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::setThreshold(double threshold) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Setting threshold for alarm " << properties.alarmName << " to " << threshold << endl;
    actualThreshold = threshold;
    eepromThreshold = fromActualToEepromThreshold(actualThreshold, properties.eepromThresholdOffset, properties.eepromThresholdScale);
    alarmThresholdSet = true;
    alarmTriggered = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
Alarm::clearThreshold() {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "######### Clearing threshold for alarm " + properties.alarmName << endl;
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
std::string
Alarm::getAlarmCurrentWeatherFieldName() const {
    return properties.currentWeatherField;
}

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

}
