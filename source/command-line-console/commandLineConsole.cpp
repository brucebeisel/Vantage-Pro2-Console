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
#include <getopt.h>
#include <iostream>

#include "json.hpp"
#include "VantageWeatherStation.h"
#include "SerialPort.h"
#include "ConsoleCommandHandler.h"
#include "VantageConfiguration.h"
#include "VantageStationNetwork.h"
#include "AlarmManager.h"
#include "ArchiveManager.h"
#include "VantageLogger.h"
#include "VantageEnums.h"
#include "GraphDataRetriever.h"

using namespace std;
using namespace vws;
using namespace nlohmann;

static const char USAGE_MESSAGE[] = "Usage: command-line-console -p <device name> [-v <debug output level (0 - 3)> ] [-h]";

void backlightArgumentPrompter(CommandData & commandData);
void cumulativeValuePrompter(CommandData & commandData);
void extremePeriodPrompter(CommandData & commandData);
void confirmPrompter(CommandData & commandData);
void yearRainPrompter(CommandData & commandData);
void yearEtPrompter(CommandData & commandData);
void archivePeriodPrompter(CommandData & commandData);

struct Command {
    std::string commandName;
    std::string printCommandName;
    void (ConsoleCommandHandler::*commandHandler)(CommandData  & cd);
    bool (VantageWeatherStation::*consoleHandler)();
    void (*argumentPrompter)(CommandData & cd);
};

static const Command commands[] = {
    "Query configuration",                    "query-configuration-data",  &ConsoleCommandHandler::handleQueryConfigurationData,              NULL,                                                               NULL,
    "Query console diagnostics",              "console-diagnostics",       &ConsoleCommandHandler::handleQueryConsoleDiagnostics,             NULL,                                                               NULL,
    "Query archive period",                   "query-archive-period",      &ConsoleCommandHandler::handleQueryArchivePeriod,                  NULL,                                                               NULL,
    "Query console type",                     "query-console-type",        &ConsoleCommandHandler::handleQueryConsoleType,                    NULL,                                                               NULL,
    "Query console firmware",                 "query-firmware",            &ConsoleCommandHandler::handleQueryFirmware,                       NULL,                                                               NULL,
    "Query monitored stations",               "query-used-transmitters",   &ConsoleCommandHandler::handleQueryMonitoredStations,              NULL,                                                               NULL,
    "Query station list",                     "query-station-list",        &ConsoleCommandHandler::handleQueryStationList,                    NULL,                                                               NULL,
    "Query receiver list",                    "query-receiver-list",       &ConsoleCommandHandler::handleQueryReceiverList,                   NULL,                                                               NULL,
    "Query units",                            "query-units",               &ConsoleCommandHandler::handleQueryUnits,                          NULL,                                                               NULL,
    "Query console time",                     "query-console-time",        &ConsoleCommandHandler::handleQueryConsoleTime,                    NULL,                                                               NULL,
    "Query Hi/Lows",                          "query-highlows",            &ConsoleCommandHandler::handleQueryHighLows,                       NULL,                                                               NULL,
    "Query alarm thresholds",                 "query-alarm-thresholds",    &ConsoleCommandHandler::handleQueryAlarmThresholds,                NULL,                                                               NULL,
    "Query network configuration",            "query-network-config",      &ConsoleCommandHandler::handleQueryNetworkConfiguration,           NULL,                                                               NULL,
    "Query calibration adjustments",          "query-cal-adjustments",     &ConsoleCommandHandler::handleQueryCalibrationAdjustments,         NULL,                                                               NULL,
    "Query active alarms",                    "query-active-alarms",       &ConsoleCommandHandler::handleQueryActiveAlarms,                   NULL,                                                               NULL,
    "Query barometer calibration parameters", "query-baro-cal-params",     &ConsoleCommandHandler::handleQueryBarometerCalibrationParameters, NULL,                                                               NULL,
    "Query network status",                   "query-network-status",      &ConsoleCommandHandler::handleQueryNetworkStatus,                  NULL,                                                               NULL,
    "Query network status for today",         "query-today-network-status",&ConsoleCommandHandler::handleQueryTodayNetworkStatus,             NULL,                                                               NULL,
    "Set back light state",                   "backlight",                 &ConsoleCommandHandler::handleBacklight,                           NULL,                                                               backlightArgumentPrompter,
    "List supported time zones",              "get-timezones",             &ConsoleCommandHandler::handleGetTimezones,                        NULL,                                                               NULL,
    "Clear cumulative values",                "clear-cumulative-values",   &ConsoleCommandHandler::handleClearCumulativeValue,                NULL,                                                               cumulativeValuePrompter,
    "Clear high values",                      "clear-high-values",         &ConsoleCommandHandler::handleClearHighValues,                     NULL,                                                               extremePeriodPrompter,
    "Clear low values",                       "clear-low-values",          &ConsoleCommandHandler::handleClearLowValues,                      NULL,                                                               extremePeriodPrompter,
    "Start archiving",                        "start-archiving",           NULL,                                                              &VantageWeatherStation::startArchiving,                             NULL,
    "Stop archiving",                         "stop-archiving",            NULL,                                                              &VantageWeatherStation::stopArchiving,                              NULL,
    "Query archiving state",                  "query-archiving-state",     &ConsoleCommandHandler::handleQueryArchivingState,                 NULL,                                                               NULL,
    "Clear active alarms",                    "clear-active-alarms",       NULL,                                                              &VantageWeatherStation::clearActiveAlarms,                          NULL,
    "Clear alarm thresholds",                 "clear-alarm-thresholds",    NULL,                                                              &VantageWeatherStation::clearAlarmThresholds,                       NULL,
    "Clear console's archive",                "clear-console-archive",     NULL,                                                              &VantageWeatherStation::clearArchive,                               confirmPrompter,
    "Clear calibration offsets",              "clear-calibration-offsets", NULL,                                                              &VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets, NULL,
    "Clear current data",                     "clear-current-data",        NULL,                                                              &VantageWeatherStation::clearCurrentData,                           NULL,
    "Clear graph points",                     "clear-graph-points",        NULL,                                                              &VantageWeatherStation::clearGraphPoints,                           NULL,
    "Put year rain",                          "put-year-rain",             &ConsoleCommandHandler::handlePutYearRain,                         NULL,                                                               yearRainPrompter,
    "Put year ET",                            "put-year-et",               &ConsoleCommandHandler::handlePutYearET,                           NULL,                                                               yearEtPrompter,
    "Update archive period",                  "update-archive-period",     &ConsoleCommandHandler::handleUpdateArchivePeriod,                 NULL,                                                               archivePeriodPrompter,
};

