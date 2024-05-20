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
#ifndef CURRENTWEATHER_H
#define CURRENTWEATHER_H

#include <string>
#include <vector>

#include "Loop2Packet.h"
#include "LoopPacket.h"

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
     * Set the underlying LOOP data.
     *
     * @param loopPacket The LOOP packet that was read in the most recent loop through the current weather processor
     */
    void setLoopData(const LoopPacket & loopPacket);

    /**
     * Set the underlying LOOP2 data.
     *
     * @param loop2Packet The LOOP2 packet that was read in the most recent loop through the current weather processor
     */
    void setLoop2Data(const Loop2Packet & loopPacket);

    /**
     * Set the time that the loop data was created.
     * This is needed because the LOOP and LOOP2 packets do not have a time field.
     *
     * @param time The time to associate with the current weather data
     */
    void setPacketTime(DateTime time);

    /**
     * Get the time that the loop data was created.
     *
     * @return The packet time
     */
    DateTime getPacketTime() const;

    /**
     * Set the dominant wind direction data that is used to create the dominant wind direction element.
     *
     * @param dominantWindDirData The dominant wind direction data
     */
    void setDominantWindDirectionData(const std::vector<std::string> & dominantWindDirData);

    /**
     * Get the underlying LOOP packet.
     *
     * @return A reference to the LOOP packet
     */
    const LoopPacket & getLoopPacket() const;

    /**
     * Get the underlying LOOP2 packet.
     *
     * @return A reference to the LOOP2 packet
     */
    const Loop2Packet & getLoop2Packet() const;

    /**
     * Get the most recent wind speed, which can be from either the LOOP or LOOP2 packet.
     */
    const Measurement<Speed> & getWindSpeed() const;

    /**
     * Get the most recent wind direction, which can be from either the LOOP or LOOP2 packet.
     */
    const Measurement<Heading> & getWindDirection() const;

    /**
     * Format the Current Weather JSON message.
     * 
     * @param pretty Whether to format the JSON with newlines and spaces
     * @return The formatted current weather message
     */
    std::string formatJSON(bool pretty = false) const;

private:
    LoopPacket               loopPacket;
    Loop2Packet              loop2Packet;
    DateTime                 packetTime;
    std::vector<std::string> dominantWindDirections;

    //
    // Since wind data changes frequently, store the wind from both loop packets
    //
    Measurement<Speed>   windSpeed;
    Measurement<Heading> windDirection;
};
}

#endif /* CURRENTWEATHER_H */
