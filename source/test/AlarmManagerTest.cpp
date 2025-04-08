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
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "Weather.h"
#include "VantageEnums.h"
#include "VantageWeatherStation.h"
#include "AlarmManager.h"

#include "SerialPort.h"
#include "VantageLogger.h"
#include "VantageDecoder.h"
#include "BaudRate.h"

using namespace vws;
using namespace std;

const char * usage = "Usage: AlarmManagerTest -d <device> [-h] [-n]\nwhere <device> = serial device\n      -h = Print help\n     -n = Do not open device, just test log file code";
int
main(int argc, char * argv[]) {

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    const char * device = NULL;
    bool noDevice = false;

    int opt;
    while ((opt = getopt(argc, argv, "d:nh")) != -1) {
        switch (opt) {
            case 'd':
                device = optarg;
                break;

            case 'h':
                cout << usage << endl;
                break;

            case 'n':
                noDevice = true;
                break;
        }
    }


    if (device == NULL) {
        cout << "Usage: AlarmManagerTest <device>" << endl;
        exit(1);
    }

    SerialPort serialPort(device, vws::BaudRate::BR_19200);
    VantageWeatherStation station(serialPort);
    AlarmManager alarmManager(".", station);

    station.addLoopPacketListener(alarmManager);
    alarmManager.processRainCollectorSizeChange(.01);
    VantageDecoder::setRainCollectorSize(.01);

    if (!noDevice) {
        if (!station.openStation())
            exit(2);

        if (!station.wakeupStation())
            exit(3);

        station.consoleConnected();
        alarmManager.consoleConnected();
        station.currentValuesLoop(1);
    }

    cout << "Active alarms: " << endl << alarmManager.formatActiveAlarmsJSON() << endl;
    DateTimeFields start(time(0) - 86400), end(time(0));
    cout << "Alarms history: " << endl << alarmManager.formatAlarmHistoryJSON(start, end) << endl;
}
