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
#include <iostream>
#include "ArchivePacket.h"
#include "VantageDecoder.h"


using namespace vws;
using namespace std;

/*
Field                   Offset    Size      Dash Value     Number   Units
Date Stamp              0           2       Not applicable
Time Stamp              2           2       Not applicable
Outside Temperature     4           2       32767
High Out Temperature    6           2       -32768
Low Out Temperature     8           2       32767
Rainfall                10          2       0
High Rain Rate          12          2       0
Barometer               14          2       0                       (inHg / 1000)
Solar Radiation         16          2       32767
Number of Wind Samples  18          2       0
Inside Temperature      20          2       32767
Inside Humidity         22          1       255
Outside Humidity        23          1       255
Average Wind Speed      24          1       255
High Wind Speed         25          1       0
Dir of Hi Wind Speed    26          1       255                     (0 - 15)
Prevailing Wind Dir     27          1       255
Average UV Index        28          1       255                     (UV Index / 10)
ET                      29          1       0                       (in / 1000)
High Solar Radiation    30          2       0
High UV Index           32          1       0
Forecast Rule           33          1       193
Leaf Temperature        34          2       255             2       (°F + 90)
Leaf Wetnesses          36          2       255             2       (0 – 15)
Soil Temperatures       38          4       255             4       (°F + 90)
Download Record Type    42          1                               0xFF = Rev A, 0x00 = Rev B
Extra Humidities        43          2       255             2
Extra Temperatures      45          3       255             3       (°F + 90)
Soil Moistures          48          4       255             4
*/
vws::byte packetData1[] = {
    (vws::byte)0x93, (vws::byte)0x30, // Date stamp
    (vws::byte)0xb3, (vws::byte)0x06, // Time stamp
    (vws::byte)0xfe, (vws::byte)0x01, // Outside temperature
    (vws::byte)0xff, (vws::byte)0x01, // High outside temperature
    (vws::byte)0xf0, (vws::byte)0x01, // Low outside temperature
    (vws::byte)0x00, (vws::byte)0x00, // Rainfall
    (vws::byte)0x00, (vws::byte)0x00, // High rainfall rate
    (vws::byte)0x7d, (vws::byte)0x75, // Barometer
    (vws::byte)0x52, (vws::byte)0x00, // Solar Radiation
    (vws::byte)0x75, (vws::byte)0x00, // Number of wind samples
    (vws::byte)0xdf, (vws::byte)0x02, // Inside temperature
    (vws::byte)51,                    // Inside humidity
    (vws::byte)76,                    // Outside humidity
    (vws::byte)9,                     // Average wind speed
    (vws::byte)12,                    // High wind speed
    (vws::byte)3,                     // Direction of high wind speed
    (vws::byte)6,                     // Direction of prevailing wind
    (vws::byte)8,                     // Average UV index
    (vws::byte)20,                    // ET
    (vws::byte)0x52, (vws::byte)0x01, // High solar radiation
    (vws::byte)5,                     // High UV index
    (vws::byte)190,                   // Forecast rule
    (vws::byte)120,                   // Leaf temperature 1
    (vws::byte)121,                   // Leaf temperature 2
    (vws::byte)10,                    // Leaf wetness 1
    (vws::byte)11,                    // Leaf wetness 2
    (vws::byte)122,                   // Soil temperature 1
    (vws::byte)123,                   // Soil temperature 2
    (vws::byte)124,                   // Soil temperature 3
    (vws::byte)125,                   // Soil temperature 4
    (vws::byte)0x00,                  // Download record type
    (vws::byte)90,                    // Extra humidity 1
    (vws::byte)95,                    // Extra humidity 2
    (vws::byte)126,                   // Extra temperature 1
    (vws::byte)127,                   // Extra temperature 2
    (vws::byte)128,                   // Extra temperature 3
    (vws::byte)2,                     // Soil Moisture 1
    (vws::byte)4,                     // Soil Moisture 2
    (vws::byte)6,                     // Soil Moisture 3
    (vws::byte)8                      // Soil Moisture 4
};

