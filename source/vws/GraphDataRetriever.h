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
#ifndef GRAPH_DATA_RETRIEVER_H
#define GRAPH_DATA_RETRIEVER_H

#include <vector>
#include "VantageWeatherStation.h"

namespace vws {
class VantageWeatherStation;
class VantageLogger;

class GraphDataRetriever {
public:
    GraphDataRetriever(VantageWeatherStation & station);
    virtual ~GraphDataRetriever();

    /**
     * Get all of the storm data from the EEPROM.
     */
    bool retrieveStormData(std::vector<StormData> & stormData);

private:
    //
    // The storm data is stored in 3 parallel buffer arrays. The EEPROM allocates 25 records for each
    // array, but only 24 records are used.
    //
    static constexpr int STORM_RAINFALL_RECORD_SIZE = 2;
    static constexpr int STORM_DATE_RECORD_SIZE = 2;
    static constexpr int EEPROM_STORM_RECORDS = 25;
    static constexpr int NUM_RAIN_STORM_RECORDS = 24;
    static constexpr int EEPROM_STORM_DATA_SIZE = (STORM_RAINFALL_RECORD_SIZE * EEPROM_STORM_RECORDS) + (STORM_DATE_RECORD_SIZE * EEPROM_STORM_RECORDS * 2);

    VantageWeatherStation & station;
    VantageLogger * logger;
};

} /* namespace vws */

#endif /* GRAPH_DATA_RETRIEVER_H */
