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
#ifndef STORM_DATA_H
#define STORM_DATA_H

#include "DateTimeFields.h"

namespace vws {

class StormData {
public:
    enum class StormState {
        STORM_IDLE,
        STORM_ACTIVE,
        STORM_ENDED
    };

    StormData();
    StormData(const DateTimeFields & stormStart, const DateTimeFields & stormEnd, Rainfall stormRain);
    virtual ~StormData();

    void resetStormData();
    bool setStormStart(const DateTimeFields & stormStart, Rainfall stormRain);
    void setStormStart(int year, int month, int day, Rainfall stormRain);

    void setStormEnd(int year, int month, int day);
    bool setStormEnd(const DateTimeFields & stormEnd);

    void setStormRain(Rainfall rainfall);

    bool setStormData(const DateTimeFields & stormStart, const DateTimeFields & stormEnd, Rainfall stormRain);

    bool isStormActive() const;
    bool hasStormEnded() const;

    const DateTimeFields & getStormStart() const;
    const DateTimeFields & getStormEnd() const;
    Rainfall getStormRain() const;

    bool operator<(const StormData & other) const;

private:
    StormState     stormState;
    DateTimeFields stormStart;
    DateTimeFields stormEnd;
    Rainfall       stormRain;
};

}

#endif