vws::byte invalidDataPacket[] = {
    (vws::byte)0x93, (vws::byte)0x30, // Date stamp
    (vws::byte)0xb3, (vws::byte)0x06, // Time stamp
    (vws::byte)0xff, (vws::byte)0x7f, // Outside temperature
    (vws::byte)0x00, (vws::byte)0x80, // High outside temperature
    (vws::byte)0xff, (vws::byte)0x7f, // Low outside temperature
    (vws::byte)0x00, (vws::byte)0x00, // Rainfall
    (vws::byte)0x00, (vws::byte)0x00, // High rainfall rate
    (vws::byte)0x00, (vws::byte)0x00, // Barometer
    (vws::byte)0xff, (vws::byte)0x7f, // Solar Radiation
    (vws::byte)0x00, (vws::byte)0x00, // Number of wind samples
    (vws::byte)0xff, (vws::byte)0x7f, // Inside temperature
    (vws::byte)0xff,                  // Inside humidity
    (vws::byte)0xff,                  // Outside humidity
    (vws::byte)0xff,                  // Average wind speed
    (vws::byte)0x00,                  // High wind speed
    (vws::byte)0xff,                  // Direction of high wind speed
    (vws::byte)0xff,                  // Direction of prevailing wind
    (vws::byte)0xff,                  // Average UV index
    (vws::byte)0x00,                  // ET
    (vws::byte)0x00, (vws::byte)0x00, // High solar radiation
    (vws::byte)0x00,                  // High UV index
    (vws::byte)193,                   // Forecast rule
    (vws::byte)0xff,                  // Leaf temperature 1
    (vws::byte)0xff,                  // Leaf temperature 2
    (vws::byte)0xff,                  // Leaf wetness 1
    (vws::byte)0xff,                  // Leaf wetness 2
    (vws::byte)0xff,                  // Soil temperature 1
    (vws::byte)0xff,                  // Soil temperature 2
    (vws::byte)0xff,                  // Soil temperature 3
    (vws::byte)0xff,                  // Soil temperature 4
    (vws::byte)0x00,                  // Download record type
    (vws::byte)0xff,                  // Extra humidity 1
    (vws::byte)0xff,                  // Extra humidity 2
    (vws::byte)0xff,                  // Extra temperature 1
    (vws::byte)0xff,                  // Extra temperature 2
    (vws::byte)0xff,                  // Extra temperature 3
    (vws::byte)0xff,                  // Soil Moisture 1
    (vws::byte)0xff,                  // Soil Moisture 2
    (vws::byte)0xff,                  // Soil Moisture 3
    (vws::byte)0xff                   // Soil Moisture 4
};

int
main(int argc, char * argv[]) {
    VantageDecoder::setRainCollectorSize(.01);
    ArchivePacket packet(packetData1, 0);
    Measurement<Temperature> t1 = packet.getHighOutsideTemperature();

    cout << "All fields valid packet:" << endl << packet.formatJSON(true) << endl << endl;

    packet.updateArchivePacketData(invalidDataPacket, 0);
    cout << "All fields invalid packet:" << endl << packet.formatJSON(true) << endl << endl;

    Measurement<Temperature> temperature = packet.getHighOutsideTemperature();

    if (!temperature.isValid())
        cout << "PASSED: High outside temperature is not valid" << endl;
    else
        cout << "FAILED: High outside temperature is valid when it should not be" << endl;

    Measurement<SolarRadiation> sr = packet.getAverageSolarRadiation();
    if (!sr.isValid())
        cout << "PASSED: Average solar radiation is not valid" << endl;
    else
        cout << "FAILED: Average solar radiation is valid when it should not be. Value = " << sr.getValue() << endl;

    sr = packet.getHighSolarRadiation();
    if (!sr.isValid())
        cout << "PASSED: High solar radiation is not valid" << endl;
    else
        cout << "FAILED: High solar radiation is valid when it should not be" << endl;

    packet.clearArchivePacketData();
    if (packet.isEmptyPacket() && !packet.getDateTimeFields().isDateTimeValid() && packet.getEpochDateTime() == 0 && packet.getWindSampleCount() == 0)
        cout << "PASSED: Cleared packet is empty" << endl;
    else
        cout << "FAILED: Cleared packet has data" << endl;

    if (!packet.updateArchivePacketDataFromFile("./badfile.dat"))
        cout << "PASSED: Non-existent file generated error" << endl;
    else
        cout << "FAILED: Non-existent file did NOT generate error" << endl;

    if (!packet.updateArchivePacketDataFromFile("./toosmallpacket.dat"))
        cout << "PASSED: Too small file generated error" << endl;
    else
        cout << "FAILED: Too small file did NOT generate error" << endl;

    if (packet.updateArchivePacketDataFromFile("./packet-data.dat")) {
        Measurement<Temperature> t2 = packet.getHighOutsideTemperature();
        if (t1 == t2)
            cout << "PASSED: Updated archive packet data from file" << endl;
        else
            cout << "FAILED: Archive packet data is not correct" << endl;
    }
    else
        cout << "FAILED: Update of archive packet data failed" << endl;

    return 0;
}
