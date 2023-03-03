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
#ifndef CURRENT_WEATHER_SOCKET_H
#define CURRENT_WEATHER_SOCKET_H

#include <netinet/in.h>
#include <string>

#include "CurrentWeather.h"
#include "CurrentWeatherPublisher.h"
#include "VantageWeatherStation.h"

namespace vws {
class CurrentWeather;
class VantageLogger;

/**
 * Class that publishes the current weather using a UDP broadcast socket.
 */
class CurrentWeatherSocket : public CurrentWeatherPublisher {
public:

    /**
     * Constructor that creates and configures the UDP multicast socket.
     */
    CurrentWeatherSocket();

    /**
     * Destructor that closes the socket.
     */
    virtual ~CurrentWeatherSocket();

    /**
     * Initialize this class.
     */
    bool initialize();

    /**
     * Publish the current weather.
     *
     * @param cw The current weather to publish
     *
     */
    virtual void publishCurrentWeather(const CurrentWeather & cw);

private:
    /**
     * Connect with the WeatherSense collector.
     *
     * @return True if the socket was created and configured successfully
     */
    bool createSocket();

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
    VantageLogger &          logger;
};
}

#endif /* CURRENT_WEATHER_PUBLISHER_H */
