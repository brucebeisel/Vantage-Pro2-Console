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
#ifndef WIND_ROSE_DATA_H
#define WIND_ROSE_DATA_H

#include <vector>
#include "Measurement.h"
#include "WeatherTypes.h"
#include "VantageProtocolConstants.h"

namespace vws {
class VantageLogger;

/**
 * Class to hold the data for a directional slice of wind for the wind rose data.
 */
class WindSlice {
public:
    /**
     * Constructor.
     *
     * @param headingIndex   The wind direction index that this slice represents: 0 = N, 15 = NNW
     * @param unit           The wind speed units for the wind speed bins
     * @param speedIncrement The amount of speed each speed bin represents
     * @param windSpeedBins  The number of speed bins
     */
    WindSlice(HeadingIndex headingIndex, ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins);

    /**
     * Destructor.
     */
    virtual ~WindSlice();

    /**
     * Apply the wind sample if it the direction falls within this slice.
     *
     * @param headingIndex   The wind direction index that this slice represents: 0 = N, 15 = NNW
     * @param speed        The speed of the wind for this sample
     */
    void applyWindSample(const Measurement<HeadingIndex> & headingIndex, Speed speed);

    /**
     * Format the wind rose data into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON() const;

private:
    HeadingIndex                     headingIndex;
    ProtocolConstants::WindUnits     speedUnits;
    Speed                            speedBinIncrement;
    int                              numSpeedBins;

    int sliceSampleCount; // Samples with the wind blowing in this directional slice
    int totalSampleCount; // The total number of samples offered to this slice
    int windySampleCount; // The total number of samples with speed > 0 offered to this slice


    std::vector<int> speedBinSampleCount; // The sample count for the wind in this slice allocated to speed bins

    Speed speedSum;     // The sum of the speed of the wind in this slice
    Speed speedAverage; // The average speed of the wind in this slice
    Speed maxSpeed;     // The maximum wind speed in this slice

};

/**
 * Class to hold data for a wind rose display.
 * This class introduces the concept of a speed bin. In a wind rose the wind direction is then broken down
 * into a number of speed bins. So if the wind was blowing 30% of the time from the east, then that 30% is broken down
 * into speed bins, that is usually represented as a set of colors
 */
class WindRoseData {
public:
    /**
     * Constructor.
     *
     * @param units          The units of the wind speed
     * @param speedIncrement The amount of speed each speed bin represents in the units specified by the first argument
     * @param windSpeedBins  The number of speed bins
     */
    WindRoseData(ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins);

    /**
     * Destructor.
     */
    virtual ~WindRoseData();

    /**
     * Apply a wind sample to the wind rose data.
     *
     * @param headingIndex The index of the heading of the wind sample: 0 = North, 15 = NNW
     * @param speed        The speed of the wind for this sample
     */
    void applyWindSample(const Measurement<HeadingIndex> & headingIndex, Speed speed);

    /**
     * Format the wind rose data into JSON.
     *
     * @return The JSON string
     */
    std::string formatJSON() const;

private:
    std::vector<WindSlice>       windSlices;         // Objects that hold the data for each direction slice of wind
    int                          totalSamples;       // The total samples that have been applied
    int                          calmSamples;        // The number of samples where the wind was calm
    Speed                        windSpeedIncrement; // The increment of each speed bin
    int                          windSpeedBins;      // The number of wind speed bins
    ProtocolConstants::WindUnits units;              // The units of the speed in the bins. Note: The units of the wind speed applied is in MPH
    VantageLogger &              logger;
};

} /* namespace vws */

#endif /* WIND_ROSE_DATA_H */
