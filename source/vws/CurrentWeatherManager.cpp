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

#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include "CurrentWeatherManager.h"
#include "CurrentWeatherPublisher.h"
#include "Weather.h"

using namespace std;

namespace vws {

static string DatasetNames[] = {
    "Outside Temperature",
    "Dew Point",
    "Wind Chill",
    "Heat Index",
    "THSW",
    "Outside Humidity",
    "Inside Temperature",
    "Inside Humidity",
    "Barometer",
    "Wind Speed",
    "2 Minute Avg Wind Speed",
    "10 Minute Avg Wind Speed",
    "Solar Radiation",
    "UV Index"
    "15 Minute Rain",
    "Hour Rain",
    "Storm Rain",
    "Today Rain"
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeatherManager::CurrentWeatherManager(const string & archiveDir, CurrentWeatherPublisher & cwPublisher) : archiveDirectory(archiveDir),
                                                                                                                 currentWeatherPublisher(cwPublisher),
                                                                                                                 firstLoop2PacketReceived(false),
                                                                                                                 dominantWindDirections(archiveDir),
                                                                                                                 logger(VantageLogger::getLogger("CurrentWeatherManager")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeatherManager::~CurrentWeatherManager() {
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
    string filename = archiveFilename(packetTime);

    //
    // If the hour file exists and it is more than an hour since it has changed, truncate it.
    //
    ios_base::openmode mode = ios::binary ;
    struct stat fileinfo;
    int sr = stat(filename.c_str(), &fileinfo);
    if (sr == 0) {
        time_t now = time(0);
        long fileAge = now - fileinfo.st_mtim.tv_sec;

        if (fileAge > 3600) {
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
    DateTime packetTime = time(0);
    currentWeather.setLoopData(packet);
    writeLoopArchive(packetTime, packet.getPacketType(), packet.getPacketData(), LoopPacket::LOOP_PACKET_SIZE);
    //
    // Build a list of past wind directions. This is to mimic what is shown on the
    // console
    //
    dominantWindDirections.processWindSample(packetTime, packet.getWindDirection().getValue(), packet.getWindSpeed().getValue());
    currentWeather.setDominantWindDirectionData(dominantWindDirections.dominantDirectionsForPastHour());

    if (firstLoop2PacketReceived)
        currentWeatherPublisher.publishCurrentWeather(currentWeather);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CurrentWeatherManager::processLoop2Packet(const Loop2Packet & packet) {
    DateTime packetTime = time(0);
    firstLoop2PacketReceived = true;
    currentWeather.setLoop2Data(packet);
    writeLoopArchive(packetTime, packet.getPacketType(), packet.getPacketData(), Loop2Packet::LOOP2_PACKET_SIZE);
    //
    // Build a list of past wind directions. This is to mimic what is shown on the
    // console
    //
    dominantWindDirections.processWindSample(packetTime, packet.getWindDirection().getValue(), packet.getWindSpeed().getValue());
    currentWeather.setDominantWindDirectionData(dominantWindDirections.dominantDirectionsForPastHour());
    currentWeatherPublisher.publishCurrentWeather(currentWeather);
    dominantWindDirections.dumpData();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CurrentWeatherManager::archiveFilename(DateTime time) {
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
    int packetTime;
    int packetType;
    byte buffer[LoopPacket::LOOP_PACKET_SIZE];
    CurrentWeather cw;

    while (ifs.good()) {
        ifs.read(reinterpret_cast<byte *>(&packetTime), sizeof(packetTime)).
            read(reinterpret_cast<byte *>(&packetType), sizeof(packetType)).
            read(buffer, sizeof(buffer));

        if (ifs.eof())
            return;

        if (packetType == LoopPacket::LOOP_PACKET_TYPE) {
            LoopPacket loopPacket;
            loopPacket.decodeLoopPacket(buffer);
            cw.setLoopData(loopPacket);
            cw.setPacketTime(packetTime);
        }
        else if (packetType == Loop2Packet::LOOP2_PACKET_TYPE) {
            Loop2Packet loop2Packet;
            loop2Packet.decodeLoop2Packet(buffer);
            cw.setLoop2Data(loop2Packet);
            cw.setPacketTime(packetTime);
            list.push_back(cw);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
CurrentWeatherManager::queryCurrentWeatherArchive(int hours, std::vector<CurrentWeather> & list) {
    list.clear();

    struct tm tm;
    Weather::localtime(time(0), tm);

    ios_base::openmode mode = ios::binary | ios::in;

    for (int currentHour = tm.tm_hour - hours; currentHour <= tm.tm_hour; currentHour++) {
        string filename = archiveFilenameByHour(currentHour);
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading loop archive file " << filename << endl;
        ifstream ifs(filename.c_str(), mode);
        if (ifs.is_open()) {
            readArchiveFile(ifs, list);
            ifs.close();
        }
        logger.log(VantageLogger::VANTAGE_DEBUG2) << "Current weather archive records found: " << list.size() << endl;
    }
}

} /* namespace vws */
