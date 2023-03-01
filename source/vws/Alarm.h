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
#ifndef ALARM_H
#define ALARM_H
#include <string>
#include <vector>

#include "LoopPacket.h"
#include "VantageWeatherStation.h"
#include "AlarmProperties.h"

namespace vws {
struct VantageLogger;

/**
 * Class to manage a single alarm monitored by the console.
 * This class uses two terms to describe the trigger thresholds of the alarm.
 * The EEPROM value is the threshold that is stored in the EEPROM as an integer.
 * The ACTUAL value is the value that is used for display purposes and is the
 * value at which the alarm will actually trigger. An alarm may have an offset
 * and a scale that is used to convert between EEPROM and ACTUAL.
 */
class Alarm {
public:
    /**
     * Constructor.
     *
     * @param The properties of this alarm
     */
    Alarm(const AlarmProperties & properties);

    /**
     * Get the name of this alarm.
     *
     * @return The alarm name
     */
    std::string getAlarmName() const;

    /**
     * Get the properties of this alarm.
     *
     * @return The properties of this alarm
     */
    AlarmProperties getAlarmProperties() const;

    /**
     * Set the EEPROM version of this alarm's threshold.
     *
     * @param eepromThreshold This alarm's threshold as it is stored in the EEPROM
     */
    void setThreshold(int eepromThreshold);

    /**
     * Set the actual value of this alarm's threshold.
     *
     * @param actualThreshold This alarm's ACTUAL threshold
     */
    void setThreshold(double actualThreshold);

    /**
     * Clear the alarm's threshold so it will not trigger.
     */
    void clearThreshold();

    /**
     * Get the ACTUAL version of the threshold.
     *
     * @return The threshold
     */
    double getActualThreshold() const;

    /**
     * Get the threshold as it is stored in the EEPROM.
     *
     * @return The threshold
     */
    int getEepromThreshold() const;

    /**
     * Check if the alarm's threshold is set.
     *
     * @return The state of the threshold value
     */
    bool isThresholdSet() const;

    /**
     * Set whether this alarm has been triggered on the console.
     *
     * @param triggered True of the alarm has been triggered on the console
     */
    void setTriggered(bool triggered);

    /**
     * Return whether this alarm is currently triggered.
     *
     * @return The triggered state of this alarm
     */
    bool isTriggered() const;

    /**
     * Calculate the EEPROM threshold value from the ACTUAL value.
     *
     * @param The ACTUAL threshold value
     * @return The EEPROM threshold value
     */
    int calculateEepromValue(double actualValue) const;

    /**
     * Calculate the displayed threshold value from the EEPROM value.
     *
     * @param The EEPROM threshold value
     * @return The ACTUAL threshold value
     */
    double calculateActualValue(int eepromValue) const;


private:
    /**
     * Calculate the display threshold value from the provided properties.
     *
     * @param eeprom The EEPROM value of the threshold
     * @param offset The value to be subtracted from the EEPROM value
     * @param offset The value by which to divide the EEPROM
     */
    static double fromEepromToActualThreshold(int eepromValue, int offset, double scale);

    /**
     * Calculate the ACTUAL threshold value from the provided properties.
     *
     * @param actualValue The ACTUAL value of the threshold
     * @param offset The value to be subtracted from the ACTUAL value
     * @param offset The value by which to divide the ACTUAL
     */
    static int fromActualToEepromThreshold(double actualValue, int offset, int scale);

    AlarmProperties properties;        // The properties that describe this alarm
    int             eepromThreshold;   // The threshold as stored in the EEPROM
    double          actualThreshold;   // The actual threshold after offset and scale applied
    bool            alarmThresholdSet; // Whether the alarm threshold is set to a value other that the "not set" value
    bool            alarmTriggered;    // Whether the alarm is currently triggered
    VantageLogger & logger;
};

/**
 * Class to manage all of the alarms of the console.
 */
class AlarmManager : public VantageWeatherStation::LoopPacketListener {
public:
    static const int NUM_ALARMS = 86;

    /**
     * Constructor.
     *
     * @param station The weather station object used to read and write from the EEPROM
     */
    AlarmManager(VantageWeatherStation & station);

    /**
     * Initialize the alarm manager.
     *
     * @return True if initialization is successful
     */
    bool initialize();

    /**
     * Get the list of alarms that are currently triggered.
     *
     * @param triggeredList The container into which the triggered alarms will be copied
     */
    void getTriggeredList(std::vector<Alarm> & triggeredList) const;

    /**
     * Process a LOOP packets as part of the LoopPacketListener interface.
     *
     * @param packet The LOOP packet
     * @return True if the LOOP packet processing loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & packet);

    /**
     * Process a LOOP2 packets as part of the LoopPacketListener interface.
     *
     * @param packet The LOOP2 packet
     * @return True if the LOOP packet processing loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & packet);

    /**
     * Format the JSON message that contains all the alarms and their current thresholds.
     *
     * @return The formatted JSON message
     */
    std::string formatAlarmThresholdsJSON();

    /**
     * Format the JSON message that contains the list of triggered alarms.
     *
     * @return The formatted JSON message
     */
    std::string formatActiveAlarmsJSON();

    /**
     * Set the threshold for the give alarm.
     *
     * @param alarmName       The alarm for which to set the threshold
     * @param actualThreshold The actual threshold value to set alarm's threshold
     * @return True if the alarm name exists
     */
    bool setAlarmThreshold(const std::string & alarmName, double actualThreshold);

    /**
     * Clear the threshold for the give alarm.
     *
     * @param alarmName The alarm for which to clear the threshold
     * @return True if the alarm name exists
     */
    bool clearAlarmThreshold(const std::string & alarmName);

private:
    /**
     * Retrieve the threshold from the weather station.
     *
     * @return True if the retrieval was successful
     */
    bool retrieveThresholds();

    /**
     * Update the thresholds in the weather station's EEPROM.
     *
     * @return True of the update was successful
     */
    bool updateThresholds();

    /**
     * Set the alarm bits as provided by the LOOP packet.
     *
     * @param alarmBits The bits that indicate which alarms are triggered
     */
    void setAlarmStates(const LoopPacket::AlarmBitSet & alarmBits);

    VantageLogger &         logger;
    std::vector<Alarm>      alarms;
    VantageWeatherStation & station;
};

}

#endif
