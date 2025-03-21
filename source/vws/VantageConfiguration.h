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
#ifndef VANTAGE_CONFIGURATION_H
#define VANTAGE_CONFIGURATION_H

#include <vector>
#include <string>
#include "json.hpp"
#include "VantageProtocolConstants.h"
#include "VantageWeatherStation.h"
#include "WeatherTypes.h"
#include "LoopPacketListener.h"
#include "ConsoleConnectionMonitor.h"
#include "UnitsSettings.h"
#include "VantageEepromConstants.h"


namespace vws {
class VantageLogger;
class LoopPacket;
class Loop2Packet;
class RainCollectorSizeListener;

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
    std::string timezoneName;                // Only used if useGmtOffset is false
    int         gmtOffsetMinutes;            // Only used if useGmtOffset is true
    bool        manualDaylightSavingsTime;
    bool        manualDaylightSavingsTimeOn; // This will change twice a year, but only if manualDaylightSavingsTime is true

    void encode(byte buffer[], int offset) const;
    void decode(byte buffer[], int offset);

    /**
     * Format the time settings into JSON.
     *
     * @return The formatted JSON
     */
    std::string formatJSON() const;
};

struct SetupBits {
    bool is24HourMode;
    bool isCurrentlyAM;           // This field always comes from the console with a value of 1.
    bool isDayMonthDisplay;
    bool isWindCupLarge;
    ProtocolConstants::RainBucketSizeType rainBucketSizeType;
    bool isNorthLatitude;         // Is kept synchronized with the position latitude field
    bool isEastLongitude;         // Is kept synchronized with the position longitude field

    void encode(byte & buffer) const;
    void decode(byte buffer);

    /**
     * Format the setup bits into JSON.
     *
     * @return The formatted JSON
     */
    std::string formatJSON() const;
};

struct PositionData {
    double latitude;
    double longitude;
    int    elevation;

    /**
     * Encode only the latitude and longitude as the elevation is updated separately.
     *
     * @param buffer The buffer into which to encode the latitude and longitude
     * @param offset The offset into the buffer in which to encode the data
     */
    void encodeLatLon(byte buffer[], int offset) const;

    /**
     * Decode the position data from the EEPROM.
     *
     * @param buffer The buffer from which to decode the position data
     * @param offset The offset within the buffer to start decoding
     */
    void decode(byte buffer[], int offset);

    /**
     * Format the position data into JSON.
     *
     * @return The formatted JSON
     */
    std::string formatJSON() const;

};

struct ConsoleConfigurationData {
    TimeSettings             timeSettings;
    SetupBits                setupBits;
    PositionData             positionData;
    UnitsSettings            unitsSettings;
    ProtocolConstants::Month rainSeasonStartMonth;
    StationId                retransmitId;
    bool                     logFinalTemperature;
    EepromConstants::SecondaryWindCupSize secondaryWindCupSize;

    /**
     * Format the configuration data into JSON.
     *
     * @return The formatted JSON
     */
    std::string formatJSON() const;
};

/**
 * This class manages the configuration settings that are stored in the EEPROM of the Vantage consoles.
 * Most of these settings are changed using the EEPROM commands, but some have their own dedicated
 * commands to set the values.
 */
class VantageConfiguration : public LoopPacketListener, public ConsoleConnectionMonitor {
public:
    /**
     * Constructor.
     *
     * @param station The weather station object used to communicate with the console
     */
    VantageConfiguration(VantageWeatherStation & station);

    /**
     * Destructor.
     */
    virtual ~VantageConfiguration();

    /**
     * Add a rain collector size listener.
     *
     * @param listener The listener that will be notified when the rain collector size changes
     */
    void addRainCollectorSizeListener(RainCollectorSizeListener & listener);

    /**
     * Process a LOOP packet received from the console.
     *
     * @param packet The LOOP packet
     * @return True if the loop packet loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & loopPacket);

    /**
     * Process a LOOP2 packet received from the console.
     *
     * @param packet The LOOP2 packet
     * @return True if the loop packet loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & loopPacket);

    /**
     * Retrieve any configuration data needed after the connection is established with the console.
     */
    virtual void consoleConnected();

    /**
     * This method does nothing, but is needed for the ConsoleConnectionMonitor interface.
     */
    virtual void consoleDisconnected();

    /**
     * Update the position of the console.
     *
     * @param position   The position of the console (not the ISS)
     * @param initialize If true, the NEWSETUP command will be sent to the console
     * @return True if the position was updated
     */
    bool updatePosition(const PositionData & position, bool initialize = true);

    /**
     * Retrieve the position of the console.
     *
     * @param position  The object into which the position will be copied
     * @return True if the position was retrieved
     */
    bool retrievePosition(PositionData & position);


    /**
     * Update the DST and time zone settings.
     *
     * @param timeSettings The time settings retrieved from the console
     * @return True if the time settings were successfully updated
     */
    bool updateTimeSettings(const TimeSettings & timeSettings);

    /**
     * Retrieve the DST and time zone settings.
     *
     * @param timeSettings The object into which the time settings will be copied
     * @return True if the time settings were successfully retrieved
     */
    bool retrieveTimeSettings(TimeSettings & timeSettings);

    /**
     * Update the units settings.
     * Note that this method sets all the units value as they share a single byte in the EEPROM and
     * updating them all simultaneously will reduce EEPROM writes. Also note that these settings
     * only change the displayed values, not the values reported in the serial protocol.
     *
     * @param unitsSettings The settings for all of the unit of display
     * @param initialize    If true, the NEWSETUP command will be sent to the console
     * @return True if the units setting were written successfully
     */
    bool updateUnitsSettings(const UnitsSettings & unitsSettings, bool intialize = true);

    /**
     * Retrieve the units settings.
     *
     * @param unitsSettings  The object into which units settings will be copied
     * @return True if the units setting were written retrieved
     */
    bool retrieveUnitsSettings(UnitsSettings & unitsSettings);

    /**
     * Update the general setup bits.
     *
     * @param setupBits  The bits that will be written to the console
     * @param initialize If true, the NEWSETUP command will be sent to the console
     * @return True if the bits were successfully updated
     */
    bool updateSetupBits(const SetupBits & setupBits, bool initialize = true);

    /**
     * Retrieve the general setup bits.
     *
     * @param setupBits The object into which the setup bits will be copied
     * @return True if the bits were successfully retrieved
     */
    bool retrieveSetupBits(SetupBits & setupBits);

    /**
     * Get the time zones that the console supports so it can be presented to a user for selection.
     *
     * @param timezoneList The list of time zones as strings
     */
    void getTimeZoneOptions(std::vector<std::string> & timezoneList);

    bool updateAllConfigurationData(const ConsoleConfigurationData & configData, bool initializeConsole = true);

    bool retrieveAllConfigurationData(ConsoleConfigurationData & configData);


private:
    void saveRainBucketSize(ProtocolConstants::RainBucketSizeType rainBucketType);

    static const int EEPROM_CONFIG_SIZE = 46;

    VantageWeatherStation &                  station;
    Measurement<Pressure>                    lastAtmosphericPressure;
    std::vector<RainCollectorSizeListener *> listeners;
    VantageLogger &                          logger;
};

}

#endif /* VANTAGE_CONFIGURATION_H */
