/*
 * DominantWindTest.cpp
 *
 *  Created on: Jan 1, 2023
 *      Author: bruce
 */

#include <string.h>
#include <vector>

#include "../vp2/DominantWindDirections.h"
#include "../vp2/Weather.h"
#include "../vp2/VP2Logger.h"

using namespace vp2;
using namespace std;

int
main(int argc, char * argv[]) {
    vector<int> headings;
    struct tm tm;

    VP2Logger::setLogLevel(VP2Logger::VP2_DEBUG3);
    DominantWindDirections dominantWinds;

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

    dominantWinds.processWindSample(t, h, s);

    dominantWinds.dumpData();
    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_min += 11;
    t = timelocal(&tm);
    h = 40;
    dominantWinds.processWindSample(t, h, s);

    for (int i = 0; i < 10; i++) {
        tm.tm_min += 1;
        t = timelocal(&tm);
        h = 40;
        s = 1;
        dominantWinds.processWindSample(t, h, s);
        dominantWinds.dumpData();
    }

    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_hour += 2;
    t = timelocal(&tm);
    h = 40;
    s = 0;
    dominantWinds.processWindSample(t, h, s);

    dominantWinds.dumpData();
    dominantWinds.dominantDirectionsForPastHour(headings);

    tm.tm_hour += 2;
    t = timelocal(&tm);
    h = 70;
    s = 1;
    dominantWinds.processWindSample(t, h, s);

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
        dominantWinds.processWindSample(t, h, s);
        h += 22;
    }
    tm.tm_hour++;
    tm.tm_min = 0;
    t = timelocal(&tm);
    dominantWinds.processWindSample(t, h, s);

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
        dominantWinds.processWindSample(t, h, s);
        tm.tm_sec++;
        h = 44;
        dominantWinds.processWindSample(t, h, s);
    }
    dominantWinds.dumpData();
    tm.tm_hour++;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    t = timelocal(&tm);
    s = 0;
    dominantWinds.processWindSample(t, h, s);
    dominantWinds.dumpData();

    return 0;
}




