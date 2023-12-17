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

namespace vws {

class GraphDataRetriever;
class VantageLogger;
struct StormData;

class StormArchiveManager {
public:
    StormArchiveManager(const std::string & archiveDirectory, GraphDataRetriever & dataRetriever);

    virtual ~StormArchiveManager();

    void updateArchive();

    DateTime queryStorms(DateTime start, DateTime end, std::vector<StormData> & list) const;

    std::string formatStormJSON(const std::vector<StormData> & storms) const;

private:
    bool readRecord(std::fstream & fs, StormData & data) const;
    void writeRecord(std::fstream & fs, StormData & data);

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
