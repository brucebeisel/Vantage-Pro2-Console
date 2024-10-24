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
#ifndef LOOP2_PACKET_H
#define LOOP2_PACKET_H

#include "Measurement.h"
#include "VantageProtocolConstants.h"
#include "WeatherTypes.h"
#include "DateTimeFields.h"

namespace vws {
class VantageLogger;

/**
 * Class that decodes and holds the data from the Vantage LOOP2 packet.
 */
class Loop2Packet {
public:
    static const int LOOP2_PACKET_SIZE = 99;
    static const int LOOP2_PACKET_TYPE = 1;

    /**
     * Constructor.
     */
    Loop2Packet();

    /**
     * Destructor.
     */
    virtual ~Loop2Packet();

    /**
     * Parse the LOOP2 packet.
     * 
     * @param The buffer from which to decode the packet
     * @return True if the buffer was decoded successfully
     */
    bool decodeLoop2Packet(const byte[]);

    /**
     * Get the underlying data buffer that contains the values.
     *
     * @return The internal buffer
     */
    const byte * getPacketData() const;

    /**
     * Get the type of this packet.
     * It can only be one value, but by providing this method it decouples the hard-coded value from the caller.
     *
     * @return The LOOP2 packet type
     */
    int getPacketType() const;

    /**
     * Get various values from the LOOP2 packet.
     * Note that any return value that is a Measurement<> template can have and invalid value.
     */
    ProtocolConstants::BarometerTrend       getBarometerTrend() const;
    std::string                             getBarometerTrendString() const;
    const Measurement<Pressure> &           getBarometricPressure() const;
    const Measurement<Temperature> &        getInsideTemperature() const;
    const Measurement<Humidity> &           getInsideHumidity() const;
    const Measurement<Temperature> &        getOutsideTemperature() const;
    const Measurement<Speed> &              getWindSpeed() const;
    const Measurement<Heading> &            getWindDirection() const;
    const Measurement<Speed> &              getWindSpeed10MinuteAverage() const;
    const Measurement<Speed> &              getWindSpeed2MinuteAverage() const;
    const Measurement<Speed> &              getWindGust10Minute() const;
    const Measurement<Heading> &            getWindGustDirection10Minute() const;
    const Measurement<Temperature> &        getDewPoint() const;
    const Measurement<Humidity> &           getOutsideHumidity() const;
    const Measurement<Temperature> &        getHeatIndex() const;
    const Measurement<Temperature> &        getWindChill() const;
    const Measurement<Temperature> &        getThsw() const;
    RainfallRate                            getRainRate() const;
    const Measurement<UvIndex> &            getUvIndex() const;
    const Measurement<SolarRadiation> &     getSolarRadiation() const;
    Rainfall                                getStormRain() const;
    const DateTimeFields &                  getStormStart() const;
    Rainfall                                getDayRain() const;
    Rainfall                                get15MinuteRain() const;
    Rainfall                                getHourRain() const;
    const Measurement<Evapotranspiration> & getDayET() const;
    Rainfall                                get24HourRain() const;
    int                                     getBarometricReductionMethod() const;
    const Measurement<Pressure> &           getUserEnteredBarometricOffset() const;
    const Measurement<Pressure> &           getBarometricCalibrationNumber() const;
    const Measurement<Pressure> &           getBarometricSensorRawReading() const;
    const Measurement<Pressure> &           getAbsoluteBarometricPressure() const;
    const Measurement<Pressure> &           getAltimeterBarometerOffset() const;
    int                                     getNext10MinuteWindSpeedGraphPointer() const;
    int                                     getNext15MinuteWindSpeedGraphPointer() const;
    int                                     getNextHourlyWindSpeedGraphPointer() const;
    int                                     getNextDailyWindSpeedGraphPointer() const;
    int                                     getNextMinuteRainGraphPointer() const;
    int                                     getNextRainStormGraphPointer() const;
    int                                     getIndexToTheMinuteWithinAnHour() const;
    int                                     getNextMonthlyRainGraphPointer() const;
    int                                     getNextYearlyRainGraphPointer() const;
    int                                     getNextSeasonalRainGraphPointer() const;

