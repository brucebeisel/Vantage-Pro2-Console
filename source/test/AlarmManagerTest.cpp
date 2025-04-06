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

int
main(int argc, char * argv[]) {

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    const char * device = NULL;

    if (argc == 2)
        device = argv[1];
    else {
        cout << "Usage: AlarmManagerTest <device>" << endl;
        exit(1);
    }

    SerialPort serialPort(device, vws::BaudRate::BR_19200);
    VantageWeatherStation station(serialPort);
    AlarmManager alarmManager(".", station);

    station.addLoopPacketListener(alarmManager);

    if (!station.openStation())
        exit(2);

    if (!station.wakeupStation())
        exit(3);

    station.consoleConnected();
    alarmManager.consoleConnected();
    alarmManager.processRainCollectorSizeChange(.01);
    VantageDecoder::rainCollectorSizeInches(.01);

    station.currentValuesLoop(1);

    cout << "Active alarms: " << endl << alarmManager.formatActiveAlarmsJSON() << endl;
}
