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
#ifndef SUMMARY_RECORD_H
#define SUMMARY_RECORD_H
#include <vector>
#include <string>
#include <sstream>
#include "Weather.h"
#include "WeatherTypes.h"
#include "Measurement.h"
#include "ArchivePacket.h"
#include "WindRoseData.h"
#include "SummaryEnums.h"

namespace vws {
class VantageLogger;
class ArchiveManager;

// TODO As a general capability, any measurements and statistics need to be qualified based on whether the weather
// station has the appropriate sensor. That is, there should be no solar radiation values in the JSON if the weather
// station does not have a solar radiation sensor.

// TODO Figure out how to work in extra temperatures, extra humidities, and soil/moisture value

/**
 * Template class to calculate the average value of a measurement.
 */
template<typename M>
class MeasurementAverage {
public:

    /**
     * Constructor.
     */
    MeasurementAverage(bool useZeroValues = true) : useZeroValues(useZeroValues), sampleCount(0) {}

    /**
     * Apply a single measurement to the average.
     *
     * @param value The value to be applied to the value
     */
    void applyMeasurement(const Measurement<M> & value) {
        //
        // Ignore invalid values
        //
        if (value.isValid() && (value.getValue() != 0.0 || useZeroValues)) {
            sampleCount++;
            sum.setValue(sum.getValue() + value.getValue());
            average.setValue(sum.getValue() / static_cast<M>(sampleCount));
        }
    }

    /**
     * Format the average value into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON(const std::string jsonName) const {
        return "\"" + jsonName + "\" : { " + average.formatJSON("value") + " }";
    }

    bool           useZeroValues; // Whether to use zero values in the average calculation
    int            sampleCount;   // The number of valid measurements applied
    Measurement<M> sum;           // The sum of the valid measurements
    Measurement<M> average;       // The average that is calculated after each valid measurement is applied

};

/**
 * Template to hold an extreme value for a summary measurement.
 */
template<typename M, SummaryExtremeType ET>
struct ExtremeMeasurement {
    const SummaryExtremeType extremeType;  // The type of extreme value being tracked
    Measurement<M>           extremeValue; // The most extreme measurement applied
    DateTime                 extremeTime;  // The time stamp of the extreme measurement

    /**
     * Constructor.
     */
    ExtremeMeasurement() : extremeType(ET), extremeTime(0) {}

    /**
     * Apply a single measurement to this extreme measurement
     *
     * @param time  The time of this measurement
     * @param value The measurement
     */
    void applyMeasurement(DateTime time, const Measurement<M> & value) {
        //
        // Ignore the measurement if it is not valid
        //
        if (value.isValid()) {
            //
            // Perform checks depending if this extreme measurement is tracking a high or low value
            //
            if (extremeType == SummaryExtremeType::LOW) {
                if (!extremeValue.isValid() || value.getValue() < extremeValue.getValue()) {
                    extremeValue = value;
                    extremeTime = time;
                }
            }
            else {
                if (!extremeValue.isValid() || value.getValue() > extremeValue.getValue()) {
                    extremeValue = value;
                    extremeTime = time;
                }
            }
        }
    }

    /**
     * Ostream operator.
     *
     * @param os  The ostream
     * @param em  The extreme measurement to be output on the stream
     * @return The ostream passed in
     */
    friend std::ostream & operator<<(std::ostream & os,  const ExtremeMeasurement & em) {
        std::string etype = em.extremeType == SummaryExtremeType::LOW ? "Low" : "High";
        os << "Extreme type: " << etype
           << " Value: " << em.extremeValue << " Time: " << Weather::formatDateTime(em.extremeTime);

        return os;
    }

    /**
     * Create a string in JSON format that represents this extreme value.
     *
     * @return The JSON string
     */
    std::string formatJSON() const {
        std::string json = "";

        //
        // If there have been no valid measurement applied to this measurement, just return an empty string
        //
        if (extremeValue.isValid()) {
            if (extremeType == SummaryExtremeType::LOW) {
                json += "\"minimum\"";
            }
            else {
                json += "\"maximum\"";
            }

            json += " : { " + extremeValue.formatJSON("value") + ", " + "\"time\" : \"" + Weather::formatDateTime(extremeTime) + "\" }";
        }

        return json;
    }
};

/**
 * Template class that represents a single measurement within a summary record.
 */
// TODO Add average high and average low to the SummaryMeasurement. This could be hard because we do not separate the
// values into days in order to figure the high and low for the day. Also average high and average low do not make sense
// when the period is Day.
template<typename M, SummaryExtremes SE>
class SummaryMeasurement {
public:
    /**
     * Constructor.
     */
    SummaryMeasurement() : summaryName(""), extremesUsed(SE) {}

