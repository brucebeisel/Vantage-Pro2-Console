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
#ifndef WIND_DIRECTION_SLICES_H
#define WIND_DIRECTION_SLICES_H

#include <vector>

#include "VantageLogger.h"
#include "Weather.h"
#include "WindDirectionSlice.h"

namespace vws {
/**
 * Class that keeps track of the most recent wind direction tendencies.
 * This algorithm does its best to approximate what the Vantage console is displaying.
 * The Vantage console does what seems to be unpredictable behavior in determining when
 * the 10 minute slices of time occur.
 *
 * The console keeps up to six dominant wind directions that have occurred over the past
 * hour. Each dominant direction is based on a 10 minute period of time. So there are
 * six 10 minute periods. If each 10 minute period has a different dominant wind direction,
 * there will be six arrows on the console. Once an hour has passed since the dominant wind
 * direction was selected, the wind direction is removed from the list of dominant wind
 * directions. The current dominant wind direction is not displayed until the full 10 minutes
 * has elapsed. It is not clear how the console determines when the first dominant wind direction
 * calculations start.
 * If the wind is calm, no dominant wind direction is selected. If there is no wind for an hour,
 * all dominant wind directions will be cleared. When the wind starts to blow again, the time of the
 * first dominant wind direction will be reset. It is not clear how the console does this.
 * This class will begin the first 10 minute period when the first wind sample with a speed > 0
 * is detected. The start of the period will be moved back to the beginning of the current minute
 * so that all 10 minute periods start on a even minute boundary.
 */
class DominantWindDirections {
public:
    /**
     * Constructor.
     */
    DominantWindDirections();

    /**
     * Destructor.
     */
    virtual ~DominantWindDirections();

    /**
     * Process a wind sample.
     * 
     * @param time    The time of the wind sample
     * @param heading The direction of the non-zero speed wind
     * @param speed   The speed of the wind sample
     */
    void processWindSample(DateTime time, Heading heading, Speed speed);

    /**
     * Return the past heading tendencies.
     * 
     * @param headings The directions that the wind has been blowing most often in the last hour
     */
    void dominantDirectionsForPastHour(std::vector<int> & headings) const;
    const std::vector<int> & dominantDirectionsForPastHour() const;

    /**
     * Dump the internal data to cout.
     */
    void dumpData() const;

    /**
     * Dump a short version of the internal data to cout.
     */
    void dumpDataShort() const;

private:
    /**
     * Find the dominant wind direction for the current 10 minute window.
     *
     * @return A pointer to the dominant wind direction or NULL if there is not one
     */
    WindDirectionSlice * findDominantWindDirection();

    /**
     * Check if the current 10 minute window has expired.
     *
     * @return True if the current 10 minute window has expired
     */
    bool checkForEndOfWindow(DateTime time);

    /**
     * Start a new 10 minute window using the provided time as the basis for the start time.
     *
     * @param time The time used as the basis for the 10 minute window's start time
     */
    void startWindow(DateTime time);

    /**
     * End the current 10 minute window based on the provided time.
     *
     * @param time The time of the latest wind sample
     */
    void endWindow(DateTime time);

    /**
     * Get the count of the number of dominant directions in the last hour.
     *
     * @return The number of 10 minute dominant durations in the past hour
     */
    int getDominantDirectionsCount() const;

    static constexpr Heading MAX_HEADING = 360.0;

    /**
     * Each wind slice (N, NNE, NE...) will be tracked for direction tendency.
     */
    static const int NUM_SLICES = 16;

    static const std::string SLICE_NAMES[NUM_SLICES];

    /**
     * The number of degrees each wind slice occupies.
     */
    static constexpr Heading DEGREES_PER_SLICE = 22.0;
    static constexpr Heading HALF_SLICE = DEGREES_PER_SLICE / 2.0;

    /**
     * The wind over the past 10 minutes is used to determine the direction tendencies.
     */
    static const int AGE_SPAN = 10 * 60;

    /**
     * Number of directions that are reported in the current weather.
     */
    static const int MAX_DOMINANT_DIRS = 6;

    /**
     * A dominant direction is reported for an hour.
     */
    static const int DOMINANT_DIR_DURATION = 3600;

    VantageLogger      log;
    WindDirectionSlice windSlices[NUM_SLICES];
    time_t             startOf10MinuteTimeWindow;
    time_t             endOf10MinuteTimeWindow;
    std::vector<int>   dominantWindDirectionList;
};
}

#endif /* WIND_DIRECTION_SLICES_H */
