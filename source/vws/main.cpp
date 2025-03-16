
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

/**
 * This is the main routine for VWS (Vantage Weather Station) software.
 * The basic design is as follows. There are three threads that make up the processing structure:
 *     1. Command Socket Thread - This thread receives commands from outside sources, routes the commands
 *        to the appropriate processing thread and sends the responses. As the name implies this thread reads
 *        and writes on a TCP socket. The command structure is documented elsewhere.
 *     2. Console Command Thread - This thread processes all commands that must communicate with the Vantage
 *        Pro 2 console as well as providing the continuous processing loop to receive the current weather data from
 *        the console. This thread will continuously read "LOOP" packets from the console, pausing only when another
 *        task is required. Other tasks include: terminating the program, processing an external command, and the
 *        availability of a new archive packet.
 *     3. Data Command Thread - The VWS keeps its own store of data due to the limited size of the buffers in the
 *        Vantage Pro 2 console. This thread processes commands the retrieve the requested data and send it back
 *        to the requester. It reads the following: 1) Archive packets which are created by the console at a
 *        configurable rate, 2) Loop packets which are read approximately every 2 seconds, 3) Storm data, which is
 *        updated very slowly as new storms start or current storms end.
 *        As part of the archive packet retrieval capability, this thread will also generate summary reports based
 *        on daily, weekly, monthly and yearly periods.
 *        Note that this thread does not write any of this data, that is handled by the Console Command Thread.
 *        Of course mutexes are used to protect the integrity of the data across the two threads.
 *
 * This process almost always transmits data in the native units of the Vantage Pro 2 console (Fahrenheit (F), Mile per hour (MPH),
 * inches of mercury (inHg), inches (in)). It is up to the client to convert to the units preferred by the viewer
 * of the data. The only exception is the wind speed in the summary reports. That unit can be specified in the command.
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
    VantageDriver consoleDriver(station, archiveManager, consoleCommandHandler, stormArchiveManager);
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

    //
    // Add the console connection monitors, adding consoleDriver last so that all other
    // configuration is complete before the driver
    //
    consoleDriver.addConnectionMonitor(configuration);
    consoleDriver.addConnectionMonitor(network);
    consoleDriver.addConnectionMonitor(alarmManager);
    consoleDriver.addConnectionMonitor(consoleDriver);

    //
    // Initialize objects that require it before entering the main loop
    //
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Initializing runtime objects" << endl;

    //
    // Create and/or clean up the loop packet archive
    //
    currentWeatherManager.initialize();

    //
    // Create the current weather broadcast UDP socket
    //
    if (!currentWeatherSocket.initialize())
        return;

    //
    // Start the thread that handles data commands.
    //
    dataCommandHandler.start();

    //
    // The driver must be initialized before any communication is performed with the console.
    // Note that any external commands will be ignored until the connection with the console is successful.
    //
    consoleDriver.start();

    //
    // Initialize the command socket last so that all others are initialized before any commands are received.
    // If the command socket could not be started, terminate the console driver thread.
    //
    if (!commandSocket.start())
        consoleDriver.terminate();

    //
    // This call will block until the console driver thread ends
    //
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Waiting for console driver thread to terminate" << endl;
    consoleDriver.join();

    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Console driver thread has terminated. Terminating other threads." << endl;

    commandSocket.terminate();
    dataCommandHandler.terminate();

    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Waiting for command socket thread to terminate" << endl;
    commandSocket.join();
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Command socket thread has terminated" << endl;
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Waiting for data command thread to terminate" << endl;
    dataCommandHandler.join();
    mainLogger->log(VantageLogger::VANTAGE_INFO) << "Data command thread has terminated" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char * usage = "Usage: vws -p <weather station serial port> -d <data directory> [-v <debug verbosity (0-3, 0 = INFO)>] [-l <log file prefix>]";

int
main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGPIPE, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
#endif

    mainLogger = &VantageLogger::getLogger("VWS Main");
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_INFO);

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
                    cerr << "Invalid debug verbosity. Must be from 0 to 3" << endl;
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
