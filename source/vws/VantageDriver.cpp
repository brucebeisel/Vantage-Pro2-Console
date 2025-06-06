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

#include "VantageDriver.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <atomic>

#include "Alarm.h"
#include "ArchiveManager.h"
#include "CommandQueue.h"
#include "StormArchiveManager.h"
#include "CurrentWeather.h"
#include "HiLowPacket.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "Weather.h"

using namespace std;
extern atomic_bool signalCaught;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
consoleThreadEntry(VantageDriver * driver) {
    driver->mainLoop();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageDriver::VantageDriver(VantageWeatherStation & station, ArchiveManager & archiveManager,  CommandHandler & cmdHandler, StormArchiveManager & stormArchiveManager) :
                                                                isConsoleConnected(false),
                                                                station(station),
                                                                archiveManager(archiveManager),
                                                                commandHandler(cmdHandler),
                                                                stormArchiveManager(stormArchiveManager),
                                                                exitLoop(false),
                                                                nextRecord(-1),
                                                                previousNextRecord(-1),
                                                                lastArchivePacketTime(0),
                                                                lastStormArchiveUpdateTime(0),
                                                                lastArchiveVerifyTime(0),
                                                                consoleThread(NULL),
                                                                logger(VantageLogger::getLogger("VantageDriver")) {
    //
    // Indicate the the console time needs to be set in the near future. 
    // We do not want the console time to be set immediately in case the computer has just started and
    // has not had a chance to synchronize its time with the Internet. This is most important with
    // computers like the Raspberry Pi.
    //
    // TODO This equation seems wrong. I would think that the time could be set after just a few minutes.
    // So perhaps consoleTimeSetTime = time(0) - TIME_SET_INTERVAL + 120; would be better?
    //
    DateTime now = time(0);
    consoleTimeSetTime = now - TIME_SET_INTERVAL + (1 * Weather::SECONDS_PER_HOUR);

    //
    // We don't want to verify the archive every time the service starts, so pretend
    // that it was just verified
    //
    lastArchiveVerifyTime = now;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageDriver::~VantageDriver() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::start() {
    consoleThread = new thread(consoleThreadEntry, this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::connectToConsole() {
    if (isConsoleConnected)
        return true;

    logger.log(VantageLogger::VANTAGE_INFO) << "Connecting to console..." << endl;

    if (!station.openStation()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open weather station" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Port is open" << endl;

    if (!station.wakeupStation()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to wake up weather station" << endl;
        return false;
    }
    else {
        logger.log(VantageLogger::VANTAGE_INFO) << "Weather Station is awake" << endl;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Console connected." << endl;

    isConsoleConnected = true;

    for (auto monitor : connectionMonitors)
        monitor->consoleConnected();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::addConnectionMonitor(ConsoleConnectionMonitor & monitor) {
    connectionMonitors.push_back(&monitor);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::consoleConnected() {
    string consoleTypeString;
    if (!station.retrieveConsoleType(&consoleTypeString)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve station type for weather station" << endl;
        return;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Weather Station Type: " << consoleTypeString << endl;

    if (!retrieveConfiguration())
        return;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::consoleDisconnected() {
    nextRecord = -1;
    previousNextRecord = -1;
    lastArchivePacketTime = 0;
    lastStormArchiveUpdateTime = 0;
    lastArchiveVerifyTime = time(0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::disconnectFromConsole() {
    station.closeStation();

    for (auto monitor : connectionMonitors)
        monitor->consoleDisconnected();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::retrieveConfiguration() {
    //
    // Get one LOOP packet weather so that the sensors are detected
    //
    LoopPacket loopPacket;
    bool loopPacketReceived = false;
    for (int i = 0; i < INITIAL_LOOP_PACKET_RETRIES && !loopPacketReceived; i++) {
        loopPacketReceived = station.retrieveLoopPacket(loopPacket);
    }

    if (!loopPacketReceived) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to receive a LOOP packet needed to determine current sensor suite" << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::terminate() {
    exitLoop = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::mainLoop() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Entering main loop" << endl;

    while (!exitLoop) {
        try {
            //
            // If an asynchronous signal was caught, then exit the loop
            //
            if (signalCaught.load()) {
                exitLoop = true;
                continue;
            }

            //
            // Try to connect to the console on each loop. Process any commands so that the user interface
            // gets a response even if we cannot talk to the console.
            //
            if (!connectToConsole()) {
                logger.log(VantageLogger::VANTAGE_ERROR) << "Not connected to console, trying again" << endl;
                commandHandler.processNextCommand();
                sleep(1);
                continue;
            }

            //
            // If the weather station could not be woken, then close and open
            // the console. It has been observed that on a rare occasion the console
            // never wakes up. Only restarting this driver fixes the issue. Reopening
            // the serial port will hopefully fix this issue.
            //
            if (!station.wakeupStation()) {
                disconnectFromConsole();
                continue;
            }

            //
            // If it has been a while since the time was set, set the time. Note that the time will not be changed
            // if the console time is close to the actual clock time.
            // The return value from station.updateConsoleTime() is not checked and the delay until the next
            // possible console time update will be consistent regardless of the success of setting the console time.
            //
            DateTime now = time(0);
            if (consoleTimeSetTime + TIME_SET_INTERVAL < now) {
                station.updateConsoleTime();
                consoleTimeSetTime = now;
            }

            //
            // Update the storm archive if enough time has passed
            //
            if (lastStormArchiveUpdateTime + STORM_ARCHIVE_UPDATE_INTERVAL < now) {
                stormArchiveManager.updateArchive();
                lastStormArchiveUpdateTime = now;
            }

            //
            // Verify the archive and store the results
            // TODO This should probably move to the DataCommand thread
            //
            if (lastArchiveVerifyTime + ARCHIVE_VERIFY_INTERVAL < now) {
                archiveManager.verifyCurrentArchiveFile();
                lastArchiveVerifyTime = now;
            }

            //
            // Get the current weather values for about a minute or until an event occurs that requires the end of the loop
            //
            station.currentValuesLoop(LOOP_PACKET_CYCLES);

            //
            // Process the next event that the command handler has received (if any).
            // Note that the events are expected to come in slowly as the
            // events are typically human driven.
            //
            commandHandler.processNextCommand();

            //
            // If the LOOP packet data indicates that a new archive packet is available
            // go get it.
            //
            if (previousNextRecord != nextRecord) {
                logger.log(VantageLogger::VANTAGE_INFO) << "New archive record available. Record ID = " << nextRecord << endl;
                previousNextRecord = nextRecord;

                if (synchronizeArchive()) {
                    previousNextRecord = nextRecord;
                }
            }
        }
        catch (std::exception & e) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught exception: " << e.what() << endl;     
        } 
    }

    station.closeStation();
    logger.log(VantageLogger::VANTAGE_INFO) << "Exiting main loop" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::synchronizeArchive() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Synchronizing local archive from Vantage console's archive" << endl;
    vector<ArchivePacket> list;
    bool result = false;

    DateTimeFields oldestRecordTime;
    DateTimeFields newestRecordTime;
    int count;
    archiveManager.getArchiveRange(oldestRecordTime, newestRecordTime, count);

    for (int i = 0; i < SYNC_ARCHIVE_RETRIES && !result; i++) {
        list.clear();
        if (station.wakeupStation() && station.dumpAfter(newestRecordTime, list)) {
            archiveManager.addPacketsToArchive(list);
            result = true;
            if (list.size() > 0)
                logger.log(VantageLogger::VANTAGE_DEBUG1) << "Newest archive packet time after sync is: " << list.at(list.size() - 1).getPacketDateTimeString() << endl;
            else
                logger.log(VantageLogger::VANTAGE_INFO) << "No archive records were retrieved from the console during sync" << endl;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::join() {

    if (consoleThread != NULL && consoleThread->joinable()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Joining the thread" << endl;
        consoleThread->join();
        consoleThread = NULL;
    }
    else
        logger.log(VantageLogger::VANTAGE_WARNING) << "Ignoring join request. Thread was not created or is not running." << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::processLoopPacket(const LoopPacket & packet) {
    nextRecord = packet.getNextRecord();

    bool signalCaughtFlag = signalCaught.load();
    bool commandReceivedFlag = commandHandler.isCommandAvailable();
    bool newArchiveRecordFlag = previousNextRecord != nextRecord;
    bool continueLoopPacketProcessing = !signalCaughtFlag && !commandReceivedFlag && !newArchiveRecordFlag;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Continue current weather loop (LOOP): " << std::boolalpha << continueLoopPacketProcessing
                                              << " (Signal Caught: " << signalCaughtFlag << " Command Received: " << commandReceivedFlag << " New Archive Record: " << newArchiveRecordFlag << ")" << endl;

    return continueLoopPacketProcessing;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::processLoop2Packet(const Loop2Packet & packet) {
    bool signalCaughtFlag = signalCaught.load();
    bool commandReceivedFlag = commandHandler.isCommandAvailable();
    bool continueLoopPacketProcessing = !signalCaughtFlag && !commandReceivedFlag;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Continue current weather loop (LOOP2): " << std::boolalpha << continueLoopPacketProcessing
                                              << " (Signal Caught: " << signalCaughtFlag << " Command Received: " << commandReceivedFlag << ")" << endl;

    return continueLoopPacketProcessing;
}
}
