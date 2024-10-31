#include <fstream>
#include <iostream>
#include <filesystem>
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
        cerr << "Usage: archive-verifier <archive file>" << endl;
        exit(1);
    }

    if (!filesystem::exists(argv[1])) {
        cerr << "Archive file '" << argv[1] << "' does not exists" << endl;
        cerr << "Usage: archive-verifier <archive file>" << endl;
        exit(1);
    }

    const char * archiveDir = NULL;
    const char * archiveFile = NULL;

    if (filesystem::is_directory(argv[1])) {
        cerr << "Specified file '" << argv[1] << "' is a directory not a file." << endl;
        exit(1);
    }

    filesystem::path path(argv[1]);
    if (!path.has_parent_path()) {
        cerr << "Directory not found in path '" << argv[1] << "' Using '.' as archive directory" << endl;
        archiveDir = ".";
    }
    else {
        archiveDir = path.parent_path().c_str();
    }

    archiveFile = path.filename().c_str();

    SerialPort port("/dev/tty", 9600);
    VantageWeatherStation station(port);

    ArchiveManager mgr(archiveDir, archiveFile, station);

    mgr.verifyArchiveFile(argv[1]);

}
