
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
#include <getopt.h>
#include "AlarmManager.h"
#include "ArchiveManager.h"
#include "CommandSocket.h"
#include "ConsoleCommandHandler.h"
#include "DataCommandHandler.h"
#include "CurrentWeatherManager.h"
#include "CurrentWeatherSocket.h"
#include "CommandQueue.h"
#include "SerialPort.h"
#include "VantageDriver.h"
#include "VantageConfiguration.h"
#include "VantageLogger.h"
#include "VantageStationNetwork.h"
#include "GraphDataRetriever.h"
#include "StormArchiveManager.h"

using namespace std;
using namespace vws;

static constexpr int DEFAULT_SOCKET_PORT = 11462;
atomic<bool> signalCaught(false);
static VantageLogger * mainLogger = NULL;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
sigHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM)
        signalCaught.store(true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
startVWS(const string & dataDirectory, const string & serialPortName, int baudRate, int socketPort) {

    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Creating runtime objects" << endl;

    //
    // Create all of the runtime object that never get destroyed
    //
    CurrentWeatherSocket currentWeatherSocket;
    CurrentWeatherManager currentWeatherManager(dataDirectory, currentWeatherSocket);
    SerialPort serialPort(serialPortName, baudRate);
    VantageWeatherStation station(serialPort);
    ArchiveManager archiveManager(dataDirectory, station);
    VantageConfiguration configuration(station);
    VantageStationNetwork network(dataDirectory, station, archiveManager);
    AlarmManager alarmManager(station);
    GraphDataRetriever graphDataRetriever(station);
    StormArchiveManager stormArchiveManager(dataDirectory, graphDataRetriever);
    ConsoleCommandHandler consoleCommandHandler(station, configuration, network, alarmManager);
    DataCommandHandler dataCommandHandler(archiveManager, stormArchiveManager, currentWeatherManager);
    VantageDriver consoleDriver(station, configuration, archiveManager, consoleCommandHandler, stormArchiveManager);
    CommandSocket commandSocket(socketPort);

    //
    // Perform configuration
    //
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Configuring runtime objects" << endl;
    station.addLoopPacketListener(currentWeatherManager);
    station.addLoopPacketListener(alarmManager);
    station.addLoopPacketListener(network);
    station.addLoopPacketListener(consoleDriver);

    commandSocket.addCommandHandler(dataCommandHandler);
    commandSocket.addCommandHandler(consoleCommandHandler);

    consoleDriver.addConnectionMonitor(network);
    consoleDriver.addConnectionMonitor(alarmManager);

    //
    // Initialize objects that require it before entering the main loop
    //
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Initializing runtime objects" << endl;

    currentWeatherManager.initialize();

    dataCommandHandler.initialize();

    if (!currentWeatherSocket.initialize())
        return;

    //
    // The driver must be initialized before any communication is performed with the console
    //
    if (!consoleDriver.initialize())
        return;

    //
    // Initialize the command socket last so that all others are initialized before any commands are received
    //
    if (!commandSocket.initialize())
        return;

    //
    // This call will block until the console driver thread ends
    //
    consoleDriver.join();

    commandSocket.terminate();
    dataCommandHandler.terminate();

    commandSocket.join();
    dataCommandHandler.join();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char * usage = "Usage: vws -p <weather station serial port> -d <data directory> [-v <debug verbosity (0-3)>] [-l <log file prefix>]";

int
main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGPIPE, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
#endif

    mainLogger = &VantageLogger::getLogger("VWS Main");
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);

    string serialPortName;
    string dataDirectory;
    string logFilePrefix;
    VantageLogger::Level debugLevel;
    int debugLevelOption;
    int socketPort = DEFAULT_SOCKET_PORT;

    bool errorFound = false;
    int opt;
    while ((opt = getopt(argc, argv, "d:l:p:s:v:h")) != -1) {
        switch (opt) {
            case 'd':
                dataDirectory = optarg;
                break;

            case 'l':
                logFilePrefix = optarg;
                VantageLogger::setLogFileParameters(logFilePrefix, 20, 25); // 20 25 MB files
                break;

            case 'p':
                cout << "Serial port: " << optarg << endl;
                serialPortName = optarg;
                break;

            case 's':
                socketPort = atoi(optarg);
                break;

            case 'v':
                debugLevelOption = atoi(optarg);
                if (debugLevelOption < 0 || debugLevelOption > 3) {
                    cerr << "Invalid debug verbosity. Must be between 0 and 3" << endl;
                    errorFound = true;
                }
                else {
                    switch (debugLevelOption) {
                        case 0:
                            debugLevel = VantageLogger::VANTAGE_INFO;
                            break;

                        case 1:
                            debugLevel = VantageLogger::VANTAGE_DEBUG1;
                            break;

                        case 2:
                            debugLevel = VantageLogger::VANTAGE_DEBUG2;
                            break;

                        case 3:
                            debugLevel = VantageLogger::VANTAGE_DEBUG3;
                            break;
                    }

                    VantageLogger::setLogLevel(debugLevel);
                }
                break;

            case 'h':
            default:
                errorFound = true;
                break;
        }
    }

    if (serialPortName == "") {
        cerr << "Serial port not specified!" << endl;
        errorFound = true;
    }

    if (dataDirectory == "") {
        cerr << "Data directory not specified!" << endl;
        errorFound = true;
    }

    if (errorFound) {
        cerr << usage << endl;
        exit(1);
    }

    startVWS(dataDirectory, serialPortName, 19200, socketPort);
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "main() is ending" << endl;
}
