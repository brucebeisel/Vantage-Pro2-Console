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
#ifndef DATA_COMMAND_HANDLER_H_
#define DATA_COMMAND_HANDLER_H_

#include <string>
#include <thread>

#include "CommandHandler.h"

namespace vws {
class VantageLogger;
class CommandQueue;
class ArchiveManager;
class StormArchiveManager;
class CurrentWeatherManager;

/**
 * Command handler that will process command that do not need to talk to the Vantage Pro2 console, but can
 * retrieve all the necessary data from local storage.
 */
class DataCommandHandler : public CommandHandler {
public:
    /**
     * Constructor.
     *
     * @param archiveManager        The manager used to query the weather data archive
     * @param stormArchiveManager   The manager to query past storms
     * @param currentWeatherManager The manager to query the loop packets that make up the current weather
     */
    DataCommandHandler(ArchiveManager & archiveManager, StormArchiveManager & stormArchiveManager, CurrentWeatherManager & currentWeatherManager);

    /**
     * Destructor.
     */
    virtual ~DataCommandHandler();

    /**
     * Initialize this object, starting the command loop thread.
     */
    void initialize();

    /**
     * Handle a command and write the response to the provided object.
     * Note that there will always be a response.
     *
     * @param       command  The command in JSON text form
     */
    virtual void handleCommand(CommandData & command);

    /**
     * Check if the command name can be processed by this command handler.
     *
     * @param commandName The name of the command
     * @return True if this command handler recognizes this command name
     */
    virtual bool offerCommand(const CommandData & commandData);

    /**
     * The main loop of the thread.
     */
    void mainLoop();

    /**
     * Trigger the thread to return.
     */
    void terminate();

    /**
     * Join the thread.
     */
    void join();

    //
    // Data query commands
    //
    void handleQueryArchiveStatistics(CommandData & commandData);

    void handleQueryArchive(CommandData & commandData);

    void handleQueryArchiveSummary(CommandData & commandData);

    void handleQueryLoopArchive(CommandData & commandData);

    void handleQueryStormArchive(CommandData & commandData);

    void handleClearExtendedArchive(CommandData & commandData);

private:
    ArchiveManager &        archiveManager;
    StormArchiveManager &   stormArchiveManager;
    CurrentWeatherManager & currentWeatherManager;
    bool                    terminating;
    std::thread *           commandThread;       // The thread that processes data commands
    VantageLogger &         logger;
};

}

#endif
