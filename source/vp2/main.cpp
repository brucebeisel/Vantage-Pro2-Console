
/* 
 * Copyright (C) 2023 Bruce Beisel
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
#ifdef _WIN32
#pragma warning(disable : 4100)
#else
#include <signal.h>
#include <stdio.h>
#endif
#include <iostream>
#include <thread>
#include <atomic>
#include <fstream>
#include "VantageLogger.h"
#include "ArchiveManager.h"
#include "VantageDriver.h"
#include "CurrentWeatherPublisher.h"

using namespace std;
using namespace vws;

atomic<bool> signalCaught(false);

//#ifndef WIN32
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
sigHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM)
        signalCaught.store(true);
}
//#endif

void
consoleThreadEntry(const string & archiveFile, const string & serialPortName, int baudRate) {
    VantageLogger & log = VantageLogger::getLogger("Vantage Main");
    log.log(VantageLogger::VANTAGE_INFO) << "Starting console thread" << endl;

    try {
        CurrentWeatherPublisher cwPublisher;
        VantageWeatherStation station(serialPortName, baudRate);
        ArchiveManager archiveManager(archiveFile, station);
        EventManager eventManager;
        VantageDriver driver(archiveManager, cwPublisher, station, eventManager);


        if (!cwPublisher.createSocket())
            return;

        //if (!driver.initialize())
        //    return;

        log.log(VantageLogger::VANTAGE_INFO) << "Entering driver's main loop" << endl;
        driver.mainLoop();
        log.log(VantageLogger::VANTAGE_INFO) << "Driver's main loop returned" << endl;
    }
    catch (std::exception & e) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Caught exception from driver's main loop " << e.what() << endl;
    }
    catch (...) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Caught unknown exception from driver's main loop" << endl;
    }

    log.log(VantageLogger::VANTAGE_INFO) << "Ending console thread" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGPIPE, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
#endif

    if (argc < 3 || argc > 4) {
        cerr << "Usage: vp2 <port> <archive file> [log file]" << endl;
        exit(1);
    }

    const string serialPortName(argv[1]);
    const string archiveFile(argv[2]);

    if (argc == 4) {
        const string logFile(argv[3]);
        ofstream logStream(logFile.c_str(), ios::app | ios::ate | ios::out);
        VantageLogger::setLogStream(logStream);
    }

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    thread consoleThread(consoleThreadEntry, archiveFile, serialPortName, 19200);

    consoleThread.join();
}
