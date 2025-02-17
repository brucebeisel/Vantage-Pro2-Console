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
#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include "CommandQueue.h"

namespace vws {

class CommandData;

/**
 * The data needed to respond to a command.
 */
class CommandHandler {
public:
    CommandHandler();

    /**
     * Destructor.
     */
    virtual ~CommandHandler();

    /**
     * Check if there is an command on the queue. Note that in a multi-threaded environment
     * the return value may no longer be valid when the consumeEvent() method is called.
     *
     * @return True if the queue is not empty at the moment
     */
    bool isCommandAvailable() const;

    void processNextCommand();

    void processCommand(CommandData & commandData);

    /**
     * Handle a command and write the response to the provided object.
     *
     * @param command  The command data needed to process a command
     */
    virtual void handleCommand(CommandData & command) = 0;

    /**
     * Check if the command name can be processed by this command handler.
     *
     * @param commandName The name of the command
     * @return True if this command handler recognizes this command name
     */
    virtual bool offerCommand(const CommandData & commandData) = 0;

protected:
    CommandQueue commandQueue;
};

}

#endif
