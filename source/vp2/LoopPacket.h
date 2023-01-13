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
#include "Weather.h"
#include "Measurement.h"
#include "VantageConstants.h"
#include "VantageLogger.h"

namespace vws {
/**
 * Class that decodes and stores the data from the LOOP packet.
 */
class LoopPacket {
public:
    /**
     * The trend of the barometer as reported in the LOOP packet
     */
    enum BaroTrend {
        STEADY =            0,
        RISING_SLOWLY =    20,
        RISING_RAPIDLY =   60,
        FALLING_RAPIDLY = 196,
        FALLING_SLOWLY =  236,
        UNKNOWN =         255
    };

    /**
     * The forecast reported by the LOOP packet.
     */
    static const unsigned int RAIN_BIT          = 0x1;
    static const unsigned int MOSTLY_CLOUDY_BIT = 0x2;
    static const unsigned int PARTLY_CLOUDY_BIT = 0x6;
    static const unsigned int SUNNY_BIT         = 0x8;
    static const unsigned int SNOW_BIT          = 0x10;

    enum Forecast {
        SUNNY =                                 SUNNY_BIT,
        PARTLY_CLOUDY =                         PARTLY_CLOUDY_BIT,
        MOSTLY_CLOUDY =                         MOSTLY_CLOUDY_BIT,
        MOSTLY_CLOUDY_WITH_RAIN =               MOSTLY_CLOUDY_BIT | RAIN_BIT,
        MOSTLY_CLOUDY_WITH_SNOW =               MOSTLY_CLOUDY_BIT | SNOW_BIT,
        MOSTLY_CLOUDY_WITH_RAIN_OR_SNOW =       MOSTLY_CLOUDY_BIT | RAIN_BIT | SNOW_BIT,
        PARTLY_CLOUDY_WITH_RAIN_LATER =         PARTLY_CLOUDY_BIT | RAIN_BIT,
        PARTLY_CLOUDY_WITH_SNOW_LATER =         PARTLY_CLOUDY_BIT | SNOW_BIT,
        PARTLY_CLOUDY_WITH_RAIN_OR_SNOW_LATER = PARTLY_CLOUDY_BIT | RAIN_BIT | SNOW_BIT
    };

    static const size_t ALARM_BITS = 16 * 8;
    typedef std::bitset<ALARM_BITS> AlarmBitSet;

    static const int LOOP_PACKET_SIZE = 99;

    /**
     * Constructor.
     */
    LoopPacket();

    /**
     * Destructor.
     */
    ~LoopPacket();

    /**
     * Parse the LOOP packet buffer.
     * 
     * @param buffer The buffer to decode
     * @return True if the LOOP packet was decoded successfully
     */
    bool                                    decodeLoopPacket(byte buffer[]);

    const byte *                            getPacketData() const;
    BaroTrend                               getBaroTrend() const;
    std::string                             getBaroTrendString() const;
    int                                     getPacketType() const;
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
    Forecast                                getForecastIcon() const;
    std::string                             getForecastIconString() const;
    int                                     getForecastRuleIndex() const;
    DateTime                                getSunriseTime() const;
    DateTime                                getSunsetTime() const;

private:
    static const int LOOP_PACKET_TYPE = 0;

    byte                            packetData[LOOP_PACKET_SIZE];
    BaroTrend                       baroTrend;
    int                             packetType;
    int                             nextRecord;
    Measurement<Pressure>           barometricPressure;
    Measurement<Temperature>        insideTemperature;
    Measurement<Humidity>           insideHumidity;
    Measurement<Temperature>        outsideTemperature;
    Measurement<Speed>              windSpeed;
    Measurement<Speed>              windSpeed10MinuteAverage;
    Measurement<Heading>            windDirection;
    Measurement<Temperature>        extraTemperature[VantageConstants::MAX_EXTRA_TEMPERATURES];
    Measurement<Temperature>        soilTemperature[VantageConstants::MAX_SOIL_TEMPERATURES];
    Measurement<Temperature>        leafTemperature[VantageConstants::MAX_LEAF_TEMPERATURES];
    Measurement<Humidity>           outsideHumidity;
    Measurement<Humidity>           extraHumidity[VantageConstants::MAX_EXTRA_HUMIDITIES];
    Rainfall                        rainRate;
    Measurement<UvIndex>            uvIndex;
    Measurement<SolarRadiation>     solarRadiation;
    Rainfall                        stormRain;
    DateTime                        stormStart;
    Rainfall                        dayRain;
    Rainfall                        monthRain;
    Rainfall                        yearRain;
    Measurement<Evapotranspiration> dayET;
    Measurement<Evapotranspiration> monthET;
    Measurement<Evapotranspiration> yearET;
    Measurement<SoilMoisture>       soilMoisture[VantageConstants::MAX_SOIL_MOISTURES];
    Measurement<LeafWetness>        leafWetness[VantageConstants::MAX_LEAF_WETNESSES];
    AlarmBitSet                     alarmBits;
    int                             transmitterBatteryStatus;
    float                           consoleBatteryVoltage;
    Forecast                        forecastIcon;
    int                             forecastRuleIndex;
    DateTime                        sunriseTime;
    DateTime                        sunsetTime;
    VantageLogger                   logger;
};
}
#endif
