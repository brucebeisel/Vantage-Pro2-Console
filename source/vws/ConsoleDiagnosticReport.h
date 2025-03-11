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
    int packetCount;
    int missedPacketCount;
    int syncCount;
    int maxPacketSequence;
    int crcErrorCount;
    VantageLogger & logger;
};

}

#endif
