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
#include "Weather.h"
#include "VantageEnums.h"
#include "VantageWeatherStation.h"
#include "ArchiveManager.h"

#include "SummaryReport.h"
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
    int packetCount;
    archiveManager.getArchiveRange(oldestPacket, newestPacket, packetCount);

    cout << "Archive time range: " << Weather::formatDateTime(oldestPacket) << " to " << Weather::formatDateTime(newestPacket) << " Packet Count: " << packetCount << endl;

    struct tm starttm{0};

    sscanf(argv[2], "%d-%d-%d", &starttm.tm_year, &starttm.tm_mon, &starttm.tm_mday);
    starttm.tm_mon--;
    starttm.tm_year -= 1900;
    starttm.tm_isdst = -1;
    DateTime startDate = mktime(&starttm);
    DateTime endDate = startDate + 7200;

    vector<ArchivePacket> packets;
    archiveManager.queryArchiveRecords(startDate, endDate, packets);
    cout << "First packet found time: " << packets[0].getPacketDateTimeString() << endl;
}