    /**
     * Constructor with a name.
     *
     * @param name The name used when generating reports
     */
    explicit SummaryMeasurement(const std::string & name) : summaryName(name), extremesUsed(SE) {
    }

    /**
     * Set the name used in report post-constuctions.
     *
     * @param name The name used when generating reports
     */
    void setSummaryName(const std::string & name) {
        summaryName = name;
    }

    /**
     * Apply a single measurement.
     *
     * @param measurementTime The time of this measurement
     * @param measurement     The value to apply to average, minimum and maximum
     */
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

    /**
     * Apply an average and an extreme measurement.
     *
     * @param measurementTime     The time of this measurement
     * @param avgMeasurement      A measurement that represent the average value over the archive packets time span
     * @param extremeMeasurement  A measurement that represent the extreme value over the archive packets time span.
     *                            This could be a minimum or maximum or both.
     */
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

    /**
     * Apply a an average, a minimum and a maximum measurement.
     *
     * @param measurementTime The time of this measurement
     * @param avgMeasurement  A measurement that represent the average value over the archive packets time span
     * @param minMeasurement  A measurement that represent the minimum extreme value over the archive packets time span.
     * @param maxMeasurement  A measurement that represent the maximum extreme value over the archive packets time span.
     */
    void applyMeasurement(DateTime measurementTime, const Measurement<M> & avgMeasurement, const Measurement<M> & minMeasurement, const Measurement<M> & maxMeasurement) {
        average.applyMeasurement(avgMeasurement);

        switch(extremesUsed) {
            case SummaryExtremes::NO_EXTREME:
            case SummaryExtremes::MAXIMUM_ONLY:
            case SummaryExtremes::MINIMUM_ONLY:
                break;

            case SummaryExtremes::MINIMUM_AND_MAXIMUM:
                high.applyMeasurement(measurementTime, maxMeasurement);
                low.applyMeasurement(measurementTime, minMeasurement);
                break;
        }
    }

    /**
     * Apply the extreme values for a given day and apply them to the average day high and day low of this measurement.
     *
     * @param highMeasurement The high measurement for a given day
     * @param lowMeasurement  The low measurement for a given day
     */
    void applyDayExtremes(const Measurement<M> & highMeasurement, const Measurement<M> & lowMeasurement) {
        averageDayHigh.applyMeasurement(highMeasurement);
        averageDayLow.applyMeasurement(lowMeasurement);
    }

    /**
     * Format the summary measurement into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON(bool addLeadingComma) const {
        std::stringstream ss;

        if (average.sampleCount == 0)
            return "";

        if (addLeadingComma)
            ss << ", ";

        ss << "\"" <<  summaryName << "\" : { " << average.formatJSON("average");

        if (extremesUsed == SummaryExtremes::MINIMUM_ONLY || extremesUsed == SummaryExtremes::MINIMUM_AND_MAXIMUM)
           ss << ", " << low.formatJSON();

        if (extremesUsed == SummaryExtremes::MAXIMUM_ONLY || extremesUsed == SummaryExtremes::MINIMUM_AND_MAXIMUM)
           ss << ", " << high.formatJSON();

        if (averageDayHigh.sampleCount > 0)
            ss << ", " << averageDayHigh.formatJSON("averageDayHigh");

        if (averageDayLow.sampleCount > 0)
            ss << ", " << averageDayLow.formatJSON("averageDayLow");

        ss << " }" << std::endl;

        return ss.str();
    }

    std::string                                    summaryName;
    const SummaryExtremes                          extremesUsed;
    MeasurementAverage<M>                          average;
    ExtremeMeasurement<M,SummaryExtremeType::HIGH> high;
    MeasurementAverage<M>                          averageDayHigh;
    ExtremeMeasurement<M,SummaryExtremeType::LOW>  low;
    MeasurementAverage<M>                          averageDayLow;
};

/**
 * A record that represents a summary for a single period of time.
 */
class SummaryRecord {
public:
    /**
     * Contructor.
     *
     * @param period    The period of time that this record represents
     * @param startDate The starting date that this record represents
     * @param endDate   The end date of this record. Note that if the period is Day, then start and end must be equal
     */
    SummaryRecord(SummaryPeriod period, DateTime startDate, DateTime endDate);

    /**
     * Destructor.
     */
    virtual ~SummaryRecord();

    /**
     * Apply the archive packet to this summary record if the time falls within the start/end date.
     *
     * @param archivePacket The packet to apply
     */
    void applyArchivePacket(const ArchivePacket & archivePacket);

