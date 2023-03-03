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
#ifndef LOOP_PACKET_H
#define LOOP_PACKET_H

#include <string>
#include <bitset>

#include "Measurement.h"
#include "VantageProtocolConstants.h"
#include "WeatherTypes.h"

namespace vws {
class VantageLogger;

/**
 * Class that decodes and stores the data from the LOOP packet.
 */
class LoopPacket {
public:
    static constexpr size_t ALARM_BYTES = 16;
    static constexpr size_t ALARM_BITS = ALARM_BYTES * 8;
    typedef std::bitset<ALARM_BITS> AlarmBitSet;

    static constexpr int LOOP_PACKET_SIZE = 99;
    static constexpr int LOOP_PACKET_TYPE = 0;

    /**
     * Constructor.
     */
    LoopPacket();

    /**
     * Destructor.
     */
    ~LoopPacket();

    /**
     * Get the type of this packet.
     * It can only be one value, but by providing this method it decouples the hard-coded value from the caller.
     *
     * @return The LOOP packet type
     */
    int getPacketType() const;

    /**
     * Parse the LOOP packet buffer.
     * 
     * @param buffer The buffer to decode
     * @return True if the LOOP packet was decoded successfully
     */
    bool decodeLoopPacket(byte buffer[]);

    /**
     * Get the underlying data buffer that contains the values.
     *
     * @return The internal buffer
     */
    const byte * getPacketData() const;

    /**
     * Get various values from the LOOP packet.
     * Note that any return value that is a Measurement<> template can have and invalid value.
     */
    ProtocolConstants::BarometerTrend       getBarometerTrend() const;
    std::string                             getBarometerTrendString() const;
    int                                     getNextRecord() const;
    const Measurement<Pressure> &           getBarometricPressure() const;
    const Measurement<Temperature> &        getInsideTemperature() const;
    const Measurement<Humidity> &           getInsideHumidity() const;
    const Measurement<Temperature> &        getOutsideTemperature() const;
    const Measurement<Speed> &              getWindSpeed() const;
    const Measurement<Speed> &              getWindSpeed10MinuteAverage() const;
    const Measurement<Heading> &            getWindDirection() const;
    const Measurement<Temperature> &        getExtraTemperature(int index) const;
    const Measurement<Temperature> &        getSoilTemperature(int index) const;
    const Measurement<Temperature> &        getLeafTemperature(int index) const;
    const Measurement<Humidity> &           getOutsideHumidity() const;
    const Measurement<Humidity> &           getExtraHumidity(int index) const;
    Rainfall                                getRainRate() const;
    const Measurement<UvIndex> &            getUvIndex() const;
    const Measurement<SolarRadiation> &     getSolarRadiation() const;
    bool                                    isStormOngoing() const;
    Rainfall                                getStormRain() const;
    DateTime                                getStormStart() const;
    Rainfall                                getDayRain() const;
    Rainfall                                getMonthRain() const;
    Rainfall                                getYearRain() const;
    const Measurement<Evapotranspiration> & getDayET() const;
    const Measurement<Evapotranspiration> & getMonthET() const;
    const Measurement<Evapotranspiration> & getYearET() const;
    const Measurement<SoilMoisture> &       getSoilMoisture(int index) const;
    const Measurement<LeafWetness> &        getLeafWetness(int index) const;
    const AlarmBitSet &                     getAlarmBits() const;
    bool                                    isTransmitterBatteryGood(int index) const;
    float                                   getConsoleBatteryVoltage() const;
    ProtocolConstants::Forecast             getForecastIcon() const;
    std::string                             getForecastIconString() const;
    int                                     getForecastRuleIndex() const;
    DateTime                                getSunriseTime() const;
    DateTime                                getSunsetTime() const;

