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
#include "string.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "VantageWeatherStation.h"
#include "SerialPort.h"
#include "ConsoleCommandHandler.h"
#include "VantageConfiguration.h"
#include "VantageStationNetwork.h"
#include "AlarmManager.h"
#include "ArchiveManager.h"
#include "VantageLogger.h"

using namespace std;
using namespace vws;

static const char USAGE_MESSAGE[] = "Usage: command-line-console [-v] [-b] [-t]  <device name>";

int
main(int argc, char *argv[]) {
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);

    bool verbose = false;
    bool dumpBinary = false;
    char *device = NULL;
    bool terse = false;

    /*
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
    */

    if (argc < 2) {
        cerr << USAGE_MESSAGE << endl;
        exit(1);
    }

    device = argv[1];

    SerialPort serialPort(device, 19200);
    VantageWeatherStation station(serialPort);
    ArchiveManager archive("./", station);
    VantageConfiguration config(station);
    VantageStationNetwork network("./", station, archive);
    AlarmManager alarm(station);

    ConsoleCommandHandler cmd(station, config, network, alarm);

    if (!station.openStation()) {
        cerr << "Could not open weather console" << endl;
        exit(1);
    }

    if (!station.wakeupStation()) {
        cerr << "Could not wake up console" << endl;
        exit(2);
    }

    while (1) {
        int commandNumber;
        cout << "Choose a command" << endl;
        cout << "    1 - Query configuration" << endl;
        cout << "    2 - Query console diagnostics" << endl;
        cin >> commandNumber;

        CommandData commandData;
        switch (commandNumber) {
            case 1:
                cmd.handleQueryConfigurationData(commandData);
                break;
        }
    }

}
