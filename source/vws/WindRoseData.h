#ifndef WIND_ROSE_DATA_H
#define WIND_ROSE_DATA_H

#include <vector>
#include "Measurement.h"
#include "WeatherTypes.h"
#include "VantageProtocolConstants.h"

namespace vws {

class WindSlice {
public:
    WindSlice(HeadingIndex headingIndex, ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins);

    virtual ~WindSlice();

    void applyWindSample(const Measurement<HeadingIndex> & headingCode, Speed speed);

    std::string formatJSON() const;

private:
    HeadingIndex                     headingIndex;
    ProtocolConstants::WindUnits     speedUnits;
    Speed                            speedBinIncrement;
    int                              numSpeedBins;

    int sliceSampleCount; // Samples with the wind blowing in this directional slice
    int totalSampleCount;
    int windySampleCount;


    std::vector<int>             speedBinSampleCount;

    Speed speedSum;
    Speed speedAverage;
    Speed maxSpeed;

};

class WindRoseData {
public:
    static constexpr int NUM_WIND_DIRECTION_SLICES = 16;
    WindRoseData(ProtocolConstants::WindUnits units, Speed speedIncrement, int windSpeedBins);

    virtual ~WindRoseData();

    void applyWindSample(const Measurement<HeadingIndex> & headingIndex, Speed speed);

    std::string formatJSON() const;

private:
    std::vector<WindSlice>       windSlices;
    int                          totalSamples;
    int                          calmSamples;
    Speed                        windSpeedIncrement;
    int                          windSpeedBins;
    ProtocolConstants::WindUnits units;

};

} /* namespace vws */

#endif /* WIND_ROSE_DATA_H */