    friend std::ostream & operator<<(std::ostream & os, const LoopPacket & packet);

private:
    static constexpr int L_OFFSET = 0;
    static constexpr int FIRST_O_OFFSET = 1;
    static constexpr int SECOND_O_OFFSET = 2;
    static constexpr int BAROMETER_TREND_OFFSET = 3;
    static constexpr int PACKET_TYPE_OFFSET = 4;
    static constexpr int NEXT_RECORD_OFFSET = 5;
    static constexpr int BAROMETER_OFFSET = 7;
    static constexpr int INSIDE_TEMPERATURE_OFFSET = 9;
    static constexpr int INSIDE_HUMIDITY_OFFSET = 11;
    static constexpr int OUTSIDE_TEMPERATURE_OFFSET = 12;
    static constexpr int WIND_SPEED_OFFSET = 14;
    static constexpr int TEN_MINUTE_AVG_WIND_SPEED_OFFSET = 15;
    static constexpr int WIND_DIRECTION_OFFSET = 16;
    static constexpr int EXTRA_TEMPERATURES_OFFSET = 18;
    static constexpr int SOIL_TEMPERATURES_OFFSET = 25;
    static constexpr int LEAF_TEMPERATURES_OFFSET = 29;
    static constexpr int OUTSIDE_HUMIDITY_OFFSET = 33;
    static constexpr int EXTRA_HUMIDITIES_OFFSET = 34;
    static constexpr int RAIN_RATE_OFFSET = 41;
    static constexpr int UV_INDEX_OFFSET = 43;
    static constexpr int SOLAR_RADIATION_OFFSET = 44;
    static constexpr int STORM_RAIN_OFFSET = 46;
    static constexpr int STORM_START_DATE_OFFSET = 48;
    static constexpr int DAY_RAIN_OFFSET = 50;
    static constexpr int MONTH_RAIN_OFFSET = 52;
    static constexpr int YEAR_RAIN_OFFSET = 54;
    static constexpr int DAY_ET_OFFSET = 56;
    static constexpr int MONTH_ET_OFFSET = 58;
    static constexpr int YEAR_ET_OFFSET = 60;
    static constexpr int SOIL_MOISTURES_OFFSET = 62;
    static constexpr int LEAF_WETNESSES_OFFSET = 66;
    static constexpr int ALARMS_OFFSET = 70;
    static constexpr int TRANSMITTER_BATTERY_STATUS_OFFSET = 86;
    static constexpr int CONSOLE_BATTERY_VOLTAGE_OFFSET = 87;
    static constexpr int FORECAST_ICONS_OFFSET = 89;
    static constexpr int FORECAST_RULE_NUMBER_OFFSET = 90;
    static constexpr int SUNRISE_TIME_OFFSET = 91;
    static constexpr int SUNSET_TIME_OFFSET = 93;
    static constexpr int LINE_FEED_OFFSET = 95;
    static constexpr int CARRIAGE_RETURN_OFFSET = 96;
    static constexpr int CRC_OFFSET = 97;


    byte                              packetData[LOOP_PACKET_SIZE];
    ProtocolConstants::BarometerTrend barometerTrend;
    int                               packetType;
    int                               nextRecord;
    Measurement<Pressure>             barometricPressure;
    Measurement<Temperature>          insideTemperature;
    Measurement<Humidity>             insideHumidity;
    Measurement<Temperature>          outsideTemperature;
    Measurement<Speed>                windSpeed;
    Measurement<Speed>                windSpeed10MinuteAverage;
    Measurement<Heading>              windDirection;
    Measurement<Temperature>          extraTemperature[ProtocolConstants::MAX_EXTRA_TEMPERATURES];
    Measurement<Temperature>          soilTemperature[ProtocolConstants::MAX_SOIL_TEMPERATURES];
    Measurement<Temperature>          leafTemperature[ProtocolConstants::MAX_LEAF_TEMPERATURES];
    Measurement<Humidity>             outsideHumidity;
    Measurement<Humidity>             extraHumidity[ProtocolConstants::MAX_EXTRA_HUMIDITIES];
    Rainfall                          rainRate;
    Measurement<UvIndex>              uvIndex;
    Measurement<SolarRadiation>       solarRadiation;
    Rainfall                          stormRain;
    DateTime                          stormStart;
    Rainfall                          dayRain;
    Rainfall                          monthRain;
    Rainfall                          yearRain;
    Measurement<Evapotranspiration>   dayET;
    Measurement<Evapotranspiration>   monthET;
    Measurement<Evapotranspiration>   yearET;
    Measurement<SoilMoisture>         soilMoisture[ProtocolConstants::MAX_SOIL_MOISTURES];
    Measurement<LeafWetness>          leafWetness[ProtocolConstants::MAX_LEAF_WETNESSES];
    AlarmBitSet                       alarmBits;
    int                               transmitterBatteryStatus;
    float                             consoleBatteryVoltage;
    ProtocolConstants::Forecast       forecastIcon;
    int                               forecastRuleIndex;
    DateTime                          sunriseTime;
    DateTime                          sunsetTime;
    VantageLogger *                   logger;
};
}
#endif
