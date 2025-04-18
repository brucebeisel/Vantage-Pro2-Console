/* 
 * Copyright (C) 2025 Bruce Beisel
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
#ifndef ARCHIVE_MANAGER_H
#define ARCHIVE_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

#include "WeatherTypes.h"
#include "ArchivePacket.h"

namespace vws {
class VantageLogger;

static const std::string DEFAULT_ARCHIVE_FILE = "weather-archive.dat";
static const std::string ARCHIVE_BACKUP_FILENAME_TAIL = "weather-archive-backup.dat";
static const std::string ARCHIVE_BACKUP_DIR = "/backup";
static const std::string ARCHIVE_SAVE_FILE_PREFIX = "save_";
static const std::string ARCHIVE_VERIFY_LOG = "/weather-archive-verify.log";
static const std::string PACKET_SAVE_DIR = "/packets";

/**
 * The ArchiveManager class manages a file that contains the raw data read from the DMP and DMPAFT command of the Vantage console.
 * This archive acts as augmented storage for the console. The console has a storage capacity of 2450 records which translates to
 * approximately 42 hours of storage at 1 minute intervals. Not only will this class keep the console memory and the disk archive in sync,
 * it will also keep backups that will enable the archive to be restored in case of an error.
 *
 * Note that the Vantage archive has a limitation with daylight savings time. When daylight savings time starts in the spring,
 * it behaves as you would expect. Assuming a 5 minute archive interval, you will find a record at 1:55 AM followed by a record
 * at 3:00 AM. Per Bruce Johnson at Davis Instruments, the logger will ignore any records for which it already has a record.
 * When DST ends, the clock is turned backward. The time of the final record during DST will be 1:55 AM. The next record that
 * the logger attempts to save is 1:00 AM. Since this record already exists, the logger will ignore the data. This behavior
 * will continue until 2:00 AM, when the logger will resume logging. From a time since epoch time perspective, there will be a
 * 3900 second gap in the logger. 3600 for the DST change and 300 for the 5 minute archive interval. All of this means that
 * this software must always assume that the 1 AM hour that occurs on the day DST ends is with DST on.
 * The issue is that some operating systems will default to DST on during the 1 AM hour, other will default to off. So we need
 * to compensate for the inconsistency.
 *
 */
class ArchiveManager {
public:
    /**
     * Constructor.
     * 
     * @param dataDirctory The directory into which the archive will be written
     */
    ArchiveManager(const std::string & dataDirectory);

    /**
     * Constructor where the archive file is specified.
     *
     * @param dataDirctory The directory into which the archive will be written
     * @param archiveFile  The name of the file to be used as the archive
     */
    ArchiveManager(const std::string & dataDirectory, const std::string & archiveFile);

    /**
     * Destructor.
     */
    ~ArchiveManager();

    /**
     * Add a list of packets to the archive.
     *
     * @param packets The list packets to be added to the archive
     */
    void addPacketsToArchive(const std::vector<ArchivePacket> & packets);

    /**
     * Query the archive records that occur between the specified times (inclusive).
     *
     * @param startTime The time that is used as the lower bound for the query
     * @param endTime   The time that is used as the upper bound for the query
     * @param list      The list into which any found archive records will be added
     * @return The time of the last record in the list
     */
    DateTimeFields queryArchiveRecords(const DateTimeFields & startTime, const DateTimeFields & endTime, std::vector<ArchivePacket> & list) const;

    /**
     * Query the archive records for a single day.
     *
     * @param date The time that is used as the date to query, the time fields are ignored
     * @param list The list into which any found archive records will be added
     * @return The number of record returned
     */
    int queryArchiveRecordsForDay(const DateTimeFields & date, std::vector<ArchivePacket> & list) const;

    /**
     * Get the newest record from the archive.
     *
     * @param packet The packet to which the newest record will be written
     * @return True if data was written to the "packet" argument
     */
    bool getNewestRecord(ArchivePacket & packet) const;

    /**
     * Get the time range of data in the archive.
     *
     * @param[out] oldest Reference to a DateTime that will be filled in with the oldest packet time or set to zero if the archive is empty
     * @param[out] newest Reference to a DateTime that will be filled in with the newest packet time or set to zero if the archive is empty
     * @param[out] count  The number of archive records in the archive
     */
    void getArchiveRange(DateTimeFields & oldest, DateTimeFields & newest, int & count) const;

    /**
     * Clear the archive file. This should only be used after the weather station has been moved to a new location or
     * when installing a new weather station.
     *
     * @return True if successful
     */
    bool clearArchiveFile();

    /**
     * Backup the archive file.
     * This is a safety feature to preserve data before clearing the archive.
     *
     * @param now  Optional time used to override default behavior for test purposes
     * @return True if successful
     */
    bool backupArchiveFile(DateTime now = 0);

    /**
     * Trim the backup directory to a reasonable number of backup files.
     */
    void trimBackupDirectory();

    /**
     * Restore the current archive file from a backup.
     *
     * @param backupFile The file from which restore the current archive file
     * @return True if the restore succeeded
     */
    bool restoreArchiveFile(const std::string & backupFile);

    /**
     * Get the list of backup archive files.
     *
     * @param fileList The vector into which the backup archive files will be added
     * @return True if successful
     */
    bool getBackupFileList(std::vector<std::string> & fileList) const;

    /**
     * Verify that the current archive file is good.
     *
     * @return True if the archive file is good
     */
    bool verifyCurrentArchiveFile() const;

    /**
     * Verify that the specified archive file is good.
     *
     * @param archiveFilePath   The path to the archive file to be verified
     * @param verifyLogFilePath The path to the log file containing verification messages
     * @return True if the archive file is good
     */
    bool verifyArchiveFile(const std::string & archiveFilePath, bool logResults = false) const;

private:
    static constexpr int BACKUP_RETAIN_DAYS = 30;

    /**
     * Position the stream to begin reading the archive based on the time.
     *
     * @param is         The stream that has the archive open
     * @param searchTime The time to search within the archive
     * @param afterTime  Whether the stream will be position on or after the search time
     */
    void positionStream(std::istream & is, DateTime searchTime, bool afterTime) const;

    /**
     * Save a packet to a file that can be replayed at a later time.
     *
     * @param packet The packet to save
     */
    void savePacketToFile(const ArchivePacket & packet);

    /**
     * Finds the time range of the archive and set the packet time members.
     */
    void findArchivePacketTimeRange();

    const std::string        archiveFile;            // The name of the archive file
    const std::string        packetSaveDirectory;    // The directory into which the packets will be saved
    const std::string        archiveBackupDir;       // The name of the archive backup directory
    const std::string        archiveVerifyLog;       // The name of the file where the verification results are written
    DateTime                 nextBackupTime;         // The next time the archive should be backed up
    ArchivePacket            newestPacket;
    ArchivePacket            oldestPacket;
    int                      archivePacketCount;     // The number of packets in the archive
    VantageLogger &          logger;
    mutable std::mutex       mutex;                  // The mutex to protect the archive file against access by multiple threads
};
}

#endif
