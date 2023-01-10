#ifndef VANTAGE_CONFIGURATION_H
#define VANTAGE_CONFIGURATION_H

#include "SensorStation.h"
#include "Sensor.h"
#include "VantageWeatherStation.h"
#include "VantageConstants.h"
#include "Weather.h"

namespace vws {

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
    bool VantageConfiguration::retrieveConfigurationParameters();

    /**
     * Update the position of the weather station.
     *
     * @param latitude  The latitude of the station
     * @param longitude The longitude of the station
     * @param elevation The elevation of the console, not the station, as the barometer which uses this value is in the console.
     */
    bool updatePosition(double latitude, double longitude, int elevation);

    /**
     * Update the units settings.
     * Note that this method sets all the units value as they share a single byte in the EEPROM and
     * updating them all simultaneously will reduce EEPROM writes. Also note that these settings
     * only change the displayed values, not the values reported in the serial protocol.
     *
     * @param baroUnits        The units for barometric pressure
     * @param temperatureUnits The units for temperature
     * @param elevationUnits   The units for elevation
     * @param rainUnits        The units for rainfall
     * @param windUnits        The units for wind speed
     * @return True if the EEPROM was updated successfully
     */
    bool updateUnitsSettings(VantageConstants::BarometerUnits baroUnits,
                             VantageConstants::TemperatureUnits temperatureUnits,
                             VantageConstants::ElevationUnits elevationUnits,
                             VantageConstants::RainUnits rainUnits,
                             VantageConstants::WindUnits windUnits);

private:
    static const int EEPROM_CONFIG_SIZE = 46;

    VantageWeatherStation  &           station;
    VantageLogger                      log;
    double                             issLatitude;
    double                             issLongitude;
    int                                consoleElevation;     // Use "BAR=0 <elevation in feet>" command to set this
    int                                timezoneIndex;
    bool                               manualDaylightSavingsTime;
    bool                               manualDaylightSavingsTimeOn;
    int                                gmtOffsetMinutes;
    bool                               useGmtOffset;
    int                                transmitterListenMask;
    bool                               isConsoleRetransmitting;
    int                                retransmitId;            // Only used if isConsoleRetransmitting = true
    std::vector<SensorStation>         sensorStations; // TBD Should this be a list of SensorStation objects or a list of structure that represent the EEPROM data?
    //
    // Unit Bits
    VantageConstants::BarometerUnits   barometerUnits;
    VantageConstants::TemperatureUnits temperatureUnits;
    VantageConstants::ElevationUnits   elevationUnits;
    VantageConstants::RainUnits        rainUnits;
    VantageConstants::WindUnits        windUnits;
    //
    // Setup Bits
    //
    bool                               amPmMode; // Logic reversed due to the name 24-hour
    bool                               isAM;     // TBD Unsure what this is. Is it real-time AM or PM
    bool                               dayMonthFormat;
    bool                               windCupLarge;
    VantageConstants::RainCupSizeType  rainCollectorSize;
    bool                               isLatitudeNorth;
    bool                               isLongitudeEast;
    int                                rainSeasonStartMonth; // January = 1
    VantageConstants::ArchivePeriod    archivePeriod;        // SETPER command used to set this value

    //std::string                firmwareDate;
    //std::string                firmwareVersion;

    /**
     * Get the list of sensor stations.
     *
     * @return the list of sensor stations
     */
    const std::vector<SensorStation> & getSensorStations() const;

    /**
     * Get the list of sensors attached to the sensor stations.
     *
     * @return The list of sensors
     */
    const std::vector<Sensor> & getSensors() const;

    void decodeData(const byte buffer[]);

};

}

#endif /* VANTAGE_CONFIGURATION_H */
