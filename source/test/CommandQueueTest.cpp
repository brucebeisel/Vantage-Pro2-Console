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

#include "CommandQueue.h"
#include "CommandData.h"

using namespace std;
using namespace vws;

CommandQueue commandQueue;
bool terminateThread = false;

void
consumerThread() {
    cout << "Started consumer thread" << endl;

    CommandData commandData;
    while (commandQueue.waitForCommand(commandData))
        cout << "Command 1: " << commandData << endl;

    cout << "Leaving waitForCommand(), entering consumeCommand()" << endl;

    while (!terminateThread) {
        if (!commandQueue.isCommandAvailable()) {
            cout << "No commands in queue" << endl;
        }

        if (commandQueue.consumeCommand(commandData))
            cout << "Command 2: " << commandData << endl;
        else
            cout << "No command found in queue" << endl;

        sleep(1);
    }
}


int
main(int argc, char *argv[]) {

    thread t(consumerThread);

    sleep(2);

    CommandData commandData;
    commandData.commandName = "Command1";
    commandData.arguments.push_back(CommandData::CommandArgument("name", "value"));
    commandData.fd = 100;
    commandData.response = "";
    commandData.responseHandler = NULL;

    commandQueue.queueCommand(commandData);
    sleep(1);
    commandQueue.interrupt();

    sleep(5);

    commandData.arguments.push_back(CommandData::CommandArgument("name2", "value2"));
    commandQueue.queueCommand(commandData);

    sleep(2);

    terminateThread = true;
    t.join();


}
