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
#include <iostream>

#include "WindDirectionSlice.h"
#include "WeatherTypes.h"

using namespace std;
using namespace vws;

static constexpr Heading MAX_HEADING = 360.0;
static const int NUM_SLICES = 16;
static constexpr Heading DEGREES_PER_SLICE = MAX_HEADING / static_cast<Heading>(NUM_SLICES);
static constexpr Heading HALF_SLICE = DEGREES_PER_SLICE / 2.0;
const std::string SLICE_NAMES[NUM_SLICES] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
};

WindDirectionSlice       windSlices[NUM_SLICES];

void
offerValue(Heading value) {
    for (int i = 0; i < NUM_SLICES; i++) {
        windSlices[i].addSample(value);
    }
}
int
main(int argc, char *argv[]) {

    Heading heading = -HALF_SLICE;
    for (int i = 0; i < NUM_SLICES; i++) {
        windSlices[i].setValues(i, SLICE_NAMES[i], heading, heading + DEGREES_PER_SLICE);
        heading += DEGREES_PER_SLICE;
    }

    offerValue(0);
    if (windSlices[0].getSampleCount() != 1)
        cout << "FAILED: Expected sample count of 1, got " << windSlices[0].getSampleCount() << endl;
    else
        cout << "PASSED: Sample count is correct" << endl;

    offerValue(355);
    if (windSlices[0].getSampleCount() != 2)
        cout << "FAILED: Expected sample count of 2, got " << windSlices[0].getSampleCount() << endl;
    else
        cout << "PASSED: Sample count is correct" << endl;

    offerValue(MAX_HEADING - HALF_SLICE);
    if (windSlices[0].getSampleCount() != 2)
        cout << "FAILED: Expected sample count of 2, got " << windSlices[0].getSampleCount() << endl;
    else
        cout << "PASSED: Sample count is correct" << endl;

    if (windSlices[15].getSampleCount() != 1)
        cout << "FAILED: Expected sample count of 1, got " << windSlices[15].getSampleCount() << endl;
    else
        cout << "PASSED: Sample count is correct" << endl;

    offerValue(HALF_SLICE);
    if (windSlices[0].getSampleCount() != 3)
        cout << "FAILED: Expected sample count of 3, got " << windSlices[0].getSampleCount() << endl;
    else
        cout << "PASSED: Sample count is correct" << endl;


}
