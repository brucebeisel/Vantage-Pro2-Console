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

#include "ArchiveManager.h"

#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <ratio>

#include "ArchivePacket.h"
#include "VantageProtocolConstants.h"
#include "VantageWeatherStation.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {

using vws::VantageLogger;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchiveManager::ArchiveManager(const string & dataDirectory, VantageWeatherStation & station) :
                                                                    archiveFile(dataDirectory + "/" + ARCHIVE_FILE),
                                                                    archiveBackupDir(dataDirectory + "/" + ARCHIVE_BACKUP_DIR),
                                                                    nextBackupTime(0),
                                                                    archiveTempFile(dataDirectory + "/" + ARCHIVE_TEMP_FILE),
                                                                    station(station),
                                                                    archivePacketCount(0),
                                                                    archivingActive(true),
                                                                    logger(VantageLogger::getLogger("ArchiveManager")) {
    findArchivePacketTimeRange();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchiveManager::ArchiveManager(const string & dataDirectory, const string & archiveFile, VantageWeatherStation & station) :
                                                                    archiveFile(dataDirectory + "/" + archiveFile),
                                                                    archiveBackupDir(dataDirectory + "/" + ARCHIVE_BACKUP_DIR),
                                                                    nextBackupTime(0),
                                                                    archiveTempFile(dataDirectory + "/" + ARCHIVE_TEMP_FILE),
                                                                    station(station),
                                                                    archivePacketCount(0),
                                                                    archivingActive(true),
                                                                    logger(VantageLogger::getLogger("ArchiveManager")) {
    findArchivePacketTimeRange();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ArchiveManager::~ArchiveManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// TODO There seems to be a bug with daylight savings time. There is an hour gap in the archive records
// when DST ends.
bool
ArchiveManager::synchronizeArchive() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Synchronizing local archive from Vantage console's archive" << endl;
    //logger.log(VantageLogger::VANTAGE_INFO) << "Synchronizing archive is temporarily disabled" << endl;
    //return true;
    vector<ArchivePacket> list;
    bool result = false;

    DateTimeFields timeFields = newestPacket.getDateTimeFields();

    for (int i = 0; i < SYNC_RETRIES && !result; i++) {
        list.clear();
        if (station.wakeupStation() && station.dumpAfter(timeFields, list)) {
            addPacketsToArchive(list);
            result = true;
            break;
        }
    }

    backupArchiveFile();

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
ArchiveManager::queryArchiveRecords(const DateTimeFields & startTime, const DateTimeFields & endTime, vector<ArchivePacket> & list) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Querying archive records between "
                                              << startTime.formatDateTime()
                                              << " and " << endTime.formatDateTime() << endl;
    list.clear();
    DateTimeFields timeOfLastRecord;
    byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary);
    if (stream.fail()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open archive file \"" << archiveFile << "\"" << endl;
        return timeOfLastRecord;
    }

    positionStream(stream, startTime.getEpochDateTime(), false);

    DateTimeFields packetTime;
    do {
        stream.read(buffer, sizeof(buffer));

        if (!stream.eof()) {
            ArchivePacket packet(buffer);
            packetTime = packet.getDateTimeFields();
            if (packetTime <= endTime) {
                list.push_back(packet);
                timeOfLastRecord = packetTime;
            }
        }
    } while (!stream.eof() && packetTime < endTime);

    stream.close();

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query found " << list.size()
                                              << " items. Time of last record is "
                                              << timeOfLastRecord.formatDateTime() << endl;
    return timeOfLastRecord;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::positionStream(istream & stream, DateTime searchTime, bool afterTime) {
    if (archivePacketCount < 2)
        return true;

    //
    // Use high resolution clock to track performance of positioning the stream
    //
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    streampos streamPosition;
    int forwardReadsPerformed = 0;
    int backwardReadsPerformed = 0;

    stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);
    long fileSize = stream.tellg();

    //
    // If we are only looking for records after the specified time, then increment the
    // search time so we can use <= in the time comparisons.
    //
    if (afterTime)
        searchTime++;

    DateTime oldestPacketTime = oldestPacket.getEpochDateTime();
    DateTime newestPacketTime = newestPacket.getEpochDateTime();

    //
    // If the start time is newer than the oldest packet in the archive, look for the packet that is after the specified time.
    // Otherwise the start time is before the beginning of the file, so just start at the beginning.
    //
    if (searchTime <= oldestPacketTime) {
        stream.seekg(0, ios::beg);
    }
    else if (searchTime > oldestPacketTime && searchTime < newestPacketTime) {
        //
        // Use the ratio of time based on the time range of the archive. This will hopefully position the stream
        // very close to the search time.
        //
        DateTime archiveRange = newestPacketTime - oldestPacketTime;
        DateTime searchDelta = searchTime - oldestPacketTime;
        double searchRatio = static_cast<double>(searchDelta) / static_cast<double>(archiveRange);

        long searchLocation = static_cast<long>(::round(static_cast<double>(fileSize) * searchRatio));
        searchLocation -= searchLocation % ArchivePacket::BYTES_PER_ARCHIVE_PACKET;
        stream.seekg(searchLocation, ios::beg);


        //
        // Skip forward past the search time. This will only skip forward one record if the ratio calculation
        // positioned the stream later than the search time.
        //
        DateTime packetTime;
        do {
            stream.read(buffer, sizeof(buffer));
            forwardReadsPerformed++;
            ArchivePacket packet(buffer);
            packetTime = packet.getEpochDateTime();
        } while (packetTime < searchTime && !stream.eof());

        //
        // Now back up in the archive until we find the packet just after the packet for which we are looking.
        // Since the algorithm backs up two packets after each read, that will put the stream right on the
        // packet being searched.
        // 
        do {
            streamPosition = stream.tellg();
            stream.read(buffer, sizeof(buffer));
            backwardReadsPerformed++;
            ArchivePacket packet(buffer);
            packetTime = packet.getEpochDateTime();
            stream.seekg(-(ArchivePacket::BYTES_PER_ARCHIVE_PACKET * 2), ios::cur);
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

    chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> timeSpan = duration_cast<chrono::duration<double>>(t2 - t1);

    logger.log(VantageLogger::VANTAGE_DEBUG2) <<  "Positioning stream to find archive record of time "
                                              << Weather::formatDateTime(searchTime)
                                              << " in archive with range of " << oldestPacket.getPacketDateTimeString()
                                              << " to " << newestPacket.getPacketDateTimeString()
                                              << " took " << timeSpan.count() << " seconds"
                                              << " and required " << forwardReadsPerformed << " forward reads and "
                                              << backwardReadsPerformed << " backward reads" << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::getNewestRecord(ArchivePacket & packet) const {
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary | ios::ate);
    if (stream.fail()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open archive file \"" << archiveFile << "\"" << endl;
        return false;
    }

    streampos fileSize = stream.tellg();

    if (fileSize >= ArchivePacket::BYTES_PER_ARCHIVE_PACKET) {
        byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
        stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);
        stream.read(buffer, sizeof(buffer));
        packet.updateArchivePacketData(buffer);
        return true;
    }
    else
        return false;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::getArchiveRange(DateTimeFields & oldest, DateTimeFields & newest, int & count) const {
    oldest = oldestPacket.getDateTimeFields();
    newest = newestPacket.getDateTimeFields();
    count = archivePacketCount;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::clearArchiveFile() {
    ofstream stream(archiveFile.c_str(), ios::out | ios::trunc);

    if (stream.good()) {
        oldestPacket.clearArchivePacketData();
        newestPacket.clearArchivePacketData();
        archivePacketCount = 0;
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::trimBackupDirectory() {
    DIR * dir;
    struct dirent * ent;
    time_t now = time(0);
    vector<string> deleteList;
    if ((dir = opendir(archiveBackupDir.c_str())) == NULL) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "trimBackupDirectory(): Failed to open archive backup directory" << endl;
        return;
    }

    //
    // Build the list of archive backup files that are too old
    //
    while ((ent = readdir(dir)) != NULL) {
        //
        // Ignore any file that starts with ".", including ".."
        //
        if (ent->d_name[0] == '.')
            continue;
        struct stat sbuf;
        stat(ent->d_name, &sbuf);
        time_t mtime = sbuf.st_mtim.tv_sec;
        if (mtime + (Weather::SECONDS_PER_DAY * BACKUP_RETAIN_DAYS) < now)
            deleteList.push_back(string(archiveBackupDir + "/" + ent->d_name));
    }

    closedir (dir);

    for (auto path : deleteList) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Deleting backup archive file '" << path << "'" << endl;
        if (unlink(path.c_str()) != 0)
            logger.log(VantageLogger::VANTAGE_WARNING) << "trimBackupDirectory(): Failed to delete archive backup file " << path << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::backupArchiveFile(DateTime now) {
    //
    // Backup the archive about once a day
    //
    if (now == 0)
        now = time(0);

    if (now < nextBackupTime)
        return true;

    nextBackupTime = now + Weather::SECONDS_PER_DAY;

    if (!std::filesystem::exists(archiveBackupDir))
        std::filesystem::create_directories(archiveBackupDir);

    //
    // Building the dateString separately fixes a warning from Eclipse
    //
    string dateString;
    dateString = Weather::formatDate(now);
    string backupFile(archiveBackupDir + "/" + dateString + "_" + ARCHIVE_BACKUP_FILE);

    std::error_code errorCode;
    if (!std::filesystem::copy_file(archiveFile, backupFile, std::filesystem::copy_options::overwrite_existing, errorCode)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to backup archive file. Error = " << errorCode.message() << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Backed up archive file '" << archiveFile << "' to '" << backupFile << "'" << endl;

    trimBackupDirectory();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
bool
ArchiveManager::restoreArchiveFile(const string & backupFile) {
    //
    // Switch the backup and the archive file so the current archive file is backed up
    //
    if (std::rename(archiveFile.c_str(), archiveTempFile.c_str()) != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to move archive file to temp file during archive file restore." << endl;
        return false;
    }

    if (std::rename(backupFile.c_str(), archiveFile.c_str()) != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to move backup archive file to main archive file during archive file restore." << endl;
        return false;
    }

    if (std::rename(archiveTempFile.c_str(), backupFile.c_str()) != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to temp file to backup archive file during archive file restore." << endl;
        return false;
    }

    return true;
}
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::verifyArchiveFile() const {
    logger.log(VantageLogger::VANTAGE_INFO) << "Verifying current archive file" << endl;
    ifstream is(archiveFile, ios::in | ios::binary);

    if (is.fail()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Failed to open archive file '" << archiveFile << " for verification" << endl;
        return false;
    }

    int packetsRead = 0;
    int errorCount = 0;
    int warningCount = 0;
    ArchivePacket lastPacket;
    DateTime lastPacketTime = 0;
    DateTime lastDelta = 0;
    int deltaTimeMismatchCount = 0;
    char buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    do {
        is.read(buffer, sizeof(buffer));
        if (!is.eof()) {
            packetsRead++;
            ios::pos_type position = is.tellg();

            ArchivePacket packet(buffer);
            DateTime currentPacketTime = packet.getEpochDateTime();

            if (currentPacketTime <= lastPacketTime) {
                logger.log(VantageLogger::VANTAGE_WARNING) << "Detected out of order packets at file location " << position << ". " << endl
                                                           << "Packet with time " << Weather::formatDateTime(currentPacketTime) << " (" << packet.getPacketDateTimeString() << ")"
                                                           << " is before packet with time: " << Weather::formatDateTime(lastPacketTime) << " (" << lastPacket.getPacketDateTimeString() << ")" << endl;
                errorCount++;
            }

            DateTime currentDelta = currentPacketTime - lastPacketTime;
            if (packetsRead > 2 && currentDelta != lastDelta) {
                logger.log(VantageLogger::VANTAGE_INFO) << "Detected inconsistent time delta between packet at file location " << position << ". " << endl
                                                        << "Expected time delta is " << lastDelta << ", actual delta is " << currentDelta
                                                        << " for packet with times " << "[" << lastPacketTime << "] " << Weather::formatDateTime(lastPacketTime) << " (" << lastPacket.getPacketDateTimeString() << ")" << " and "
                                                        << "[" << currentPacketTime << "] " << Weather::formatDateTime(currentPacketTime) << " (" << packet.getPacketDateTimeString() << ")" << endl;
                warningCount++;

                //
                // Only change the expected delta if more than two mismatches happen in a row
                //
                if (++deltaTimeMismatchCount > 1)
                    lastDelta = currentDelta;
            }
            else
                deltaTimeMismatchCount = 0;

            if (packetsRead == 2)
                lastDelta = currentDelta;

            lastPacketTime = currentPacketTime;
            lastPacket.updateArchivePacketData(buffer, 0);
        }

    } while (!is.eof());

    logger.log(VantageLogger::VANTAGE_INFO) << "Archive verification complete for archive with " << packetsRead << " packets. Found " << errorCount << " errors and " << warningCount << " warnings" << endl;

    return errorCount == 0 && warningCount == 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::setArchivingState(bool active) {
    archivingActive = active;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ArchiveManager::getArchivingState() const {
    return archivingActive;
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
    stream.open(archiveFile.c_str(), ios::out | ios::app | ios::binary);
    if (stream.fail()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open archive file \"" << archiveFile << "\"" << endl;
        return;
    }

    for (vector<ArchivePacket>::const_iterator it = packets.begin(); it != packets.end(); ++it) {
        //
        // Only save the packet to the archive if it is newer than the newest packet in the archive
        //
        if (newestPacket.getDateTimeFields() < it->getDateTimeFields()) {
            stream.write(it->getBuffer(), ArchivePacket::BYTES_PER_ARCHIVE_PACKET);
            newestPacket = *it;
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Archived packet with time: "
                                                      << it->getDateTimeFields().formatDateTime() << endl;
        }
        else
            logger.log(VantageLogger::VANTAGE_INFO) << "Skipping archive of packet with time "
                                                    << it->getDateTimeFields().formatDateTime() << endl;
    }

    streampos fileSize = stream.tellp();
    archivePacketCount = fileSize / ArchivePacket::BYTES_PER_ARCHIVE_PACKET;

    stream.close();

    determineIfArchivingIsActive();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::findArchivePacketTimeRange() {
    ifstream stream(archiveFile.c_str(), ios::in | ios::binary | ios::ate);

    streampos fileSize = stream.tellg();
    archivePacketCount = fileSize / ArchivePacket::BYTES_PER_ARCHIVE_PACKET;

    if (fileSize >= ArchivePacket::BYTES_PER_ARCHIVE_PACKET) {
        byte buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];

        //
        // Read the packet at the beginning of the file
        //
        stream.seekg(0, ios::beg);
        stream.read(buffer, sizeof(buffer));
        ArchivePacket packet(buffer);
        oldestPacket.updateArchivePacketData(buffer, 0);

        //
        // Read the packet at the end of the file
        //
        stream.seekg(-ArchivePacket::BYTES_PER_ARCHIVE_PACKET, ios::end);
        stream.read(buffer, sizeof(buffer));
        newestPacket.updateArchivePacketData(buffer);
        determineIfArchivingIsActive();
    }
    else {
        archivePacketCount = 0;
    }

    stream.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
ArchiveManager::determineIfArchivingIsActive() {

    archivingActive = false;

    if (archivePacketCount == 0 || station.getArchivePeriod() == 0)
        return;

    archivingActive = newestPacket.getEpochDateTime() > (time(0) - (station.getArchivePeriod() * 60));
}
}
