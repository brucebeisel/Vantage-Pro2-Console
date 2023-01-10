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
#ifndef VANTAGE_DRIVER_H
#define VANTAGE_DRIVER_H

#ifdef _WIN32
#pragma warning(disable : 4512)
#endif

#include <string>
#include "Weather.h"
#include "ArchiveManager.h"
#include "EventManager.h"
#include "SensorStation.h"
#include "VantageWeatherStation.h"

namespace vws {
class VantageLogger;
class CurrentWeather;
class CurrentWeatherPublisher;

/**
 * Class that coordinates the communications with the Vantage console.
 */
class VantageDriver : VantageWeatherStation::LoopPacketListener {
public:
    /**
     * Constructor.
     * 
     * @param archiveManager The archive manager that will maintain the file containing the raw archive packets
     * @param cwp            The publisher that will be called each time a current weather record has been received
     * @param station        The object that handles the command protocols with the Vantage console
     */
    VantageDriver(ArchiveManager & archiveManager, CurrentWeatherPublisher & cwp, VantageWeatherStation & station, EventManager & eventManager);

    /**
     * Destructor.
     */
    virtual ~VantageDriver();

    /**
     * Initialize the driver.
     * 
     * @return True on success
     */
    bool initialize();

    /**
     * Retrieve the configuration data from the console.
     *
     * @return True if all of the configuration data was retrieved
     */
     bool retrieveConfiguration();

    /**
     * Close and open the station.
     */
    bool reopenStation();

    /**
     * Request that the main loop exits.
     */
    void stop();

    /**
     * The main loop that is the main "thread" of the driver.
     */
    void mainLoop();

    /**
     * Process a LOOP packet in a callback.
     *
     * @param packet The LOOP packet
     * @return True if the loop packet processing loop should continue
     */
    bool processLoopPacket(const LoopPacket & packet);

    /**
     * Process a LOOP2 packet in a callback.
     *
     * @param packet The LOOP2 packet
     * @return True if the loop packet processing loop should continue
     */
    bool processLoop2Packet(const Loop2Packet & packet);

private:
    /**
     * The number of LOOP/LOOP2 packet pairs that are received in succession. Note that if a new archive record is available
     * the loop packet cycle will be interrupted early.
     */
    static const int LOOP_PACKET_CYCLES = 12;

    static const int INITIAL_LOOP_PACKET_RETRIES = 5;

    /**
     * How often to set the time on the console.
     */
    static const int TIME_SET_INTERVAL = 3600;

    VantageWeatherStation &   station;
    CurrentWeatherPublisher & currentWeatherPublisher;
    ArchiveManager &          archiveManager;
    EventManager &            eventManager;
    bool                      exitLoop;
    int                       nextRecord;
    int                       previousNextRecord;
    DateTime                  lastArchivePacketTime;
    DateTime                  consoleTimeSetTime;
    //DateTime                  sensorStationSendTime;
    VantageLogger             log;
};

}
#endif /* VANTAGE_DRIVER_H */
