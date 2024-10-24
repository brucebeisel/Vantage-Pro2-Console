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
#ifndef WIND_SLICE_H
#define WIND_SLICE_H

#include <vector>
#include <iostream>

#include "WeatherTypes.h"

namespace vws {
/**
 * Class that keeps track of the number of wind samples that have occurred in a given slice of wind direction.
 */
class WindDirectionSlice {
public:
    /**
     * Default constructor used in array declarations.
     */
    WindDirectionSlice();

    /**
     * Constructor.
     * 
     * @param slice The slice number from 1 - max
     * @param name The name of the wind direction (N, NW, etc.)
     * @param low The lowest direction in this slice (exclusive)
     * @param high The highest direction in this slice (inclusive)
     */
    WindDirectionSlice(int slice, const std::string & name, Heading low, Heading high);

    /**
     * Destructor.
     */
    virtual ~WindDirectionSlice();

    /**
     * Set the values that describe the slice.
     * 
     * @param slice The slice number from 1 - max
     * @param name The name of the wind direction (N, NW, etc.)
     * @param low The lowest direction in this slice (exclusive)
     * @param high The highest direction in this slice (inclusive)
     */
    void setValues(int slice, const std::string & name, Heading low, Heading high);

    /**
     * Get the heading at the center of this slice.
     * 
     * @return The center heading
     */
    Heading getCenter() const;

    /**
     * Get the name of the heading.
     *
     * @return The name of the heading
     */
    std::string getName() const;

    /**
     * Check if a heading is in the wind slice.
     * 
     * @param heading The heading to check
     * @return  True if the heading is within this slice
     */
    bool isInSlice(Heading heading) const;

    /**
     * Remove all wind samples time stamps.
     */
    void clearSamples();

    /**
     * Add a sample to this slice if the heading fall within this slice.
     * A sample is nothing more than a sample that the wind was blowing within this slice.
     * 
     * @param heading The heading measured at the specified time
     */
    void addSample(Heading heading);

    /**
     * Get the number of samples currently stored by this slice.
     * 
     * @return The sample size
     */
    int getSampleCount() const;

    /**
     * Set the sample count.
     * This is used to restore a checkpoint.
     *
     * @param count The count from the checkpoint
     */
    void setSampleCount(int count);

    /**
     * Clear the data (sample count and dominant time).
     */
    void clearData();

    /**
     * Set the last time this heading was the 10 minute dominant wind direction.
     *
     * @param The time this heading was the 10 minute dominant wind direction
     */
    void setLast10MinuteDominantTime(time_t time);

    /**
     * Get the time this heading was the 10 minute dominant wind direction.
     *
     * @return The time this heading was the 10 minute dominant wind direction
     */
    time_t getLast10MinuteDominantTime() const;

    /**
     * ostream operator for output.
     */
    friend std::ostream & operator<<(std::ostream &, const WindDirectionSlice &);

private:
    int         slice;
    std::string name;
    Heading     lowHeading;
    Heading     highHeading;
    int         sampleCount;
    time_t      last10MinuteDominantTime;
};
}

#endif /* WIND_SLICE_H */
