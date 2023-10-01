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
#ifndef SUMMARY_RECORD_H
#define SUMMARY_RECORD_H
#include <vector>
#include <string>
#include "Weather.h"
#include "WeatherTypes.h"
#include "Measurement.h"
#include "ArchivePacket.h"

namespace vws {

/*
{
    "summary-report" :
    {
        "type" : "day", "start-date" : "2023-10-10", "end-date" : "2023-10-20",
        "summaries" :
        [
            "summary" :
            {
                "type" : "day", "date" : "2023-10-10",
                "measurements" :
                [
                    {
                        "name" : "Indoor temperature",
                        "extremes" : "min/max",
                        "values" {
                            "average" : 72.2,
                            "minimum" : { "value" : 70.0, "time" : "2023-10-10 00:23:12" }
                            "maximum" : { "value" : 76.0, "time" : "2023-10-10 15:23:12" },
                        }
                    }
                ]
            }
         ],
         "rainfall-by-hour" : [ 2.1, 0.0, 1.1 ]
     }
}
 */
enum class SummaryExtremes {
    NO_EXTREME,
    MAXIMUM_ONLY,
    MINIMUM_ONLY,
    MINIMUM_AND_MAXIMUM
};

template<typename M>
struct MeasurementAverage {
    int            sampleCount;
    Measurement<M> sum;
    Measurement<M> average;

    MeasurementAverage() : sampleCount(0) {}

    void applyMeasurement(const Measurement<M> & value) {
        if (value.isValid()) {
            sampleCount++;
            sum.setValue(sum.getValue() + value.getValue());
            average.setValue(sum.getValue() / static_cast<M>(sampleCount));
        }
    }

    std::string formatJSON() const {
        return average.formatJSON("average");
    }
};

enum class ExtremeType {
    HIGH,
    LOW
};

template<typename M, ExtremeType ET>
struct ExtremeMeasurement {
    ExtremeType    extremeType;
    Measurement<M> extremeValue;
    DateTime       extremeTime;
    void applyMeasurement(DateTime time, const Measurement<M> & value) {
        if (value.isValid()) {
            if (extremeType == ExtremeType::LOW) {
                if (value.getValue() < extremeValue.getValue()) {
                    extremeValue = value;
                    extremeTime = time;
                }
            }
            else {
                if (value.getValue() > extremeValue.getValue()) {
                    extremeValue = value;
                    extremeTime = time;
                }
            }
        }
    }

    std::string formatJSON() const {
        std::string json = "";
        if (extremeType == ExtremeType::LOW) {
            json += "\"minimum\"";
        }
        else {
            json += "\"maximum\"";
        }

        json += " : { " + extremeValue.formatJSON("value") + "\"time\" : " + Weather::formatDateTime(extremeTime) + " }";
        return json;
    }
};

template<typename M, SummaryExtremes SE>
class SummaryMeasurement {
public:
    explicit SummaryMeasurement(const std::string & name) : summaryName(name), extremesUsed(SE) {}

    void applyMeasurement(DateTime measurementTime, const Measurement<M> & measurement) {
        average.applyMeasurement(measurement);
        switch(extremesUsed) {
            case SummaryExtremes::NO_EXTREME:
                break;
            case SummaryExtremes::MAXIMUM_ONLY:
                high.applyMeasurement(measurementTime, measurement);
                break;
            case SummaryExtremes::MINIMUM_ONLY:
                low.applyMeasurement(measurementTime, measurement);
                break;
            case SummaryExtremes::MINIMUM_AND_MAXIMUM:
                high.applyMeasurement(measurementTime, measurement);
                low.applyMeasurement(measurementTime, measurement);
                break;
        }
    }

    void applyMeasurement(DateTime measurementTime, const Measurement<M> & avgMeasurement, const Measurement<M> & extremeMeasurement) {
        average.applyMeasurement(avgMeasurement);
        switch(extremesUsed) {
            case SummaryExtremes::NO_EXTREME:
                break;
            case SummaryExtremes::MAXIMUM_ONLY:
                high.applyMeasurement(measurementTime, extremeMeasurement);
                break;
            case SummaryExtremes::MINIMUM_ONLY:
                low.applyMeasurement(measurementTime, extremeMeasurement);
                break;
            case SummaryExtremes::MINIMUM_AND_MAXIMUM:
                high.applyMeasurement(measurementTime, extremeMeasurement);
                low.applyMeasurement(measurementTime, extremeMeasurement);
                break;
        }
    }

