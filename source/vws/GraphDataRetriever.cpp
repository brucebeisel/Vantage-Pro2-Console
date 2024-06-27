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
#include "GraphDataRetriever.h"

#include <algorithm>
#include "Weather.h"
#include "VantageWeatherStation.h"
#include "VantageEepromConstants.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "StormData.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GraphDataRetriever::GraphDataRetriever(VantageWeatherStation & station) : station(station), logger(&VantageLogger::getLogger("GraphDataRetriever")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GraphDataRetriever::~GraphDataRetriever() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
GraphDataRetriever::retrieveStormData(std::vector<StormData> & storms) {
    logger->log(VantageLogger::VANTAGE_DEBUG2) << "Retrieving storm data from EEPROM" << endl;
    storms.clear();

    byte buffer[EEPROM_STORM_DATA_SIZE];

    if (!station.eepromBinaryRead(VantageEepromConstants::EE_RAIN_STORM_DATA_ADDRESS, sizeof(buffer), buffer))
        return false;

    //
    // This is a ring buffer, so we will read the entire buffer, storing the valid storms, then sort.
    // The ring buffer actually has room for 25 storms, but the 25th record is always the "dashed" values and
    // is therefore ignored.
    // During a storm, a new rain storm record will be stored at midnight. It will contain the start date, an end date of 0,
    // and the amount of rain accumulated as of midnight. It is currently (12/2023) unknown if the record will be updated
    // on the next midnight if the storm has not ended yet.
    //
    for (int i = 0; i < NUM_RAIN_STORM_RECORDS; i++) {
        DateTimeFields stormStart = VantageDecoder::decodeStormDate(buffer, (STORM_RAINFALL_RECORD_SIZE * EEPROM_STORM_RECORDS) + (i * STORM_DATE_RECORD_SIZE));
        DateTimeFields stormEnd = VantageDecoder::decodeStormDate(buffer, ((STORM_RAINFALL_RECORD_SIZE + STORM_DATE_RECORD_SIZE) * EEPROM_STORM_RECORDS) + (i * STORM_DATE_RECORD_SIZE));
        Rainfall rainfall = VantageDecoder::decodeStormRain(buffer, i * STORM_RAINFALL_RECORD_SIZE);

        StormData storm(stormStart, stormEnd, rainfall);

        logger->log(VantageLogger::VANTAGE_DEBUG2) << "Retrieved storm record from EEPROM. Record[" << i << "]: "
                                                   << "Start: " << storm.getStormStart().formatDate()
                                                   << " End: " << storm.getStormEnd().formatDate()
                                                   << " Rainfall: " << storm.getStormRain() << endl;

        if (storm.hasStormEnded())
            storms.push_back(storm);

    }

    logger->log(VantageLogger::VANTAGE_DEBUG2) << "Retrieved " << storms.size() << " storm records from EEPROM" << endl;

    std::sort(storms.begin(), storms.end());

    return true;
}

} /* namespace vws */
