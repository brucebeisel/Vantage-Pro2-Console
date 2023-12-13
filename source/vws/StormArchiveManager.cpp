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
#include "StormArchiveManager.h"

#include <string>
#include <fstream>
#include <iomanip>
#include <time.h>
#include "GraphDataRetriever.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
StormArchiveManager::StormArchiveManager(const string & archiveDirectory, GraphDataRetriever & retriever) : stormArchiveFilename(archiveDirectory + "/" + string(STORM_ARCHIVE_FILENAME)),
                                                                                                            dataRetriever(retriever),
                                                                                                            logger(VantageLogger::getLogger("StormArchive")){
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
StormArchiveManager::~StormArchiveManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
StormArchiveManager::updateArchive() {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Updating storm archive file at " << stormArchiveFilename << endl;

    fstream stream;
    stream.open(stormArchiveFilename.c_str(), ios::in | ios::out | ios::app);
    if (stream.fail()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open storm archive file \"" << stormArchiveFilename << "\"" << endl;
        return;
    }

    vector<StormData> stormData;
    if (!dataRetriever.retrieveStormData(stormData)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Error in retrieving storm data" << endl;
        return;
    }

    stream.seekg(-STORM_RECORD_LENGTH, ios::end);

    StormData lastRecord;
    DateTime lastRecordTime = 0;
    if (readRecord(stream, lastRecord)) {
        lastRecordTime = lastRecord.stormEnd;
    }

    for (StormData record : stormData) {
        if (record.stormStart > lastRecordTime)
            writeRecord(stream, record);
    }

    stream.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTime
StormArchiveManager::queryStorms(DateTime start, DateTime end, std::vector<StormData> & list) const {
    DateTime lastRecordTime = 0;

    fstream stream;
    stream.open(stormArchiveFilename.c_str(), ios::in);

    while (stream.good()) {
        StormData stormData;
        readRecord(stream, stormData);
        if (stormData.stormStart >= start && stormData.stormStart <= end) {
            list.push_back(stormData);
            lastRecordTime = stormData.stormStart;
        }
    }

    return lastRecordTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
StormArchiveManager::formatStormJSON(const std::vector<StormData> & storms) const {
    ostringstream oss;
    oss << "{ \"storms\" : [";
    bool first = true;
    for (StormData storm : storms) {
        if (!first) oss << ", "; first = false;

        oss << "{ "
            << "\"start\" : \"" << Weather::formatDate(storm.stormStart) << "\", "
            << "\"end\" : \"" << Weather::formatDate(storm.stormEnd) << "\", "
            << "\"rainfall\" : " << Weather::formatDate(storm.stormRain)
            << "}";
    }
    oss << "] }" << endl;

    return oss.str();
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
StormArchiveManager::readRecord(fstream & fs, StormData & data) const {
    char buffer[STORM_RECORD_LENGTH + 1];
    fs.read(buffer, STORM_RECORD_LENGTH);
    buffer[STORM_RECORD_LENGTH] = '\0';

    if (fs.eof())
        return false;

    char startDateString[20];
    char endDateString[20];
    double stormRain;
    sscanf(buffer, "%s %s %lf", startDateString, endDateString, &stormRain);

    data.stormStart = Weather::parseDate(startDateString);
    data.stormEnd = Weather::parseDate(endDateString);
    data.stormRain = stormRain;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
StormArchiveManager::writeRecord(fstream & fs, StormData & data) {
    string stormStartString;
    stormStartString = Weather::formatDate(data.stormStart);
    string stormEndString;
    stormEndString = Weather::formatDate(data.stormEnd);

    fs << stormStartString << " " << stormEndString << " " << fixed << setprecision(2) << setw(5) << data.stormRain << endl;
}
} /* namespace vws */