    void applyMeasurement(DateTime measurementTime, const Measurement<M> & avgMeasurement, const Measurement<M> & lowMeasurement, const Measurement<M> & highMeasurement) {
        average.applyMeasurement(avgMeasurement);
        switch(extremesUsed) {
            case SummaryExtremes::NO_EXTREME:
                break;
            case SummaryExtremes::MAXIMUM_ONLY:
                high.applyMeasurement(measurementTime, highMeasurement);
                break;
            case SummaryExtremes::MINIMUM_ONLY:
                low.applyMeasurement(measurementTime, lowMeasurement);
                break;
            case SummaryExtremes::MINIMUM_AND_MAXIMUM:
                high.applyMeasurement(measurementTime, highMeasurement);
                low.applyMeasurement(measurementTime, lowMeasurement);
                break;
        }
    }

    std::string formatJSON() const;

    std::string           summaryName;
    SummaryExtremes       extremesUsed;
    MeasurementAverage<M> average;
    ExtremeMeasurement<M,ExtremeType::HIGH> high;
    ExtremeMeasurement<M,ExtremeType::LOW> low;
};

enum class SummaryPeriod {
    DAY,
    WEEK,
    MONTH,
    YEAR
};

class SummaryRecord {
public:
    SummaryRecord(SummaryPeriod period, DateTime startDate, DateTime endDate);
    virtual ~SummaryRecord();

    /**
     * Apply the archive packet to this summary record if the time falls within the start/end date.
     */
    void applyArchivePacket(const ArchivePacket & archivePacket);
    std::string formatJSON() const;

private:
    //static constexpr int DIR_OF_HIGH_WIND_SPEED_OFFSET = 26;
    //static constexpr int PREVAILING_WIND_DIRECTION_OFFSET = 27;

    SummaryPeriod period;
    DateTime startDate;
    DateTime endDate;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> outsideTemperature;
    Rainfall totalRainfall;
    SummaryMeasurement<Rainfall,SummaryExtremes::MAXIMUM_ONLY> rainfallRate;
    SummaryMeasurement<Pressure,SummaryExtremes::MINIMUM_AND_MAXIMUM> barometer;
    SummaryMeasurement<SolarRadiation,SummaryExtremes::MAXIMUM_ONLY> solarRadiation;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> insideTemperature;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> insideHumidity;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> outsideHumidity;
    SummaryMeasurement<Speed,SummaryExtremes::MAXIMUM_ONLY> sustainedWindSpeed;
    SummaryMeasurement<Speed,SummaryExtremes::MAXIMUM_ONLY> gustWindSpeed;
    SummaryMeasurement<UvIndex,SummaryExtremes::MAXIMUM_ONLY> uvIndex;
    SummaryMeasurement<Rainfall,SummaryExtremes::MAXIMUM_ONLY> et;

    /*
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> extraTemperatures[ArchivePacket::MAX_EXTRA_TEMPERATURES];
    SummaryMeasurement<Humidity,SummaryExtremes::MINIMUM_AND_MAXIMUM> extraHumidities[ArchivePacket::MAX_EXTRA_HUMIDITIES];
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> leafTemperatures[ArchivePacket::MAX_LEAF_TEMPERATURES];
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> soilTemperatures[ArchivePacket::MAX_SOIL_TEMPERATURES];
    SummaryMeasurement<LeafWetness,SummaryExtremes::MINIMUM_AND_MAXIMUM> leafWetnesses[ArchivePacket::MAX_LEAF_WETNESSES];
    SummaryMeasurement<SoilMoisture,SummaryExtremes::MINIMUM_AND_MAXIMUM> soilMoistures[ArchivePacket::MAX_SOIL_MOISTURES];
    */
};

class ArchiveManager;

/**
 *
 */
class SummaryReport {
public:
    SummaryReport(SummaryPeriod period, DateTime startDate, DateTime endDate, ArchiveManager & archiveManager);
    virtual ~SummaryReport();

    void setParameters(SummaryPeriod period, DateTime startDate, DateTime endDate);
    void loadData();

    std::string formatJSON() const;
private:
    static DateTime normalizeStartTime(DateTime time, SummaryPeriod period);
    static DateTime normalizeEndTime(DateTime startTime, DateTime endTime, SummaryPeriod period);
    static DateTime calculateMidnight(DateTime time);
    static DateTime calculateLastSecondOfDay(DateTime time);
    static DateTime calculateEndTime(DateTime startTime, SummaryPeriod period);
    static DateTime incrementStartTime(DateTime time, SummaryPeriod period);

    SummaryPeriod period;
    DateTime startDate;
    DateTime endDate;
    ArchiveManager & archiveManager;
    std::vector<SummaryRecord> summaryRecords;
    Rainfall hourRainfallBuckets[24];            // Tracks when it has rained over the summary period
};

} /* namespace vws */

#endif /* SUMMARY_RECORD_H */
