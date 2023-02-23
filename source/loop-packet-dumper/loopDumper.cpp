/* 
 * Copyright (C) 2015 Bruce Beisel
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
#include "string.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "LoopPacket.h"
#include "Loop2Packet.h"
#include "VantageProtocolConstants.h"
#include "VantageDecoder.h"
#include "CurrentWeather.h"
#include "Weather.h"

using namespace std;
using namespace vws;

static const char USAGE_MESSAGE[] = "Usage: loop-dumper [-v] [-b] <filename>";

int
main(int argc, char *argv[]) {
    bool verbose = false;
    bool dumpBinary = false;
    char *file = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-v") == 0)
                verbose = true;
            else if (strcmp(argv[i], "-b") == 0)
                dumpBinary = true;
            else  {
                cerr << USAGE_MESSAGE << endl;
                exit(1);
            }
        }
        else
            file = argv[i];
    }

    if (file == NULL) {
        cerr << USAGE_MESSAGE << endl;
        exit(1);
    }

    VantageDecoder::setRainCollectorSize(.01);

    LoopPacket loopPacket;
    Loop2Packet loop2Packet;
    char loopBuffer[LoopPacket::LOOP_PACKET_SIZE];
    char loop2Buffer[Loop2Packet::LOOP2_PACKET_SIZE];

    ifstream stream(file, ifstream::in | ios::binary);
    string jsonString;
    char *buffer;
    long bufferLength;
    string loopString;
    CurrentWeather cw;

    int record = 0;
    while (true) {
        DateTime time;
        int packetType;
        stream.read(reinterpret_cast<char *>(&time), sizeof(time));
        stream.read(reinterpret_cast<char *>(&packetType), sizeof(packetType));
        if (!stream) {
            stream.close();
            exit(0);
        }

        if (packetType == LoopPacket::LOOP_PACKET_TYPE) {
            loopString = "LOOP  ";
            stream.read(loopBuffer, sizeof(loopBuffer));
            if (!stream) {
                stream.close();
                exit(0);
            }

            loopPacket.decodeLoopPacket(loopBuffer);
            buffer = loopBuffer;
            bufferLength = sizeof(loopBuffer);
            cw.setLoopData(loopPacket);
        }
        else if (packetType == Loop2Packet::LOOP2_PACKET_TYPE) {
            loopString = "LOOP2 ";
            stream.read(loop2Buffer, sizeof(loop2Buffer));
            if (!stream) {
                stream.close();
                exit(0);
            }
            loop2Packet.decodeLoop2Packet(loop2Buffer);
            buffer = loop2Buffer;
            bufferLength = sizeof(loop2Buffer);
            cw.setLoop2Data(loop2Packet);
        }

        if (verbose) {
            if (dumpBinary)
                cout << Weather::dumpBuffer(buffer, bufferLength);

            cout << cw.formatJSON() << endl << endl;
        }
        else {
            cout << loopString << setw(5) << setfill('0') << record << " - " << Weather::formatDateTime(time) << endl; // @suppress("Invalid overload") @suppress("Ambiguous problem")
            if (dumpBinary)
                cout << Weather::dumpBuffer(buffer, bufferLength);
        }

        record++;
    }
}
