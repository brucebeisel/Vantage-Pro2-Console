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
#include "string.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "ArchivePacket.h"
#include "VantageProtocolConstants.h"
#include "VantageDecoder.h"
#include "Weather.h"

using namespace std;
using namespace vws;

static const char USAGE_MESSAGE[] = "Usage: archive-dumper [-v] [-b] <filename>";

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

    char buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];

    ifstream stream(file, ifstream::in | ios::binary);

    int record = 0;
    while (true) {
        stream.read(buffer, sizeof(buffer));
        if (!stream) {
            stream.close();
            exit(0);
        }

        ArchivePacket packet(buffer, 0);

        if (verbose) {
            if (dumpBinary)
                cout << Weather::dumpBuffer(buffer, sizeof(buffer));

            cout << packet.formatJSON() << endl << endl;
        }
        else {
            cout << setw(5) << setfill('0') << record << " - " << packet.getPacketDateTimeString() << " " << Weather::formatDateTime(packet.getEpochDateTime()) << endl;
            if (dumpBinary)
                cout << Weather::dumpBuffer(buffer, sizeof(buffer));
        }

        record++;
    }
}
