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

namespace vws {
struct VantageLogger;

struct AlarmProperties {
    std::string alarmName;
    int         eepromThresholdByte;
    int         eepromThresholdSize;
    int         eepromThresholdOffset;
    int         eepromThresholdScale;
    int         eepromNotSetThreshold;
    int         alarmBit;
};

/**
 * Class to manage a single alarm monitored by the console.
 */
class Alarm {
public:
    Alarm(const AlarmProperties & properties);


    std::string getAlarmName() const;
    AlarmProperties getAlarmProperties() const;

    void setThreshold(int eepromThreshold);
    double getThreshold() const;

    bool isThresholdSet() const;

    void setTriggered(bool triggered);
    bool isTriggered() const;


private:
    AlarmProperties properties;
    int             eepromThreshold;   // The threshold as stored in the EEPROM
    double          actualThreshold;   // The actual threshold after offset and scale applied
    bool            alarmThresholdSet; // Whether the alarm threshold is set to a value other that the "not set" value
    bool            alarmTriggered;    // Whether the alarm is currently triggered
};

/**
 * Class to manage all of the alarms of the console.
 */
class AlarmManager : public VantageWeatherStation::LoopPacketListener {
public:
    static const int NUM_ALARMS = 86;
    AlarmManager(VantageWeatherStation & station);

    bool initialize();

    void getTriggeredList(std::vector<Alarm> & triggeredList) const;

    virtual bool processLoopPacket(const LoopPacket & packet);
    virtual bool processLoop2Packet(const Loop2Packet & packet);

    std::string formatAlarmThresholdsJSON();
    std::string formatActiveAlarmsJSON();

private:
    bool loadThresholds();
    void setAlarmStates(const LoopPacket::AlarmBitSet & alarmBits);

    VantageLogger &         logger;
    std::vector<Alarm>      alarms;
    VantageWeatherStation & station;
};

}

#endif
