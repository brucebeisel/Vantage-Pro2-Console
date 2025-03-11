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

#include "ConsoleDiagnosticReport.h"
#include <sstream>
#include "VantageLogger.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ConsoleDiagnosticReport::ConsoleDiagnosticReport() : logger(VantageLogger::getLogger("ConsoleDiagnosticReport")),
                                                     packetCount(0), missedPacketCount(0), syncCount(0), maxPacketSequence(0), crcErrorCount(0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
ConsoleDiagnosticReport::decode(const std::string & report) {
    if (sscanf(report.c_str(), "%d %d %d %d %d", &packetCount,
                                                 &missedPacketCount,
                                                 &syncCount,
                                                 &maxPacketSequence,
                                                 &crcErrorCount) != 5) {

        logger.log(VantageLogger::VANTAGE_WARNING) << "Console diagnostic report did not receive 5 tokens. Response: " << report << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ConsoleDiagnosticReport::formatJSON() const {
    ostringstream oss;

    oss << "{ "
        << "\"consoleDiagnosticReport\" : { "
        << "\"totalPacketsReceived\" : " << packetCount << ", "
        << "\"totalPacketsMissed\" : " << missedPacketCount << ", "
        << "\"resyncCount\" : " << syncCount << ", "
        << "\"packetReceptionHwm\" : " << maxPacketSequence << ", "
        << "\"crcErrorCount\" : " << crcErrorCount
        << " } }";

    return oss.str();
}

}
