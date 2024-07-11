/* 
 * Copyright (C) 2024 Bruce Beisel
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
#ifndef ARCHIVEPACKET_H
#define ARCHIVEPACKET_H
#include <string>

#include "WeatherTypes.h"
#include "Measurement.h"
#include "DateTimeFields.h"

namespace vws {

class VantageLogger;

/**
 * A class that represents a Vantage archive packet. Very little data is needed outside the construction of the archive
 * message that is sent to the collector.
 */
class ArchivePacket {
public:
    static constexpr int BYTES_PER_ARCHIVE_PACKET = 52;
    static constexpr int PACKET_NO_VALUE = 0xFF;

    static constexpr int MAX_EXTRA_TEMPERATURES = 3;
    static constexpr int MAX_EXTRA_HUMIDITIES = 2;

    //
    // The serial protocol document says this is 4, but the 4th value is not set to the Dash value when there
    // are not soil temperature sensors.
    //
    static constexpr int MAX_SOIL_TEMPERATURES = 3;
    static constexpr int MAX_SOIL_MOISTURES = 4;
    static constexpr int MAX_LEAF_WETNESSES = 2;
    static constexpr int MAX_LEAF_TEMPERATURES = 2;

    static constexpr int TIME_STAMP_BUFFER_LENGTH = 2;
    static constexpr int DATE_STAMP_BUFFER_LENGTH = 2;

    static constexpr int ARCHIVE_PACKET_REV_A = 0xff;
    static constexpr int ARCHIVE_PACKET_REV_B = 0x00;

    /**
     * Default constructor required for STL containers and arrays.
     */
    ArchivePacket();

    /**
     * Constructor.
     * 
     * @param buffer The buffer containing the raw archive packet
     * @param offset The offset into the buffer that the archive packet begins
     */
    ArchivePacket(const byte buffer[], int offset = 0);

    /**
     * Destructor.
     */
    virtual ~ArchivePacket();

    /**
     * Update the underlying data of this archive packet.
     *
     * @param buffer The buffer with which to update the archive data
     * @param offset The offset within the specified buffer from which to copy the data
     */
    void updateArchivePacketData(const byte buffer[], int offset = 0);

    /**
     * Clear the contents of this packet.
     */
    void clearArchivePacketData();

    /**
     * Get the raw packet data used to extract the archive data.
     * 
     * @return The raw packet
     */
    const byte * getBuffer() const;
    
    /**
     * The number of wind samples that were collector by the weather station over the archive period.
     * 
     * @return The wind sample count from the archive packet
     */
    int getWindSampleCount() const;
    
    /**
     * Get the epoch based date/time that was extracted from the packet.
     * 
     * @return The date.time
     */
    DateTime getEpochDateTime() const;

    /**
     * Get the Date/Time fields that were extracted from the packet.
     *
     * @return the DataTimeFields structure
     */
    const DateTimeFields & getDateTimeFields() const;

    /**
     * Return a date string in the format yyyy-mm-dd hh:mm built using the raw packet data.
     *
     * @return The date string
     */
    std::string getPacketDateTimeString() const;

    /**
     * Check whether this is an empty packet, the empty packet concept is used to avoid the use of NULL.
     * 
     * @return True if it is an empty packet
     */
    bool isEmptyPacket() const;
    
    /**
     * Checks if an archive packet buffer contains data.
     *
     * @param buffer The buffer containing the packet
     * @param offset The offset within the buffer where the packet starts
     *
     * @return True if the packet contains data
     */
    static bool archivePacketContainsData(const byte * buffer, int offset);


    Measurement<Temperature> getAverageOutsideTemperature() const;
    Measurement<Temperature> getLowOutsideTemperature() const;
    Measurement<Temperature> getHighOutsideTemperature() const;


    Measurement<Rainfall> getRainfall() const;
    Measurement<Rainfall> getHighRainfallRate() const;

    Measurement<Pressure> getBarometricPressure() const;

    Measurement<SolarRadiation> getAverageSolarRadiation() const;
    Measurement<Temperature> getInsideTemperature() const;
    Measurement<Humidity> getInsideHumidity() const;
    Measurement<Humidity> getOutsideHumidity() const;
    Measurement<Speed> getAverageWindSpeed() const;
    Measurement<HeadingIndex> getPrevailingWindHeadingIndex() const;

