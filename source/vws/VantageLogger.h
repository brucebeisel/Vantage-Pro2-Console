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
#ifndef VANTAGE_LOGGER_H
#define VANTAGE_LOGGER_H
#include <map>
#include <string>
#include <iostream>

namespace vws {
/**
 * Home grown logger class that does not need any 3rd party libraries. It does not have the power of other available loggers, but
 * it is small and functional.
 */
class VantageLogger {
public:
    /**
     * The logger levels.
     */
    enum Level {
        VANTAGE_ERROR,
        VANTAGE_WARNING,
        VANTAGE_INFO,
        VANTAGE_DEBUG1,
        VANTAGE_DEBUG2,
        VANTAGE_DEBUG3
    };

    /**
     * Get a logger for the given name.
     * 
     * @param name The logger name
     * @return The logger with the specified name
     */
    static VantageLogger & getLogger(const std::string & name);

    /**
     * Set the overall logging level.
     * 
     * @param level The level of logging to create
     */
    static void setLogLevel(Level);

    /**
     * Set the stream to which all log messages will be written.
     * 
     * @param stream The stream
     */
    static void setLogStream(std::ostream & stream);

    /**
     * Set the pattern to use for multiple log files.
     * 
     * @param prefix The pattern to be used for the files. The pattern must contain a printf formatting string for integers.
     * @param maxFiles The maximum number of files that will be maintained. Files beyond this value will be deleted
     * @param maxFileSizeMb The maximum file size in megabytes. A new file will be created when the current file exceeds this size.
     */
    static void setLogFileParameters(const std::string & prefix, int maxFiles, int maxFileSizeMb);

    /**
     * Destructor.
     */
    virtual ~VantageLogger();

    /**
     * Check if the specified level of logging is enabled.
     * 
     * @param The level
     * @return True if the level is currently enabled
     */
    bool isLogEnabled(Level level) const;

    /**
     * Create a log entry.
     * 
     * @param level The level of the log entry
     * @return The stream so that standard c++ stream mechanisms can be used
     */
    std::ostream & log(Level level) const;

private:
    static const int MAX_FILE_SIZE_INFINITE = -1;
    typedef std::map<std::string, VantageLogger *> LoggerMap;

    /**
     * Private constructor, only getLogger() can create a new logger.
     * 
     * @param name The name of the logger to create
     */
    VantageLogger(const std::string & name);

    /**
     * Build the log file name given the provided sequence number
     *
     * @param sequenceNumber The log file sequence number
     * @return The log file name
     */
    static std::string buildLogFilename(int sequenceNumber);

    /**
     * Create an ofstream using the current log file.
     */
    static void openLogFile();

    /**
     * Close the current ofstream.
     */
    static void closeLogFile();

    /**
     * Move to the next log file.
     */
    static void advanceLogFile();

    /**
     * Check the size of the current log file. If it's too large, close it and open a new one.
     */
    static void checkFileSize();

    /**
     * Collection of loggers, so that only one is create per name.
     */
    static LoggerMap loggers;

    /**
     * The current log level.
     */
    static Level currentLevel;

    /**
     * The stream used when the log level is enabled
     */
    static std::ostream * loggerStream;

    /**
     * The stream used when the log level is not enabled
     */
    static std::ostream nullStream;

    static bool        usingFilePattern;
    static std::string logFilePattern;
    static int         maxFiles;
    static int         maxFileSizeInMb;
    static std::string currentLogFile;

    std::string loggerName;
};

}

#endif /* VANTAGE_LOGGER_H */
