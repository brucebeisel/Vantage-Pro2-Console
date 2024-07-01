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
bool
StormArchiveManager::validateArchive(fstream & stream) const {
    streampos oldPosition = stream.tellg();
    stream.seekg(0, ios::end);

    if (!stream.good()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed seeking stream to end" << endl;
        return false;
    }

    long archiveSize = stream.tellg();

    if (!stream.good()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed querying archive size via stream.tellg()" << endl;
        return false;
    }

    if (archiveSize % STORM_RECORD_LENGTH != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Archive is not valid. It does not contain the right number of bytes for the fixed record archive. Size = "
                                                 << archiveSize << " Modulus: " << (archiveSize % STORM_RECORD_LENGTH) << endl;
        return false;
    }

    //
    // Put the stream back where the called had it
    //
    stream.seekg(oldPosition, ios::beg);

    return true;

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

    if (!validateArchive(stream))
        return;

    vector<StormData> stormData;
    if (!dataRetriever.retrieveStormData(stormData)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Error in retrieving storm data" << endl;
        return;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Read " << stormData.size() << " storm records from EEPROM" << endl;

    StormData lastRecord;
    DateTimeFields lastRecordTime;
    stream.seekg(0, ios::end);

    if (stream.tellg() >= STORM_RECORD_LENGTH) {
        stream.seekg(-STORM_RECORD_LENGTH, ios::end);

        if (readRecord(stream, lastRecord)) {
            lastRecordTime = lastRecord.getStormEnd();
            logger.log(VantageLogger::VANTAGE_DEBUG2) << "Last storm record time " << lastRecordTime.formatDate() << endl;
        }
        else {
            //
            // Unable to read the last record in the archive, clear out the storm vector so that no records are added to the archive
            //
            logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read last record of storm archive. Skipping update." << endl;
            stormData.clear();
        }
    }

    for (StormData record : stormData) {
        //
        // Only store new storms and storms that are not in progress (stormEnd == 0)
        //
        if (record.getStormStart() > lastRecordTime && record.getStormEnd().isDateTimeValid()) {
            logger.log(VantageLogger::VANTAGE_DEBUG2) << "Writing storm record with start time " << record.getStormStart().formatDate() << endl;
            writeRecord(stream, record);
        }
    }

    stream.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields
StormArchiveManager::queryStorms(const DateTimeFields & start, const DateTimeFields & end, std::vector<StormData> & list) const {
    list.clear();
    DateTimeFields lastRecordTime;

    fstream stream;
    stream.open(stormArchiveFilename.c_str(), ios::in);

    if (!validateArchive(stream))
        return lastRecordTime;

    while (stream.good()) {
        StormData stormData;
        if (readRecord(stream, stormData) && stormData.getStormStart() >= start && stormData.getStormStart() <= end) {
            list.push_back(stormData);
            lastRecordTime = stormData.getStormStart();
        }
    }

    return lastRecordTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
StormArchiveManager::formatStormJSON(const std::vector<StormData> & storms) {
    ostringstream oss;
    oss << "{ \"storms\" : [";
    bool first = true;
    for (StormData storm : storms) {
        if (!first) oss << ", "; first = false;

        oss << "{ "
            << "\"start\" : \"" << storm.getStormStart().formatDate() << "\", "
            << "\"end\" : \"" << storm.getStormEnd().formatDate() << "\", "
            << "\"rainfall\" : " << storm.getStormRain()
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

    if (fs.eof()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Reached EOF when reading storm archive record" << endl;
        return false;
    }

    if (!fs.good()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Reading storm archive record failed. "
                                                   << " Bad bit: " << boolalpha << fs.bad()
                                                   << " Fail bit: " << fs.fail() << noboolalpha << endl;
        return false;
    }

    char startDateString[20];
    char endDateString[20];
    double stormRain;

    if (sscanf(buffer, "%s %s %lf", startDateString, endDateString, &stormRain) != 3) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Storm record did not contain 3 tokens <start> <end> <rain>: '" << buffer << "'" << endl;
        return false;
    }

    DateTimeFields stormStart;
    DateTimeFields stormEnd;

    if (!stormStart.parseDate(startDateString)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Storm start date string is not valid: '" << startDateString << "'" << endl;
        return false;
    }

    if (!stormEnd.parseDate(endDateString)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Storm end date string is not valid: '" << endDateString << "'" << endl;
        return false;
    }

    data.setStormData(stormStart, stormEnd, stormRain);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
StormArchiveManager::writeRecord(fstream & fs, StormData & data) {
    string stormStartString;
    stormStartString = data.getStormStart().formatDate();
    string stormEndString;
    stormEndString = data.getStormEnd().formatDate();

    fs << stormStartString << " " << stormEndString << " " << fixed << setprecision(2) << setw(5) << data.getStormRain() << endl;
}
} /* namespace vws */
