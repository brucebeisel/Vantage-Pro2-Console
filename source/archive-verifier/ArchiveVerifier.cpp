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

    SerialPort port("/dev/tty", vws::BaudRate::BR_9600);
    VantageWeatherStation station(port);

    ArchiveManager mgr(archiveDir, archiveFile, station);

    mgr.verifyArchiveFile(argv[1]);

}
