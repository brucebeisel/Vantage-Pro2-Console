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
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <atomic>
#include "CurrentWeather.h"
#include "CurrentWeatherPublisher.h"
#include "HiLowPacket.h"
#include "SensorStation.h"
#include "Alarm.h"
#include "VantageConstants.h"
#include "VantageDecoder.h"
#include "VantageDriver.h"
#include "VantageLogger.h"

using namespace std;
extern atomic_bool signalCaught;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageDriver::VantageDriver(ArchiveManager & archiveManager, CurrentWeatherPublisher & cwp, VantageWeatherStation & station, EventManager & evtMgr) :
                                                                station(station),
                                                                currentWeatherPublisher(cwp),
                                                                archiveManager(archiveManager),
                                                                eventManager(evtMgr),
                                                                exitLoop(false),
                                                                nextRecord(-1),
                                                                previousNextRecord(-1),
                                                                lastArchivePacketTime(0),
                                                                log(VantageLogger::getLogger("VantageDriver")) {
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
    log.log(VantageLogger::VANTAGE_INFO) << "Initializing..." << endl;

    station.setCallback(*this);

    if (!station.openStation()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to open weather station" << endl;
        return false;
    }

    log.log(VantageLogger::VANTAGE_INFO) << "Port is open" << endl;

    if (!station.wakeupStation()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to wake up weather station" << endl;
        return false;
    }
    else {
        log.log(VantageLogger::VANTAGE_INFO) << "Weather Station is awake" << endl;
    }

    if (!station.retrieveStationType()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve station type for weather station" << endl;
        return false;
    }

    /*
    if (!station.retrieveConfigurationData()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve configuration data for weather station" << endl;
        return false;
    }

    AlarmManager::getInstance().initialize();
    */

    log.log(VantageLogger::VANTAGE_INFO) << "Initialization complete." << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::retrieveConfiguration() {

    VantageDecoder::setRainCollectorSize(station.getRainCollectorSize()); // TBD, this should come from VantageConfiguration class

    /*
    if (!station.retrieveConfigurationParameters()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to retrieve configuration parameters" << endl;
        return false;
    }
    */

    //
    // Get one LOOP packet weather so that the sensors are detected
    //
    LoopPacket loopPacket;
    bool loopPacketReceived = false;
    for (int i = 0; i < INITIAL_LOOP_PACKET_RETRIES && !loopPacketReceived; i++) {
        loopPacketReceived = station.retrieveLoopPacket(loopPacket);
    }

    if (!loopPacketReceived) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to receive a LOOP packet needed to determine current sensor suite" << endl;
        return false;
    }

    // TBD Figure out what to extract from the loop packet for initialization purposes.

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
VantageDriver::processCurrentWeather(const CurrentWeather & cw) {
    nextRecord = cw.getNextPacket();

    currentWeatherPublisher.sendCurrentWeather(cw);

    log.log(VantageLogger::VANTAGE_DEBUG1) << "Previous Next Record: " << previousNextRecord << " Next Record: " << nextRecord << endl;

    bool sc = signalCaught.load();
    bool em = eventManager.isEventAvailable();
    bool nr = previousNextRecord != nextRecord;
    bool stopCurrentWeatherLoop = sc || em || nr;

    log.log(VantageLogger::VANTAGE_DEBUG1) << "Stop current weather loop: " << stopCurrentWeatherLoop
                                   << " Signal: " << sc << " Event: " << em << " Next Record: " << nr << endl;

    //return signalCaught.load() || eventManager.isEventAvailable() || previousNextRecord != nextRecord;
    return signalCaught.load();
}

/*
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::processArchive(const vector<ArchivePacket> & archive) {

    log.log(VantageLogger::VANTAGE_DEBUG1) << "Processing " << archive.size() << " archive packets" << endl;

    for (vector<ArchivePacket>::const_iterator it = archive.begin(); it != archive.end(); ++it) {
        DateTime now = time(0);
        DateTime age = now - it->getDateTime();
        if (age < SECONDS_PER_HOUR) {
            int maxPackets = static_cast<int>(((static_cast<float>(station.getArchivePeriod()) * 60.0F) / ((41.0F + 1.0F - 1.0F) / 16.0F)));
            int actualPackets = it->getWindSampleCount();
            int issReception = (actualPackets * 100) / maxPackets;
            if (issReception > 100)
                issReception = 100;

            log.log(VantageLogger::VANTAGE_DEBUG2) << "IIS Reception for archive interval ending at " << it->getDateTime()
                                           << " is " << issReception
                                           << ". Max Packets = " << maxPackets
                                           << ", Actual Packets - " << actualPackets << endl;

            vector<SensorStation> sensorStations = station.getSensorStations();
            for (vector<SensorStation>::iterator it2 = sensorStations.begin(); it2 != sensorStations.end(); ++it2) {
                if (it2->getSensorStationType() == SensorStation::INTEGRATED_SENSOR_STATION)
                    it2->setLinkQuality(issReception);
            }

            string ssMessage = SensorStation::formatSensorStationStatusMessage(sensorStations, it->getDateTime());
            socket.sendData(ssMessage);
        }

        string message = it->formatMessage();
        log.log(VantageLogger::VANTAGE_INFO) << "=== Archive === " << Weather::formatDateTime(it->getDateTime()) << " =============" << endl;
        socket.sendData(message);
    }

    return true;
}
*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageDriver::reopenStation() {
    station.closeStation();
    bool success = station.openStation();

    if (!success)
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to reopen weather station" << endl;

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageDriver::mainLoop() {
    vector<ArchivePacket> list;
    list.reserve(VantageConstants::NUM_ARCHIVE_RECORDS);

    if (!initialize())
        return;

    /*
    if (!retrieveConfiguration())
        return;

    if (!archiveManager.synchronizeArchive()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to read the archive during initialization" << endl;
        return;
    }

    if (!station.wakeupStation()) {
        log.log(VantageLogger::VANTAGE_ERROR) << "Failed to wake up console after initialization" << endl;
        return;
    }
    */

    while (!exitLoop) {
        try {
            /*
            usleep(500000);
            VantageWeatherStation::ConsoleDiagnosticReport report;
            station.retrieveConsoleDiagnosticsReport(report);
            log.log(VantageLogger::VANTAGE_INFO) << "Diagnostic Report: " << "Packets: " << report.packetCount << "       Missed Packets: " << report.missedPacketCount
                                         << "      CRC Errors: " << report.crcErrorCount << endl;
            if (signalCaught.load()) {
                exitLoop = true;
            }
            */
            //exitLoop = true;

            //
            // If the weather station could not be woken, then close and open
            // the console. It has been observed that on a rare occasion the console
            // never wakes up. Only restarting this driver fixes the issue. Reopening
            // the serial port will hopefully fix this issue.
            //
            if (!station.wakeupStation()) {
                reopenStation();
                continue;
            }

            DateTime consoleTime;
            if (station.retrieveConsoleTime(consoleTime))
                log.log(VantageLogger::VANTAGE_INFO) << "Station Time: " << Weather::formatDateTime(consoleTime) << endl;
            else
                log.log(VantageLogger::VANTAGE_INFO) << "Station Time retrieval failed" << endl;

            //
            // If it has been a while since the time was set, set the time
            //
            DateTime now = time(0);
            if (consoleTimeSetTime + TIME_SET_INTERVAL < now) {
                if (!station.updateConsoleTime())
                    log.log(VantageLogger::VANTAGE_ERROR) << "Failed to set station time " << endl;

                consoleTimeSetTime = now;
            }

            //
            // Get the high/low values
            //
            /*
            HiLowPacket packet;
            if (station.retrieveHiLowValues(packet))
                cout << "Got hi/low packet" << endl;
                */

            //
            // Get the current weather values for about a minute
            //
            station.currentValuesLoop(LOOP_PACKET_CYCLES);

            //
            // If an asynchronous signal was caught, then exit the loop
            //
            if (signalCaught.load()) {
                exitLoop = true;
                continue;
            }

            string event;
            while (eventManager.consumeEvent(event)) {

            }

            //
            // If the LOOP packet data indicates that a new archive packet is available
            // go get it.
            //
            /*
            if (previousNextRecord != nextRecord) {
                if (archiveManager.synchronizeArchive()) {
                    ArchivePacket packet;
                    // TBD What if multiple archive packets are received?
                    archiveManager.getNewestRecord(packet);
                    log.log(VantageLogger::VANTAGE_DEBUG1) << "Most recent archive packet time is: "
                                                           << Weather::formatDateTime(packet.getDateTime())
                                                           << " Station Reception: " << station.calculateStationReceptionPercentage(packet.getWindSampleCount()) << endl; // TBD Get the actual archive period
                    previousNextRecord = nextRecord;
                }
            }
            */
        }
        catch (std::exception & e) {
            log.log(VantageLogger::VANTAGE_ERROR) << "Caught exception: " << e.what() << endl;     
        } 
    }
}
}
