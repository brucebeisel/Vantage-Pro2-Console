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

#ifndef CONSOLE_DIAGNOSTIC_REPORT_H_
#define CONSOLE_DIAGNOSTIC_REPORT_H_
#include <string>

namespace vws {
class VantageLogger;

/**
 * Class the holds the data from a console diagnostics report.
 * Note that the console will reset this data at midnight and any time the console time is changed.
 */
class ConsoleDiagnosticReport {
public:
    /**
     * Constructor.
     */
    ConsoleDiagnosticReport();

    /**
     * Decode the string received from the console into the report elements.
     *
     * @param report The diagnostic report from the console
     * @return True if the report is valid
     */
    bool decode(const std::string & report);

    /**
     * Format the diagnostic report into JSON.
     *
     * @return The report in JSON format
     */
    std::string formatJSON() const;

private:
    int             packetCount;         // The number of packets received since the data was reset
    int             missedPacketCount;   // The number of missed packets
    int             resyncCount;         // The number of times the console resynchronized with the ISS
    int             maxPacketSequence;   // The maximum number of packets the console received in a row with an error
    int             crcErrorCount;       // The number of CRC errors detected
    VantageLogger & logger;
};

}

#endif