    Measurement<Speed> getHighWindSpeed() const;
    Measurement<HeadingIndex> getHighWindHeadingIndex() const;

    Measurement<UvIndex> getAverageUvIndex() const;
    Measurement<Evapotranspiration> getEvapotranspiration() const;
    Measurement<SolarRadiation> getHighSolarRadiation() const;
    Measurement<UvIndex> getHighUvIndex() const;

    int getForecastRule() const;

    Measurement<Humidity> getExtraHumidity(int index) const;
    Measurement<Temperature> getExtraTemperature(int index) const;
    Measurement<Temperature> getLeafTemperature(int index) const;
    Measurement<LeafWetness> getLeafWetness(int index) const;
    Measurement<Temperature> getSoilTemperature(int index) const;
    Measurement<SoilMoisture> getSoilMoisture(int index) const;

    int getArchiveRecordType() const;

    bool operator==(const ArchivePacket & other);
    bool operator<(const ArchivePacket & other);


    /**
     * Format the Archive packet as JSON.
     * 
     * @param pretty Whether to format the JSON with indenting and spacing
     * @return The formatted message
     */
    std::string formatJSON(bool pretty = false) const;

private:
    void decodeDateTimeValues();

    static constexpr DateTime EMPTY_ARCHIVE_PACKET_TIME = 0;
    static constexpr int UNKNOWN_ET = 0;
    static constexpr int UNKNOWN_SOLAR_RADIATION = 0;
    static constexpr int YEAR_OFFSET = 2000;

    //
    // Archive packet (Rev B)
    //
    static constexpr int REV_A_RECORD_TYPE = 0xFF;
    static constexpr int REV_B_RECORD_TYPE = 0;

    static constexpr int DATE_STAMP_OFFSET = 0;
    static constexpr int TIME_STAMP_OFFSET = 2;
    static constexpr int OUTSIDE_TEMPERATURE_OFFSET = 4;
    static constexpr int HIGH_OUTSIDE_TEMPERATURE_OFFSET = 6;
    static constexpr int LOW_OUTSIDE_TEMPERATURE_OFFSET = 8;
    static constexpr int RAINFALL_OFFSET = 10;
    static constexpr int HIGH_RAIN_RATE_OFFSET = 12;
    static constexpr int BAROMETER_OFFSET = 14;
    static constexpr int SOLAR_RADIATION_OFFSET = 16;
    static constexpr int NUM_WIND_SAMPLES_OFFSET = 18;
    static constexpr int INSIDE_TEMPERATURE_OFFSET = 20;
    static constexpr int INSIDE_HUMIDITY_OFFSET = 22;
    static constexpr int OUTSIDE_HUMIDITY_OFFSET = 23;
    static constexpr int AVG_WIND_SPEED_OFFSET = 24;
    static constexpr int HIGH_WIND_SPEED_OFFSET = 25;
    static constexpr int DIR_OF_HIGH_WIND_SPEED_OFFSET = 26;
    static constexpr int PREVAILING_WIND_DIRECTION_OFFSET = 27;
    static constexpr int AVG_UV_INDEX_OFFSET = 28;
    static constexpr int ET_OFFSET = 29;
    static constexpr int HIGH_SOLAR_RADIATION_OFFSET = 30;
    static constexpr int HIGH_UV_INDEX_OFFSET = 32;
    static constexpr int FORECAST_RULE_OFFSET = 33;
    static constexpr int LEAF_TEMPERATURE_BASE_OFFSET = 34;
    static constexpr int LEAF_WETNESS_BASE_OFFSET = 36;
    static constexpr int SOIL_TEMPERATURE_BASE_OFFSET = 38;
    static constexpr int RECORD_TYPE_OFFSET = 42;
    static constexpr int EXTRA_HUMIDITIES_BASE_OFFSET = 43;
    static constexpr int EXTRA_TEMPERATURES_BASE_OFFSET = 45;
    static constexpr int SOIL_MOISTURES_BASE_OFFSET = 48;


    DateTime        packetEpochDateTime;
    DateTimeFields  packetDateTimeFields;
    int             windSampleCount;
    byte            buffer[BYTES_PER_ARCHIVE_PACKET];
    VantageLogger * logger;
};

}

#endif /* ARCHIVEPACKET_H */
