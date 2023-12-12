/* 
 * Copyright (C) 2024 Bruce Beisel
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
#include "AlarmManager.h"
#include "ArchiveManager.h"
#include "CommandSocket.h"
#include "CommandHandler.h"
#include "CurrentWeatherManager.h"
#include "CurrentWeatherSocket.h"
#include "EventManager.h"
#include "SerialPort.h"
#include "VantageDriver.h"
#include "VantageConfiguration.h"
#include "VantageLogger.h"
#include "VantageStationNetwork.h"
#include "GraphDataRetriever.h"
#include "StormArchiveManager.h"

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
consoleThreadEntry(const string & dataDirectory, const string & serialPortName, int baudRate) {
    VantageLogger & logger = VantageLogger::getLogger("Vantage Main");
    logger.log(VantageLogger::VANTAGE_INFO) << "Starting console thread" << endl;

    try {
        logger.log(VantageLogger::VANTAGE_INFO) << "Creating runtime objects" << endl;
        //
        // Create all of the runtime object that never get destroyed
        //
        CurrentWeatherSocket currentWeatherPublisher;
        CurrentWeatherManager currentWeatherManager(dataDirectory, currentWeatherPublisher);
        SerialPort serialPort(serialPortName, baudRate);
        VantageWeatherStation station(serialPort);
        ArchiveManager archiveManager(dataDirectory, station);
        VantageConfiguration configuration(station);
        VantageStationNetwork network(dataDirectory, station, archiveManager);
        AlarmManager alarmManager(station);
        GraphDataRetriever graphDataRetriever(station);
        StormArchiveManager stormArchiveManager(dataDirectory, graphDataRetriever);
        CommandHandler commandHandler(station, configuration, archiveManager, network, alarmManager, currentWeatherManager);
        EventManager eventManager(commandHandler);
        CommandSocket commandSocket(11462, eventManager);
        VantageDriver driver(station, configuration, archiveManager, eventManager, stormArchiveManager);

        //
        // Perform configuration
        //
        logger.log(VantageLogger::VANTAGE_INFO) << "Configuring runtime objects" << endl;
        station.addLoopPacketListener(currentWeatherManager);
        station.addLoopPacketListener(alarmManager);
        station.addLoopPacketListener(network);
        station.addLoopPacketListener(driver);
        station.addLoopPacketListener(graphDataRetriever);

        //
        // Initialize objects that require it before entering the main loop
        //
        logger.log(VantageLogger::VANTAGE_INFO) << "Initializing runtime objects" << endl;
        //
        // The driver must be initialized before any communication is performed with the console
        //
        if (!driver.initialize())
            return;

        if (!currentWeatherPublisher.initialize())
            return;

        if (!network.initializeNetwork())
            return;

        if (!alarmManager.initialize())
            return;

        //
        // Initialize the command socket last so that all other's are initialized before any commands are received
        //
        if (!commandSocket.initialize())
            return;

        logger.log(VantageLogger::VANTAGE_INFO) << "Entering driver's main loop" << endl;
        driver.mainLoop();
        logger.log(VantageLogger::VANTAGE_INFO) << "Driver's main loop returned. Joining CommandSocket" << endl;
        commandSocket.terminate();
        commandSocket.join();
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
        cerr << "Usage: vws <weather station serial port> <data directory> [log file prefix]" << endl;
        exit(1);
    }

    const string serialPortName(argv[1]);
    const string dataDirectory(argv[2]);

    if (argc == 4) {
        const string logFilePrefix(argv[3]);
        VantageLogger::setLogFileParameters(logFilePrefix, 20, 25); // 20 25 MB files
    }

    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    thread consoleThread(consoleThreadEntry, dataDirectory, serialPortName, 19200);

    consoleThread.join();
}
