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
#include <set>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "ArchivePacket.h"

using namespace std;
using namespace vws;

const std::string BASE_ARCHIVE_FILE = "weather-archive-base.dat";

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: ArchiveRebuilder <root dir> <output file>" << endl;
        exit(1);
    }

    const char * directory = argv[1];
    const char * outputFile = argv[2];

    if (!std::filesystem::is_directory(directory)) {
        cerr << "File '" << directory << "' must be a directory" << endl;
        exit(2);
    }

    if (std::filesystem::exists(outputFile)) {
        cerr << "Output file '" << outputFile << "' must not exist" << endl;
        exit(3);
    }

    set<filesystem::path> files;

    for (const filesystem::directory_entry & dir_entry : filesystem::recursive_directory_iterator(directory)) {
        cout << dir_entry.path() << endl;
        if (dir_entry.is_regular_file())
            files.insert(dir_entry.path());
    }

    //
    // First copy any base archive file to the output file
    //
    if (std::filesystem::exists(BASE_ARCHIVE_FILE)) {
        std::filesystem::copy(BASE_ARCHIVE_FILE, outputFile);
    }

    //
    // Now append each of the packets to the output file
    //
    ArchivePacket packet;
    ofstream ofs(outputFile, fstream::binary);
    for (auto path : files) {
        cout << "Path: " << path << endl;
        if (!packet.updateArchivePacketDataFromFile(path.string())) {
            cerr << "Failed to load packet from file '" << path << "'" << endl;
        }
        else
            ofs.write(packet.getBuffer(), ArchivePacket::BYTES_PER_ARCHIVE_PACKET);
    }

    ofs.close();
}
