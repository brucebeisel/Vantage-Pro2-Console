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
#include "VantageLogger.h"
#include "VantageDecoder.h"

using namespace vws;
using namespace std;

int
main(int argc, char * argv[]) {

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    VantageDecoder::setRainCollectorSize(.01);

    cout << "Summary Test" << endl;

    if (argc != 5) {
        cout << "Usage: summaryTest <archive-file> <start-date> <end-date> <period>" << endl
             << "    where: archive-directory is the name of the directory where the archive file resides" << endl
             << "           start-date = start of summary in yyyy-mm-dd format" << endl
             << "           end-date = end of summary in yyyy-mm-dd format" << endl
             << "           period = {day, week, month, year}" << endl;

        exit(1);
    }

    char * archiveFile = argv[1];
    char * startDateArg = argv[2];
    char * endDateArg = argv[3];
    char * periodArg = argv[4];

    struct tm starttm = {0};
    struct tm endtm = {0};
    starttm.tm_isdst = -1;
    endtm.tm_isdst = -1;

    try {
        sscanf(startDateArg, "%d-%d-%d", &starttm.tm_year, &starttm.tm_mon, &starttm.tm_mday);
        starttm.tm_mon--;
        starttm.tm_year -= 1900;
        DateTime startDate = mktime(&starttm);

        sscanf(endDateArg, "%d-%d-%d", &endtm.tm_year, &endtm.tm_mon, &endtm.tm_mday);
        endtm.tm_mon--;
        endtm.tm_year -= 1900;
        DateTime endDate = mktime(&endtm);

        SummaryPeriod period = summaryPeriodEnum.stringToValue(periodArg);

        cout << "Summarizing " << period << " period from " << startDate << " to " << endDate << " from file " << archiveFile << endl;

        SerialPort port("port", 19200);
        VantageWeatherStation station(port);
        ArchiveManager archiveManager(archiveFile, station);


        SummaryReport report(period, startDate, endDate, archiveManager);
        if (report.loadData())
            cout << report.formatJSON() << endl;
        else
            cout << "No summary data available" << endl;

    }
    catch (const std::exception & e) {
        cout << "Caught exception: " << e.what() << endl;
        exit(2);
    }


}

