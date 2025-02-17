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
class EventManager;
class ArchiveManager;
class StormArchiveManager;
class CurrentWeatherManager;

class DataCommandHandler : public CommandHandler {
public:
    DataCommandHandler(ArchiveManager & archiveManager, StormArchiveManager & stormArchiveManager, CurrentWeatherManager & currentWeatherManager);

    /**
     * Destructor.
     */
    virtual ~DataCommandHandler();

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
    virtual bool isCommandNameForHandler(const std::string & commandName) const;

    void initialize();

    void terminate();

    void mainLoop();

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
