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

using namespace std;
using namespace vws;
using namespace nlohmann;

static const char USAGE_MESSAGE[] = "Usage: command-line-console <device name>";

void backlightArgumentPrompter(CommandData & commandData);
void cumulativeValuePrompter(CommandData & commandData);
void extremePeriodPrompter(CommandData & commandData);

struct Command {
    std::string commandName;
    std::string printCommandName;
    void (ConsoleCommandHandler::*handler)(CommandData  & cd);
    void (*argumentPrompter)(CommandData & cd);
};

static const Command commands[] = {
    "Query configuration",                    "query-configuration-data", &ConsoleCommandHandler::handleQueryConfigurationData,              NULL,
    "Query console diagnostics",              "console-diagnostics",      &ConsoleCommandHandler::handleQueryConsoleDiagnostics,             NULL,
    "Query archive period",                   "query-archive-period",     &ConsoleCommandHandler::handleQueryArchivePeriod,                  NULL,
    "Query console type",                     "query-console-type",       &ConsoleCommandHandler::handleQueryConsoleType,                    NULL,
    "Query console firmware",                 "query-firmware",           &ConsoleCommandHandler::handleQueryFirmware,                       NULL,
    "Query monitored stations",               "query-used-transmitters",  &ConsoleCommandHandler::handleQueryMonitoredStations,              NULL,
    "Query station list",                     "query-station-list",       &ConsoleCommandHandler::handleQueryStationList,                    NULL,
    "Query receiver list",                    "query-receiver-list",      &ConsoleCommandHandler::handleQueryReceiverList,                   NULL,
    "Query units",                            "query-units",              &ConsoleCommandHandler::handleQueryUnits,                          NULL,
    "Query console time",                     "query-console-time",       &ConsoleCommandHandler::handleQueryConsoleTime,                    NULL,
    "Query Hi/Lows",                          "query-highlows",           &ConsoleCommandHandler::handleQueryHighLows,                       NULL,
    "Query alarm thresholds",                 "query-alarm-thresholds",   &ConsoleCommandHandler::handleQueryAlarmThresholds,                NULL,
    "Query network configuration",            "query-network-config",     &ConsoleCommandHandler::handleQueryNetworkConfiguration,           NULL,
    "Query calibration adjustments",          "query-cal-adjustments",    &ConsoleCommandHandler::handleQueryCalibrationAdjustments,         NULL,
    "Query active alarms",                    "query-active-alarms",      &ConsoleCommandHandler::handleQueryActiveAlarms,                   NULL,
    "Query barometer calibration parameters", "query-baro-cal-params",    &ConsoleCommandHandler::handleQueryBarometerCalibrationParameters, NULL,
    "Set back light state",                   "backlight",                &ConsoleCommandHandler::handleBacklight,                           backlightArgumentPrompter,
    "List supported time zones",              "get-timezones",            &ConsoleCommandHandler::handleGetTimezones,                        NULL,
    "Clear cumulative values",                "clear-cumulative-values",  &ConsoleCommandHandler::handleClearCumulativeValue,                cumulativeValuePrompter,
    "Clear high values",                      "clear-high-values",        &ConsoleCommandHandler::handleClearHighValues,                     extremePeriodPrompter,
    "Clear low values",                       "clear-low-values",         &ConsoleCommandHandler::handleClearLowValues,                      extremePeriodPrompter,
};

/*
    "clear-active-alarms",           NULL,                                                              &VantageWeatherStation::clearActiveAlarms,
    "clear-alarm-thresholds",        NULL,                                                              &VantageWeatherStation::clearAlarmThresholds,
    "clear-console-archive",         NULL,                                                              &VantageWeatherStation::clearArchive,
    "clear-calibration-offsets",     NULL,                                                              &VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets,
    "clear-current-data",            NULL,                                                              &VantageWeatherStation::clearCurrentData,
    "clear-graph-points",            NULL,                                                              &VantageWeatherStation::clearGraphPoints,
    "query-network-status",          &ConsoleCommandHandler::handleQueryNetworkStatus,                  NULL,
    "query-today-network-status",    &ConsoleCommandHandler::handleQueryTodayNetworkStatus,             NULL,
    "put-year-rain",                 &ConsoleCommandHandler::handlePutYearRain,                         NULL,
    "put-year-et",                   &ConsoleCommandHandler::handlePutYearET,                           NULL,
    "start-archiving",               NULL,                                                              &VantageWeatherStation::startArchiving,
    "stop-archiving",                NULL,                                                              &VantageWeatherStation::stopArchiving,
    "update-alarm-thresholds",       &ConsoleCommandHandler::handleUpdateAlarmThresholds,               NULL,
    "update-archive-period",         &ConsoleCommandHandler::handleUpdateArchivePeriod,                 NULL,
    "update-baro-reading-elevation", &ConsoleCommandHandler::handleUpdateBarometerReadingAndElevation,  NULL,
    "update-cal-adjustments",        &ConsoleCommandHandler::handleUpdateCalibrationAdjustments,        NULL,
    "update-configuration-data",     &ConsoleCommandHandler::handleUpdateConfigurationData,             NULL,
    "update-network-config",         &ConsoleCommandHandler::handleUpdateNetworkConfiguration,          NULL,
    "update-units",                  &ConsoleCommandHandler::handleUpdateUnits,                         NULL
    */

int
main(int argc, char *argv[]) {
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);

    char *device = NULL;

    if (argc < 2) {
        cerr << USAGE_MESSAGE << endl;
        exit(1);
    }

    device = argv[1];

    SerialPort serialPort(device, 19200);
    VantageWeatherStation station(serialPort);
    ArchiveManager archive("./", station);
    VantageConfiguration config(station);
    VantageStationNetwork network("./", station, archive);
    AlarmManager alarm(station);

    ConsoleCommandHandler cmd(station, config, network, alarm);

    if (!station.openStation()) {
        cerr << "Could not open weather console" << endl;
        exit(1);
    }

    if (!station.wakeupStation()) {
        cerr << "Could not wake up console" << endl;
        exit(2);
    }

    while (1) {
        int commandNumber;
        cout << "Choose a command" << endl;
        cout << "    0 - Exit" << endl;
        cout << "  999 - run NEWSETUP" << endl;
        int cmdNumber = 1;
        for (auto cmd : commands) {
            cout << "   " << setw(2) << cmdNumber << " - " << cmd.printCommandName << endl;
            cmdNumber++;
        }
        cout << ": ";
        cin >> commandNumber;

        if (commandNumber > sizeof(commands) / sizeof(commands[0]) && commandNumber != 999) {
            cout << "Invalid command number" << endl;
            continue;
        }

        if (commandNumber == 0)
            exit(0);

        if (commandNumber == 999) {
            station.initializeSetup();
            continue;
        }

        commandNumber--;

        CommandData commandData;
        commandData.commandName = commands[commandNumber].printCommandName;
        commandData.arguments.clear();
        commandData.response = "";
        commandData.responseHandler = NULL;
        commandData.loadResponseTemplate();

        if (commands[commandNumber].argumentPrompter != NULL)
            (*commands[commandNumber].argumentPrompter)(commandData);

        (cmd.*commands[commandNumber].handler)(commandData);
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
