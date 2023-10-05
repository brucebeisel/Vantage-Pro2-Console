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

    cout << "Archive Manager" << endl;

    if (argc != 3) {
        cout << "Usage: ArchiveTest <archive-directory> <start date>" << endl;
        exit(1);
    }

    char * archiveDirectory = argv[1];

    SerialPort port("port", 19200);
    VantageWeatherStation station(port);
    ArchiveManager archiveManager(archiveDirectory, station);

    DateTime oldestPacket;
    DateTime newestPacket;
    archiveManager.getArchiveRange(oldestPacket, newestPacket);

    cout << "Archive time range: " << Weather::formatDateTime(oldestPacket) << " to " << Weather::formatDateTime(newestPacket) << endl;

    struct tm starttm;

    sscanf(argv[2], "%d-%d-%d", &starttm.tm_year, &starttm.tm_mon, &starttm.tm_mday);
    starttm.tm_mon--;
    starttm.tm_year -= 1900;
    starttm.tm_isdst = -1;
    DateTime startDate = mktime(&starttm);
    DateTime endDate = startDate + 6000;

    vector<ArchivePacket> packets;
    archiveManager.queryArchiveRecords(startDate, endDate, packets);
    cout << "First packet found time: " << Weather::formatDateTime(packets[0].getDateTime()) << endl;
}