    /**
     * Apply a summary record with a period of DAY to a summary record
     *
     * @param daySummary The summary record for a day
     */
    void applyDaySummaryRecord(const SummaryRecord  & daySummary);

    /**
     * Format the summary record into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON() const;

    template<typename M, SummaryExtremes SE>
    std::string arrayFormatJSON(const std::string & name, const SummaryMeasurement<M,SE> sm[], int numSummaries) const;


    int           packetCount;
    SummaryPeriod period;
    DateTime      startDate;
    DateTime      endDate;
    Rainfall      totalRainfall;

    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  outsideTemperature;
    MeasurementAverage<Temperature>                                       averageHighOutdoorTemperature;
    MeasurementAverage<Temperature>                                       averageLowOutdoorTemperature;
    SummaryMeasurement<Rainfall,SummaryExtremes::MAXIMUM_ONLY>            rainfallRate;
    SummaryMeasurement<Pressure,SummaryExtremes::MINIMUM_AND_MAXIMUM>     barometer;
    SummaryMeasurement<SolarRadiation,SummaryExtremes::MAXIMUM_ONLY>      solarRadiation;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  insideTemperature;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  insideHumidity;
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  outsideHumidity;
    SummaryMeasurement<Speed,SummaryExtremes::MAXIMUM_ONLY>               sustainedWindSpeed;
    SummaryMeasurement<Speed,SummaryExtremes::MAXIMUM_ONLY>               gustWindSpeed;
    SummaryMeasurement<UvIndex,SummaryExtremes::MAXIMUM_ONLY>             uvIndex;
    SummaryMeasurement<Rainfall,SummaryExtremes::MAXIMUM_ONLY>            et;

    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  extraTemperatures[ArchivePacket::MAX_EXTRA_TEMPERATURES];
    SummaryMeasurement<Humidity,SummaryExtremes::MINIMUM_AND_MAXIMUM>     extraHumidities[ArchivePacket::MAX_EXTRA_HUMIDITIES];
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  leafTemperatures[ArchivePacket::MAX_LEAF_TEMPERATURES];
    SummaryMeasurement<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM>  soilTemperatures[ArchivePacket::MAX_SOIL_TEMPERATURES];
    SummaryMeasurement<LeafWetness,SummaryExtremes::MINIMUM_AND_MAXIMUM>  leafWetnesses[ArchivePacket::MAX_LEAF_WETNESSES];
    SummaryMeasurement<SoilMoisture,SummaryExtremes::MINIMUM_AND_MAXIMUM> soilMoistures[ArchivePacket::MAX_SOIL_MOISTURES];

    VantageLogger & logger;
};

/**
 * Statistics
 *
 * TODO Added average high and average low
 */
template<typename M, SummaryExtremes SE>
class MeasurementStatistics {
public:
    MeasurementStatistics(const std::string & name, bool low, bool includeZero, bool range) : name(name),
                                                                                              useLowValue(low),
                                                                                              includeZeroValuesInAverage(includeZero),
                                                                                              computeRange(range),
                                                                                              highValue(0.0),
                                                                                              highValueTime(0),
                                                                                              minimumDayHighValue(0.0),
                                                                                              minimumDayHighValueDate(0),
                                                                                              lowValue(0.0),
                                                                                              lowValueTime(0),
                                                                                              maximumDayLowValue(0.0),
                                                                                              maximumDayLowValueDate(0),
                                                                                              averageSamples(0),
                                                                                              sum(0.0),
                                                                                              average(0.0),
                                                                                              highAverageDayValue(0.0),
                                                                                              highAverageDayDate(0),
                                                                                              lowAverageDayValue(0.0),
                                                                                              lowAverageDayDate(0),
                                                                                              minimumRange(0),
                                                                                              minimumRangeDate(0),
                                                                                              maximumRange(0),
                                                                                              maximumRangeDate(0) {
    }

