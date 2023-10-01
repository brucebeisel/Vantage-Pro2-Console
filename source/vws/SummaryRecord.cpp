#include <sstream>
#include "SummaryRecord.h"
#include "ArchivePacket.h"
#include "ArchiveManager.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryRecord::SummaryRecord(SummaryPeriod period, DateTime startDate, DateTime endDate) : period(period),
                                                                                           startDate(startDate),
                                                                                           endDate(endDate),
                                                                                           totalRainfall(0.0),
                                                                                           outsideTemperature("Outside Temperature"),
                                                                                           solarRadiation("Solar Radiation"),
                                                                                           rainfallRate("High Rainfall Rate"),
                                                                                           barometer("Barometer"),
                                                                                           insideTemperature("Inside Temperature"),
                                                                                           insideHumidity("Inside Humidity"),
                                                                                           outsideHumidity("Outside Humidity"),
                                                                                           sustainedWindSpeed("Sustained Wind Speed"),
                                                                                           gustWindSpeed("Wind Gust Speed"),
                                                                                           uvIndex("UV Index"),
                                                                                           et("Evapotranspiration") {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryRecord::~SummaryRecord() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SummaryRecord::applyArchivePacket(const ArchivePacket & archivePacket) {
    DateTime packetTime = archivePacket.getDateTime();

    outsideTemperature.applyMeasurement(packetTime,
                                        archivePacket.getAverageOutsideTemperature(),
                                        archivePacket.getLowOutsideTemperature(),
                                        archivePacket.getHighOutsideTemperature());

    insideTemperature.applyMeasurement(packetTime, archivePacket.getInsideTemperature());

    solarRadiation.applyMeasurement(packetTime, archivePacket.getAverageSolarRadiation(), archivePacket.getHighSolarRadiation());

    totalRainfall += archivePacket.getRainfall();
    rainfallRate.applyMeasurement(packetTime, archivePacket.getHighRainfallRate());

    barometer.applyMeasurement(packetTime, archivePacket.getBarometricPressure());
    insideTemperature.applyMeasurement(packetTime, archivePacket.getInsideTemperature());
    insideHumidity.applyMeasurement(packetTime, archivePacket.getInsideHumidity());
    outsideHumidity.applyMeasurement(packetTime, archivePacket.getOutsideHumidity());

    sustainedWindSpeed.applyMeasurement(packetTime, archivePacket.getAverageWindSpeed());
    gustWindSpeed.applyMeasurement(packetTime, archivePacket.getHighWindSpeed());

    uvIndex.applyMeasurement(packetTime, archivePacket.getAverageUvIndex(), archivePacket.getHighUvIndex());
    et.applyMeasurement(packetTime, archivePacket.getEvapotranspiration());

    /*
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
        */
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryReport::SummaryReport(SummaryPeriod period, DateTime startDate, DateTime endDate, ArchiveManager & archiveManager) : period(period), startDate(startDate), endDate(endDate), archiveManager(archiveManager) {
    //
    // Set the start and end times to the start and end of the days
    //
    startDate = normalizeStartTime(startDate, period);

    struct tm tm;
    localtime_r(&endDate, &tm);
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    endDate = timelocal(&tm);
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
SummaryReport::normalizeStartTime(DateTime time, SummaryPeriod period) {
    struct tm tm;
    DateTime startTime;

    switch (period) {
        case SummaryPeriod::DAY:
            startTime = calculateMidnight(time);
            break;

        case SummaryPeriod::WEEK:
            localtime_r(&time, &tm);
            tm.tm_mday -= tm.tm_wday;
            startTime = calculateMidnight(mktime(&tm));
            break;
            break;

        case SummaryPeriod::MONTH:
            localtime_r(&time, &tm);
            tm.tm_mday = 1;
            startTime = calculateMidnight(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&time, &tm);
            tm.tm_mon = 0;
            tm.tm_mday = 1;
            startTime = calculateMidnight(mktime(&tm));
            break;
    }

    return startTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
SummaryReport::normalizeEndTime(DateTime startTime, DateTime endTime, SummaryPeriod period) {
    DateTime normalizedTime;
    struct tm tm;

    switch (period) {
        case SummaryPeriod::DAY:
            normalizedTime = calculateLastSecondOfDay(startTime);
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
            startTime = calculateLastSecondOfDay(mktime(&tm));
            break;
    }

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
            tm.tm_isdst = -1;
            endTime = calculateLastSecondOfDay(mktime(&tm));
            break;

        case SummaryPeriod::YEAR:
            localtime_r(&startTime, &tm);
            tm.tm_year += 1;
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
void
SummaryReport::loadData() {
    startDate = normalizeStartTime(startDate, period);
    endDate = normalizeEndTime(startDate, endDate, period);

    vector<ArchivePacket> packets;
    DateTime lastRecordDate = archiveManager.queryArchiveRecords(startDate, endDate, packets);
    summaryRecords.clear();

    DateTime summaryStart = startDate;
    DateTime summaryEnd = calculateEndTime(summaryStart, period);

    while (summaryEnd <= endDate) {
        //
        // Add more than 25 hours, then reset it back to 23:59:59
        //
        SummaryRecord record(period, summaryStart, summaryEnd);
        summaryRecords.push_back(record);

        summaryStart = incrementStartTime(summaryStart, period);
        summaryEnd = calculateEndTime(summaryStart, period);
    }

    //
    // Now that we have create all of the summary records, go through and apply the ArchivePackets
    //
    for (auto packet : packets) {
        for (auto summaryRecord : summaryRecords) {
            summaryRecord.applyArchivePacket(packet);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// TODO Add period enum to VantageEnums.h
std::string
SummaryReport::formatJSON() const {
    std::stringstream ss;
    ss << "{ \"summary-report\" : {"
       << "\"type\" : \"" << static_cast<int>(period) << "\""
       << "\"start-date\" : \"" << Weather::formatDate(startDate) << "\", "
       << "\"end-date\" : \"" << Weather::formatDate(endDate) << "\", "
       << "\"summaries\" : [";

    ss << " ], \"rainfall-by-hour\" : [";
    for (int i = 0; i < 24; i++) {
        if (i != 0)
            ss << ", ";

        ss << hourRainfallBuckets[i];
    }

    ss << " ] } }";
    return ss.str();
}
} /* namespace vws */
