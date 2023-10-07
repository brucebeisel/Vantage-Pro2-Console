#include <sstream>
#include "ArchivePacket.h"
#include "ArchiveManager.h"
#include "SummaryReport.h"
#include "WindRoseData.h"
#include "VantageEnums.h"

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
                                                                                           et("evapotranspiration") {


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
    DateTime packetTime = archivePacket.getDateTime();

    //cout << "Checking time of " << Weather::formatDateTime(packetTime)
    //     << " against SummaryRecord time range: " << Weather::formatDateTime(startDate) << " to " << Weather::formatDateTime(endDate) << endl;

    if (packetTime < startDate || packetTime > endDate) {
        //cout << "------Ignoring packet" << endl;
        return;
    }

    packetCount++;

    //cout << "Outside temperature for " << Weather::formatDate(startDate) << endl;

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

    ss << " { \"" << name << "\" : [ " << endl;

    std::string lastString;

    for (int i = 0; i < numSummaries; i++) {
        if (i != 0 && lastString.length() > 0) ss << ", " << endl;
        lastString = sm[i].formatJSON();
        ss << lastString;
    }

    ss << " ] }";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
SummaryRecord::formatJSON() const {
    std::stringstream ss;
    ss << " { \"summary\" : { \"type\" : \"" << summaryPeriodEnum.valueToString(period) <<  "\", "
       << "\"startDate\" : \"" << Weather::formatDate(startDate) << "\", "
       << "\"endDate\" : \"" << Weather::formatDate(endDate) << "\", " << endl
       << "\"measurements\" : [ " << endl;
    if (packetCount != 0) {
        ss << outsideTemperature.formatJSON() << ", " << endl
           << outsideHumidity.formatJSON() << ", " << endl
           << solarRadiation.formatJSON() << ", " << endl
           << insideTemperature.formatJSON() << ", " << endl
           << insideHumidity.formatJSON() << ", " << endl
           << barometer.formatJSON() << ", " << endl
           << rainfallRate.formatJSON() << ", " << endl
           << uvIndex.formatJSON() << ", " << endl
           << et.formatJSON() << ", " << endl
           << sustainedWindSpeed.formatJSON() << "," << endl
           << gustWindSpeed.formatJSON() << ", " << endl
           << " { \"rainfall\" : " << totalRainfall << " }, " << endl;

        ss << arrayFormatJSON("extraTemperatures", extraTemperatures, ArchivePacket::MAX_EXTRA_TEMPERATURES) << ", " << endl
           << arrayFormatJSON("extraHumidities", extraHumidities, ArchivePacket::MAX_EXTRA_HUMIDITIES) << ", " << endl
           << arrayFormatJSON("leafTemperatures", leafTemperatures, ArchivePacket::MAX_LEAF_TEMPERATURES) << ", " << endl
           << arrayFormatJSON("soilTemperatures", soilTemperatures, ArchivePacket::MAX_SOIL_TEMPERATURES) << ", " << endl
           << arrayFormatJSON("leafWetnesses", leafWetnesses, ArchivePacket::MAX_LEAF_WETNESSES) << ", " << endl
           << arrayFormatJSON("soilMoistures", soilMoistures, ArchivePacket::MAX_SOIL_MOISTURES) <<  endl; 
    }

    ss << "] } }";

    return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SummaryReport::SummaryReport(SummaryPeriod period,
                             DateTime startDate,
                             DateTime endDate,
                             ArchiveManager & archiveManager,
                             WindRoseData & wrd) : period(period),
                                                   startDate(startDate),
                                                   endDate(endDate),
                                                   archiveManager(archiveManager),
                                                   windRoseData(wrd) {
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
            startTime = calculateLastSecondOfDay(mktime(&tm));
            break;
    }
    cout << "Normalize end time from " << Weather::formatDateTime(endTime) << " to " << Weather::formatDateTime(normalizedTime) << endl;

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
    startDate = normalizeStartTime(startDate, period);
    endDate = normalizeEndTime(startDate, endDate, period);

    cout << "Getting archive between " << Weather::formatDate(startDate) << " and " << Weather::formatDate(endDate) << endl;

    vector<ArchivePacket> packets;
    DateTime lastRecordDate = archiveManager.queryArchiveRecords(startDate, endDate, packets);
    if (lastRecordDate == 0) {
        cout << "Failed to read archive" << endl;
        return false;
    }

    cout << "Received " << packets.size() << " packets from the archive" << endl;
    summaryRecords.clear();

    DateTime summaryStart = startDate;
    DateTime summaryEnd = calculateEndTime(summaryStart, period);

    while (summaryEnd <= endDate) {
        SummaryRecord record(period, summaryStart, summaryEnd);
        summaryRecords.push_back(record);

        summaryStart = incrementStartTime(summaryStart, period);
        summaryEnd = calculateEndTime(summaryStart, period);
    }

    cout << "Created " << summaryRecords.size() << " summary records" << endl;

    //
    // Now that we have create all of the summary records, go through and apply the ArchivePackets
    //
    for (auto & packet : packets) {
        for (auto & summaryRecord : summaryRecords) {
            summaryRecord.applyArchivePacket(packet);
        }
        DateTime packetTime = packet.getDateTime();
        struct tm tm;
        localtime_r(&packetTime, &tm);
        hourRainfallBuckets[tm.tm_hour] += packet.getRainfall();
        windRoseData.applyWindSample(packet.getPrevailingWindDirectionIndex(), packet.getAverageWindSpeed());
    }

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

    ss << " ], " << endl << windRoseData.formatJSON();
    ss << " } }";
    return ss.str();
}
} /* namespace vws */
