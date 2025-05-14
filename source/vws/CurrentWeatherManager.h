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
#ifndef CURRENT_WEATHER_MANAGER_H
#define CURRENT_WEATHER_MANAGER_H

#include <mutex>
#include <fstream>
#include "CurrentWeather.h"
#include "DominantWindDirections.h"
#include "VantageWeatherStation.h"
#include "LoopPacketListener.h"

namespace vws {
class VantageLogger;
class CurrentWeatherPublisher;

static const std::string LOOP_ARCHIVE_DIR = "/loop/";

/**
 * Class to manage the current weather archive. This includes managing the rotating hour files and performing queries.
 */
class CurrentWeatherManager : public LoopPacketListener {
public:
    /**
     * Constructor.
     *
     * @param dataDirectory The directory into which the loop archive will be written
     * @param cwPublisher   The publisher of current weather data
     */
    CurrentWeatherManager(const std::string & dataDirectory, CurrentWeatherPublisher & cwPublisher);

    /**
     * Destructor.
     */
    virtual ~CurrentWeatherManager();

    /**
     * Initialize the archive which includes creating the archive directory and deleting
     * any obsolete archive files.
     */
    void initialize();

    /**
     * Get the current weather values.
     *
     * @return The current weather values
     */
    CurrentWeather getCurrentWeather() const;

    /**
     * Process a LOOP packet in a callback.
     *
     * @param packet The LOOP packet
     * @return True if the loop packet processing loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & packet);

    /**
     * Process a LOOP2 packet in a callback.
     *
     * @param packet The LOOP2 packet
     * @return True if the loop packet processing loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & packet);

    /**
     * Build current weather records using the LOOP packets stored in the archive.
     * Note that a current weather record will be store when a LOOP2 packet is encountered.
     * Theoretically, the archive will alternate between LOOP and LOOP2
     *
     * @param hours How many hours to go back into the archive. The archive file will not be searched, but
     *              the records will start at the top of the hour. So if it's 2:30 and hours=2, then all
     *              records after 12 PM will be retrieved.
     * @param list  The vector into which the current weather records will be written
     *
     */
    void queryCurrentWeatherArchive(int hours, std::vector<CurrentWeather> & list);

private:
    /**
     * Save the LOOP/LOOP2 packet to the archive file.
     *
     * @param packetTime The time that this packet was received
     * @param packetType Whether this is a LOOP or a LOOP2 packet
     * @param packetData The buffer containing the packet data
     * @param length     The length of the packet data
     */
    void writeLoopArchive(DateTime packetTime, int packetType, const byte * packetData, size_t length);

    /**
     * Read the archive file that is open in the stream.
     *
     * @param ifs        The stream to read
     * @param list       The list of current weather records read from the file
     */
    void readArchiveFile(std::ifstream & ifs, std::vector<CurrentWeather> & list);

    /**
     * Build the name of the archive file based on a time.
     *
     * @param time The time from which to build the archive filename
     *
     * @return The name of the archive file
     */
    std::string archiveFilenameByTime(DateTime time);

    /**
     * Build the name of the archive file based on an hour value.
     *
     * @param hour The hour from which to build the archive filename
     *
     * @return The name of the archive file
     */
    std::string archiveFilenameByHour(int hour);

    /**
     * Create archive directory.
     */
    void createArchiveDirectory();

    /**
     * Cleanup the archive, removing any files that are too old.
     */
    void cleanupArchive();

    mutable std::mutex        mutex;
    std::string               archiveDirectory;
    CurrentWeatherPublisher & currentWeatherPublisher;
    CurrentWeather            currentWeather;
    bool                      firstLoop2PacketReceived;
    DominantWindDirections    dominantWindDirections;   // The past wind direction measurements used to determine the arrows on the wind display
    bool                      initialized;
    VantageLogger &           logger;
};

} /* namespace vws */

#endif
