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
#ifndef ARCHIVE_MANAGER_H
#define ARCHIVE_MANAGER_H

#include <string>
#include <vector>

#include "WeatherTypes.h"

namespace vws {
class ArchivePacket;
class VantageWeatherStation;
class VantageLogger;

static const std::string ARCHIVE_FILE = "weather-archive.dat";

/**
 * The ArchiveManager class manages a file that contains the raw data read from the DMP and DMPAFT command of the Vantage console.
 * This archive acts as augmented storage for the console.
 */
class ArchiveManager {
public:
    /**
     * Constructor.
     * 
     * @param dataDirctory The directory into which the archive will be written
     */
    ArchiveManager(const std::string & dataDirectory, VantageWeatherStation & station);

    /**
     * Destructor.
     */
    ~ArchiveManager();

    /**
     * Synchronize the archive file with the contents from the console.
     *
     * @return True if successful
     */
    bool synchronizeArchive();

    /**
     * Get the archive records after the specified time.
     *
     * @param afterTime The time that is used to find archive records that are older
     * @param list      The list into which any found archive records will be added
     * @return The time of the last record in the list
     */ 
    DateTime getArchiveRecordsAfter(DateTime afterTime, std::vector<ArchivePacket> & list);

    /**
     * Query the archive records that occur between the specified times (inclusive).
     *
     * @param startTime The time that is used as the lower bound for the query
     * @param endTime   The time that is used as the upper bound for the query
     * @param list      The list into which any found archive records will be added
     * @return The time of the last record in the list
     */
    DateTime queryArchiveRecords(DateTime startTime, DateTime endTime, std::vector<ArchivePacket> & list);

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
     * @param[out] oldest Reference to a DateTime that will be filled in with the oldest packet time
     * @param[out] newest Reference to a DateTime that will be filled in with the newest packet time
     * @param[out] count  The number of archive records in the archive
     */
    void getArchiveRange(DateTime & oldest, DateTime & newest, int & count) const;

    /**
     * Clear the archive file. This should only be used after the weather station has been moved to a new location or
     * when installing a new weather station.
     *
     * @return True if successful
     */
    bool clearArchiveFile();

    /**
     * Set the state of archiving.
     * Note that this state can be explicitly set of implicitly determined based on the
     * archive interval and the time of the newest record in the archive.
     *
     * @param active True if archiving is active
     */
    void setArchivingState(bool active);

    /**
     * Get the state of archiving.
     *
     * @return True if archiving is active
     */
    bool getArchivingState() const;

private:
    static constexpr int SYNC_RETRIES = 5;

    /**
     * Position the stream to begin reading the archive based on the time.
     *
     * @param is         The stream that has the archive open
     * @param searchTime The time to search within the archive
     * @param afterTime  Whether the stream will be position on or after the search time
     * @return True if the stream was positioned successfully
     */
    bool positionStream(std::istream & is, DateTime searchTime, bool afterTime);

    /**
     * Add a single packet to the archive.
     *
     * @param packet The packet to add to the archive
     */
    void addPacketToArchive(const ArchivePacket & packet);

    /**
     * Add a list of packets to the archive.
     * 
     * @param packets The list packets to be added to the archive
     */
    void addPacketsToArchive(const std::vector<ArchivePacket> & packets);

    /**
     * Finds the time range of the archive and set the packet time members.
     */
    void findArchivePacketTimeRange();

    /**
     * Determine if archiving is active.
     * The newest packet time must be set for this method to determine if archiving is active or not.
     */
    void determineIfArchivingIsActive();

    std::string              archiveFile;          // The name of the archive file
    DateTime                 newestPacketTime;     // The time of the newest packet in the archive file
    DateTime                 oldestPacketTime;     // The time of the oldest packet in the archive file
    int                      archivePacketCount;   // The number of packets in the archive
    bool                     archivingActive;      // Whether archiving is active
    VantageWeatherStation &  station;              // Reference to the Vantage weather station object
    VantageLogger &          logger;
};
}

#endif
