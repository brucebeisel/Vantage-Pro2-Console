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

#include <iostream>
#include <thread>
#include <unistd.h>

#include "VantageLogger.h"
#include "CommandQueue.h"
#include "CommandData.h"

using namespace std;
using namespace vws;

CommandQueue * commandQueue;
bool terminateThread = false;
int commandCount = 0;

void
waitForThread() {
    cout << "Started waitFor thread" << endl;

    CommandData commandData;
    while (!terminateThread) {
        if (commandQueue->waitForCommand(commandData)) {
            commandCount++;
            cout << "Command " << commandCount << ": " << commandData << endl;
        }
    }

}

void
consumeThread() {
    cout << "Started consume thread" << endl;

    CommandData commandData;

    while (!terminateThread) {
        if (!commandQueue->isCommandAvailable()) {
            cout << "No commands in queue" << endl;
        }

        if (commandQueue->consumeCommand(commandData)) {
            cout << "Command 2: " << commandData << endl;
            commandCount++;
        }
        else
            cout << "No command found in queue" << endl;

        sleep(1);
    }
}


int
main(int argc, char *argv[]) {
    commandQueue = new CommandQueue();


    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);

    //
    // Queue two commands then start the thread. The waitForCommand() loop should consume both
    // commands.
    //
    CommandData commandData;
    commandData.commandName = "Command1";
    commandData.arguments.push_back(CommandData::CommandArgument("name", "value"));
    commandData.socketId = 100;
    commandData.response = "";
    commandData.responseHandler = NULL;

    commandQueue->queueCommand(commandData);
    commandData.socketId = 101;
    commandQueue->queueCommand(commandData);

    thread t(waitForThread);

    sleep(2);

    if (commandCount == 2)
    cout << "PASSED: CommandQueue offered 2 commands" << endl;
    else
    cout << "FAILED: CommandQueue offered " << commandCount << " commands, instead of 2" << endl;

    terminateThread = true;
    commandQueue->interrupt();
    t.join();

    /*
    commandData.arguments.push_back(CommandData::CommandArgument("name2", "value2"));
    commandQueue->queueCommand(commandData);

    sleep(2);

    terminateThread = true;
    t.join();
    */
}
