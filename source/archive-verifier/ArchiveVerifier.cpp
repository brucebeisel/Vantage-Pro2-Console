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

    if (argc < 2) {
        cerr << "Usage: archive-verifier <archive directory> [archive file]" << endl;
        exit(1);
    }

    const char * archiveDir = argv[1];
    char * archiveFile = NULL;
    if (argc == 3)
        archiveFile = argv[2];


    SerialPort port("/dev/tty", 9600);
    VantageWeatherStation station(port);
    ArchiveManager *mgr;
    if (archiveFile == NULL)
        mgr = new ArchiveManager(archiveDir, station);
    else
        mgr = new ArchiveManager(archiveDir, archiveFile, station);

    mgr->verifyCurrentArchiveFile();

}
