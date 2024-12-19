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
#include <fstream>
#include <iostream>
#include "ArchivePacket.h"
#include "Weather.h"

using namespace std;
using namespace vws;

int
main(int argc, char *argv[]) {

    if (argc != 3) {
        cerr << "Usage: archive-fixer <archive file> <output file>" << endl;
        exit(1);
    }

    const char * archiveFilename = argv[1];
    const char * outputFilename = argv[2];

    ifstream is(archiveFilename, ios::in | ios::binary);
    ofstream os(outputFilename, ios::out | ios::binary);

    if (is.fail()) {
        cerr << "Failed to open archive file '" << archiveFilename << " for reading" << endl;
        exit(2);
    }

    if (os.fail()) {
        cerr << "Failed to open file '" << outputFilename << " for writing" << endl;
        exit(2);
    }

    int packetsRead = 0;
    int packetsWritten = 0;
    DateTime lastPacketTime = 0;
    char buffer[ArchivePacket::BYTES_PER_ARCHIVE_PACKET];
    do {
        is.read(buffer, sizeof(buffer));
        if (!is.eof()) {
            ArchivePacket packet(buffer);
            packetsRead++;

            DateTime currentPacketTime = packet.getEpochDateTime();
            cout << "Processing packet with time: " << Weather::formatDateTime(currentPacketTime) << endl;

            if (currentPacketTime > lastPacketTime) {
                os.write(buffer, sizeof(buffer));
                packetsWritten++;
                lastPacketTime = currentPacketTime;
            }
            else {
                cout << "Discarding packet with time " << Weather::formatDateTime(currentPacketTime) << endl
                     << " Packet is before packet with time: " << Weather::formatDateTime(lastPacketTime) << endl;
            }
        }

    } while (!is.eof());

    cout << "Packets read: " << packetsRead << " Packets written: " << packetsWritten << endl;
    cout << "Discarded " << packetsRead - packetsWritten << " packets" << endl;

}
