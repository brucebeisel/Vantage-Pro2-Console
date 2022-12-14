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
#ifndef CURRENTWEATHER_H
#define CURRENTWEATHER_H

#include <string>

#include "DominantWindDirections.h"
#include "LoopPacket.h"
#include "Loop2Packet.h"

namespace vws {
/**
 * Class that contains the data needed to create a current weather message. The Vantage console has two packets that report the
 * current weather, the LOOP packet and the LOOP2 packet. This class combines those packets together with a set of wind directions
 * that mimic the recent wind direction indicators on the Vantage console.
 */
class CurrentWeather {
public:
    /**
     * Constructor.
     */
    CurrentWeather();

    /**
     * Destuctor.
     */
    virtual ~CurrentWeather();

    /**
     * Set the underlying data.
     *
     * @param loopPacket The LOOP packet that was read in the most recent loop through the current weather processor
     * @param loop2Packet The LOOP2 packet that was read in the most recent loop through the current weather processor
     * @param The dominant directions that the wind has been blowing over the last hour
     */
    //void setData(const LoopPacket & loopPacket, const Loop2Packet & loop2Packet, const WindDirectionSlices  & pastWindDirs);
    void setLoopData(const LoopPacket & loopPacket);
    void setLoop2Data(const Loop2Packet & loopPacket);
    void setDominantWindDirectionData(const std::vector<int> & windDirs);

    /**
     * Get the next packet field that was extracted from the LOOP packet.
     * 
     * @return The next packet field
     * TODO Can this be removed in favor of passing the next packet value in as a constructor argument or passed as an 
     * argument to processCurrentWeather()
     */
    int getNextPacket() const;

    /**
     * Format the Current Weather XML message.
     *
     * @return The formatted Current Weather message
     */
    std::string formatXML() const;

    /**
     * Format the Current Weather JSON message.
     * 
     * @return The formatted current weather message
     */
    std::string formatJSON() const;

private:
    LoopPacket             loopPacket;
    Loop2Packet            loop2Packet;
    std::vector<int>       dominantWindDirections;

    //
    // Since wind data changes frequently, store the wind from both loop packets
    //
    Measurement<Speed>   windSpeed;
    Measurement<Heading> windDirection;
    Measurement<Speed>   windSpeed10MinuteAverage;
};
}

#endif /* CURRENTWEATHER_H */
