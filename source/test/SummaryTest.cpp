/*
 * SummaryTest.cpp
 *
 *  Created on: Oct 4, 2023
 *      Author: bruce
 */

#include <iostream>
#include "SummaryRecord.h"
#include "Weather.h"
#include "VantageEnums.h"
#include "VantageWeatherStation.h"
#include "ArchiveManager.h"
#include "SerialPort.h"

using namespace vws;
using namespace std;

int
main(int argc, char * argv[]) {

    cout << "Summary Test" << endl;

    if (argc != 5) {
        cout << "Usage: summaryTest <archive-file> <start-date> <end-date> <period>" << endl
             << "    where: archive-file is the name of the file where the archive records reside"
             << "           start-date = start of summary in yyyy-mm-dd format" << endl
             << "           end-date = end of summary in yyyy-mm-dd format" << endl
             << "           period = {day, week, month, year}" << endl;

        exit(1);
    }

    char * archiveFile = argv[1];
    char * startDateArg = argv[2];
    char * endDateArg = argv[3];
    char * periodArg = argv[4];

    struct tm tm = {0};
    tm.tm_isdst = -1;

    try {
        sscanf(startDateArg, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
        tm.tm_mon--;
        tm.tm_year -= 1900;
        DateTime startDate = mktime(&tm);

        sscanf(endDateArg, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
        tm.tm_mon--;
        tm.tm_year -= 1900;
        DateTime endDate = mktime(&tm);

        SummaryPeriod period = summaryPeriodEnum.stringToValue(periodArg);

        cout << "Summarizing " << period << " period from " << startDate << " to " << endDate << " from file " << archiveFile << endl;

        SerialPort port("port", 19200);
        VantageWeatherStation station(port);
        ArchiveManager archiveManager(archiveFile, station);
        SummaryReport report(period, startDate, endDate, archiveManager);
        report.loadData();

        cout << report.formatJSON() << endl;

    }
    catch (const std::exception & e) {
        cout << "Caught exception: " << e.what() << endl;
        exit(2);
    }


}