    void applySummaryMeasurement(DateTime summaryDate, const SummaryMeasurement<M,SE> & summaryMeasurement) {
        if (!summaryMeasurement.high.extremeValue.isValid())
            return ;

        if (summaryMeasurement.high.extremeValue > highValue || highValueTime == 0) {
            highValue = summaryMeasurement.high.extremeValue;
            highValueTime = summaryMeasurement.high.extremeTime;
        }

        if (summaryMeasurement.high.extremeValue < minimumDayHighValue || minimumDayHighValueDate == 0) {
            minimumDayHighValue = summaryMeasurement.high.extremeValue;
            minimumDayHighValueDate = summaryDate;
        }

        if (summaryMeasurement.low.extremeValue < lowValue || lowValueTime == 0) {
            lowValue = summaryMeasurement.low.extremeValue;
            lowValueTime = summaryMeasurement.low.extremeTime;
        }

        if (summaryMeasurement.low.extremeValue > maximumDayLowValue || maximumDayLowValueDate == 0) {
            maximumDayLowValue = summaryMeasurement.low.extremeValue;
            maximumDayLowValueDate = summaryDate;
        }

        averageSamples += summaryMeasurement.average.sampleCount;
        sum += summaryMeasurement.average.sum;
        average = sum / static_cast<M>(averageSamples);

        if (summaryMeasurement.average.average > highAverageDayValue || highAverageDayDate == 0) {
            highAverageDayValue = summaryMeasurement.average.average;
            highAverageDayDate = summaryDate;
        }

        if (summaryMeasurement.average.average < lowAverageDayValue || lowAverageDayDate == 0) {
            lowAverageDayValue = summaryMeasurement.average.average;
            lowAverageDayDate = summaryDate;
        }

        M range = summaryMeasurement.high.extremeValue - summaryMeasurement.low.extremeValue;
        if (range < minimumRange || minimumRangeDate == 0) {
            minimumRange = range;
            minimumRangeDate = summaryDate;
        }

        if (range > maximumRange || maximumRangeDate == 0) {
            maximumRange = range;
            maximumRangeDate = summaryDate;
        }
    }

    std::string formatJSON() const {
        std::ostringstream ss;
        ss << "\"" << name << "\"" << " : { " << std::endl;

       ss  << "\"high\" : { " << std::endl
           << "\"maximum\" :    { \"value\" : " << highValue << ", \"time\" : \"" << Weather::formatDateTime(highValueTime) << "\" }, " << std::endl
           << "\"dayMinimum\" : { \"value\" : " << minimumDayHighValue << ", \"date\" : \"" << Weather::formatDate(minimumDayHighValueDate) << "\" } " << std::endl
           << " }, " << std::endl;

       if (useLowValue) {
           ss  << "\"low\" : { " << std::endl
               << " \"minimum\" : { \"value\" : " << lowValue << ", \"time\" : \"" << Weather::formatDateTime(lowValueTime) << "\" }," << std::endl
               << " \"dayMaximum\" : { \"value\" : " << maximumDayLowValue << ", \"date\" : \"" << Weather::formatDate(maximumDayLowValueDate) << "\" }" << std::endl
               << " }," << std::endl;
       }

       ss << "\"averages\" : {" << std::endl
          << "\"average\" : " << average << "," << std::endl
          << "\"high\" : { \"value\" : " << highAverageDayValue << ", \"date\" : \"" << Weather::formatDate(highAverageDayDate) << "\"}," << std::endl
          << "\"low\" : { \"value\" : " << lowAverageDayValue << ", \"date\" : \"" << Weather::formatDate(lowAverageDayDate) << "\"}" << std::endl
          << "}";

       if (computeRange) {
           ss << "," << std::endl << "\"ranges\" : { " << std::endl
              << "\"smallest\" : " << "{ \"range\" : " << minimumRange << ", \"date\" : \""  << Weather::formatDate(minimumRangeDate) << "\" }, " << std::endl
              << "\"largest\" : "  << "{ \"range\" : " << maximumRange << ", \"date\" : \""  << Weather::formatDate(maximumRangeDate) << "\" } " << std::endl
              << " } ";
       }

       ss << std::endl << "}" << std::endl;

       return ss.str();
    }

private:
    std::string name;
    bool useLowValue;
    bool includeZeroValuesInAverage;
    bool computeRange;

    M highValue;
    DateTime highValueTime;

    M minimumDayHighValue;
    DateTime minimumDayHighValueDate;

    // All but solar radiation, UV index
    M lowValue;
    DateTime lowValueTime;

    M maximumDayLowValue;
    DateTime maximumDayLowValueDate;

    // Do zero values count for averages?
    int averageSamples;
    M sum;
    M average;

    M highAverageDayValue;
    DateTime highAverageDayDate;

    M lowAverageDayValue;
    DateTime lowAverageDayDate;

    // Ranges for temperature, humidity, barometer,
    M minimumRange;
    DateTime minimumRangeDate;

