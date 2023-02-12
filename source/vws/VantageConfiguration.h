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
#ifndef VANTAGE_CONFIGURATION_H
#define VANTAGE_CONFIGURATION_H

#include <vector>
#include <string>
#include "Sensor.h"
#include "SensorStation.h"
#include "VantageProtocolConstants.h"
#include "VantageWeatherStation.h"
#include "WeatherTypes.h"


namespace vws {

/**
 * This file will be use to evaluate the different types of data that the console contains so that
 * the proper set of classes can be created to manage the data. This data does not include any real-time
 * weather data. That is already handled by existing classes.
 *
 * Fast changing
 * ---------------------------------------------------------
 * Console battery voltage - LOOP
 * Transmitter battery status - LOOP
 * Number of wind samples - Archive
 *
 * Slow changing
 * ---------------------------------------------------------
 * Stations detected - RECEIVERS command
 *
 * Static, read-write (Data that can be changed, but does not change without user intervention and usually only changes when there are physical changes to the weather station network)
 * ---------------------------------------------------------
 * Barometric calibration data - BARDATA command
 * Archive interval - SETPER command, EEPROM to read ----- User set, infrequently changed
 * Lat/lon/elevation - EEPROM                        ----- Initialization: User set, should not change once set
 * Time management (TZ, DST, etc.) EEPROM            ----- Initialization: User set, should not change once set
 * Transmitters to which to listen - EEPROM          ----- Initialization: User set, changed when sensor stations are added or removed
 * Retransmit setting - EEPROM                       ----- Initialization: User set, only changed when secondary consoles are added or removed
 * Station list - EEPROM                             ----- Initialization: User set, changed when sensor stations are added or removed
 * Units - EEPROM                                    ----- Initialization: User set, changed if user desires
 * Setup Bits - EEPROM, contains wind cup size which is duplicated in the EEPROM data
 * Rain season start - EEPROM                        ----- Initialization: User set, should not change once set
 * Inside temperature calibration - EEPROM           ----- Change as needed
 * Outside temperature calibration - EEPROM          ----- Change as needed
 * Extra temperature calibration - EEPROM            ----- Change as needed
 * Inside humidity calibration - EEPROM              ----- Change as needed
 * Outside humidity calibration - EEPROM             ----- Change as needed
 * Extra humidity calibration - EEPROM               ----- Change as needed
 * Alarm thresholds - EEPROM                         ----- User changed as desired
 * Graph time span - EEPROM                          ----- Unknown
 * Graph data - EEPROM, Note that the graph data is different between the Pro2 and Vue
 * Archive temperature calculation type (average vs end of time period)
 *
 * Static, read-only
 * ---------------------------------------------------------
 * Firmware Date - VER command
 * Firmware Version - NVER command
 * Console Type - WRD command
 *
 */

struct TimeSettings {
    bool        useGmtOffset;
    std::string timezoneName;               // Only used if useGmtOffset is false
    int         gmtOffsetMinutes;            // Only used if useGmtOffset is true
    bool        manualDaylightSavingsTime;
    bool        manualDaylightSavingsTimeOn; // This will change twice a year, but only if manualDaylightSavingsTime is true
};

struct SetupBits {
    bool is24HourMode;
    bool isCurrentlyAM;
    bool isDayMonthDisplay;
    bool isWindCupLarge;
    ProtocolConstants::RainCupSizeType rainCollectorSizeType;
    bool isNorthLatitude;
    bool isEastLongitude;
};

struct PositionData {
    double latitude;
    double longitude;
    int    elevation;
};

struct UnitsSettings {
    BarometerUnits baroUnits;
    TemperatureUnits temperatureUnits;
    ElevationUnits elevationUnits;
    RainUnits rainUnits;
    WindUnits windUnits;
};

struct ConsoleConfigurationData {
    TimeSettings             timeSettings;
    SetupBits                setupBits;
    PositionData             positionData;
    UnitsSettings            unitsSettings;
    int                      secondaryWindCupSize;
    ProtocolConstants::Month rainSeasonStartMonth;
    StationId                retransmitId;
    bool                     logFinalTemperature;

    std::string formatJSON();
};

/**
 * This class manages the configuration settings that are stored in the EEPROM of the Vantage consoles.
 * Most of these settings are changed using the EEPROM commands, but some have their own dedicated
 * commands to set the values.
 */
class VantageConfiguration {
public:
    VantageConfiguration(VantageWeatherStation & station);
    virtual ~VantageConfiguration();

    /**
     * Read the configuration settings from the EEPROM.
     */
    bool retrieveConfigurationParameters();

    /**
     * Update the position of the weather station.
     *
     * @param position  The position of the station (not the ISS)
     * @return True if the position was updated
     */
    bool updatePosition(const PositionData & position);
    bool retrievePosition(PositionData & position);


    /**
     * Update the DST and time zone settings.
     */
    bool updateTimeSettings(const TimeSettings & timeSettings);
    bool retrieveTimeSettings(TimeSettings & timeSettings);

    /**
     * Update the units settings.
     * Note that this method sets all the units value as they share a single byte in the EEPROM and
     * updating them all simultaneously will reduce EEPROM writes. Also note that these settings
     * only change the displayed values, not the values reported in the serial protocol.
     *
     * @param unitsSettings    The settings for all of the unit of display
     * @return True if the units setting were written successfully
     */
    bool updateUnitsSettings(const UnitsSettings & unitsSettings);
    bool retrieveUnitsSettings(UnitsSettings & unitsSettings);

    bool updateSetupBits(const SetupBits & setupBits);
    bool retrieveSetupBits(SetupBits & setupBits);

    void retrieveTimeZoneOptions(std::vector<std::string> & timezoneList);

    std::string retrieveAllConfigurationData();

private:
    void decodePosition(byte * buffer, int offset, PositionData & positionData);
    void decodeTimeSettings(const byte * buffer, int offset, TimeSettings & timeSettings);
    void decodeUnitsSettings(const byte * buffer, int offset, UnitsSettings & unitsSettings);
    void decodeSetupBits(const byte * buffer, int offset, SetupBits & setupBits);

    void saveRainCollectorSize(ProtocolConstants::RainCupSizeType rainCupType);

    static const int EEPROM_CONFIG_SIZE = 46;

    VantageWeatherStation &  station;
    VantageLogger &          logger;
    int                      secondaryWindCupSize;
    ProtocolConstants::Month rainSeasonStartMonth;
    StationId                retransmitId;
    bool                     logFinalTemperature;  // Whether to log temperature at the end of an archive period,
                                                   // false indicates that the average will be logged

    /**
     * Get the list of sensor stations.
     *
     * @return the list of sensor stations
     */
    //const std::vector<SensorStation> & getSensorStations() const;

    /**
     * Get the list of sensors attached to the sensor stations.
     *
     * @return The list of sensors
     */
    //const std::vector<Sensor> & getSensors() const;
};

}

#endif /* VANTAGE_CONFIGURATION_H */
