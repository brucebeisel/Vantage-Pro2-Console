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
#ifndef STORM_ARCHIVE_H
#define STORM_ARCHIVE_H

#include <string>
#include <fstream>
#include <vector>
#include "Weather.h"
#include "StormData.h"

namespace vws {

class GraphDataRetriever;
class VantageLogger;

/**
 * Class to manager the storm archive. Note that the console only stores data for the past 24 storms.
 * This class will read the storm data and archive it.
 */
class StormArchiveManager {
public:
    /**
     * Constructor.
     *
     * @param archiveDirectory The directory into which the archive will be stored
     * @param dataRetriever    The object that knows how to query the console's graph data`
     */
    StormArchiveManager(const std::string & archiveDirectory, GraphDataRetriever & dataRetriever);

    /**
     * Destructor.
     */
    virtual ~StormArchiveManager();

    /**
     * Update the storm archive.
     */
    void updateArchive();

    /**
     * Query the storm data.
     *
     * @param start The lower range of the query
     * @param end   The upper range of the query
     * @param list  The results of the query
     */
    DateTimeFields queryStorms(const DateTimeFields & start, const DateTimeFields & end, std::vector<StormData> & list) const;

    /**
     * Format in JSON the provided list of storms.
     *
     * @param storms The list of storms to format in JSON
     * @return The formatted JSON
     */
    static std::string formatStormJSON(const std::vector<StormData> & storms);

private:
    /**
     * Read a record from the storm archive.
     *
     * @param fs   The file stream that is reading from the storm archive
     * @param data The resulting data that was read from the storm archive
     * @return True if the read was successful
     */
    bool readRecord(std::fstream & fs, StormData & data) const;

    /**
     * Write a record to the storm archive.
     *
     * @param fs   The file stream that is writing to the storm archive
     * @param data The storm data to be written
     */
    void writeRecord(std::fstream & fs, StormData & data);

    /**
     * Validate that the archive is valid.
     *
     * @param fs The file stream that is connected to the archive file
     * @return True if the archive is valid
     */
    bool validateArchive(std::fstream & fs) const;

    //
    // Format of record: YYYY-MM-DD<SP>YYYY-MM-DD<SP>00.00<LF>
    //
    static constexpr int              STORM_RECORD_LENGTH = 28;
    static constexpr std::string_view STORM_ARCHIVE_FILENAME = "storm-archive.dat";

    std::string          stormArchiveFilename;
    GraphDataRetriever & dataRetriever;
    VantageLogger &      logger;
};

} /* namespace vws */

#endif /* STORM_ARCHIVE_H */
