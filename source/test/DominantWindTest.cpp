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
#include <string.h>
#include <vector>
#include <fstream>

#include "DominantWindDirections.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace vws;
using namespace std;

int
main(int argc, char * argv[]) {
    vector<std::string> headings;
    struct tm tm;

    cout << "Testing bad checkpoint data" << endl;
    VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    DominantWindDirections dw(".", "dominant-wind-checkpoint-bad.dat");
    dw.dumpData();

    cout << "Dominant wind data should be all zeros" << endl;
    cout << "--------------------------------------" << endl;

    DominantWindDirections dominantWinds(".");

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 122;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 30;

    DateTime t = timelocal(&tm);
    Heading h = 20;
    Speed s = 2;

    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")

    dominantWinds.dumpData();
    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_min += 11;
    t = timelocal(&tm);
    h = 40;
    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")

    for (int i = 0; i < 10; i++) {
        tm.tm_min += 1;
        t = timelocal(&tm);
        h = 40;
        s = 1;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
        dominantWinds.dumpData();
    }

    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_hour += 2;
    t = timelocal(&tm);
    h = 40;
    s = 0;
    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")

    dominantWinds.dumpData();
    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_hour += 2;
    t = timelocal(&tm);
    h = 70;
    s = 1;
    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")

    dominantWinds.dumpData();
    dominantWinds.dominantDirectionsForPastHour(headings);

    cout << "----- Testing having 6 dominant wind directions -----" << endl;
    tm.tm_hour += 2;
    tm.tm_min = 0;
    h = 0;
    s = 1;
    for (int i = 0; i < 6; i++) {
        t = timelocal(&tm);
        tm.tm_min += 10;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
        h += 22;
    }
    tm.tm_hour++;
    tm.tm_min = 0;
    t = timelocal(&tm);
    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")

    dominantWinds.dumpData();

    cout << "----- Testing having equal weight wind directions -----" << endl;
    tm.tm_hour += 2;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    h = 0;
    s = 1;
    for (int i = 0; i < 30; i++) {
        t = timelocal(&tm);
        tm.tm_sec++;
        h = 22;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
        tm.tm_sec++;
        h = 44;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
    }
    dominantWinds.dumpData();
    tm.tm_hour++;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    t = timelocal(&tm);
    s = 0;
    dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
    dominantWinds.dumpData();

    //
    // Fix this test so that a sample comes in at the exact time as the end of window time
    //
    cout << "----- Replicate test where multiple dominant winds are selected in one window -----" << endl;
    t = time(0) - 3600;

    for (int i = 0; i < 1000; i++) {
        h = (i % 4) * 22;
        s = 1;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
        dominantWinds.dumpData();
        t += 7;
    }

    t = time(0) - 3600;

    for (int i = 0; i < 1000; i++) {
        h = 32767;
        s = 1;
        dominantWinds.processWindSample(t, h, s); // @suppress("Ambiguous problem")
        dominantWinds.dumpData();
        t += 7;
    }

    //
    // Create a checkpoint file to load
    //
    ofstream of("./checkpoint-test.dat", std::ofstream::out | std::ofstream::trunc);

    time_t now = time(0);
    now -= now % 60;
    now -= 4200;
    float heading = 0.0;
    for (int i = 0; i < 16; i++) {
        heading = (float)i * 22.5;
        if (now < time(0)) {
            of << fixed << setprecision(1) << heading << " " << now << " " << i + 1 << endl;
            cout << fixed << setprecision(1) << heading << " " << now << " " << i + 1 << endl;
        }
        else {
            of << fixed << setprecision(1) << heading << " " << 0 << " " << i + 1 << endl;
            cout << fixed << setprecision(1) << heading << " " << 0 << " " << i + 1 << endl;
        }
        now += 10 * 60;
    }

    of.close();

    DominantWindDirections dwd(".", "checkpoint-test.dat");
    dwd.dumpData();

    ofstream of2("./checkpoint-test2.dat", std::ofstream::out | std::ofstream::trunc);

    now = time(0);
    now -= now % 60;
    now -= 7200;
    heading = 0.0;
    for (int i = 0; i < 16; i++) {
        heading = (float)i * 22.5;
        if (now < time(0) - 3660) {
            of2 << fixed << setprecision(1) << heading << " " << now << " " << i + 1 << endl;
            cout << fixed << setprecision(1) << heading << " " << now << " " << i + 1 << endl;
        }
        else {
            of2 << fixed << setprecision(1) << heading << " " << 0 << " " << i + 1 << endl;
            cout << fixed << setprecision(1) << heading << " " << 0 << " " << i + 1 << endl;
        }
        now += 10 * 60;
    }

    of.close();

    DominantWindDirections dwd2(".", "checkpoint-test2.dat");
    dwd2.dumpData();

    return 0;
}




