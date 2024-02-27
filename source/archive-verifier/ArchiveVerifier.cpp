#include <fstream>
#include <iostream>
#include "ArchiveManager.h"
#include "VantageWeatherStation.h"
#include "SerialPort.h"
#include "ArchivePacket.h"
#include "Weather.h"

using namespace std;
using namespace vws;

int
main(int argc, char *argv[]) {

    if (argc != 2) {
        cerr << "Usage: archive-verifier <archive directory>" << endl;
        exit(1);
    }

    const char * archiveDir = argv[1];

    SerialPort port("/dev/tty", 9600);
    VantageWeatherStation station(port);
    ArchiveManager mgr(archiveDir, station);

    mgr.verifyArchiveFile();

}
