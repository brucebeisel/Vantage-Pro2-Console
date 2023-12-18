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
    storms.clear();

    byte buffer[EEPROM_STORM_DATA_SIZE];

    if (!station.eepromBinaryRead(VantageEepromConstants::EE_RAIN_STORM_DATA_ADDRESS, sizeof(buffer), buffer))
        return false;

    //
    // This is a ring buffer, so we will read the entire buffer, storing the valid storms, then sort.
    // Note that the current insert location in the ring buffer is marked by a start and end date of 0xFFFF
    // and a rainfall amount of 0.0 inches.
    // During a storm, a new rain storm record will be stored at midnight. It will contain the start date, an end date of 0,
    // and the amount of rain accumulated as of midnight. It is currently (12/2023) unknown if the record will be updated
    // on the next midnight if the storm has not ended yet.
    //
    StormData storm;
    for (int i = 0; i < NUM_RAIN_STORM_RECORDS; i++) {
        storm.stormRain = VantageDecoder::decodeStormRain(buffer, i * STORM_RAINFALL_RECORD_SIZE);
        storm.stormStart = VantageDecoder::decodeStormDate(buffer, (STORM_RAINFALL_RECORD_SIZE * EEPROM_STORM_RECORDS) + (i * STORM_DATE_RECORD_SIZE));
        storm.stormEnd = VantageDecoder::decodeStormDate(buffer, ((STORM_RAINFALL_RECORD_SIZE + STORM_DATE_RECORD_SIZE) * EEPROM_STORM_RECORDS) + (i * STORM_DATE_RECORD_SIZE));
        logger->log(VantageLogger::VANTAGE_DEBUG2) << "Retrieved storm record from EEPROM. Record[" << i << "]: "
                                                   << "Start: " << Weather::formatDate(storm.stormStart)
                                                   << " End: " << Weather::formatDate(storm.stormEnd)
                                                   << " Rainfall: " << storm.stormRain << endl;
        if (storm.stormStart != 0)
            storms.push_back(storm);

    }

    logger->log(VantageLogger::VANTAGE_DEBUG2) << "Retrieved " << storms.size() << " storm records from EEPROM" << endl;

    std::sort(storms.begin(), storms.end(), [](const StormData & a, const StormData & b) {return a.stormStart < b.stormStart;});

    return true;
}

} /* namespace vws */
