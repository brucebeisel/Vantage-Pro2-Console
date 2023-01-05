/* 
 * Copyright (C) 2023 Bruce Beisel
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
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <time.h>

#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {
VantageLogger::Level VantageLogger::currentLevel = VantageLogger::VANTAGE_INFO;
ostream * VantageLogger::loggerStream = &cerr;
ostream VantageLogger::nullStream(0);
map<string, VantageLogger *> VantageLogger::loggers;
int VantageLogger::maxFileSize;
int VantageLogger::maxFiles;
string VantageLogger::logFilePattern;

const static char *LEVEL_STRINGS[] = {"ERROR  ", "WARNING", "INFO   ", "DEBUG1 ", "DEBUG2 ", "DEBUG3 "};

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
    LogIterator logger = loggers.find(name);

    if (logger != loggers.end())
        return *(logger->second);

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
    maxFileSize = MAX_FILE_SIZE_INFINITE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::setLogFilePattern(std::string& pattern, int maxFiles, int maxFileSizeMb) {
    VantageLogger::logFilePattern = pattern;
    VantageLogger::maxFiles = maxFiles;
    VantageLogger::maxFileSize = maxFileSizeMb;
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
    char filename[1024];

    snprintf(filename, sizeof(filename), logFilePattern.c_str(), 0);
    loggerStream = new ofstream(filename, ios::app | ios::ate | ios::out);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageLogger::checkFileSize() {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ostream &
VantageLogger::log(Level level) const {
    char buffer[100];
    if (isLogEnabled(level)) {
        time_t now = time(0);
        struct tm tm;
        Weather::localtime(now, tm);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        *loggerStream << setw(20) << loggerName << ": " << buffer << " --- " << LEVEL_STRINGS[level] << " --- ";
        return *loggerStream;
    }
    else
        return nullStream;
}
} /* End namespace */
