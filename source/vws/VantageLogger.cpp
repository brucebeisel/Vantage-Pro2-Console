/* 
 * Copyright (C) 2024 Bruce Beisel
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

#include "VantageLogger.h"

#include <filesystem>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "Weather.h"

using namespace std;

namespace vws {
VantageLogger::Level     VantageLogger::currentLevel = VantageLogger::VANTAGE_INFO;
VantageLogger::LoggerMap VantageLogger::loggers;
ostream *                VantageLogger::loggerStream = &cerr;
ostream                  VantageLogger::nullStream(0);
int                      VantageLogger::maxFileSizeInMb;
int                      VantageLogger::maxFiles;
string                   VantageLogger::logFilePattern;
string                   VantageLogger::currentLogFile("");
bool                     VantageLogger::usingFilePattern = false;

const static char *LEVEL_STRINGS[] = {"ERROR  ", "WARNING", "INFO   ", "DEBUG1 ", "DEBUG2 ", "DEBUG3 "};

static std::mutex mutex;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageLogger::VantageLogger(const string & name) : loggerName(name) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageLogger::~VantageLogger() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageLogger &
VantageLogger::getLogger(const std::string & name) {
    LoggerMap::iterator loggerIterator = loggers.find(name);

    if (loggerIterator != loggers.end()) {
        VantageLogger * logger = loggerIterator->second;
        return *logger;
    }

    VantageLogger * newLogger = new VantageLogger(name);
    loggers.insert(std::pair<string,VantageLogger *>(name, newLogger));

    return *newLogger;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::setLogLevel(Level level) {
    currentLevel = level;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::setLogStream(std::ostream &stream) {
    loggerStream = &stream;
    maxFileSizeInMb = MAX_FILE_SIZE_INFINITE;
    usingFilePattern = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
VantageLogger::buildLogFilename(int sequenceNumber) {
    char filename[1024];
    snprintf(filename, sizeof(filename), logFilePattern.c_str(), sequenceNumber);

    std::string filenameString(filename);

    return filenameString;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::setLogFileParameters(const std::string& prefix, int maximumFiles, int maxFileSizeMb) {
    int digits = int(log10(maximumFiles) + 1);
    logFilePattern = prefix + "_%0" + std::to_string(digits) + "d.log";
    maxFiles = maximumFiles;
    maxFileSizeInMb = maxFileSizeMb;
    usingFilePattern = true;
    currentLogFile = buildLogFilename(0);  // Current log file never changes
    openLogFile();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageLogger::isLogEnabled(Level level) const {
    return level <= currentLevel;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::openLogFile() {
    loggerStream = new ofstream(currentLogFile, ios::app | ios::ate | ofstream::out);
    if (loggerStream->bad()) {
        cerr << "Failed to create stream for file logger. Switching to cerr" << endl;
        loggerStream = &cerr;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::closeLogFile() {
    loggerStream->flush();
    if (loggerStream != &cerr && loggerStream != &cout)
        delete loggerStream;

    loggerStream = NULL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::advanceLogFile() {
    closeLogFile();

    //
    // First remove the last file.
    //
    string deleteFile = buildLogFilename(maxFiles - 1);

    if (filesystem::exists(deleteFile))
        std::remove(deleteFile.c_str());

    //
    // Now move the older files down the list
    //
    for (int i = maxFiles - 1; i > 0; i--) {
        string destinationFile = buildLogFilename(i);
        string sourceFile = buildLogFilename(i - 1);
        if (filesystem::exists(sourceFile))
            filesystem::rename(sourceFile.c_str(), destinationFile.c_str());
    }

    openLogFile();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::checkFileSize() {
    if (!usingFilePattern)
        return;

    auto logFileSize = std::filesystem::file_size(currentLogFile);
    auto logFileSizeInMb = logFileSize / 1024 / 1024;

    if (logFileSizeInMb >= maxFileSizeInMb)
        advanceLogFile();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ostream &
VantageLogger::log(Level level) const {
    std::lock_guard<std::mutex> guard(mutex);
    char timeString[100];
    if (isLogEnabled(level)) {
        checkFileSize();
        time_t now = time(0);
        struct tm tm;
        Weather::localtime(now, tm);
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &tm);
        *loggerStream << setw(20) << loggerName << ": " << timeString << " --- " << LEVEL_STRINGS[level] << " --- ";
        return *loggerStream;
    }
    else {
        return nullStream;
    }
}
} /* End namespace */
