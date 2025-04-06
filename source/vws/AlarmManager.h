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
#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include "VantageWeatherStation.h"
#include "LoopPacket.h"
#include "Alarm.h"
#include "ConsoleConnectionMonitor.h"
#include "RainCollectorSizeListener.h"
#include "LoopPacketListener.h"
#include "CurrentWeather.h"

namespace vws {
class VantageLogger;

/**
 * Class to manage all of the alarms of the console.
 */
class AlarmManager : public LoopPacketListener, public ConsoleConnectionMonitor, public RainCollectorSizeListener {
public:
    static const std::string ALARM_FILENAME;
    static constexpr int NUM_ALARMS = 86;
    typedef std::pair<std::string,double> Threshold;

    /**
     * Constructor.
     *
     * @param station The weather station object used to read and write from the EEPROM
     */
    AlarmManager(const std::string & logDirectory, VantageWeatherStation & station);

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
     * Called when the rain collector size changes or at startup.
     *
     * @param bucketSize The size of the rain collector bucket
     */
    virtual void processRainCollectorSizeChange(Rainfall bucketSize);

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
    std::string formatActiveAlarmsJSON() const;

    /**
     * Set the threshold for the give alarm.
     *
     * @param alarmName       The alarm for which to set the threshold
     * @param actualThreshold The actual threshold value to set alarm's threshold
     * @return True if the alarm name exists
     */
    bool setAlarmThreshold(const std::string & alarmName, double actualThreshold);

    /**
     * Set the thresholds for a number of alarms, first clearing all alarm thresholds.
     *
     * @param threshold The list of thresholds to set
     * @return True if successful
     */
    bool setAlarmThresholds(const std::vector<Threshold> & thresholds);

    /**
     * Clear the threshold for the give alarm.
     *
     * @param alarmName The alarm for which to clear the threshold
     * @return True if the alarm name exists
     */
    bool clearAlarmThreshold(const std::string & alarmName);

    /**
     * Called when a connection is established with the console.
     */
    virtual void consoleConnected();

    /**
     * Called when the connection with the console is lost.
     */
    virtual void consoleDisconnected();

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
     * Clear all the thresholds to disabled values.
     */
    void clearAllThresholds();

    /**
     * Set the alarm bits as provided by the LOOP packet.
     */
    void setAlarmStates();

    void loadAlarmStatesFromFile();

    void writeAlarmTransition(const Alarm & alarm, const DateTimeFields & transitionTime);

    bool findWeatherValue(const std::string & field, double & value);

    std::vector<Alarm>      alarms;
    VantageWeatherStation & station;
    Rainfall                rainCollectorSize;
    std::string             alarmLogFile;
    CurrentWeather          currentWeather;
    VantageLogger &         logger;
};

}
#endif
