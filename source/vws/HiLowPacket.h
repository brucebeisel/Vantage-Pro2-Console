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
#ifndef HILOW_PACKET_H
#define HILOW_PACKET_H

#include <string>

#include "Measurement.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "WeatherTypes.h"

namespace vws {
/**
 * Class that decodes and stores the data from the High/Low packet.
 */
class HiLowPacket {
public:
    HiLowPacket();
    ~HiLowPacket();

    /**
     * Parse the High/Lows packet buffer.
     * 
     * @param buffer The buffer to decodes
     * @return True if the Hi/Low packet was decodes successfully
     */
    bool decodeHiLowPacket(byte buffer[]);

    std::string formatXML() const;
    std::string formatJSON() const;

    //
    // Barometer High/Lows
    //
    Pressure getDayLowBarometer() const;
    DateTime getDayLowBarometerTime() const;
    Pressure getDayHighBarometer() const;
    DateTime getDayHighBarometerTime() const;
    Pressure getMonthLowBarometer() const;
    Pressure getMonthHighBarometer() const;
    Pressure getYearLowBarometer() const;
    Pressure getYearHighBarometer() const;

    //
    // Wind Highs
    //
    Speed    getDayHighWind() const;
    DateTime getDayHighWindTime() const;
    Speed    getMonthHighWind() const;
    Speed    getYearHighWind() const;

    //
    // Indoor Temperature High/Lows
    //
    Temperature getDayLowIndoorTemperature() const;
    DateTime    getDayLowIndoorTemperatureTime() const;
    Temperature getDayHighIndoorTemperature() const;
    DateTime    getDayHighIndoorTemperatureTime() const;
    Temperature getMonthLowIndoorTemperature() const;
    Temperature getMonthHighIndoorTemperature() const;
    Temperature getYearLowIndoorTemperature() const;
    Temperature getYearHighIndoorTemperature() const;

    //
    // Outdoor Temperature High/Lows
    //
    Temperature getDayLowOutdoorTemperature() const;
    DateTime    getDayLowOutdoorTemperatureTime() const;
    Temperature getDayHighOutdoorTemperature() const;
    DateTime    getDayHighOutdoorTemperatureTime() const;
    Temperature getMonthLowOutdoorTemperature() const;
    Temperature getMonthHighOutdoorTemperature() const;
    Temperature getYearLowOutdoorTemperature() const;
    Temperature getYearHighOutdoorTemperature() const;

    //
    // Indoor Humidity High/Lows
    //
    Humidity getDayLowIndoorHumidity() const;
    DateTime getDayLowIndoorHumidityTime() const;
    Humidity getDayHighIndoorHumidity() const;
    DateTime getDayHighIndoorHumidityTime() const;
    Humidity getMonthLowIndoorHumidity() const;
    Humidity getMonthHighIndoorHumidity() const;
    Humidity getYearLowIndoorHumidity() const;
    Humidity getYearHighIndoorHumidity() const;

    //
    // Outdoor Humidity High/Lows
    //
    Humidity getDayLowOutdoorHumidity() const;
    DateTime getDayLowOutdoorHumidityTime() const;
    Humidity getDayHighOutdoorHumidity() const;
    DateTime getDayHighOutdoorHumidityTime() const;
    Humidity getMonthLowOutdoorHumidity() const;
    Humidity getMonthHighOutdoorHumidity() const;
    Humidity getYearLowOutdoorHumidity() const;
    Humidity getYearHighOutdoorHumidity() const;

    //
    // Dew Point High/Lows
    //
    Temperature getDayLowDewPoint() const;
    DateTime    getDayLowDewPointTime() const;
    Temperature getDayHighDewPoint() const;
    DateTime    getDayHighDewPointTime() const;
    Temperature getMonthLowDewPoint() const;
    Temperature getMonthHighDewPoint() const;
    Temperature getYearLowDewPoint() const;
    Temperature getYearHighDewPoint() const;

