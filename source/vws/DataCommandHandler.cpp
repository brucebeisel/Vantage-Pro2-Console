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

#include <vector>
#include "VantageLogger.h"
#include "CommandData.h"
#include "DateTimeFields.h"
#include "StormArchiveManager.h"
#include "ArchiveManager.h"
#include "AlarmManager.h"
#include "CommandQueue.h"
#include "SummaryReport.h"
#include "SummaryEnums.h"
#include "CurrentWeather.h"
#include "CurrentWeatherManager.h"
#include "WindRoseData.h"
#include "VantageEnums.h"

using namespace std;

namespace vws {
struct DataCommandEntry {
    std::string commandName;
    void (DataCommandHandler::*handler)(CommandData &);
};

/**
 * Table used to map the command name to the handler function.
 */
static const DataCommandEntry dataCommandList[] = {
        "query-archive-statistics", &DataCommandHandler::handleQueryArchiveStatistics,
        "query-archive",            &DataCommandHandler::handleQueryArchive,
        "query-archive-summary",    &DataCommandHandler::handleQueryArchiveSummary,
        "query-storm-archive",      &DataCommandHandler::handleQueryStormArchive,
        "clear-extended-archive",   &DataCommandHandler::handleClearExtendedArchive,
        "query-current-weather",    &DataCommandHandler::handleQueryLoopArchive,
        "query-alarm-history",      &DataCommandHandler::handleQueryAlarmHistory
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
dataCommandThreadEntry(DataCommandHandler * dch) {
    dch->mainLoop();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DataCommandHandler::DataCommandHandler(ArchiveManager & am, StormArchiveManager & sam, CurrentWeatherManager & cwm, AlarmManager & alm) : archiveManager(am),
                                                                                                                                         stormArchiveManager(sam),
                                                                                                                                         currentWeatherManager(cwm),
                                                                                                                                         alarmManager(alm),
                                                                                                                                         terminating(false),
                                                                                                                                         commandThread(NULL),
                                                                                                                                         logger(VantageLogger::getLogger("DataCommandHandler")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DataCommandHandler::~DataCommandHandler() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::start() {
    commandThread = new thread(dataCommandThreadEntry, this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleCommand(CommandData & commandData) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Processing command " << commandData << endl;
    for (auto & commandEntry : dataCommandList) {
        if (commandData.commandName == commandEntry.commandName) {
            (this->*commandEntry.handler)(commandData);
            return;
        }
    }

    logger.log(VantageLogger::VANTAGE_WARNING) << "handleCommand() received unexpected command named '" << commandData.commandName << "'" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DataCommandHandler::offerCommand(const CommandData & commandData) {
    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Being offered command " << commandData.commandName << endl;
    for (auto & entry : dataCommandList) {
        if (commandData.commandName == entry.commandName) {
            commandQueue.queueCommand(commandData);
            logger.log(VantageLogger::VANTAGE_DEBUG3) << "Offer of command " << commandData.commandName << " accepted" << endl;
            return true;
        }
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Offer of command " << commandData.commandName << " rejected" << endl;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::terminate() {
    terminating = true;
    commandQueue.interrupt();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::mainLoop() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Entering Data Command Handler thread" << endl;
    while (!terminating) {
        try {
            CommandData commandData;
            if (commandQueue.waitForCommand(commandData)) {
                processCommand(commandData);
            }
        }
        catch (const std::exception & e) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught exception in DataCommandHandler::mainLoop. " << e.what() << endl;
        }
        catch (...) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Caught unknown exception in DataCommandHandler::mainLoop." << endl;
        }
    }
    logger.log(VantageLogger::VANTAGE_INFO) << "Exiting Data Command Handler thread" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::join() {
    if (commandThread != NULL && commandThread->joinable()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Joining the thread" << endl;
        commandThread->join();
        delete commandThread;
        commandThread = NULL;
    }
    else
        logger.log(VantageLogger::VANTAGE_WARNING) << "Ignoring join request. Thread was not created or is not running." << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryArchiveStatistics(CommandData & commandData) {
    DateTimeFields oldestRecordTime;
    DateTimeFields newestRecordTime;
    int            archiveRecordCount;

    archiveManager.getArchiveRange(oldestRecordTime, newestRecordTime, archiveRecordCount);

    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : { "
        << "\"oldestRecordTime\" : \"" << oldestRecordTime.formatDateTime() << "\", "
        << "\"newestRecordTime\" : \"" << newestRecordTime.formatDateTime() << "\", "
        << "\"recordCount\" : " << archiveRecordCount
        << "}";

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryArchive(CommandData & commandData) {
    DateTimeFields startTime;
    DateTimeFields endTime;

    for (CommandData::CommandArgument arg : commandData.arguments) {
        if (arg.first == "start-time") {
            startTime.parseDateTime(arg.second);
        }
        else if (arg.first == "end-time") {
            endTime.parseDateTime(arg.second);
        }
    }

    if (!startTime.isDateTimeValid() || !endTime.isDateTimeValid()) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query the archive with times: " << startTime.formatDateTime() << " - " << endTime.formatDateTime() << endl;
        vector<ArchivePacket> packets;
        archiveManager.queryArchiveRecords(startTime, endTime, packets);

        ostringstream oss;
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : [ ";

        bool first = true;
        for (ArchivePacket packet : packets) {
            if (!first) oss << ", "; else first = false;
            oss << packet.formatJSON();
        }

        oss << "]";
        commandData.response.append(oss.str());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryArchiveSummary(CommandData & commandData) {
    DateTimeFields startTime;
    DateTimeFields endTime;
    SummaryPeriod summaryPeriod;
    int speedBinCount = 0;
    Speed speedBinIncrement = 0.0;
    bool foundSummaryPeriodArgument = false;
    ProtocolConstants::WindUnits windUnits;
    bool foundWindUnits = false;

    try {
        for (CommandData::CommandArgument arg : commandData.arguments) {
            if (arg.first == "start-time") {
                startTime.parseDateTime(arg.second);
            }
            else if (arg.first == "end-time") {
                endTime.parseDateTime(arg.second);
            }
            else if (arg.first == "summary-period") {
                summaryPeriod = summaryPeriodEnum.stringToValue(arg.second);
                foundSummaryPeriodArgument = true;
            }
            else if (arg.first == "speed-bin-count") {
                speedBinCount = atoi(arg.second.c_str());
            }
            else if (arg.first == "speed-bin-increment") {
                speedBinIncrement = atof(arg.second.c_str());
            }
            else if (arg.first == "speed-units") {
                windUnits = windUnitsEnum.stringToValue(arg.second);
                foundWindUnits = true;
            }
        }

        if (!startTime.isDateTimeValid() || !endTime.isDateTimeValid() ||
            speedBinCount == 0 || speedBinIncrement == 0.0 ||
            !foundWindUnits ||
            !foundSummaryPeriodArgument)
            commandData.response.append(CommandData::buildFailureString("Missing argument"));
        else {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Query summaries from the archive with times: " << startTime << " - " << endTime << endl;
            WindRoseData windRoseData(windUnits, speedBinIncrement, speedBinCount);
            SummaryReport report(summaryPeriod, startTime, endTime, archiveManager, windRoseData);
            report.loadData();

            ostringstream oss;
            oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
            oss << report.formatJSON();
            commandData.response.append(oss.str());
        }
    }
    catch (const std::exception & e) {
        commandData.response.append(CommandData::buildFailureString("Invalid summary period or wind speed unit"));
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryLoopArchive(CommandData & commandData) {
    int hours = 1;
    for (CommandData::CommandArgument arg : commandData.arguments) {
        if (arg.first == "hours") {
            hours = atoi(arg.second.c_str());
        }
    }

    vector<CurrentWeather> list;
    currentWeatherManager.queryCurrentWeatherArchive(hours, list);
    ostringstream oss;
    oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : [ ";

    bool first = true;
    for (CurrentWeather cw : list) {
        if (!first) oss << ", "; else first = false;
        oss << cw.formatJSON();
    }

    oss << " ]";

    commandData.response.append(oss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryStormArchive(CommandData & commandData) {
    DateTimeFields startDate;
    DateTimeFields endDate;

    for (CommandData::CommandArgument arg : commandData.arguments) {
        int year, month, monthDay;
        if (arg.first == "start-time") {
            startDate.parseDate(arg.second);
        }
        else if (arg.first == "end-time") {
            endDate.parseDate(arg.second);
        }
    }

    if (!startDate.isDateTimeValid() || !endDate.isDateTimeValid()) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
    }
    else {
        vector<StormData> storms;
        stormArchiveManager.queryStorms(startDate, endDate, storms);
        ostringstream oss;
        oss << SUCCESS_TOKEN << ", " << DATA_TOKEN << " : ";
        oss << StormArchiveManager::formatStormJSON(storms);
        commandData.response.append(oss.str());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleClearExtendedArchive(CommandData & commandData) {
    bool success = false;
    if (archiveManager.backupArchiveFile())
        success = archiveManager.clearArchiveFile();

    if (success)
        commandData.response.append(SUCCESS_TOKEN);
    else
        commandData.response.append(CONSOLE_COMMAND_FAILURE_STRING);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DataCommandHandler::handleQueryAlarmHistory(CommandData & commandData) {
    DateTimeFields startDate;
    DateTimeFields endDate;

    for (CommandData::CommandArgument arg : commandData.arguments) {
        int year, month, monthDay;
        if (arg.first == "start-time") {
            startDate.parseDate(arg.second);
        }
        else if (arg.first == "end-time") {
            endDate.parseDate(arg.second);
        }
    }

    if (!startDate.isDateTimeValid() || !endDate.isDateTimeValid()) {
        commandData.response.append(CommandData::buildFailureString("Missing argument"));
    }
    else {
        string alarmHistoryJson = alarmManager.formatAlarmHistoryJSON(startDate, endDate);
        commandData.response.append(SUCCESS_TOKEN).append(", ").append(DATA_TOKEN).append(" : ").append(alarmHistoryJson);
    }
}

}
