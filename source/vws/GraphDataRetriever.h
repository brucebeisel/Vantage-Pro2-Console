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
#ifndef GRAPH_DATA_RETRIEVER_H
#define GRAPH_DATA_RETRIEVER_H

#include <vector>
#include "VantageWeatherStation.h"

namespace vws {
class VantageWeatherStation;

struct StormData {
    DateTime stormStart;
    DateTime stormEnd;
    Rainfall stormRain;
};

class GraphDataRetriever: public VantageWeatherStation::LoopPacketListener {
public:
    GraphDataRetriever(VantageWeatherStation & station);
    virtual ~GraphDataRetriever();

    virtual bool processLoopPacket(const LoopPacket & packet);
    virtual bool processLoop2Packet(const Loop2Packet & packet);

    /**
     * Get all of the storm data from the EEPROM.
     */
    bool retrieveStormData(std::vector<StormData> & stormData);

private:
    static constexpr int RAIN_STORM_RECORD_SIZE = 6;
    static constexpr int NUM_RAIN_STORM_RECORDS = 25;
    static constexpr int RAIN_STORM_DATA_SIZE = RAIN_STORM_RECORD_SIZE * NUM_RAIN_STORM_RECORDS;

    VantageWeatherStation & station;
    int nextRainStormDataPointer;
};

} /* namespace vws */

#endif /* SOURCE_VWS_GRAPHDATARETRIEVER_H_ */
