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

#include "CurrentWeatherManager.h"

#include <fstream>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

#include "CurrentWeatherPublisher.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeatherManager::CurrentWeatherManager(const string & dataDirectory, CurrentWeatherPublisher & cwPublisher) : archiveDirectory(dataDirectory + LOOP_ARCHIVE_DIR),
                                                                                                                    initialized(false),
                                                                                                                    currentWeatherPublisher(cwPublisher),
                                                                                                                    firstLoop2PacketReceived(false),
                                                                                                                    dominantWindDirections(dataDirectory),
                                                                                                                    logger(VantageLogger::getLogger("CurrentWeatherManager")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeatherManager::~CurrentWeatherManager() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::initialize() {
    if (initialized)
        return;

    createArchiveDirectory();
    cleanupArchive();
    initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeather
CurrentWeatherManager::getCurrentWeather() const {
    std::lock_guard<std::mutex> guard(mutex);

    return currentWeather;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * The archive is written as a ring buffer in 24 hour files. The archive will contain
 * between 23 and 24 hours of data. As each hour starts, the hour file will be truncated.
 * Each file will contain up to 1800 records (one every two seconds). The format for each
 * record is:
 *    <time: 4 bytes><packet type: 4 bytes><packet data: 99 bytes>
 * for a total of 107 bytes per record. Each file should contain no more than 192K bytes.
 * The file can be queried to create "CurrentWeather" records from the past. This data
 * can be used to create graphs with very fine grained time axes.
 */
void
CurrentWeatherManager::writeLoopArchive(DateTime packetTime, int packetType, const byte * packetData, size_t length) {
    string filename = archiveFilenameByTime(packetTime);

    //
    // If the hour file exists and it is more than an hour since it has changed, truncate it.
    //
    ios_base::openmode mode = ios::binary ;
    struct stat fileinfo;
    int sr = stat(filename.c_str(), &fileinfo);
    if (sr == 0) {
        time_t now = time(0);
        long fileAge = now - fileinfo.st_mtim.tv_sec;

        if (fileAge > Weather::SECONDS_PER_HOUR) {
            mode |= ios::trunc;
        }
        else {
            mode |= ios::app;
        }
    }
    else
        mode |= ios::app;

    ofstream ofs(filename, mode);
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<byte *>(&packetTime), sizeof(packetTime)).
            write(reinterpret_cast<byte *>(&packetType), sizeof(packetType)).
            write(packetData, length);
        if (!ofs.good()) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Write to LOOP packet archive failed" << endl;
        }
        ofs.close();
    }
    else
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open LOOP/LOOP2 packet archive file " << filename << endl;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CurrentWeatherManager::processLoopPacket(const LoopPacket & packet) {
    std::lock_guard<std::mutex> guard(mutex);
    DateTime packetTime = time(0);
    currentWeather.setLoopData(packet);
    writeLoopArchive(packetTime, packet.getPacketType(), packet.getPacketData(), LoopPacket::LOOP_PACKET_SIZE);
    //
    // Build a list of past wind directions. This is to mimic what is shown on the
    // console
    //
    if (packet.getWindSpeed().isValid()) {
        dominantWindDirections.processWindSample(packetTime, packet.getWindDirection().getValue(), packet.getWindSpeed().getValue());
        currentWeather.setDominantWindDirectionData(dominantWindDirections.dominantDirectionsForPastHour());
    }

    if (firstLoop2PacketReceived)
        currentWeatherPublisher.publishCurrentWeather(currentWeather);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CurrentWeatherManager::processLoop2Packet(const Loop2Packet & packet) {
    std::lock_guard<std::mutex> guard(mutex);
    DateTime packetTime = time(0);
    firstLoop2PacketReceived = true;
    currentWeather.setLoop2Data(packet);
    writeLoopArchive(packetTime, packet.getPacketType(), packet.getPacketData(), Loop2Packet::LOOP2_PACKET_SIZE);
    //
    // Build a list of past wind directions. This is to mimic what is shown on the
    // console
    //
    if (packet.getWindSpeed().isValid()) {
        dominantWindDirections.processWindSample(packetTime, packet.getWindDirection().getValue(), packet.getWindSpeed().getValue());
        currentWeather.setDominantWindDirectionData(dominantWindDirections.dominantDirectionsForPastHour());
    }
    currentWeatherPublisher.publishCurrentWeather(currentWeather);
    dominantWindDirections.dumpData();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeatherManager::archiveFilenameByTime(DateTime time) {
    struct tm tm;
    Weather::localtime(time, tm);
    return archiveFilenameByHour(tm.tm_hour);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeatherManager::archiveFilenameByHour(int hour) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/LoopPacketArchive_%02d.dat", archiveDirectory.c_str(), hour);
    return std::string(filename);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::readArchiveFile(std::ifstream & ifs, vector<CurrentWeather> & list) {
    DateTime packetTime;
    int packetType;
    byte buffer[LoopPacket::LOOP_PACKET_SIZE];
    CurrentWeather cw;
    bool loopPacketProcessed = false;

    while (ifs.good()) {
        ifs.read(reinterpret_cast<byte *>(&packetTime), sizeof(packetTime)).
            read(reinterpret_cast<byte *>(&packetType), sizeof(packetType)).
            read(buffer, sizeof(buffer));

        if (ifs.eof())
            return;

        if (packetType == LoopPacket::LOOP_PACKET_TYPE) {
            LoopPacket loopPacket;
            if (loopPacket.decodeLoopPacket(buffer)) {
                cw.setLoopData(loopPacket);
                cw.setPacketTime(packetTime);
                loopPacketProcessed = true;
            }
        }
        else if (packetType == Loop2Packet::LOOP2_PACKET_TYPE) {
            Loop2Packet loop2Packet;
            if (loop2Packet.decodeLoop2Packet(buffer)) {
                cw.setLoop2Data(loop2Packet);
                cw.setPacketTime(packetTime);
                //
                // Ignore the LOOP2 packet if it is the first in the file or there was an error processing the LOOP packet.
                // If the first packet in the the file is a LOOP2 packet, then one LOOP/LOOP2 packet pair will be discarded
                // as the last packet in the previous file should have been a LOOP packet. Given the circular buffer technique
                // used for the Current Weather Archive, loosing a single packet is not a significant loss.
                //
                if (loopPacketProcessed) {
                    list.push_back(cw);
                    loopPacketProcessed = false;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::queryCurrentWeatherArchive(int hours, std::vector<CurrentWeather> & list) {
    std::lock_guard<std::mutex> guard(mutex);
    list.clear();

    if (hours >= 24)
        hours = 23;

    DateTime archiveTime = time(0) - (Weather::SECONDS_PER_HOUR * hours);

    ios_base::openmode mode = ios::binary | ios::in;

    for (int i = 0; i <= hours; i++) {
        struct tm tm;
        Weather::localtime(archiveTime, tm);
        string filename = archiveFilenameByHour(tm.tm_hour);
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading loop archive file " << filename << endl;
        ifstream ifs(filename.c_str(), mode);
        if (ifs.is_open()) {
            readArchiveFile(ifs, list);
            ifs.close();
        }
        archiveTime += Weather::SECONDS_PER_HOUR;
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Current weather archive records found: " << list.size() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::createArchiveDirectory() {
    if (!std::filesystem::exists(archiveDirectory)) {
        logger.log(VantageLogger::VANTAGE_INFO) <<  "Creating loop archive directory: " << archiveDirectory << endl;
        if (!std::filesystem::create_directory(archiveDirectory))
            logger.log(VantageLogger::VANTAGE_INFO) <<  "Failed to create loop archive directory: " << archiveDirectory << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::cleanupArchive() {
    //
    // If an archive file is older than 24 hours, the file is obsolete and must be deleted
    //
    DateTime tooOldFileTime = time(0) - Weather::SECONDS_PER_DAY;
    for (int i = 0; i < 24; i++) {
        string archiveFilename = archiveFilenameByHour(i);
        struct stat statbuf;
        if (stat(archiveFilename.c_str(), &statbuf) == 0) {
            DateTime fileTime = statbuf.st_mtim.tv_sec;
            logger.log(VantageLogger::VANTAGE_INFO) << "Checking Current Weather Archive file " << archiveFilename << " for deletion with last write time of " << fileTime << endl;
            if (fileTime < tooOldFileTime) {
                if (!std::filesystem::remove(archiveFilename))
                    logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to remove Current Weather Archive file " << archiveFilename << endl;
                else
                    logger.log(VantageLogger::VANTAGE_INFO) << "Deleted old current weather archive file " << archiveFilename << endl;
            }
        }
    }
}

} /* namespace vws */
