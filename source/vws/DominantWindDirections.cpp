/* 
 * Copyright (C) 2024 Bruce Beisel
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

#include "DominantWindDirections.h"

#include <time.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {

const std::string DominantWindDirections::SLICE_NAMES[NUM_SLICES] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
};

static char dateBuffer[100];

static const char *
dateFormat(time_t t) {
    struct tm tm;
    Weather::localtime(t, tm);
    strftime(dateBuffer, sizeof(dateBuffer), "%H:%M:%S", &tm);
    return dateBuffer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DominantWindDirections::DominantWindDirections(const std::string & cpd) : startOf10MinuteTimeWindow(0),
                                                                          endOf10MinuteTimeWindow(0),
                                                                          checkpointDirectory(cpd),
                                                                          logger(VantageLogger::getLogger("DominantWindDirections")) {
    Heading heading = -HALF_SLICE;
    for (int i = 0; i < NUM_SLICES; i++) {
        windSlices[i].setValues(i, SLICE_NAMES[i], heading, heading + DEGREES_PER_SLICE);
        heading += DEGREES_PER_SLICE;
    }

    restoreCheckpoint();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DominantWindDirections::~DominantWindDirections() {
    saveCheckpoint();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindDirectionSlice *
DominantWindDirections::findDominantWindDirection() {
    WindDirectionSlice * dominantDirection = NULL;

    //
    // This algorithm will favor lower valued directions in the case of a
    // tie.
    //
    for (int i = 0; i < NUM_SLICES; i++) {
        int samples = windSlices[i].getSampleCount();
        if (samples > 0) {
            if (dominantDirection == NULL || samples > dominantDirection->getSampleCount())
                dominantDirection = &windSlices[i];
        }
    }

    return dominantDirection;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::startWindow(DateTime time) {
    for (int i = 0; i < NUM_SLICES; i++)
        windSlices[i].clearSamples();

    //
    // What happens if there is more than a 10 minute gap in the samples?
    // Can we assert that this kind of a gap will not occur?
    // We need to advance the start window time to a time that current sample will fall within.
    // This treats the skipped windows as nothing but calm winds.
    //
    if (startOf10MinuteTimeWindow == 0) {
        startOf10MinuteTimeWindow = time - (time % 60);
    }
    else if (endOf10MinuteTimeWindow + DOMINANT_DIR_DURATION < time) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Resetting end window time due to large gap in samples" << endl;
        startOf10MinuteTimeWindow = time - (time % 60);
    }
    else {
        while (time >= startOf10MinuteTimeWindow + AGE_SPAN)
            startOf10MinuteTimeWindow += AGE_SPAN;
    }

    endOf10MinuteTimeWindow = startOf10MinuteTimeWindow + AGE_SPAN;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Starting new window: " << dateFormat(startOf10MinuteTimeWindow) << "-" << dateFormat(endOf10MinuteTimeWindow) << endl;

    saveCheckpoint();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::endWindow(DateTime time) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Ending window: " << dateFormat(startOf10MinuteTimeWindow) << "-" << dateFormat(endOf10MinuteTimeWindow) << endl;
    WindDirectionSlice * slice = findDominantWindDirection();

    if (slice != NULL) {
        slice->setLast10MinuteDominantTime(endOf10MinuteTimeWindow);
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dominant wind direction is " << slice->getName() << endl;
    }

    //
    // Reset last dominant time to zero if the time is over an hour old
    //
    for (int i = 0; i < NUM_SLICES; i++) {
        windSlices[i].clearSamples();
        if (windSlices[i].getLast10MinuteDominantTime() + DOMINANT_DIR_DURATION < time) {
            windSlices[i].setLast10MinuteDominantTime(0);
        }
    }

    //
    // If there are no dominant wind direction, then reset the start and end of the windows
    //
    if (getDominantDirectionsCount() == 0) {
        startOf10MinuteTimeWindow = 0;
        endOf10MinuteTimeWindow = 0;
    }

    dominantWindDirectionList.clear();
    for (int i = 0; i < NUM_SLICES; i++) {
        if (windSlices[i].getLast10MinuteDominantTime() != 0)
            dominantWindDirectionList.push_back(windSlices[i].getName());
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DominantWindDirections::checkForEndOfWindow(DateTime time) {
    //
    // If the end of the time window is zero, there is no window to end
    //
    if (endOf10MinuteTimeWindow == 0)
        return false;

    if (time >= endOf10MinuteTimeWindow) {
        endWindow(time);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::processWindSample(DateTime time, Heading heading, Speed speed) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Processing wind sample at time " << dateFormat(time) << " Heading = " << heading << " Speed = " << speed << endl;
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Active window: " << dateFormat(startOf10MinuteTimeWindow) << "-" << dateFormat(endOf10MinuteTimeWindow) << endl;
    bool windowEnded = checkForEndOfWindow(time);

    //
    // The heading only has meaning if the speed > 0.0
    //
    if (speed > 0.0) {
        if (endOf10MinuteTimeWindow == 0 || windowEnded)
            startWindow(time);
        //
        // Normalize heading to handle the North slice that spans 348.5 to 11.5 degrees.
        // This will change any heading greater than 348.5 to -11.5 to 0.
        //
        if (heading > MAX_HEADING - HALF_SLICE)
            heading -= static_cast<Heading>(MAX_HEADING);

        //
        // Offer the heading to all of the slices
        //
        for (int i = 0; i < NUM_SLICES; i++) {
            windSlices[i].addSample(heading);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
DominantWindDirections::getDominantDirectionsCount() const {
    int count = 0;
    for (int i = 0; i < NUM_SLICES; i++) {
        if (windSlices[i].getLast10MinuteDominantTime() != 0)
            count++;
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::saveCheckpoint() const {
    string path = checkpointDirectory + "/" + CHECKPOINT_FILE;
    ofstream ofs(path.c_str(), ios::trunc);

    if (!ofs.is_open()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to open Dominant Wind Direction checkpoint file for writing" << endl;
        return;
    }

    for (int i = 0; i < NUM_SLICES; i++) {
        Heading heading = windSlices[i].getCenter();
        DateTime dtime = windSlices[i].getLast10MinuteDominantTime();
        int count = windSlices[i].getSampleCount();
        //
        // Note the time string at the end of the line is for readability only
        //
        char buffer[100];
        struct tm tm;
        Weather::localtime(dtime, tm);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        ofs << heading << " " << dtime << " " << count << " " << buffer << endl;
    }

    ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::restoreCheckpoint() {
    string path = checkpointDirectory + "/" + CHECKPOINT_FILE;
    ifstream ifs(path.c_str());

    if (!ifs.is_open()) {
        logger.log(VantageLogger::VANTAGE_INFO) << "Failed to open Dominant Wind Direction checkpoint file for reading" << endl;
        return;
    }

    Heading heading;
    DateTime dtime;
    int count;
    std::string line;
    DateTime newestTime = 0;
    DateTime now = time(0);

    while (std::getline(ifs, line)) {
        sscanf(line.c_str(), "%f %d %d", &heading, &dtime, &count);

        newestTime = ::max(newestTime, dtime);

        //
        // Only save the dominant time if it's less than an hour old
        //
        if (now - dtime <= DOMINANT_DIR_DURATION) {
            for (int i = 0; i < NUM_SLICES; i++) {
                if (windSlices[i].isInSlice(heading)) {
                    windSlices[i].setLast10MinuteDominantTime(dtime);
                    windSlices[i].setSampleCount(count);
                    dominantWindDirectionList.push_back(windSlices[i].getName());
                }
            }
        }

        //
        // If the latest dominant time is more than 10 minutes old, then
        // clear out the sample counts.
        //
        if (now - newestTime > AGE_SPAN) {
            for (int i = 0; i < NUM_SLICES; i++) {
                windSlices[i].clearSamples();
            }
        }

        //
        // Set the time window based on the newest dominant time, if there is one.
        // Use the newest time, then add 10 minutes to the start and end window times
        // until the end window time is in the future.
        //
        if (newestTime != 0) {
            startOf10MinuteTimeWindow = newestTime;
            endOf10MinuteTimeWindow = newestTime + AGE_SPAN;
            while (endOf10MinuteTimeWindow <= now) {
                startOf10MinuteTimeWindow += AGE_SPAN;
                endOf10MinuteTimeWindow += AGE_SPAN;
            }

        }
    }
    ifs.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::dumpDataShort() const {
    ostringstream oss;
    for (int i = 0; i < NUM_SLICES; i++) {
        char buffer[100];
        DateTime dtime = windSlices[i].getLast10MinuteDominantTime();
        if (dtime > 0) {
            struct tm tm;
            Weather::localtime(dtime, tm);
            strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        }
        else
            strcpy(buffer, "Never");

        oss << "[" << setw(3) << windSlices[i].getName() << " " << windSlices[i].getSampleCount() << "], ";
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << oss.str() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::dumpData() const {
    ostringstream oss;
    for (int i = 0; i < NUM_SLICES; i++) { 
        char buffer[100];
        DateTime dtime = windSlices[i].getLast10MinuteDominantTime();
        if (dtime > 0) {
            struct tm tm;
            Weather::localtime(dtime, tm);
            strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        }
        else
            strcpy(buffer, "Never");

        oss << "Direction: " << setw(3) << windSlices[i].getName() << " (" << setw(5) << windSlices[i].getCenter()
            << ") Count: " << setw(3) << windSlices[i].getSampleCount() << " Last Dominant Time: " << setw(8) << buffer
            << endl;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << endl << oss.str() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
DominantWindDirections::dominantDirectionsForPastHour(vector<std::string> & headings) const {
    //
    // Pull out the wind directions that have been dominant for a 10 minute period over the past hour
    //
    headings.clear();
    const std::vector<std::string> & dwd = dominantDirectionsForPastHour();
    headings.assign(dwd.begin(), dwd.end());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const std::vector<std::string> &
DominantWindDirections::dominantDirectionsForPastHour() const {
    return dominantWindDirectionList;
}
}
