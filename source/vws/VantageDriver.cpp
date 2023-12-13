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

#include "VantageDriver.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <atomic>

#include "Alarm.h"
#include "ArchiveManager.h"
#include "EventManager.h"
#include "StormArchiveManager.h"
#include "CurrentWeather.h"
#include "HiLowPacket.h"
#include "VantageDecoder.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "VantageConfiguration.h"
#include "Weather.h"

using namespace std;
extern atomic_bool signalCaught;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageDriver::VantageDriver(VantageWeatherStation & station, VantageConfiguration & configuration, ArchiveManager & archiveManager,  EventManager & evtMgr, StormArchiveManager & stormArchiveManager) :
                                                                station(station),
                                                                configuration(configuration),
                                                                archiveManager(archiveManager),
                                                                eventManager(evtMgr),
                                                                stormArchiveManager(stormArchiveManager),
                                                                exitLoop(false),
                                                                nextRecord(-1),
                                                                previousNextRecord(-1),
                                                                lastArchivePacketTime(0),
                                                                lastStormArchiveUpdateTime(0),
                                                                logger(VantageLogger::getLogger("VantageDriver")) {
    //
    // Indicate the the console time needs to be set in the near future. 
    // We do not want the console time to be set immediately in case the computer has just started and
    // has not had a chance to synchronize its time with the Internet. This is most important with
    // computers like the Raspberry Pi.
    //
    consoleTimeSetTime = time(0) - TIME_SET_INTERVAL + (1 * SECONDS_PER_HOUR);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageDriver::~VantageDriver() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::initialize() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Initializing..." << endl;

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

    string consoleTypeString;
    if (!station.retrieveConsoleType(&consoleTypeString)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve station type for weather station" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_INFO) << "Weather Station Type: " << consoleTypeString << endl;

    if (!retrieveConfiguration())
        return false;

    logger.log(VantageLogger::VANTAGE_INFO) << "Initialization complete." << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::retrieveConfiguration() {


    //
    // Get the setup bits first so that the size of the rain bucket is saved before any LOOP packets
    // or archive packets are received.
    //
    SetupBits setupBits;
    if (!configuration.retrieveSetupBits(setupBits)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve setup bits" << endl;
        return false;
    }

    ArchivePeriod period;
    station.retrieveArchivePeriod(period);

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
VantageDriver::stop() {
    exitLoop = true;
    station.closeStation();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::reopenStation() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Reopening weather station" << endl;
    station.closeStation();
    bool success = station.openStation();

    if (!success)
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to reopen weather station" << endl;

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::mainLoop() {
    //
    // Synchronize the historical archive data from the console to disk
    //
    if (!archiveManager.synchronizeArchive()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read the archive during initialization" << endl;
        return;
    }

    while (!exitLoop) {
        try {
            //
            // If the weather station could not be woken, then close and open
            // the console. It has been observed that on a rare occasion the console
            // never wakes up. Only restarting this driver fixes the issue. Reopening
            // the serial port will hopefully fix this issue.
            //
            if (!station.wakeupStation()) {
                exitLoop = !reopenStation();
                continue;
            }

            //
            // If it has been a while since the time was set, set the time
            //
            DateTime now = time(0);
            if (consoleTimeSetTime + TIME_SET_INTERVAL < now) {
                if (station.updateConsoleTime())
                    consoleTimeSetTime = now;
                else
                    logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to set station time " << endl;

            }

            //
            // Update the storm archive if enough time has passed
            //
            if (lastStormArchiveUpdateTime + STORM_ARCHIVE_UPDATE_INTERVAL < now) {
                stormArchiveManager.updateArchive();
                lastStormArchiveUpdateTime = now;
            }

            //
            // Get the current weather values for about a minute or until an event occurs that requires the end of the loop
            //
            station.currentValuesLoop(LOOP_PACKET_CYCLES);

            //
            // If an asynchronous signal was caught, then exit the loop
            //
            if (signalCaught.load()) {
                exitLoop = true;
                continue;
            }

            //
            // Process the next event that the event manager has received.
            // Note that the events are expected to come in slowly as the
            // events are typically human driven.
            //
            eventManager.processNextEvent();

            //
            // If the LOOP packet data indicates that a new archive packet is available
            // go get it.
            //
            if (previousNextRecord != nextRecord) {
                logger.log(VantageLogger::VANTAGE_INFO) << "New archive record available. Record ID = " << nextRecord << endl;
                previousNextRecord = nextRecord;

                if (archiveManager.synchronizeArchive()) {
                    ArchivePacket packet;
                    archiveManager.getNewestRecord(packet);
                    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Most recent archive packet time is: "
                                                              << Weather::formatDateTime(packet.getDateTime());
                    previousNextRecord = nextRecord;
                }
            }
        }
        catch (std::exception & e) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught exception: " << e.what() << endl;     
        } 
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::processLoopPacket(const LoopPacket & packet) {
    nextRecord = packet.getNextRecord();

    bool sc = signalCaught.load();
    bool em = eventManager.isEventAvailable();
    bool nr = previousNextRecord != nextRecord;
    bool continueLoopPacketProcessing = !sc && !em && !nr;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Continue current weather loop (LOOP): " << std::boolalpha << continueLoopPacketProcessing
                                              << " Signal: " << sc << " Event: " << em << " Next Record: " << nr << endl;

    return continueLoopPacketProcessing;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::processLoop2Packet(const Loop2Packet & packet) {
    bool sc = signalCaught.load();
    bool em = eventManager.isEventAvailable();
    bool continueLoopPacketProcessing = !sc && !em;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Continue current weather loop (LOOP2): " << std::boolalpha << continueLoopPacketProcessing
                                              << " Signal: " << sc << " Event: " << em << endl;

    return continueLoopPacketProcessing;
}
}
