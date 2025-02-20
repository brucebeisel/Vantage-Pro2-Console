#include <unistd.h>
#include <iostream>
#include "ArchiveManager.h"
#include "StormArchiveManager.h"
#include "CurrentWeatherManager.h"
#include "DataCommandHandler.h"
#include "GraphDataRetriever.h"
#include "CurrentWeatherSocket.h"
#include "CommandData.h"
#include "SerialPort.h"
#include "ResponseHandler.h"
#include "VantageDecoder.h"

using namespace vws;
using namespace std;

class Responder : public ResponseHandler {
    virtual void handleCommandResponse(const CommandData & commandData) {
        cout << "RESPONSE: '" << commandData.response << "'" << endl;
    }
};

int
main(int argc, char *argv[]) {
    VantageDecoder::setRainCollectorSize(.01);
    string dataDirectory = "./";
    SerialPort serialPort("device", 19200);
    VantageWeatherStation station(serialPort);
    ArchiveManager archiveManager(dataDirectory, station);
    GraphDataRetriever graphDataRetriever(station);
    StormArchiveManager stormArchiveManager(dataDirectory, graphDataRetriever);
    CurrentWeatherSocket currentWeatherPublisher;
    CurrentWeatherManager currentWeatherManager(dataDirectory, currentWeatherPublisher);
    Responder responseHandler;

    DataCommandHandler handler(archiveManager, stormArchiveManager, currentWeatherManager);

    handler.initialize();

    sleep(1);
    CommandData command;
    command.commandName = "Foobar";
    command.responseHandler = &responseHandler;
    if (!handler.offerCommand(command)) {
        cout << "Command: " << command << " rejected by DataCommandHandler" << endl;
    }

    command.commandName = "query-archive-statistics";
    command.loadResponseTemplate();
    if (!handler.offerCommand(command)) {
        cout << "Command: " << command << " rejected by DataCommandHandler" << endl;
    }

    sleep(1);

    command.commandName = "query-archive";
    command.loadResponseTemplate();
    command.arguments.clear();
    command.arguments.push_back(pair("start-time", "2024-01-01 00:00"));
    command.arguments.push_back(pair("end-time", "2024-01-01 00:15"));
    if (!handler.offerCommand(command)) {
        cout << "Command: " << command << " rejected by DataCommandHandler" << endl;
    }

    sleep(1);

    command.commandName = "query-archive-summary";
    command.loadResponseTemplate();
    command.arguments.clear();
    command.arguments.push_back(pair("start-time", "2024-01-01 00:00"));
    command.arguments.push_back(pair("end-time", "2024-01-31 23:59"));
    command.arguments.push_back(pair("summary-period", "Day"));
    command.arguments.push_back(pair("speed-bin-count", "5"));
    command.arguments.push_back(pair("speed-bin-increment", "2.5"));
    command.arguments.push_back(pair("speed-units", "mph"));
    if (!handler.offerCommand(command)) {
        cout << "Command: " << command << " rejected by DataCommandHandler" << endl;
    }


    sleep(5);
    handler.terminate();
    handler.join();
}
