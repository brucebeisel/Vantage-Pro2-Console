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
#ifndef CURRENT_WEATHER_PUBLISHER_H
#define CURRENT_WEATHER_PUBLISHER_H

#include <netinet/in.h>
#include <string>

#include "VantageLogger.h"
#include "Weather.h"
#include "CurrentWeather.h"
#include "DominantWindDirections.h"
#include "VantageWeatherStation.h"

namespace vws {
class CurrentWeather;

/**
 * Class that publishes the current weather using a UDP broadcast socket.
 */
class CurrentWeatherPublisher : public VantageWeatherStation::LoopPacketListener {
public:

    /**
     * Constructor that creates and configures the UDP multicast socket.
     */
    CurrentWeatherPublisher();

    /**
     * Destructor that closes the socket.
     */
    virtual ~CurrentWeatherPublisher();

    //
    // TBD move the loop packet processing to a LoopPacketManager class that will
    // save 24 hours of loop packets. This way the past 24 hours of current weather
    // records can be queried for display or analysis purposes.
    //

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

    /**
     * Connect with the WeatherSense collector.
     *
     * @return True if the socket was created and configured successfully
     */
    bool createSocket();

private:
    /**
     * Publish the current weather.
     * 
     * @param cw The current weather to publish
     * 
     */
    void sendCurrentWeather(const CurrentWeather & cw);

    /**
     * Get the local IP address for the multicast socket. Note that this
     * returns the first non-loopback interface found.
     *
     * @param saddr The socket address of the found IP address
     * @return True if an IP address was found
     */
    bool getLocalIpAddress(struct sockaddr_in & saddr);

    static const std::string MULTICAST_HOST;
    static const int         MULTICAST_PORT = 11461;
    static const int         NO_SOCKET = -1;
    int                      socketId;
    struct sockaddr_in       groupAddr;
    VantageLogger            log;
    CurrentWeather           currentWeather;
    bool                     firstLoop2PacketReceived;
    DominantWindDirections   dominantWindDirections;   // The past wind direction measurements used to determine the arrows on the wind display
};
}

#endif /* CURRENT_WEATHER_PUBLISHER_H */
