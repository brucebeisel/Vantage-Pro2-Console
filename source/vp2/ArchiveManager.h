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

#include "VantageLogger.h"
#include "WeatherTypes.h"

namespace vws {
class ArchivePacket;
class VantageWeatherStation;

/**
 * The ArchiveManager class manages a file that contains the raw data read from the DUMP and DMPAFT command of the Vantage console.
 * This archive acts as a augmented storage for the console.
 */
class ArchiveManager {
public:
    /**
     * Constructor.
     * 
     * @param archiveFilename The file in which the archive will be maintained
     */
    ArchiveManager(const std::string & archiveFilename, VantageWeatherStation & station);

    /**
     * Destructor.
     */
    ~ArchiveManager();

    /**
     * Synchronize the archive file with the contents from the weather station.
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

    std::string              archiveFile;
    DateTime                 newestPacketTime;
    DateTime                 oldestPacketTime;
    VantageWeatherStation &  station;
    VantageLogger            logger;
};
}

#endif
