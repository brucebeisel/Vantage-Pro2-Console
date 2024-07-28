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
#include <cstring>
#include <unistd.h>
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

namespace vws {

static const unsigned char archivePage[] = {
    0x00, 0xde, 0x30, 0xc4, 0x04, 0x3b, 0x03, 0x3e, 0x03, 0x38, 0x03, 0x00, 0x00, 0x00, 0x00, 0x25,
    0x75, 0xee, 0x03, 0x75, 0x00, 0x09, 0x03, 0x36, 0x54, 0x03, 0x07, 0x04, 0x03, 0x58, 0x00, 0x22,
    0x04, 0x5b, 0xbd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0x30, 0xc9, 0x04, 0x3c, 0x03, 0x41, 0x03, 0x38, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x75, 0xc7, 0x02, 0x74, 0x00, 0x08, 0x03, 0x35, 0x55, 0x02, 0x07, 0x05,
    0x07, 0x42, 0x00, 0x12, 0x04, 0x55, 0xbd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0x30, 0xce, 0x04, 0x40, 0x03, 0x42,
    0x03, 0x3d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x75, 0x27, 0x02, 0x76, 0x00, 0x05, 0x03, 0x35,
    0x54, 0x02, 0x07, 0x04, 0x05, 0x3c, 0x00, 0x56, 0x02, 0x3f, 0xbd, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd5, 0x30, 0xeb,
    0x05, 0x89, 0x03, 0x8c, 0x03, 0x86, 0x03, 0x00, 0x00, 0x00, 0x00, 0xcf, 0x75, 0x3b, 0x03, 0x75,
    0x00, 0x03, 0x03, 0x34, 0x33, 0x02, 0x04, 0x0c, 0x0d, 0x44, 0x00, 0x45, 0x03, 0x46, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xd5, 0x30, 0xf0, 0x05, 0x8a, 0x03, 0x8c, 0x03, 0x86, 0x03, 0x00, 0x00, 0x00, 0x00, 0xce,
    0x75, 0x25, 0x03, 0x75, 0x00, 0x02, 0x03, 0x33, 0x34, 0x02, 0x07, 0x0b, 0x0c, 0x41, 0x00, 0x45,
    0x03, 0x44, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xa9
};

void
changePacketTime(vws::byte * packetData, const DateTimeFields & packetTime) {
    int datestamp = packetTime.getMonthDay() + (packetTime.getMonth() * 32) + ((packetTime.getYear() - 2000) * 512);
    int timestamp = (packetTime.getHour() * 100) + packetTime.getMinute();

    BitConverter::getBytes(datestamp, packetData, 0, 2);
    BitConverter::getBytes(timestamp, packetData, 2, 2);
}

VantageWeatherStation::VantageWeatherStation(SerialPort & serialPort) : serialPort(serialPort),
                                                                        archivePeriodMinutes(0),
                                                                        consoleType(VANTAGE_PRO_2),
                                                                        logger(VantageLogger::getLogger("VantageWeatherStation")) {
}

VantageWeatherStation::~VantageWeatherStation() {

}

bool
VantageWeatherStation::VantageWeatherStation::wakeupStation() {
    return true;
}

static int numberOfPacketsToDump = 1;

bool
VantageWeatherStation::dumpAfter(const DateTimeFields & after, vector<ArchivePacket> & packets) {
    cout << "Entered dummy dumpAfter(). After = " << after.formatDateTime() << endl;
    packets.clear();

    vws::byte packetData[54];
    memcpy(packetData, &archivePage[1], ArchivePacket::BYTES_PER_ARCHIVE_PACKET);

    DateTime packetEpochTime;
    if (after.isDateTimeValid())
        packetEpochTime = after.getEpochDateTime();
    else {
        DateTimeFields today(time(0));
        today.setTime(0, 0, 0);
        packetEpochTime = today.getEpochDateTime();
    }

    for (int i = 0; i < numberOfPacketsToDump; i++) {
        packetEpochTime += 300;
        DateTimeFields packetTime;
        packetTime.setFromEpoch(packetEpochTime);
        changePacketTime(packetData, packetTime);

        ArchivePacket packet(packetData);
        packets.push_back(packet);

        cout << "Added packet with time: " << packet.getPacketDateTimeString() << endl;
    }

    return true;
};


int
VantageWeatherStation::getArchivePeriod() const {
    return 5;
}
};


int
main(int argc, char * argv[]) {

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    VantageDecoder::setRainCollectorSize(.01);

    if (argc != 3) {
        cout << "Usage: ArchiveTest <archive-directory> <start date>" << endl;
        exit(1);
    }

    char * archiveDirectory = argv[1];

    SerialPort port("port", 19200);
    VantageWeatherStation station(port);
    ArchiveManager archiveManager(archiveDirectory, station);

    numberOfPacketsToDump = 1;
    archiveManager.synchronizeArchive();

    DateTimeFields oldestPacket;
    DateTimeFields newestPacket;
    int packetCount;
    archiveManager.getArchiveRange(oldestPacket, newestPacket, packetCount);

    cout << "Archive time range: " << oldestPacket.formatDateTime() << " to " << newestPacket.formatDateTime() << " Packet Count: " << packetCount << endl;

    struct tm starttm{0};

    sscanf(argv[2], "%d-%d-%d", &starttm.tm_year, &starttm.tm_mon, &starttm.tm_mday);
    starttm.tm_mon--;
    starttm.tm_year -= 1900;
    starttm.tm_isdst = -1;
    DateTime t = mktime(&starttm);
    DateTimeFields startDate;
    DateTimeFields endDate;
    startDate.setFromEpoch(t);
    endDate.setFromEpoch(t + 7200);

    vector<ArchivePacket> packets;
    archiveManager.queryArchiveRecords(startDate, endDate, packets);
    cout << "First packet found time: " << packets[0].getPacketDateTimeString() << endl;

    bool valid = archiveManager.verifyArchiveFile();

    if (valid)
        cout << "Archive is valid" << endl;
    else
        cout << "Archive is NOT valid" << endl;

    DateTimeFields dtf1;
    DateTimeFields dtf2;

    dtf1.setDateTime(2024, 11, 03, 1, 50, 0);
    dtf2.setDateTime(2024, 11, 03, 1, 55, 0);

    cout << "Epoch time for " << dtf1.formatDateTime(true) << " is " << dtf1.getEpochDateTime() << endl;
    cout << "Epoch time for " << dtf2.formatDateTime(true) << " is " << dtf2.getEpochDateTime() << endl;

    unlink(std::string(std::string(archiveDirectory) + "/test-archive.dat").c_str());
    ArchiveManager am2(archiveDirectory, "test-archive.dat", station);

    am2.synchronizeArchive();

    am2.getArchiveRange(oldestPacket, newestPacket, packetCount);

    if (packetCount == 1)
        cout << "PASSED: New archive contains exactly 1 packet" << endl;
    else
        cout << "FAILED: New archive does not contain exactly 1 packet. Count: " << packetCount << endl;

    if (!am2.clearArchiveFile())
        cout << "clearArchiveFile() returned FALSE" << endl;

    am2.getArchiveRange(oldestPacket, newestPacket, packetCount);

    if (packetCount == 0)
        cout << "PASSED: Cleared archive contains exactly 0 packet" << endl;
    else
        cout << "FAILED: Cleared archive does not contain exactly 0 packet. Count: " << packetCount << endl;

}