/*


    "update-alarm-thresholds",       &ConsoleCommandHandler::handleUpdateAlarmThresholds,               NULL,
    "update-baro-reading-elevation", &ConsoleCommandHandler::handleUpdateBarometerReadingAndElevation,  NULL,
    "update-cal-adjustments",        &ConsoleCommandHandler::handleUpdateCalibrationAdjustments,        NULL,
    "update-configuration-data",     &ConsoleCommandHandler::handleUpdateConfigurationData,             NULL,
    "update-network-config",         &ConsoleCommandHandler::handleUpdateNetworkConfiguration,          NULL,
    "update-units",                  &ConsoleCommandHandler::handleUpdateUnits,                         NULL
    */

int
main(int argc, char *argv[]) {
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_INFO);

    string serialPortName;
    VantageLogger::Level debugLevel;
    int debugLevelOption;
    int opt;
    bool errorFound = false;
    while ((opt = getopt(argc, argv, "p:v:h")) != -1) {
        switch (opt) {
            case 'p':
                cout << "Serial port: " << optarg << endl;
                serialPortName = optarg;
                break;

            case 'v':
                debugLevelOption = atoi(optarg);
                if (debugLevelOption < 0 || debugLevelOption > 3) {
                    cerr << "Invalid debug verbosity. Must be from 0 to 3" << endl;
                    errorFound = true;
                }
                else {
                    switch (debugLevelOption) {
                        case 0:
                            debugLevel = VantageLogger::VANTAGE_INFO;
                            break;

                        case 1:
                            debugLevel = VantageLogger::VANTAGE_DEBUG1;
                            break;

                        case 2:
                            debugLevel = VantageLogger::VANTAGE_DEBUG2;
                            break;

                        case 3:
                            debugLevel = VantageLogger::VANTAGE_DEBUG3;
                            break;
                    }

                    VantageLogger::setLogLevel(debugLevel);
                }
                break;

            case 'h':
                errorFound = true;
                break;
        }
    }

    if (errorFound || serialPortName == "") {
        cerr << USAGE_MESSAGE << endl;
        exit(1);
    }


    SerialPort serialPort(serialPortName, vws::BaudRate::BR_19200);
    VantageWeatherStation station(serialPort);
    ArchiveManager archive("./");
    VantageConfiguration config(station);
    VantageStationNetwork network("./", station, archive);
    AlarmManager alarm(station);

    config.addRainCollectorSizeListener(alarm);
    config.addRainCollectorSizeListener(station);

    ConsoleCommandHandler cmd(station, config, network, alarm);

    GraphDataRetriever gdr(station);

    if (!station.openStation()) {
        cerr << "Could not open weather console" << endl;
        exit(1);
    }

    if (!station.wakeupStation()) {
        cerr << "Could not wake up console" << endl;
        exit(2);
    }

    //
    // Use the console connected processing to get the rain collector bucket size
    //
    config.consoleConnected();
    station.consoleConnected();

    while (1) {
        int commandNumber;
        cout << "Choose a command" << endl;
        cout << "    0 - Exit" << endl;
        cout << "   99 - Retrieve RX Percentage Data" << endl;
        cout << "  999 - run NEWSETUP" << endl;
        int cmdNumber = 1;
        for (auto cmd : commands) {
            cout << "   " << setw(2) << cmdNumber << " - " << cmd.printCommandName << endl;
            cmdNumber++;
        }
        cout << ": ";
        cin >> commandNumber;

        if (commandNumber > sizeof(commands) / sizeof(commands[0]) && commandNumber != 999 && commandNumber != 99) {
            cout << "Invalid command number" << endl;
            continue;
        }

        if (commandNumber == 0)
            exit(0);

        if (commandNumber == 999) {
            station.initializeSetup();
            continue;
        }

        if (commandNumber == 99) {
            gdr.retrieveDayReceivePercentages();
            continue;
        }

        commandNumber--;

        CommandData commandData;
        commandData.commandName = commands[commandNumber].printCommandName;
        commandData.arguments.clear();
        commandData.response = "";
        commandData.responseHandler = NULL;
        commandData.loadResponseTemplate();

        const Command & command = commands[commandNumber];

        if (command.argumentPrompter != NULL)
            (*command.argumentPrompter)(commandData);

        if (command.commandHandler != NULL)
            (cmd.*command.commandHandler)(commandData);
        else if (command.consoleHandler != NULL)
            (station.*command.consoleHandler)();

        commandData.response.append("}");

        if (!json::accept(commandData.response))
            cerr << "Command response is not valid JSON: '" << commandData.response << "'" << endl;
        else {
            json dom = json::parse(commandData.response);
            cout << "--------------------" << endl;
            cout << setw(4) << dom << endl;
            cout << "--------------------" << endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
backlightArgumentPrompter(CommandData & commandData) {
    string answer;

    while (answer != "on" && answer != "off") {
        cout << "Backlight on or off? ";
        cin >> answer;
    }

    std::pair<string,string> arg;
    arg.first = "state";
    arg.second = answer;
    commandData.arguments.push_back(arg);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T, int C>
void
promptForEnum(VantageEnum<T,C> & e, CommandData & commandData, const char * argName) {
    vector<string> values;
    e.enumStrings(values);

    int answer = 0;
    while (answer < 1 || answer > values.size()) {
        int n = 1;
        for (auto value : values) {
            cout << "    " << n << " - " << value << endl;
            n++;
        }

        cout << "? ";
        cin >> answer;
    }

    CommandData::CommandArgument arg;
    arg.first = argName;
    arg.second = values.at(answer - 1);
    commandData.arguments.push_back(arg);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
cumulativeValuePrompter(CommandData & commandData) {
    cout << "Choose the cumulative value to clear" << endl;
    promptForEnum(cumulativeValueEnum, commandData, "value");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
extremePeriodPrompter(CommandData & commandData) {
    cout << "Choose period for which to clear high values" << endl;
    promptForEnum(extremePeriodEnum, commandData, "period");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
archivePeriodPrompter(CommandData & commandData) {
    cout << "Choose archive period" << endl;
    promptForEnum(archivePeriodEnum, commandData, "period");

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
confirmPrompter(CommandData & commandData) {
    cout << "Command " << commandData.commandName << " is destructive to the data on the console" << endl;
    string answer;

    while (answer != "Yes" && answer != "No") {
        cout << "Proceed (Yes or No)? ";
        cin >> answer;
    }

    if (answer != "Yes")
        exit(2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
std::string
numberPrompter(const std::string & prompt, T min, T max, int precision) {
    cout << "Enter value for " << prompt << ". Range " << min << " - " << max << endl;

    T value = min - 1;;
    while (value <= min && value >= max) {
        cout << "? ";
        cin >> value;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
yearRainPrompter(CommandData & commandData) {
    std::string value = numberPrompter("Year Rain", 0.0, 327.0, 2);
    CommandData::CommandArgument arg;
    arg.first = "value";
    arg.second = value;
    commandData.arguments.push_back(arg);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
yearEtPrompter(CommandData & commandData) {
    std::string value = numberPrompter("Year Rain", 0.0, 327.0, 3);
    CommandData::CommandArgument arg;
    arg.first = "value";
    arg.second = value;
    commandData.arguments.push_back(arg);
}