    M maximumRange;
    DateTime maximumRangeDate;

};

/*
   statistics : {
       totalDays: 100,
       outsideTemperature : { // inside temperature, outsideHumidity, insideHumidity
           high: {
               maximum : { value: 95.0, time: "2024-02-15 13:25:25" },
               dayMinimum: { value: 95.0, date: "2024-02-15" }
           },
           low: {
               minimum: { value: 32, time: "2024-02-10 01:15:12" },
               dayMaximum: { value: 95.0, date: "2024-02-15" }
           },
           averages: {
               average: 65,
               high: { value: 68, date: "2024-02-23" },
               low: { value: 63, date: "2024-02-02" }
           },
           ranges: {
               smallest: { range: 2.5, date: "2024-01-02" },
               largest:  { range: 30.2, date: "2024-01029" }
           }
       },
       solarRadiation : { // uvIndex
           high: {
               { value: 95.0, time: "2024-02-15 13:25:25" },
               { dayMinimum: 95.0, date: "2024-02-15" }
           },
           averages: {
               { average: 65, date: "2024-01-17" },
               { high: 68, date: "2024-02-23" }
           }
        },
        rain : {
            rainDays: 10,
            totalRain: 5.5,
            averageRainPerDay : .04,
            averageRainPerRainyDay : .1,
            highRainDay: { value: 2.5, date: "2024-03-12" },
            highRainRate { value: 1.5, time: "2024-03-12 10:10:11" }
        }

 */
class SummaryStatistics {
public:
    SummaryStatistics();
    void applySummaryRecord(const SummaryRecord & record);
    std::string formatJSON() const;

private:
    int totalDays;

    int rainDays;
    Rainfall     totalRainfall;
    Rainfall     highDayRainfall;
    DateTime     highDayRainfallDate;
    RainfallRate highDayRainfallRate;
    DateTime     highDayRainfallRateTime;

    MeasurementStatistics<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> outsideTemperature; // true, true, true
    MeasurementStatistics<Temperature,SummaryExtremes::MINIMUM_AND_MAXIMUM> insideTemperature;  // true, true, true
    MeasurementStatistics<Humidity,SummaryExtremes::MINIMUM_AND_MAXIMUM> outsideHumidity;       // true, true, true
    MeasurementStatistics<Humidity,SummaryExtremes::MINIMUM_AND_MAXIMUM> insideHumidity;        // true, true, true
    MeasurementStatistics<SolarRadiation,SummaryExtremes::MAXIMUM_ONLY> solarRadiation;         // false, false, false
    MeasurementStatistics<UvIndex,SummaryExtremes::MAXIMUM_ONLY> uvIndex;                       // false, false, false
    MeasurementStatistics<Speed,SummaryExtremes::MAXIMUM_ONLY> windSpeed;                       // true, true, false
    MeasurementStatistics<Speed,SummaryExtremes::MAXIMUM_ONLY> windGust;                        // true, true, false
    MeasurementStatistics<Evapotranspiration,SummaryExtremes::MAXIMUM_ONLY> et;                 // false, false, false
    MeasurementStatistics<Evapotranspiration,SummaryExtremes::MINIMUM_AND_MAXIMUM> barometer;   // true, true, true
};

/**
 * Holder of a summary report.
 */
class SummaryReport {
public:
    /**
     * Constructor.
     *
     * @param period         The time period that each summary record represents
     * @param startDate      The date on which this summary report starts
     * @param endDate        The date on which this summary report ends
     * @param archiveManager The manager used to retrieve the ArchivePackets needed to build the summary report
     * @param wrd            The WindRoseData object used to analyze wind direction and speed data
     */
    SummaryReport(SummaryPeriod period, const DateTimeFields & startDate, const DateTimeFields & endDate, ArchiveManager & archiveManager, WindRoseData & wrd);

    /**
     * Destructor.
     */
    virtual ~SummaryReport();

    /**
     * Load the data from the archive into the summary report.
     *
     * @return True if the data was retrieved successfully
     */
    bool loadData();

    /**
     * Format the report into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON() const;

private:
    static DateTime normalizeStartTime(DateTime time, SummaryPeriod period);
    static DateTime normalizeEndTime(DateTime endTime, SummaryPeriod period);
    static DateTime calculateMidnight(DateTime time);
    static DateTime calculateLastSecondOfDay(DateTime time);
    static DateTime calculateEndTime(DateTime startTime, SummaryPeriod period);
    static DateTime incrementStartTime(DateTime time, SummaryPeriod period);

    SummaryPeriod              period;
    DateTime                   startDate;
    DateTime                   endDate;
    ArchiveManager &           archiveManager;
    std::vector<SummaryRecord> summaryRecords;
    Rainfall                   hourRainfallBuckets[24];   // Tracks when it has rained over the summary period
    WindRoseData &             windRoseData;
    SummaryStatistics          summaryStatistics;
    VantageLogger &            logger;
};

} /* namespace vws */

#endif /* SUMMARY_REPORT_H */
