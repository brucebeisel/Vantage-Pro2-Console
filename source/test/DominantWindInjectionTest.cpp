/*
 * Copyright (C) 2025 Bruce Beisel
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
#include <string.h>
#include <vector>
#include <fstream>

#include "LoopPacket.h"
#include "Loop2Packet.h"
#include "VantageProtocolConstants.h"
#include "VantageDecoder.h"
#include "CurrentWeather.h"
#include "DominantWindDirections.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace vws;
using namespace std;

/**
 * This test code is to inject LOOP packet files into the dominant wind class to test against real world data.
 */
int
main(int argc, char * argv[]) {
    vector<std::string> headings;

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    DominantWindDirections dominantWinds(".");

    if (argc != 2) {
        cout << "Usage: DominantWindInjectionTest <loop packet file>" << endl;
        exit(1);
    }

    const char * loopPacketFile = argv[1];

    LoopPacket loopPacket;
    Loop2Packet loop2Packet;
    char loopBuffer[LoopPacket::LOOP_PACKET_SIZE];
    char loop2Buffer[Loop2Packet::LOOP2_PACKET_SIZE];

    ifstream stream(loopPacketFile, ifstream::in | ios::binary);

    int record = 0;
    while (stream) {
        DateTime time;
        int packetType;
        Measurement<Speed> windSpeed;
        Measurement<Heading> windDirection;
        stream.read(reinterpret_cast<char *>(&time), sizeof(time));
        stream.read(reinterpret_cast<char *>(&packetType), sizeof(packetType));

        if (stream) {
            if (packetType == LoopPacket::LOOP_PACKET_TYPE) {
                stream.read(loopBuffer, sizeof(loopBuffer));

                if (stream) {
                    loopPacket.decodeLoopPacket(loopBuffer);
                    windSpeed = loopPacket.getWindSpeed();
                    windDirection = loopPacket.getWindDirection();
                }

            }
            else if (packetType == Loop2Packet::LOOP2_PACKET_TYPE) {
                stream.read(loop2Buffer, sizeof(loop2Buffer));
                if (stream) {
                    loop2Packet.decodeLoop2Packet(loop2Buffer);
                    windSpeed = loop2Packet.getWindSpeed();
                    windDirection = loop2Packet.getWindDirection();
                }
            }
        }

        if (stream) {
            dominantWinds.processWindSample(time, windDirection.getValue(), windSpeed.getValue());
        }
    }
}
