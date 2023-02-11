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
#ifndef VANTAGE_DECODER_H
#define VANTAGE_DECODER_H

#include "Measurement.h"
#include "VantageEepromConstants.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "WeatherTypes.h"

namespace vws {

/**
 * Class that contains static methods for converting bytes in a buffer into measurement values
 */
class VantageDecoder {
public:
    /**
     * Decode a Vantage 16 bit temperature.
     *
     * @param buffer The buffer from which to decode the temperature
     * @param offset The offset into "buffer" from which to decode the temperature
     *
     * @return The converted measurement value along with a valid flag
     */
    static Measurement<Temperature> decode16BitTemperature(const byte buffer[], int offset, bool scaleValue = true);

    static Measurement<Temperature> decode8BitTemperature(const byte buffer[], int offset);


    static Measurement<Humidity> decodeHumidity(const byte buffer[], int offset);

    static Measurement<Pressure> decodeBarometricPressure(const byte buffer[], int offset);

    static Measurement<UvIndex> decodeUvIndex(const byte buffer[], int offset);

    static Measurement<Evapotranspiration> decodeArchiveET(const byte buffer[], int offset);

    static Measurement<Evapotranspiration> decodeDayET(const byte buffer[], int offset);

    static Measurement<Evapotranspiration> decodeMonthYearET(const byte buffer[], int offset);

    static Measurement<SolarRadiation> decodeSolarRadiation(const byte buffer[], int offset);

    static Measurement<Speed> decodeWindSpeed(const byte buffer[], int offset);

    static Measurement<Speed> decode16BitWindSpeed(const byte buffer[], int offset);

    static Measurement<Speed> decodeAverageWindSpeed(const byte buffer[], int offset);

    static Measurement<Heading> decodeWindDirectionSlice(const byte buffer[], int offset);

    static Measurement<Heading> decodeWindDirection(const byte buffer[], int offset);

    static Rainfall decodeStormRain(const byte buffer[], int offset);

    static void setRainCollectorSize(Rainfall collectorSize);

    static Rainfall decodeRain(const byte buffer[], int offset);

    static DateTime decodeStormStartDate(const byte buffer[], int offset);

    static float decodeConsoleBatteryVoltage(const byte buffer[], int offset);

    static Measurement<LeafWetness> decodeLeafWetness(const byte buffer[], int offset);

    static Measurement<SoilMoisture> decodeSoilMoisture(const byte buffer[], int offset);
    

    static DateTime decodeTime(const byte buffer[], int offset);
    static DateTime decodeDate(const byte buffer[], int offset);
    static DateTime decodeDateTime(const byte buffer[], int dateOffset, int timeOffset);

    struct SensorStationData {
        VantageEepromConstants::RepeaterId        repeaterId;
        VantageEepromConstants::SensorStationType stationType;
        int                                       extraHumidityIndex;     // Index 1 - 8
        int                                       extraTemperatureIndex;  // Index 0 - 7
    };

    static void decodeSensorStationList(const byte buffer[], int offset, SensorStationData stationList[ProtocolConstants::MAX_STATIONS]);

private:
    static Rainfall rainCollectorSizeInches;
    static bool     rainCollectorSizeSet;

    VantageDecoder();
    ~VantageDecoder();
    VantageDecoder(const VantageDecoder &);
};

}

#endif
