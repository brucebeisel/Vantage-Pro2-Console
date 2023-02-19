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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CurrentWeatherManager::CurrentWeatherManager(const string & archiveDir, CurrentWeatherPublisher & cwPublisher) : archiveDirectory(archiveDir),
                                                                                                                 currentWeatherPublisher(cwPublisher),
                                                                                                                 firstLoop2PacketReceived(false),
                                                                                                                 logger(VantageLogger::getLogger("CurrentWeatherManager")){

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
    struct tm tm;

    Weather::localtime(packetTime, tm);
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/LoopPacketArchive_%02d.dat", archiveDirectory.c_str(), tm.tm_hour);
    ios_base::openmode mode = ofstream::binary;

    //
    // If the hour file exists and it is more than an hour since it has changed, truncate it.
    //
    struct stat fileinfo;
    int sr = stat(filename, &fileinfo);
    time_t now = time(0);
    long fileAge = now - fileinfo.st_mtim.tv_sec;

    if (sr != -1 && fileAge > 3600)
        mode |= std::ofstream::trunc;

    mode |= std::ofstream::app;

    ofstream ofs(filename, mode);
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<byte *>(&packetTime), sizeof(packetTime)).
            write(reinterpret_cast<byte *>(&packetType), sizeof(packetType)).
            write(packetData, length);
        if (!ofs.good()) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Write to LOOP packet archive failed" << endl;
        }
    }
    else
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open LOOP/LOOP2 packet archive file " << filename << endl;

    ofs.close();
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

} /* namespace vws */
