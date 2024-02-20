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
#include <iostream>
#include "SummaryReport.h"
#include "Weather.h"
#include "VantageEnums.h"
#include "VantageWeatherStation.h"
#include "ArchiveManager.h"
#include "SerialPort.h"
#include "VantageLogger.h"
#include "VantageDecoder.h"
#include "WindRoseData.h"

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
             << "           period = {Day, Week, Month, Year}" << endl;

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
        WindRoseData windRoseData(ProtocolConstants::WindUnits::MPH, 5.0, 2);


        SummaryReport report(period, startDate, endDate, archiveManager, windRoseData);
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

