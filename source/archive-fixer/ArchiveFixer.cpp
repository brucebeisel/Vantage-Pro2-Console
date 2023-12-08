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
    ofstream os(archiveFilename, ios::out | ios::binary);

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
        ArchivePacket packet(buffer);
        packetsRead++;

        DateTime currentPacketTime = packet.getDateTime();
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

    } while (!is.eof());

    cout << "Packets read: " << packetsRead << " Packets written: " << packetsWritten << endl;
    cout << "Discarded " << packetsRead - packetsWritten << " packets" << endl;

}
