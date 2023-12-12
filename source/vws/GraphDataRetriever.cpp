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
#include "LoopPacket.h"
#include "Loop2Packet.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GraphDataRetriever::GraphDataRetriever(VantageWeatherStation & station) : station(station),
                                                                          nextRainStormDataPointer(0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GraphDataRetriever::~GraphDataRetriever() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
GraphDataRetriever::processLoopPacket(const LoopPacket & packet) {
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
GraphDataRetriever::processLoop2Packet(const Loop2Packet & packet) {
    //
    // Pick out the graph data pointers for easier access to the graph data
    //
    nextRainStormDataPointer = packet.getNextRainStormGraphPointer();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
GraphDataRetriever::retrieveStormData(std::vector<StormData> & storms) {
    storms.clear();

    byte buffer[RAIN_STORM_DATA_SIZE];

    if (!station.eepromBinaryRead(VantageEepromConstants::EE_RAIN_STORM_DATA_ADDRESS, sizeof(buffer), buffer))
        return false;

    //
    // This is a ring buffer, so we will read the entire buffer, storing the valid storms, then sort.
    //
    StormData storm;
    for (int i = 0; i < NUM_RAIN_STORM_RECORDS; i++) {
        storm.stormRain = VantageDecoder::decodeStormRain(buffer, i * 2);
        storm.stormStart = VantageDecoder::decodeStormDate(buffer, (2 * NUM_RAIN_STORM_RECORDS) + (i * 2));
        storm.stormEnd = VantageDecoder::decodeStormDate(buffer, (4 * NUM_RAIN_STORM_RECORDS) + (i * 2));
        if (storm.stormStart != 0)
            storms.push_back(storm);
    }

    std::sort(storms.begin(), storms.end(), [](const StormData & a, const StormData & b) {return a.stormStart < b.stormStart;});

    return true;
}

} /* namespace vws */
