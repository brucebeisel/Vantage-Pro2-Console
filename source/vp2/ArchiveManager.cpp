/* 
 * Copyright (C) 2023 Bruce Beisel
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
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "VantageProtocolConstants.h"
#include "ArchivePacket.h"
#include "ArchiveManager.h"
#include "VantageWeatherStation.h"
#include "Weather.h"

using namespace std;

namespace vws {

using vws::VantageLogger;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchiveManager::ArchiveManager(const std::string & archiveFilename, VantageWeatherStation & station) :
                                                                    archiveFile(archiveFilename),
                                                                    station(station),
                                                                    logger(VantageLogger::getLogger("ArchiveManager")) {
    findArchivePacketTimeRange();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchiveManager::~ArchiveManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::synchronizeArchive() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Synchronizing local archive from Vantage console's archive" << endl;
    vector<ArchivePacket> list;
    bool result = false;

    for (int i = 0; i < SYNC_RETRIES && !result; i++) {
        list.clear();
        if (station.wakeupStation() && station.dumpAfter(newestPacketTime, list)) {
            addPacketsToArchive(list);
            result = true;
            break;
        }
    }

    if (list.size() > 0)
        cout << "Archive: " << list[list.size() - 1].formatXML() << endl;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
ArchiveManager::getArchiveRecordsAfter(DateTime afterTime, std::vector<ArchivePacket>& list) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading packets after " << Weather::formatDateTime(afterTime) << endl;
    byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary);
    positionStream(stream, afterTime, true);
    list.clear();
    
    //
    // Cap the number of records to the number of archive records that the console holds.
    // If there are more records, then the caller needs to call this method until the
    // list returns empty.
    //
    DateTime timeOfLastRecord = 0;
    while (list.size() < ProtocolConstants::NUM_ARCHIVE_RECORDS) {
        stream.read(buffer, sizeof(buffer));

        if (stream.eof())
            break;

        ArchivePacket packet(buffer, 0);
        list.push_back(packet);
        timeOfLastRecord = packet.getDateTime();
    }

    stream.close();

    return timeOfLastRecord;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
ArchiveManager::queryArchiveRecords(DateTime startTime, DateTime endTime, std::vector<ArchivePacket> & list) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Querying archive records between "
                                              << Weather::formatDateTime(startTime)
                                              << " and " << Weather::formatDateTime(endTime) << endl;
    byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary);
    positionStream(stream, startTime, false);
    list.clear();

    //
    // Cap the number of records to the number of archive records that the console holds.
    // If there are more records, then the caller needs to call this method until the
    // list returns empty.
    //
    DateTime packetTime = 0;
    DateTime timeOfLastRecord = 0;
    do {
        stream.read(buffer, sizeof(buffer));

        if (!stream.eof()) {
            ArchivePacket packet(buffer, 0);
            packetTime = packet.getDateTime();
            if (packetTime <= endTime) {
                list.push_back(packet);
                timeOfLastRecord = packetTime;
            }
        }
    } while (!stream.eof() && packetTime < endTime);

    stream.close();

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query found " << list.size() << " items. Time of last record is " << Weather::formatDateTime(timeOfLastRecord) << endl;
    return timeOfLastRecord;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::positionStream(istream & stream, DateTime searchTime, bool afterTime) {
    byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    streampos streamPosition;

    stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);

    //
    // If we are only looking for records after the specified time, then increment the
    // search time so we can use <= in the time comparisons.
    //
    if (afterTime)
        searchTime++;

    //
    // If the start time is newer than the oldest packet in the archive, look for the packet that is after the specified time.
    // Otherwise the start time is before the beginning of the file, so just start at the beginning.
    // This a linear search from the end of the archive. A binary search would be more efficient.
    //
    if (searchTime >= oldestPacketTime) {
        DateTime packetTime;
        do {
            streamPosition = stream.tellg();
            stream.read(buffer, sizeof(buffer));
            ArchivePacket packet(buffer, 0);
            stream.seekg(-(ArchivePacket::BYTES_PER_ARCHIVE_PACKET * 2), ios::cur);
            packetTime = packet.getDateTime();
        } while (searchTime <= packetTime && streamPosition > 0);

        //
        // The stream will not be good if the final seekg() call went past the beginning of the file.
        // Clear the error and position the next to be the beginning of the file.
        //
        if (!stream.good()) {
            stream.clear();
            stream.seekg(0, ios::beg);
        }
        else
            stream.seekg(ArchivePacket::BYTES_PER_ARCHIVE_PACKET * 2, ios::cur);
    }
    else
        stream.seekg(0, ios::beg);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::getNewestRecord(ArchivePacket & packet) const {
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary | ios::ate);
    streampos fileSize = stream.tellg();

    if (fileSize >= ArchivePacket::BYTES_PER_ARCHIVE_PACKET) {
        byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
        stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);
        stream.read(buffer, sizeof(buffer));
        packet.updateArchivePacketData(buffer, 0);
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::addPacketToArchive(const ArchivePacket & packet) {
    vector<ArchivePacket> list;
    list.push_back(packet);
    addPacketsToArchive(list);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::addPacketsToArchive(const vector<ArchivePacket> & packets) {
    if (packets.size() == 0)
        return;

    ofstream stream;
    stream.open(archiveFile.c_str(), ofstream::out | ios::app | ios::binary);
    for (vector<ArchivePacket>::const_iterator it = packets.begin(); it != packets.end(); ++it) {
        if (newestPacketTime < it->getDateTime()) {
            stream.write(it->getBuffer(), ArchivePacket::BYTES_PER_ARCHIVE_PACKET);
            newestPacketTime = it->getDateTime();
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Archived packet with time: " << Weather::formatDateTime(it->getDateTime()) << endl;
        }
        else
            logger.log(VantageLogger::VANTAGE_INFO) << "Skipping archive of packet with time " << Weather::formatDateTime(it->getDateTime()) << endl;
    }
    stream.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::findArchivePacketTimeRange() {
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary | ios::ate);

    streampos fileSize = stream.tellg();
    if (fileSize >= ArchivePacket::BYTES_PER_ARCHIVE_PACKET) {
        byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];

        //
        // Read the packet at the beginning of the file
        //
        stream.seekg(0, ios::beg);
        stream.read(buffer, sizeof(buffer));
        ArchivePacket packet(buffer, 0);
        oldestPacketTime = packet.getDateTime();

        //
        // Read the packet at the end of the file
        //
        stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);
        stream.read(buffer, sizeof(buffer));
        packet.updateArchivePacketData(buffer, 0);
        newestPacketTime = packet.getDateTime();
    }
    else {
        oldestPacketTime = 0;
        newestPacketTime = 0;
    }

    stream.close();
}

}
