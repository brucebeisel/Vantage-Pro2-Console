
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
#include "../vws/VantageLogger.h"
#include "../vws/ArchiveManager.h"
#include "../vws/CommandSocket.h"
#include "../vws/CommandHandler.h"
#include "../vws/VantageDriver.h"
#include "../vws/VantageConfiguration.h"
#include "../vws/CurrentWeatherSocket.h"
#include "../vws/CurrentWeatherManager.h"

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
consoleThreadEntry(const string & archiveFile, const std::string & loopPacketArchiveDir, const string & serialPortName, int baudRate) {
    VantageLogger & logger = VantageLogger::getLogger("Vantage Main");
    logger.log(VantageLogger::VANTAGE_INFO) << "Starting console thread" << endl;

    try {
        logger.log(VantageLogger::VANTAGE_INFO) << "Creating runtime objects" << endl;
        //
        // Create all of the runtime object that never get destroyed
        //
        CurrentWeatherSocket currentWeatherPublisher;
        CurrentWeatherManager currentWeatherManager(loopPacketArchiveDir, currentWeatherPublisher);
        VantageWeatherStation station(serialPortName, baudRate);
        ArchiveManager archiveManager(archiveFile, station);
        VantageConfiguration configuration(station);
        CommandHandler commandHandler(station, configuration, archiveManager);
        EventManager eventManager(commandHandler);
        CommandSocket commandSocket(11462, eventManager);
        VantageDriver driver(station, configuration, archiveManager, eventManager);

        //
        // Perform configuration
        //
        logger.log(VantageLogger::VANTAGE_INFO) << "Configuring runtime objects" << endl;
        station.addLoopPacketListener(currentWeatherManager);

        //
        // Initialize objects that require it before entering the main loop
        //
        logger.log(VantageLogger::VANTAGE_INFO) << "Initializing runtime objects" << endl;
        if (!commandSocket.initialize())
            return;

        if (!currentWeatherPublisher.initialize())
            return;

        if (!driver.initialize())
            return;

        logger.log(VantageLogger::VANTAGE_INFO) << "Entering driver's main loop" << endl;
        driver.mainLoop();
        logger.log(VantageLogger::VANTAGE_INFO) << "Driver's main loop returned" << endl;
    }
    catch (std::exception & e) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Caught exception from driver's main loop " << e.what() << endl;
    }
    catch (...) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Caught unknown exception from driver's main loop" << endl;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Ending console thread" << endl;
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
        cerr << "Usage: vws <weather station serial port> <archive file> <loop packet archive dir> [log file]" << endl;
        exit(1);
    }

    const string serialPortName(argv[1]);
    const string archiveFile(argv[2]);
    const string loopPacketArchiveDir(argv[3]);

    if (argc == 5) {
        const string logFile(argv[4]);
        ofstream logStream(logFile.c_str(), ios::app | ios::ate | ios::out);
        VantageLogger::setLogStream(logStream);
    }

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    thread consoleThread(consoleThreadEntry, archiveFile, loopPacketArchiveDir, serialPortName, 19200);

    consoleThread.join();
}
