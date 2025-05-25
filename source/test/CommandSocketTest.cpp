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

#include <sys/time.h>
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

bool changeSocketId = false;

class TestCommandHandler : public CommandHandler {
public:
    virtual void handleCommand(CommandData & commandData) {
        glogger->log(VantageLogger::VANTAGE_INFO) << "Handling command: " << commandData << endl;

        if (changeSocketId)
            commandData.socketId = 9999;

        commandData.response.append("\"success\"}");
        commandData.responseHandler->handleCommandResponse(commandData);
    }

    virtual bool offerCommand(const CommandData & commandData) {
        glogger->log(VantageLogger::VANTAGE_INFO) << "Being offered command: " << commandData << endl;
        CommandData data = commandData;
        handleCommand(data);
        return true;
    }
};

int
connectSocket() {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(11463);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        glogger->log(VantageLogger::VANTAGE_WARNING) << "setsockopt() failed (" << glogger->strerror() << ")" << endl;
    }


    connect(s, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    return s;
}

std::string
readResponse(int s) {
    char buffer[1024];
    buffer[0] = '\0';
    int n = read(s, buffer, sizeof(buffer));

    if (n > 0) {
        buffer[n] = '\0';
        glogger->log(VantageLogger::VANTAGE_INFO) << "Response: '" << buffer << "'" << endl;
    }
    else if (n < 0)
        glogger->log(VantageLogger::VANTAGE_ERROR) << "read() returned an error (" << glogger->strerror() << ")" << endl;
    else
        glogger->log(VantageLogger::VANTAGE_ERROR) << "read() returned 0" << endl;

    return string(buffer);
}

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

    int s = connectSocket();
    const char * command  = "VANTAGE 000054 { \"command\" : \"query-console-time1\", \"arguments\" : [] }";
    write(s, (void *)command, strlen(command));
    string response = readResponse(s);
    close(s);

    sleep(1);

    changeSocketId = true;
    s = connectSocket();
    command  = "VANTAGE 000055 { \"command\" : \"query-console-time2\", \"arguments\" : [] }";
    write(s, (void *)command, strlen(command));
    response = readResponse(s);
    close(s);

    changeSocketId = false;
    s = connectSocket();
    command  = "VANTAGE 000055 { \"command\" : \"query-console-time3\", \"arguments\" : [] }";
    write(s, (void *)command, strlen(command));
    response = readResponse(s);
    close(s);

    sleep(1);

    int s1 = connectSocket();
    int s2 = connectSocket();

    sleep(1);

    close(s1);

    sleep(1);

    close(s2);

    sleep(1);

    commandSocket.terminate();
    commandSocket.join();
}
