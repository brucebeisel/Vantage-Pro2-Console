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
#include "SummaryReport.h"

#include <sstream>
#include "ArchivePacket.h"
#include "ArchiveManager.h"
#include "WindRoseData.h"
#include "VantageEnums.h"
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryRecord::SummaryRecord(SummaryPeriod period, DateTime startDate, DateTime endDate) : period(period),
                                                                                           startDate(startDate),
                                                                                           endDate(endDate),
                                                                                           packetCount(0),
                                                                                           totalRainfall(0.0),
                                                                                           outsideTemperature("outsideTemperature"),
                                                                                           outsideHumidity("outsideHumidity"),
                                                                                           solarRadiation("solarRadiation"),
                                                                                           rainfallRate("highRainfallRate"),
                                                                                           barometer("barometer"),
                                                                                           insideTemperature("insideTemperature"),
                                                                                           insideHumidity("insideHumidity"),
                                                                                           sustainedWindSpeed("sustainedWindSpeed"),
                                                                                           gustWindSpeed("windGustSpeed"),
                                                                                           uvIndex("uvIndex"),
                                                                                           et("evapotranspiration"),
                                                                                           logger(VantageLogger::getLogger("SummaryRecord")) {


    for (int i = 0; i < ArchivePacket::MAX_EXTRA_TEMPERATURES; i++)
        extraTemperatures[i].setSummaryName("extraTemperature" + std::to_string(i));

    for (int i = 0; i < ArchivePacket::MAX_EXTRA_HUMIDITIES; i++)
        extraHumidities[i].setSummaryName("extraHumidity" + std::to_string(i));

    for (int i = 0; i < ArchivePacket::MAX_LEAF_TEMPERATURES; i++)
        leafTemperatures[i].setSummaryName("leafTemperature" + std::to_string(i));

    for (int i = 0; i < ArchivePacket::MAX_SOIL_TEMPERATURES; i++)
        soilTemperatures[i].setSummaryName("soilTemperature" + std::to_string(i));

    for (int i = 0; i < ArchivePacket::MAX_LEAF_WETNESSES; i++)
        leafWetnesses[i].setSummaryName("leafWetness" + std::to_string(i));

    for (int i = 0; i < ArchivePacket::MAX_SOIL_MOISTURES; i++)
        soilMoistures[i].setSummaryName("soilMoisture" + std::to_string(i));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryRecord::~SummaryRecord() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SummaryRecord::applyArchivePacket(const ArchivePacket & archivePacket) {
    DateTime packetTime = archivePacket.getEpochDateTime();

    // Note, this logger and the one below are commented out due to the impact on performance
    //logger.log(VantageLogger::VANTAGE_DEBUG2) << "Checking time of " << Weather::formatDateTime(packetTime)
    //                                          << " against SummaryRecord time range: " << Weather::formatDateTime(startDate)
    //                                          << " to " << Weather::formatDateTime(endDate) << endl;

    if (packetTime < startDate || packetTime > endDate) {
        //logger.log(VantageLogger::VANTAGE_DEBUG3) << "------Ignoring packet" << endl;
        return;
    }

    packetCount++;

    outsideTemperature.applyMeasurement(packetTime,
                                        archivePacket.getAverageOutsideTemperature(),
                                        archivePacket.getLowOutsideTemperature(),
                                        archivePacket.getHighOutsideTemperature());

    outsideHumidity.applyMeasurement(packetTime, archivePacket.getOutsideHumidity());
    solarRadiation.applyMeasurement(packetTime, archivePacket.getAverageSolarRadiation(), archivePacket.getHighSolarRadiation());
    insideTemperature.applyMeasurement(packetTime, archivePacket.getInsideTemperature());
    rainfallRate.applyMeasurement(packetTime, archivePacket.getHighRainfallRate());
    barometer.applyMeasurement(packetTime, archivePacket.getBarometricPressure());
    insideHumidity.applyMeasurement(packetTime, archivePacket.getInsideHumidity());
    sustainedWindSpeed.applyMeasurement(packetTime, archivePacket.getAverageWindSpeed());
    gustWindSpeed.applyMeasurement(packetTime, archivePacket.getHighWindSpeed());
    uvIndex.applyMeasurement(packetTime, archivePacket.getAverageUvIndex(), archivePacket.getHighUvIndex());
    et.applyMeasurement(packetTime, archivePacket.getEvapotranspiration());
    totalRainfall += archivePacket.getRainfall();

    for (int i = 0; i < ArchivePacket::MAX_EXTRA_TEMPERATURES; i++)
        extraTemperatures[i].applyMeasurement(packetTime, archivePacket.getExtraTemperature(i));

    for (int i = 0; i < ArchivePacket::MAX_EXTRA_HUMIDITIES; i++)
        extraHumidities[i].applyMeasurement(packetTime, archivePacket.getExtraHumidity(i));

    for (int i = 0; i < ArchivePacket::MAX_LEAF_TEMPERATURES; i++)
        leafTemperatures[i].applyMeasurement(packetTime, archivePacket.getLeafTemperature(i));

    for (int i = 0; i < ArchivePacket::MAX_SOIL_TEMPERATURES; i++)
        soilTemperatures[i].applyMeasurement(packetTime, archivePacket.getSoilTemperature(i));

    for (int i = 0; i < ArchivePacket::MAX_LEAF_WETNESSES; i++)
        leafWetnesses[i].applyMeasurement(packetTime, archivePacket.getLeafWetness(i));

    for (int i = 0; i < ArchivePacket::MAX_SOIL_MOISTURES; i++)
        soilMoistures[i].applyMeasurement(packetTime, archivePacket.getSoilMoisture(i));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename M, SummaryExtremes SE>
std::string SummaryRecord::arrayFormatJSON(const std::string & name, const SummaryMeasurement<M,SE> sm[], int numSummaries) const {
    std::stringstream ss;

    ss << "\"" << name << "\" : [ ";

    bool first = true;

    for (int i = 0; i < numSummaries; i++) {
        string s = sm[i].formatJSON(false);

        if (s.length() > 0) {
            if (!first) ss << ", "; else first = false;

            ss << " { " << s << " } ";
        }
    }

    ss << " ]";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
SummaryRecord::formatJSON() const {
    std::stringstream ss;
    ss << " { \"type\" : \"" << summaryPeriodEnum.valueToString(period) <<  "\", "
       << "\"startDate\" : \"" << Weather::formatDate(startDate) << "\", "
       << "\"endDate\" : \"" << Weather::formatDate(endDate) << "\"";
    if (packetCount != 0) {
        ss << outsideTemperature.formatJSON(true)
           << outsideHumidity.formatJSON(true)
           << solarRadiation.formatJSON(true)
           << insideTemperature.formatJSON(true)
           << insideHumidity.formatJSON(true)
           << barometer.formatJSON(true)
           << rainfallRate.formatJSON(true)
           << uvIndex.formatJSON(true)
           << et.formatJSON(true)
           << sustainedWindSpeed.formatJSON(true)
           << gustWindSpeed.formatJSON(true)
           << ", \"rainfall\" : { \"total\" : { \"value\" : " << totalRainfall << " } }, ";

        ss << arrayFormatJSON("extraTemperatures", extraTemperatures, ArchivePacket::MAX_EXTRA_TEMPERATURES) << ", "
           << arrayFormatJSON("extraHumidities", extraHumidities, ArchivePacket::MAX_EXTRA_HUMIDITIES) << ", "
           << arrayFormatJSON("leafTemperatures", leafTemperatures, ArchivePacket::MAX_LEAF_TEMPERATURES) << ", "
           << arrayFormatJSON("soilTemperatures", soilTemperatures, ArchivePacket::MAX_SOIL_TEMPERATURES) << ", "
           << arrayFormatJSON("leafWetnesses", leafWetnesses, ArchivePacket::MAX_LEAF_WETNESSES) << ", "
           << arrayFormatJSON("soilMoistures", soilMoistures, ArchivePacket::MAX_SOIL_MOISTURES);
    }

    ss << " }";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryReport::SummaryReport(SummaryPeriod period,
                             const DateTimeFields & start,
                             const DateTimeFields & end,
                             ArchiveManager & archiveManager,
                             WindRoseData & wrd) : period(period),
                                                   startDate(start.getEpochDateTime()),
                                                   endDate(end.getEpochDateTime()),
                                                   archiveManager(archiveManager),
                                                   windRoseData(wrd),
                                                   logger(VantageLogger::getLogger("SummaryRecord")) {
    //
    // Set the start and end times to the start and end of the days
    //
    startDate = normalizeStartTime(startDate, period);
    endDate = normalizeEndTime(endDate, period);

    struct tm tm;
    localtime_r(&endDate, &tm);
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    endDate = timelocal(&tm);

    for (int i = 0; i < 24; i++)
        hourRainfallBuckets[i] = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryReport::~SummaryReport() {
    for (int i = 0; i < 24; i++)
        hourRainfallBuckets[i] = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::normalizeStartTime(DateTime startTime, SummaryPeriod period) {
    DateTime normalizedTime;
    struct tm tm;

    switch (period) {
        case SummaryPeriod::DAY:
            normalizedTime = calculateMidnight(startTime);
            break;

        case SummaryPeriod::WEEK:
            localtime_r(&startTime, &tm);
            tm.tm_mday -= tm.tm_wday;
            normalizedTime = calculateMidnight(mktime(&tm));
            break;
            break;

        case SummaryPeriod::MONTH:
            localtime_r(&startTime, &tm);
            tm.tm_mday = 1;
            normalizedTime = calculateMidnight(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&startTime, &tm);
            tm.tm_mon = 0;
            tm.tm_mday = 1;
            normalizedTime = calculateMidnight(mktime(&tm));
            break;
    }

    //cout << "Normalized start time from " << Weather::formatDateTime(startTime) << " to " << Weather::formatDateTime(normalizedTime) << endl;
    return normalizedTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::normalizeEndTime(DateTime endTime, SummaryPeriod period) {
    DateTime normalizedTime;
    struct tm tm;

    switch (period) {
        case SummaryPeriod::DAY:
            normalizedTime = calculateLastSecondOfDay(endTime);
            break;

        case SummaryPeriod::WEEK:
            localtime_r(&endTime, &tm);
            tm.tm_mday += (6 - tm.tm_wday);
            normalizedTime = calculateLastSecondOfDay(mktime(&tm));
            break;

        case SummaryPeriod::MONTH:
            localtime_r(&endTime, &tm);
            tm.tm_mon += 1;
            tm.tm_mday = 0;
            normalizedTime = calculateLastSecondOfDay(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&endTime, &tm);
            tm.tm_year += 1;
            tm.tm_mon = 0;
            tm.tm_mday = 0;
            normalizedTime = calculateLastSecondOfDay(mktime(&tm));
            break;
    }

    //cout << "Normalized end time from " << Weather::formatDateTime(endTime) << " to " << Weather::formatDateTime(normalizedTime) << endl;

    return normalizedTime;

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::calculateMidnight(DateTime time) {
    struct tm tm;
    localtime_r(&time, &tm);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return timelocal(&tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::calculateLastSecondOfDay(DateTime time) {
    struct tm tm;
    localtime_r(&time, &tm);
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    return timelocal(&tm);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::calculateEndTime(DateTime startTime, SummaryPeriod period) {
    DateTime endTime;
    struct tm tm;
    startTime = calculateMidnight(startTime);

    switch (period) {
        case SummaryPeriod::DAY:
            endTime = calculateLastSecondOfDay(startTime);
            break;

        case SummaryPeriod::WEEK:
            localtime_r(&startTime, &tm);
            tm.tm_mday += 6;
            tm.tm_isdst = -1;
            endTime = calculateLastSecondOfDay(mktime(&tm));
            break;

        case SummaryPeriod::MONTH:
            localtime_r(&startTime, &tm);
            tm.tm_mon += 1;
            tm.tm_mday = 0;
            tm.tm_isdst = -1;
            endTime = calculateLastSecondOfDay(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&startTime, &tm);
            tm.tm_year += 1;
            tm.tm_mon = 0;
            tm.tm_mday = 0;
            tm.tm_isdst = -1;
            endTime = calculateLastSecondOfDay(mktime(&tm));
            break;
    }

    return endTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::incrementStartTime(DateTime time, SummaryPeriod period) {
    DateTime startTime;
    struct tm tm;

    switch (period) {
        case SummaryPeriod::DAY:
            localtime_r(&time, &tm);
            tm.tm_mday += 1;
            tm.tm_isdst = -1;
            startTime = calculateMidnight(mktime(&tm));
            break;

        case SummaryPeriod::WEEK:
            localtime_r(&time, &tm);
            tm.tm_mday += 7;
            tm.tm_isdst = -1;
            startTime = calculateMidnight(mktime(&tm));
            break;

        case SummaryPeriod::MONTH:
            localtime_r(&time, &tm);
            tm.tm_mon += 1;
            tm.tm_isdst = -1;
            startTime = calculateMidnight(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&time, &tm);
            tm.tm_year += 1;
            tm.tm_isdst = -1;
            startTime = calculateMidnight(mktime(&tm));
            break;
    }

    return startTime;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SummaryReport::loadData() {

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Loading data for summary report..." << endl;

    vector<ArchivePacket> packets;
    DateTimeFields sd;
    DateTimeFields ed;
    sd.setFromEpoch(startDate);
    ed.setFromEpoch(endDate);
    DateTimeFields lastRecordDate = archiveManager.queryArchiveRecords(sd, ed, packets);

    if (!lastRecordDate.isDateTimeValid()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Failed to read archive for summary report" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Summary report received " << packets.size() << " packets from the archive" << endl;

    summaryRecords.clear();

    DateTime summaryStart = startDate;
    DateTime summaryEnd = calculateEndTime(summaryStart, period);

    while (summaryEnd <= endDate) {
        // TODO Should we create a new record if the summary start date is after today or
        // before the start of the data archive?
        SummaryRecord record(period, summaryStart, summaryEnd);
        summaryRecords.push_back(record);

        summaryStart = incrementStartTime(summaryStart, period);
        summaryEnd = calculateEndTime(summaryStart, period);
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Created " << summaryRecords.size() << " summary records" << endl;

    //
    // Now build the summary records for calculating day-based statistics
    //
    vector<SummaryRecord> dayRecords;
    summaryStart = startDate;
    summaryEnd = calculateEndTime(summaryStart, SummaryPeriod::DAY);
    while (summaryEnd <= endDate) {
        // TODO Should we create a new record if the summary start date is after today?
        SummaryRecord record(SummaryPeriod::DAY, summaryStart, summaryEnd);
        dayRecords.push_back(record);

        summaryStart = incrementStartTime(summaryStart, SummaryPeriod::DAY);
        summaryEnd = calculateEndTime(summaryStart, SummaryPeriod::DAY);
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Created " << dayRecords.size() << " day summary records" << endl;

    //
    // Now that we have created all of the summary records, go through and apply the ArchivePackets
    //
    bool firstPacket = true;
    for (auto & packet : packets) {
        for (auto & summaryRecord : summaryRecords) {
            summaryRecord.applyArchivePacket(packet);
        }

        for (auto & summaryRecord : dayRecords) {
            summaryRecord.applyArchivePacket(packet);
        }

        DateTime packetTime = packet.getEpochDateTime();
        struct tm tm;
        localtime_r(&packetTime, &tm);
        hourRainfallBuckets[tm.tm_hour] += packet.getRainfall();
        windRoseData.applyWindSample(packet.getPrevailingWindHeadingIndex(), packet.getAverageWindSpeed());
    }

    for (auto & summaryRecord : dayRecords)
        summaryStatistics.applySummaryRecord(summaryRecord);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
SummaryReport::formatJSON() const {
    std::stringstream ss;
    ss << "{ \"summaryReport\" : {"
       << "\"type\" : \"" << summaryPeriodEnum.valueToString(period) << "\", "
       << "\"startDate\" : \"" << Weather::formatDate(startDate) << "\", "
       << "\"endDate\" : \"" << Weather::formatDate(endDate) << "\", "
       << "\"summaries\" : [";


    bool first = true;
    for (auto summaryRecord : summaryRecords) {
        if (!first) ss << ", "; else first = false;
        ss << summaryRecord.formatJSON();
    }

    ss << " ], \"rainfallHourBuckets\" : [";
    for (int i = 0; i < 24; i++) {
        if (i != 0)
            ss << ", ";

        ss << hourRainfallBuckets[i];
    }

    ss << " ], " << endl << windRoseData.formatJSON() << ", " << endl;
    ss << summaryStatistics.formatJSON();
    ss << " } }";
    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
 *  statistics : {
 *      totalDays: 100,
 *      outsideTemperature : { // inside temperature, outsideHumidity, insideHumidity
 *          high: {
 *              { value: 95.0, time: "2024-02-15 13:25:25" },
 *              { dayMinimum: 95.0, date: "2024-02-15" }
 *          },
 *          low: {
 *              { value: 32, time: "2024-02-10 01:15:12" },
 *              { dayMaximum: 95.0, date: "2024-02-15" }
 *          },
 *          averages: {
 *              { average: 65, date: "2024-01-17" },
 *              { high: 68, date: "2024-02-23" },
 *              { low: 63, date: "2024-02-02" }
 *          },
 *          ranges: {
 *              smallest: { range: 2.5, date: "2024-01-02" },
 *              largest:  { range: 30.2, date: "2024-01029" }
 *          }
 *      },
 *      solarRadiation : { // uvIndex
 *          high: {
 *              { value: 95.0, time: "2024-02-15 13:25:25" },
 *              { dayMinimum: 95.0, date: "2024-02-15" }
 *          },
 *          averages: {
 *              { average: 65, date: "2024-01-17" },
 *              { high: 68, date: "2024-02-23" }
 *          }
 *       },
 *       rain : {
 *           rainDays: 10,
 *           totalRain: 5.5,
 *           averageRainPerDay : .04,
 *           averageRainPerRainyDay : .1,
 *           highRainDay: { value: 2.5, date: "2024-03-12" },
 *           highRainRate { value: 1.5, time: "2024-03-12 10:10:11" }
 *       }
 *
 */
SummaryStatistics::SummaryStatistics() : outsideTemperature("outsideTemperature", true, true, true),
                                         insideTemperature("insideTemperature", true, true, true),
                                         outsideHumidity("outsideHumidity", true, true, true),
                                         insideHumidity("insideHumidity", true, true, true),
                                         solarRadiation("solarRadiation", false, false, false),
                                         uvIndex("uvIndex", false, false, false),
                                         windSpeed("windSpeed", false, false, false),
                                         windGust("windGust", false, false, false),
                                         et("ET", false, false, false),
                                         barometer("barometricPressure", true, true, true),
                                         totalDays(0),
                                         rainDays(0),
                                         totalRainfall(0.0),
                                         highDayRainfall(0.0),
                                         highDayRainfallDate(0),
                                         highDayRainfallRate(0.0),
                                         highDayRainfallRateTime(0) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SummaryStatistics::applySummaryRecord(const SummaryRecord & record) {
    totalDays++;
    outsideTemperature.applySummaryMeasurement(record.startDate, record.outsideTemperature);
    insideTemperature.applySummaryMeasurement(record.startDate, record.insideTemperature);
    outsideHumidity.applySummaryMeasurement(record.startDate, record.outsideHumidity);
    insideHumidity.applySummaryMeasurement(record.startDate, record.insideHumidity);
    solarRadiation.applySummaryMeasurement(record.startDate, record.solarRadiation);
    windSpeed.applySummaryMeasurement(record.startDate, record.sustainedWindSpeed);
    windGust.applySummaryMeasurement(record.startDate, record.gustWindSpeed);
    uvIndex.applySummaryMeasurement(record.startDate, record.uvIndex);
    et.applySummaryMeasurement(record.startDate, record.et);
    barometer.applySummaryMeasurement(record.startDate, record.barometer);

    if (record.totalRainfall > 0.0) {
        totalRainfall += record.totalRainfall;
        rainDays++;
    }

    if (record.totalRainfall > highDayRainfall) {
        highDayRainfall = record.totalRainfall;
        highDayRainfallDate = record.startDate;
    }

    if (record.rainfallRate.high.extremeValue > highDayRainfallRate) {
        highDayRainfallRate = record.rainfallRate.high.extremeValue;
        highDayRainfallRateTime = record.rainfallRate.high.extremeTime;
    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
SummaryStatistics::formatJSON() const {
    ostringstream ss;

    ss << "\"statistics\" : { " << endl
       << " \"totalDays\" : " << totalDays << ", " << endl;
    ss << outsideTemperature.formatJSON() << ", " << endl;
    ss << outsideHumidity.formatJSON() << ", " << endl;
    ss << insideTemperature.formatJSON() << ", " << endl;
    ss << insideHumidity.formatJSON() << ", " << endl;
    ss << barometer.formatJSON() << ", " << endl;
    ss << windSpeed.formatJSON() << ", " << endl;
    ss << windGust.formatJSON() << ", " << endl;
    ss << solarRadiation.formatJSON() << ", " << endl;
    ss << uvIndex.formatJSON() << ", " << endl;
    ss << et.formatJSON() << ", " << endl;
    ss << "\"rain\" : {" << endl;
    ss << "\"rainDays\" : " << rainDays << ", " << endl
       << "\"totalRain\" : " << totalRainfall << ", " << endl
       << "\"highDayRain\" : { \"value\" : "  << highDayRainfall << ", \"date\" : \"" << Weather::formatDate(highDayRainfallDate) << "\" }, " << endl
       << "\"highDayRainRate\" : { \"value\" : "  << highDayRainfallRate << ", \"time\" : \"" << Weather::formatDateTime(highDayRainfallRateTime) << "\" } " << endl;
    ss << " } " << endl;
    ss << "} ";
    return ss.str();
}

} /* namespace vws */