    friend std::ostream & operator<<(std::ostream & os, const Loop2Packet & packet);

private:
    static constexpr int L_OFFSET = 0;
    static constexpr int FIRST_O_OFFSET = 1;
    static constexpr int SECOND_O_OFFSET = 2;
    static constexpr int BAROMETER_TREND_OFFSET = 3;
    static constexpr int PACKET_TYPE_OFFSET = 4;
    static constexpr int UNUSED_WORD_5 = 5;
    static constexpr int BAROMETER_OFFSET = 7;
    static constexpr int INSIDE_TEMPERATURE_OFFSET = 9;
    static constexpr int INSIDE_HUMIDITY_OFFSET = 11;
    static constexpr int OUTSIDE_TEMPERATURE_OFFSET = 12;
    static constexpr int WIND_SPEED_OFFSET = 14;
    static constexpr int UNUSED_BYTE_15 = 15;
    static constexpr int WIND_DIRECTION_OFFSET = 16;
    static constexpr int TEN_MINUTE_AVG_WIND_SPEED_OFFSET = 18;
    static constexpr int TWO_MINUTE_AVG_WIND_SPEED_OFFSET = 20;
    static constexpr int TEN_MINUTE_WIND_GUST_OFFSET = 22;
    static constexpr int TEN_MINUTE_WIND_GUST_DIRECTION_OFFSET = 24;
    static constexpr int UNUSED_WORD_26 = 26;
    static constexpr int UNUSED_WORD_28 = 28;
    static constexpr int DEW_POINT_OFFSET = 30;
    static constexpr int UNUSED_BYTE_32 = 32;
    static constexpr int OUTSIDE_HUMIDITY_OFFSET = 33;
    static constexpr int UNUSED_BYTE_34 = 34;
    static constexpr int HEAT_INDEX_OFFSET = 35;
    static constexpr int WIND_CHILL_OFFSET = 37;
    static constexpr int THSW_OFFSET = 39;
    static constexpr int RAIN_RATE_OFFSET = 41;
    static constexpr int UV_INDEX_OFFSET = 43;
    static constexpr int SOLAR_RADIATION_OFFSET = 44;
    static constexpr int STORM_RAIN_OFFSET = 46;
    static constexpr int STORM_START_DATE_OFFSET = 48;
    static constexpr int DAY_RAIN_OFFSET = 50;
    static constexpr int FIFTEEN_MINUTE_RAIN_OFFSET = 52;
    static constexpr int HOUR_RAIN_OFFSET = 54;
    static constexpr int DAY_ET_OFFSET = 56;
    static constexpr int TWENTY_FOUR_HOUR_RAIN_OFFSET = 58;
    static constexpr int BAROMETRIC_REDUCTION_METHOD_OFFSET = 60;
    static constexpr int USER_ENTERED_BAROMETRIC_OFFSET_OFFSET = 61;
    static constexpr int BAROMETRIC_CALIBRATION_NUMBER_OFFSET = 63;
    static constexpr int BAROMETRIC_SENSOR_RAW_READING_OFFSET = 65;
    static constexpr int ABSOLUTE_BAROMETRIC_PRESSURE_OFFSET = 67;
    static constexpr int ALTIMETER_SETTING_OFFSET = 69;
    static constexpr int NEXT_10_MINUTE_WIND_SPEED_GRAPH_POINTER_OFFSET = 73;
    static constexpr int NEXT_15_MINUTE_WIND_SPEED_GRAPH_POINTER_OFFSET = 74;
    static constexpr int NEXT_HOURLY_WIND_SPEED_GRAPH_POINTER_OFFSET = 75;
    static constexpr int NEXT_DAILY_WIND_SPEED_GRAPH_POINTER_OFFSET = 76;
    static constexpr int NEXT_MINUTE_RAIN_GRAPH_POINTER_OFFSET = 77;
    static constexpr int NEXT_RAIN_STORM_GRAPH_POINTER_OFFSET = 78;
    static constexpr int NEXT_MONTHLY_RAIN_GRAPH_POINTER_OFFSET = 80;
    static constexpr int NEXT_YEARLY_RAIN_GRAPH_POINTER_OFFSET = 81;
    static constexpr int NEXT_SEASONAL_RAIN_GRAPH_POINTER_OFFSET = 82;
    static constexpr int UNUSED_WORD_83 = 83;
    static constexpr int UNUSED_WORD_85 = 85;
    static constexpr int UNUSED_WORD_87 = 87;
    static constexpr int UNUSED_WORD_89 = 89;
    static constexpr int UNUSED_WORD_91 = 91;
    static constexpr int UNUSED_WORD_93 = 93;
    static constexpr int LINE_FEED_OFFSET = 95;
    static constexpr int CARRIAGE_RETURN_OFFSET = 96;
    static constexpr int CRC_OFFSET = 97;

    static constexpr int UNUSED_WORD_VALUE = 0x7FFF;
    static constexpr int UNUSED_BYTE_VALUE = 0xFF;

    byte                              packetData[LOOP2_PACKET_SIZE];
    int                               packetType;
    ProtocolConstants::BarometerTrend barometerTrend;
    Measurement<Pressure>             barometricPressure;
    Measurement<Temperature>          insideTemperature;
    Measurement<Humidity>             insideHumidity;
    Measurement<Temperature>          outsideTemperature;
    Measurement<Speed>                windSpeed;
    Measurement<Heading>              windDirection;
    Measurement<Speed>                windSpeed10MinuteAverage;
    Measurement<Speed>                windSpeed2MinuteAverage;
    Measurement<Speed>                windGust10Minute;
    Measurement<Heading>              windGustDirection10Minute;
    Measurement<Temperature>          dewPoint;
    Measurement<Humidity>             outsideHumidity;
    Measurement<Temperature>          heatIndex;
    Measurement<Temperature>          windChill;
    Measurement<Temperature>          thsw;
    RainfallRate                      rainRate;
    Measurement<UvIndex>              uvIndex;
    Measurement<SolarRadiation>       solarRadiation;
    Rainfall                          stormRain;
    DateTimeFields                    stormStart;
    Rainfall                          rainDay;
    Rainfall                          rain15Minute;
    Rainfall                          rainHour;
    Measurement<Evapotranspiration>   dayET;
    Rainfall                          rain24Hour;
    int                               barometricReductionMethod;
    Measurement<Pressure>             userEnteredBarometricOffset;
    Measurement<Pressure>             barometricCalibrationNumber;
    Measurement<Pressure>             barometricSensorRawReading;
    Measurement<Pressure>             absoluteBarometricPressure;
    Measurement<Pressure>             altimeterSetting;
    int                               next10MinuteWindSpeedGraphPointer;
    int                               next15MinuteWindSpeedGraphPointer;
    int                               nextHourlyWindSpeedGraphPointer;
    int                               nextDailyWindSpeedGraphPointer;
    int                               nextMinuteRainGraphPointer;
    int                               nextRainStormGraphPointer;
    int                               indexToTheMinuteWithinAnHour;
    int                               nextMonthlyRainGraphPointer;
    int                               nextYearlyRainGraphPointer;
    int                               nextSeasonalRainGraphPointer;
    VantageLogger *                   logger;
};
}
#endif
