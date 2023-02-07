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
#ifndef CURRENT_WEATHER_MANAGER_H
#define CURRENT_WEATHER_MANAGER_H

#include "../vws/CurrentWeather.h"
#include "../vws/DominantWindDirections.h"
#include "../vws/VantageLogger.h"
#include "../vws/VantageWeatherStation.h"

namespace vws {

class CurrentWeatherPublisher;

class CurrentWeatherManager : public VantageWeatherStation::LoopPacketListener {
public:
    /**
     * Constructor.
     *
     * @param cwPublisher The publisher of current weather data
     */
    CurrentWeatherManager(const std::string & archiveDir, CurrentWeatherPublisher & cwPublisher);

    /**
     * Destructor.
     */
    virtual ~CurrentWeatherManager();

    //
    // TODO Add methods to write the LOOP and LOOP2 packet data to a ring buffer that can
    // hold 24 hours of data.
    // Also write methods to query the archived data.
    //
    /**
     * Save the LOOP/LOOP2 packet pair to the archive file
     */
    void writeLoopArchive(DateTime packetTime, int packetType, const byte * packetData, size_t length);

    /**
     * Process a LOOP packet in a callback.
     *
     * @param packet The LOOP packet
     * @return True if the loop packet processing loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & packet);

    /**
     * Process a LOOP2 packet in a callback.
     *
     * @param packet The LOOP2 packet
     * @return True if the loop packet processing loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & packet);

private:
    std::string               archiveDirectory;
    CurrentWeatherPublisher & currentWeatherPublisher;
    CurrentWeather            currentWeather;
    bool                      firstLoop2PacketReceived;
    DominantWindDirections    dominantWindDirections;   // The past wind direction measurements used to determine the arrows on the wind display
    VantageLogger             logger;
};

} /* namespace vws */

#endif
