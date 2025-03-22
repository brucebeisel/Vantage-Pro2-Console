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
#ifndef ALARM_PROPERTIES_H
#define ALARM_PROPERTIES_H
#include <string>

namespace vws {

/**
 * Properties for a single alarm threshold stored in the EEPROM.
 */
struct AlarmProperties {
    std::string alarmName;             // The name of this alarm
    std::string currentWeatherField;   // The current weather field to which this alarm applies
    int         eepromThresholdByte;   // The byte within the EEPROM block that this alarm's threshold is stored
    int         eepromThresholdSize;   // The number of bytes the threshold value uses
    int         eepromThresholdOffset; // The amount to subtract from the value in the EEPROM
    double      eepromThresholdScale;  // The amount to divide the EEPROM value by
    int         eepromNotSetThreshold; // The value that is used to indicate that the threshold is not set
    int         alarmBit;              // The bit within the LOOP packet that indicates if this alarm has been triggered
    int         minimumValue;          // The minimum value the threshold can be
    int         maximumValue;          // The maximum value the threshold can be
    bool        isRainAlarm;           // Whether this alarm is rain related and the rain bucket size is to be used as the scale
    bool        fieldValid;            // Whether this field is valid in the current weather packets

    /**
     * Get the number of alarm properties.
     *
     * @return The number of alarm property structures
     */
    static int  getAlarmPropertyCount();

    /**
     * Get the entire list of alarm properties.
     *
     * @param [O] count The number of properties in the array returned
     * @return An array of alarm properties
     */
    static const AlarmProperties * getAlarmProperties(int & count);
};
}
#endif
