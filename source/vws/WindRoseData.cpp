#include <sstream>
#include "WindRoseData.h"
#include "VantageEnums.h"

using namespace std;
namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
WindSlice::WindSlice(int headingIndex, ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins) :
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
WindSlice::applyWindSample(int sampleHeadingIndex, Speed sampleSpeed) {
    totalSampleCount++;

    if (sampleSpeed > 0.0)
        windySampleCount++;

    if (sampleHeadingIndex == headingIndex) {
        sliceSampleCount++;
        if (sampleSpeed > maxSpeed)
            maxSpeed = sampleSpeed;

        speedSum += sampleSpeed;
        speedAverage = speedSum / static_cast<Speed>(sliceSampleCount);

        double speedBin = sampleSpeed / speedBinIncrement;
        int speedBinIndex = static_cast<int>(speedBin);
        speedBinIndex = std::min(speedBinIndex, numSpeedBins - 1);
    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
{
    "headingIndex" : 0,
    "maximumSpeed" : 14,
    "averageSpeed" : 5,
    "percentageOfSamples", 10,
    "speedBinPercentage" : [15.5, 19.5, 29.9, 25.1 ]
}
*/
string
WindSlice::formatJSON() const {
    stringstream ss;

    ss << "{ "
       << "\"headingIndex\" : " << headingIndex << ", "
       << "\"maximumSpeed\" : " << maxSpeed << ", "
       << "\"averageSpeed\" : " << speedAverage << ", "
       << "\"percentageOfSamples\" : " << (sliceSampleCount / windySampleCount * 100.0) << ", "
       << "\"speedBinPercentages\" [ ";

    bool first = true;
    for (int count : speedBinSampleCount) {
        if (!first) ss << ", "; else first = false;
        ss << count;
    }

    ss << "] }" << endl;

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
WindRoseData::applyWindSample(int headingIndex, Speed speed) {
    // TODO convert speed unit from MPH to specified units
    for (auto slice : windSlices)
        slice.applyWindSample(headingIndex, speed);
}

/*
{
    "windRoseData :
    {
        "sampleCount" : 288,
        "calmWindSampleCount" : 110,
        "speedBins" : [ 5, 10, 15, 20 ],
        "speedUnits" : "MPH",
        "windSlices" :
        [
            {
                "headingIndex" : 0,
                "maximumSpeed" : 14,
                "averageSpeed" : 5,
                "percentageOfSamples", 10,
                "speedBinPercentage" : [15.5, 19.5, 29.9, 25.1 ]
            },
            {
                "heading" : "NNE",
                "maximumSpeed" : 10,
                "averageSpeed" : 3,
                "percentageOfSamples", 12,
                "speedBinPercentage" : [15.5, 19.5, 29.9, 25.1 ]
            }
        ]
    }
}
 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
WindRoseData::formatJSON() const {
    stringstream ss;

    ss << "{ \"windRoseData\" : { "
       << "\"sampleCount\" : " << totalSamples << ", "
       << "\"calmWindSampleCount\" : " << calmSamples << ", "
       << "\"speedBins\" : [ ";

    Speed binSpeed = windSpeedIncrement;
    for (int i = 0; i < windSpeedBins; i++) {
        if (i != 0) ss << ", ";
        ss << binSpeed;
        binSpeed += windSpeedIncrement;
    }

    ss << " ], \"speedUnits\" : \"" << windUnitsEnum.valueToString(units) << "\", "
       << "\"windSlices\" : [ ";

    bool first = true;
    for (auto slice : windSlices) {
        if (!first) ss << ", "; else first = false;
        ss << slice.formatJSON();
    }

    ss << " ] } }";

    return ss.str();
}

} /* namespace vws */
