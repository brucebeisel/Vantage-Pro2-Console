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
#include <math.h>
#include <sstream>
#include "WindRoseData.h"
#include "VantageEnums.h"

using namespace std;
namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindSlice::WindSlice(HeadingIndex headingIndex, ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins) :
                                                             headingIndex(headingIndex),
                                                             speedUnits(units),
                                                             speedBinIncrement(speedIncrement),
                                                             numSpeedBins(windSpeedBins),
                                                             sliceSampleCount(0),
                                                             totalSampleCount(0),
                                                             windySampleCount(0),
                                                             maxSpeed(0.0),
                                                             speedSum(0.0),
                                                             speedAverage(0.0) {

    for (int i = 0; i < numSpeedBins; i++)
        speedBinSampleCount.push_back(0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindSlice::~WindSlice() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
WindSlice::applyWindSample(const Measurement<HeadingIndex> & sampleHeadingIndex, Speed sampleSpeed) {
    totalSampleCount++;

    if (sampleSpeed > 0.0)
        windySampleCount++;

    if (sampleSpeed > 0.0 && headingIndex == sampleHeadingIndex.getValue()) {
        if (headingIndex == 1)
            cout << "Speed for heading code NNE: " << sampleSpeed << endl;
        sliceSampleCount++;

        if (sampleSpeed > maxSpeed)
            maxSpeed = sampleSpeed;

        speedSum += sampleSpeed;
        speedAverage = speedSum / static_cast<Speed>(sliceSampleCount);

        double speedBin = ::round(sampleSpeed / speedBinIncrement);
        int speedBinIndex = static_cast<int>(speedBin) - 1;
        speedBinIndex = std::min(speedBinIndex, numSpeedBins - 1);
        speedBinIndex = std::max(speedBinIndex, 0);
        speedBinSampleCount[speedBinIndex]++;

    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
WindSlice::formatJSON() const {
    stringstream ss;

    float percentOfSamples = 0.0;
    if (windySampleCount > 0)
        percentOfSamples =  static_cast<float>(sliceSampleCount) / static_cast<float>(windySampleCount);

    ss << "{ "
       << "\"headingIndex\" : " << headingIndex << ", "
       << "\"maximumSpeed\" : " << maxSpeed << ", "
       << "\"averageSpeed\" : " << speedAverage << ", "
       << "\"percentageOfSamples\" : " << (percentOfSamples * 100.0) << ", "
       << "\"speedBinPercentages\" : [ ";

    bool first = true;
    for (int count : speedBinSampleCount) {
        if (!first) ss << ", "; else first = false;
        if (sliceSampleCount > 0)
            percentOfSamples =  static_cast<float>(count) / static_cast<float>(sliceSampleCount) * 100.0;
        else
            percentOfSamples = 0.0;

        ss << percentOfSamples;
    }

    ss << "] }";

    return ss.str();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindRoseData::WindRoseData(ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins) : windSpeedIncrement(speedIncrement),
                                                                                                          windSpeedBins(windSpeedBins),
                                                                                                          units(ProtocolConstants::WindUnits::MPH),
                                                                                                          totalSamples(0),
                                                                                                          calmSamples(0)  {
    for (int i = 0; i < NUM_WIND_DIRECTION_SLICES; i++) {
        WindSlice slice(i, units, speedIncrement, windSpeedBins);
        windSlices.push_back(slice);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindRoseData::~WindRoseData() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
WindRoseData::applyWindSample(const Measurement<HeadingIndex> & headingIndex, Speed speed) {
    if (!headingIndex.isValid() && speed > 0.0)
        return;

    totalSamples++;

    if (speed == 0.0)
        calmSamples++;

    // TODO convert speed unit from MPH to specified units
    for (auto & slice : windSlices)
        slice.applyWindSample(headingIndex, speed);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
WindRoseData::formatJSON() const {
    stringstream ss;

    ss << "\"windRoseData\" : { "
       << "\"sampleCount\" : " << totalSamples << ", "
       << "\"calmWindSampleCount\" : " << calmSamples << ", "
       << "\"speedBins\" : [ ";

    Speed binSpeed = 0;
    for (int i = 0; i < windSpeedBins; i++) {
        if (i != 0) ss << ", ";
        ss << binSpeed;
        binSpeed += windSpeedIncrement;
    }

    ss << " ], \"speedUnits\" : \"" << windUnitsEnum.valueToString(units) << "\", "
       << "\"windSlices\" : [ " << endl;

    bool first = true;
    for (auto & slice : windSlices) {
        if (!first) ss << ", " << endl; else first = false;
        ss << slice.formatJSON();
    }

    ss << " ] }";

    return ss.str();
}

} /* namespace vws */
