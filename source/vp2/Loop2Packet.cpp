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
#include <iostream>
#include "BitConverter.h"
#include "UnitConverter.h"
#include "Loop2Packet.h"
#include "VantageConstants.h"
#include "VantageCRC.h"
#include "VantageDecoder.h"



using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Loop2Packet::Loop2Packet() : log(VantageLogger::getLogger("Loop2Packet")),
                             rain15Minute(0.0),
                             rainHour(0.0),
                             rain24Hour(0.0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Loop2Packet::~Loop2Packet() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed() const {
    return windSpeed;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindGust10Minute() const {
    return windGust10Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
Loop2Packet::getWindDirection() const {
    return windDirection;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Heading> &
Loop2Packet::getWindGustDirection10Minute() const {
    return windGustDirection10Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed2MinuteAverage() const {
    return windSpeed2MinuteAverage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Speed> &
Loop2Packet::getWindSpeed10MinuteAverage() const {
    return windSpeed10MinuteAverage;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getRain15Minute() const {
    return rain15Minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getRainHour() const {
    return rainHour;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Rainfall
Loop2Packet::getRain24Hour() const {
    return rain24Hour;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getDewPoint() const {
    return dewPoint;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getHeatIndex() const {
    return heatIndex;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getWindChill() const {
    return windChill;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Temperature> &
Loop2Packet::getThsw() const {
    return thsw;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const Measurement<Pressure> &
Loop2Packet::getAtmPressure() const {
    return atmPressure;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
Loop2Packet::decodeLoop2Packet(const byte buffer[]) {

    //
    // Perform packet validation before decoding the actual data
    //
    if (!VantageCRC::checkCRC(buffer, 97)) {
        log.log(VantageLogger::VANTAGE_ERROR) << "LOOP2 packet failed CRC check" << endl;
        return false;
    }

    if (buffer[0] != 'L' || buffer[1] != 'O' || buffer[2] != 'O') {
        log.log(VantageLogger::VANTAGE_ERROR) << "LOOP2 buffer does not begin with LOO: "
                                      << "[0] = " << buffer[0] << " [1] = " << buffer[1] << " [2] = " << buffer[2] << endl;
        return false;
    }

    int packetType = BitConverter::toInt8(buffer, 4);
    if (packetType != LOOP2_PACKET_TYPE) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Invalid packet type for LOOP2 packet. "
                                      << "Expected " << LOOP2_PACKET_TYPE << " Received: " << packetType << endl;
        return false;
    }

    if (buffer[95] != VantageConstants::LINE_FEED || buffer[96] != VantageConstants::CARRIAGE_RETURN) {
        log.log(VantageLogger::VANTAGE_ERROR) << "<LF><CR> not found" << endl;
        return false;
    }

    windSpeed = VantageDecoder::decodeWindSpeed(buffer, 14);
    windDirection = VantageDecoder::decodeWindDirection(buffer, 16);
    windSpeed10MinuteAverage = VantageDecoder::decodeAverageWindSpeed(buffer, 18);
    windSpeed2MinuteAverage = VantageDecoder::decodeAverageWindSpeed(buffer, 20);
    windGust10Minute = VantageDecoder::decode16BitWindSpeed(buffer, 22);
    windGustDirection10Minute = VantageDecoder::decodeWindDirection(buffer, 24);

    rain15Minute = VantageDecoder::decodeRain(buffer, 52);
    rainHour = VantageDecoder::decodeRain(buffer, 54);
    rain24Hour = VantageDecoder::decodeRain(buffer, 58);

    VantageDecoder::decodeNonScaled16BitTemperature(buffer, 30, dewPoint);
    VantageDecoder::decodeNonScaled16BitTemperature(buffer, 35, heatIndex);
    VantageDecoder::decodeNonScaled16BitTemperature(buffer, 37, windChill);
    VantageDecoder::decodeNonScaled16BitTemperature(buffer, 39, thsw);

    VantageDecoder::decodeBarometricPressure(buffer, 65, atmPressure);

    return true;
}
}
