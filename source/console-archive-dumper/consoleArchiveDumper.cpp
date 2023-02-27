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

#include "ArchivePacket.h"
#include "VantageProtocolConstants.h"
#include "VantageDecoder.h"
#include "Weather.h"
#include "VantageWeatherStation.h"
#include "VantageLogger.h"

using namespace std;
using namespace vws;

static const char USAGE_MESSAGE[] = "Usage: console-archive-dumper [-v] [-b] [-t]  <device name>";

int
main(int argc, char *argv[]) {
    bool verbose = false;
    bool dumpBinary = false;
    char *device = NULL;
    bool terse = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-v") == 0)
                verbose = true;
            else if (strcmp(argv[i], "-b") == 0)
                dumpBinary = true;
            else if (strcmp(argv[i], "-t") == 0)
                terse = true;
            else  {
                cerr << USAGE_MESSAGE << endl;
                exit(1);
            }
        }
        else
            device = argv[i];
    }

    if (device == NULL) {
        cerr << USAGE_MESSAGE << endl;
        exit(1);
    }


    VantageDecoder::setRainCollectorSize(.01);

    //VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);

    VantageWeatherStation ws(device, 19200);

    if (!ws.openStation()) {
        cerr << "Could not open weather console" << endl;
        exit(1);
    }

    if (!ws.wakeupStation()) {
        cerr << "Could not wake up console" << endl;
        exit(2);
    }

    vector<ArchivePacket> packets;

    ws.dump(packets);

    cout << "Retrieved " << packets.size() << " packets from console's archive" << endl;

    DateTime oldestRecord = 0;
    DateTime newestRecord = 0;
    int record = 0;
    for (auto packet : packets) {
        DateTime packetTime = packet.getDateTime();
        if (oldestRecord == 0) {
            oldestRecord = packetTime;
            newestRecord = packetTime;
        }
        else {
            oldestRecord = ::min(oldestRecord, packetTime);
            newestRecord = ::max(newestRecord, packetTime);
        }

        if (!terse) {
            const char * buffer = packet.getBuffer();
            if (verbose) {
                if (dumpBinary)
                    cout << Weather::dumpBuffer(buffer, sizeof(buffer));

                cout << packet.formatJSON() << endl << endl;
            }
            else {
                cout << setw(5) << setfill('0') << record << " - " << Weather::formatDateTime(packetTime) << endl;
                if (dumpBinary)
                    cout << Weather::dumpBuffer(buffer, sizeof(buffer));
            }
            record++;
        }
    }

    cout << "Archive contains " << packets.size()
         << " Date range: " << Weather::formatDateTime(oldestRecord)
         << " to " << Weather::formatDateTime(newestRecord) << endl;
}
