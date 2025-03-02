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

#include "CommandSocket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "CommandHandler.h"
#include "CommandData.h"
#include "VantageLogger.h"


using namespace std;
using namespace vws;

VantageLogger * glogger;


class TestCommandHandler : public CommandHandler {
public:
    virtual void handleCommand(CommandData & command) {
        command.response.append("\"success\"}");
        command.responseHandler->handleCommandResponse(command);
    }

    virtual bool offerCommand(const CommandData & commandData) {
        glogger->log(VantageLogger::VANTAGE_INFO) << "Being offered command: " << commandData << endl;
        CommandData data = commandData;
        handleCommand(data);
        return true;
    }
};

int
main(int argc, char *argv[]) {
    VantageLogger logger(VantageLogger::getLogger("CommandSocketTest"));
    glogger = & logger;
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    TestCommandHandler handler;
    CommandSocket commandSocket(11463);
    commandSocket.addCommandHandler(handler);

    commandSocket.start();

    sleep(1);

    int s = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(11463);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(s, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    const char * command  = "VANTAGE 000054 { \"command\" : \"query-console-time\", \"arguments\" : [] }";

    write(s, (void *)command, strlen(command));

    char buffer[1024];
    int n = read(s, buffer, sizeof(buffer));
    buffer[n] = '\0';

    logger.log(VantageLogger::VANTAGE_INFO) << "Response: '" << buffer << "'" << endl;
    close(s);

    sleep(1);

    commandSocket.terminate();
    commandSocket.join();
}