    //
    // Wind Chill Lows
    //
    Temperature getDayLowWindChill() const;
    DateTime    getDayLowWindChillTime() const;
    Temperature getMonthLowWindChill() const;
    Temperature getYearLowWindChill() const;

    //
    // Heat Index Highs
    //
    Temperature getDayHighHeatIndex() const;
    DateTime    getDayHighHeatIndexTime() const;
    Temperature getMonthHighHeatIndex() const;
    Temperature getYearHighHeatIndex() const;

    //
    // THSW Highs
    //
    Temperature getDayHighTHSW() const;
    DateTime    getDayHighTHSWTime() const;
    Temperature getMonthHighTHSW() const;
    Temperature getYearHighTHSW() const;

    //
    // Solar Radiation Highs
    //
    SolarRadiation getDayHighSolarRadiation() const;
    DateTime       getDayHighSolarRadiationTime() const;
    SolarRadiation getMonthHighSolarRadiation() const;
    SolarRadiation getYearHighSolarRadiation() const;

    //
    // UV Index Highs
    //
    UvIndex   getDayHighUvIndex() const;
    DateTime  getDayHighUvIndexTime() const;
    UvIndex   getMonthHighUvIndex() const;
    UvIndex   getYearHighUvIndex() const;

    //
    // Rain Rate Highs
    //
    Rainfall  getDayHighRainRate() const;
    DateTime  getDayHighRainRateTime() const;
    Rainfall  getMonthHighRainRate() const;
    Rainfall  getYearHighRainRate() const;

    //
    // Extra temperatures and soil and leaf wetness goes here
    //

private:

    template<typename T>
    struct Values {
        Measurement<T> todayExtremeValue;
        int            todayExtremeValueTime;
        Measurement<T> monthExtremeValue;
        Measurement<T> yearExtremeValue;
        bool isValid() const;
        std::string    formatXML(bool low) const;
        std::string    formatJSON(bool low) const;
        std::string    formatExtremeValueTime() const;
    };

    template<typename T> using LowValues = Values<T>;
    template<typename T> using HighValues = Values<T>;

    template<typename T>
    struct HighLowValues {
        Values<T>   lows;
        Values<T>   highs;
        bool isValid() const;
        std::string formatXML() const;
        std::string formatJSON() const;
    };

    bool decodeHiLowTemperature(const byte buffer[], HighLowValues<Temperature> & values, int baseOffset);

    HighLowValues<Pressure>     barometer;
    HighValues<Speed>           wind;
    HighLowValues<Temperature>  indoorTemperature;
    HighLowValues<Humidity>     indoorHumidity;
    HighLowValues<Temperature>  outdoorTemperature;
    HighLowValues<Humidity>     outdoorHumidity;
    HighLowValues<Temperature>  dewPoint;
    HighValues<Temperature>     heatIndex;
    LowValues<Temperature>      windChill;
    HighValues<Temperature>     thsw;
    HighValues<SolarRadiation>  solarRadiation;
    HighValues<UvIndex>         uvIndex;
    HighValues<Rainfall>        rainRate;
    Rainfall                    highHourRainRate;
    HighLowValues<Temperature>  extraTemperature[ProtocolConstants::MAX_EXTRA_TEMPERATURES];
    HighLowValues<Temperature>  soilTemperature[ProtocolConstants::MAX_SOIL_TEMPERATURES];
    HighLowValues<Temperature>  leafTemperature[ProtocolConstants::MAX_LEAF_TEMPERATURES];
    HighLowValues<Humidity>     extraHumidity[ProtocolConstants::MAX_EXTRA_HUMIDITIES];
    HighLowValues<SoilMoisture> soilMoisture[ProtocolConstants::MAX_SOIL_MOISTURES];
    HighLowValues<LeafWetness>  leafWetness[ProtocolConstants::MAX_LEAF_WETNESSES];
    VantageLogger               logger;
};
}
#endif
