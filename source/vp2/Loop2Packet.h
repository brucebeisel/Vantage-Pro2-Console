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
#ifndef LOOP2_PACKET_H
#define LOOP2_PACKET_H

#include "VantageLogger.h"
#include "Weather.h"

namespace vws {

/**
 * Class that decodes and holds the data from the Vantage LOOP2 packet.
 */
class Loop2Packet {
public:
    static const int LOOP2_PACKET_SIZE = 99;

    /**
     * Constructor.
     */
    Loop2Packet();

    /**
     * Destructor.
     */
    virtual ~Loop2Packet();

    /**
     * Parse the LOOP2 packet.
     * 
     * @param The buffer from which to decode the packet
     * @return True if the buffer was decoded successfully
     */
    bool decodeLoop2Packet(const byte[]);

    const Measurement<Speed> &       getWindSpeed() const;
    const Measurement<Heading> &     getWindDirection() const;
    const Measurement<Speed> &       getWindGust10Minute() const;
    const Measurement<Heading> &     getWindGustDirection10Minute() const;
    const Measurement<Speed> &       getWindSpeed2MinuteAverage() const;
    const Measurement<Speed> &       getWindSpeed10MinuteAverage() const;
    Rainfall                         getRainHour() const;
    Rainfall                         getRain15Minute() const;
    Rainfall                         getRain24Hour() const;
    const Measurement<Temperature> & getDewPoint() const;
    const Measurement<Temperature> & getHeatIndex() const;
    const Measurement<Temperature> & getWindChill() const;
    const Measurement<Temperature> & getThsw() const;
    const Measurement<Pressure> &    getAtmPressure() const;

private:
    static const int LOOP2_PACKET_TYPE = 1;

    Measurement<Speed>       windSpeed;
    Measurement<Heading>     windDirection;
    Measurement<Speed>       windGust10Minute;
    Measurement<Heading>     windGustDirection10Minute;
    Measurement<Speed>       windSpeed2MinuteAverage;
    Measurement<Speed>       windSpeed10MinuteAverage;
    Rainfall                 rain15Minute;
    Rainfall                 rainHour;
    Rainfall                 rain24Hour;
    Measurement<Temperature> dewPoint;
    Measurement<Temperature> heatIndex;
    Measurement<Temperature> windChill;
    Measurement<Temperature> thsw;
    Measurement<Pressure>    atmPressure;
    VantageLogger            log;
};
}
#endif
