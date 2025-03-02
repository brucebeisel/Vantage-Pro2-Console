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
#ifndef VANTAGE_DRIVER_H
#define VANTAGE_DRIVER_H

#ifdef _WIN32
#pragma warning(disable : 4512)
#endif

#include <string>
#include <thread>
#include "WeatherTypes.h"
#include "VantageWeatherStation.h"
#include "CommandHandler.h"
#include "ConsoleConnectionMonitor.h"

namespace vws {
class ArchiveManager;
class CommandQueue;
class VantageLogger;
class CurrentWeather;
class CurrentWeatherSocket;
class VantageConfiguration;
class StormArchiveManager;

/**
 * Class that coordinates the communications with the Vantage console.
 */
class VantageDriver : public VantageWeatherStation::LoopPacketListener, public ConsoleConnectionMonitor {
public:
    /**
     * Constructor.
     * 
     * @param station             The object that handles the command protocols with the Vantage console
     * @param configuration       The object that handles configuration changes and retrievals with the console
     * @param archiveManager      The archive manager that will maintain the file containing the raw archive packets
     * @param commandHandler      The command handler that will provide commands coming in from clients
     * @param stormArchiveManager The manager of the storm archive
     */
    VantageDriver(VantageWeatherStation & station, VantageConfiguration & configuration, ArchiveManager & archiveManager, CommandHandler & commandHandler, StormArchiveManager & stormArchiveManager);

    /**
     * Destructor.
     */
    virtual ~VantageDriver();

    /**
     * Start the console driver thread.
     */
    void start();

    /**
     * Retrieve the configuration data from the console.
     *
     * @return True if all of the configuration data was retrieved
     */
     bool retrieveConfiguration();

    /**
     * Request that the main loop exits.
     */
    void terminate();

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

    /**
     * Join the console driver thread.
     */
    void join();

    /**
     * Add a connection monitor to the list of objects that will be notified when the state of the console
     * connected changes.
     */
    void addConnectionMonitor(ConsoleConnectionMonitor & monitor);

    /**
     * Called when a connection with the console is established.
     */
    virtual void consoleConnected();

    /**
     * Called when the connection with the console is lost.
     */
    virtual void consoleDisconnected();

private:
    /**
     * The number of LOOP/LOOP2 packet pairs that are received in succession. Note that if a new archive record is available
     * or a command is received, the loop packet cycle will be interrupted early.
     */
    static const int LOOP_PACKET_CYCLES = 60;

    /**
     * A single LOOP packet is used to retrieve some of the Vantage station configuration parameters.
     * This is the number of times it will try to get that LOOP packet before giving up.
     */
    static const int INITIAL_LOOP_PACKET_RETRIES = 5;

    /**
     * How often to set the time on the console.
     */
    static const int TIME_SET_INTERVAL = 3600;

    /**
     * How often to update the storm archive.
     */
    static const int STORM_ARCHIVE_UPDATE_INTERVAL = 3600 * 2;

    /**
     * How often to verify the archive.
     */
    static constexpr int ARCHIVE_VERIFY_INTERVAL = 86400;

    /**
     * Open console and retrieve configuration data.
     *
     * @return True if connected to the console
     */
    bool connectToConsole();

    /**
     * Disconnect from the console and initialize state members.
     */
    void disconnectFromConsole();

    bool                                    isConsoleConnected;
    VantageWeatherStation &                 station;
    VantageConfiguration &                  configuration;
    ArchiveManager &                        archiveManager;
    CommandHandler &                        commandHandler;
    StormArchiveManager &                   stormArchiveManager;
    std::vector<ConsoleConnectionMonitor *> connectionMonitors;
    bool                                    exitLoop;
    int                                     nextRecord;
    int                                     previousNextRecord;
    DateTime                                lastArchivePacketTime;
    DateTime                                consoleTimeSetTime;
    DateTime                                lastStormArchiveUpdateTime;
    DateTime                                lastArchiveVerifyTime;
    std::thread *                           consoleThread;
    VantageLogger &                         logger;
};

}
#endif /* VANTAGE_DRIVER_H */
