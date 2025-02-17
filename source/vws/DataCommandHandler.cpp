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

#include "DataCommandHandler.h"

#include "VantageLogger.h"
#include "CommandData.h"
#include "EventManager.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
dataCommandThreadEntry(DataCommandHandler * dch) {
    dch->mainLoop();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DataCommandHandler::DataCommandHandler(ArchiveManager & am, StormArchiveManager & sam, CurrentWeatherManager & cwm) : archiveManager(am),
                                                                                                                      stormArchiveManager(sam),
                                                                                                                      currentWeatherManager(cwm),
                                                                                                                      terminating(false),
                                                                                                                      logger(VantageLogger::getLogger("DataCommandHandler")),
                                                                                                                      commandThread(NULL) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DataCommandHandler::~DataCommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::initialize() {
    commandThread = new thread(dataCommandThreadEntry, this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleCommand(CommandData & command) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DataCommandHandler::isCommandNameForHandler(const std::string & commandName) const {
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::terminate() {
    terminating = true;
    eventManager.interrupt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::mainLoop() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Entering Data Command Handler thread" << endl;
    while (!terminating) {
        CommandData commandData;
        if (eventManager.waitForEvent(commandData)) {
            handleCommand(commandData);
        }

    }
    logger.log(VantageLogger::VANTAGE_INFO) << "Exiting Data Command Handler thread" << endl;
}
}
