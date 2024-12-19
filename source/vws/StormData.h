/*
 * Copyright (C) 2025 Bruce Beisel
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

#include <iostream>
#include "DateTimeFields.h"

namespace vws {

class StormData {
public:
    enum class StormState {
        STORM_IDLE,
        STORM_ACTIVE,
        STORM_ENDED
    };

    /**
     * Constructor.
     */
    StormData();

    /**
     * Constructor with initial values.
     *
     * @param stormStart The date the storm started
     * @param stormEnd   The date the storm ended
     * @param stormRain  The amount of rain for this storm
     */
    StormData(const DateTimeFields & stormStart, const DateTimeFields & stormEnd, Rainfall stormRain);

    /**
     * Destructor.
     */
    virtual ~StormData();

    /**
     * Reset the storm data to the idle state.
     */
    void resetStormData();

    /**
     * Set the start of the storm and rain.
     *
     * @param stormStart The start of the storm
     * @param stormRain  The amount of rain reported when the storm was detected
     * @return True of the storm start date is valid
     */
    bool setStormStart(const DateTimeFields & stormStart, Rainfall stormRain);

    /**
     * Set the start of the storm and rain using individual values.
     *
     * @param year       The year the storm started
     * @param month      The month the storm started
     * @param day        The day of the month the storm started
     * @param stormRain  The amount of rain reported when the storm was detected
     */
    void setStormStart(int year, int month, int day, Rainfall stormRain);

    /**
     * Set the date of the end of the storm using individual values.
     *
     * @param year       The year the storm ended
     * @param month      The month the storm ended
     * @param day        The day of the month the storm ended
     *
     */
    void setStormEnd(int year, int month, int day);

    /**
     * Set the date of the end of the storm.
     *
     * @param stormEnd The end of the storm
     */
    bool setStormEnd(const DateTimeFields & stormEnd);

    /**
     * Set the amount of rain in this storm.
     *
     * @param rainfall The amount of rain
     */
    void setStormRain(Rainfall rainfall);

    /**
     * Set all of the data for this storm.
     *
     * @param stormStart The start of the storm
     * @param stormEnd   The end of the storm
     * @param rainfall   The amount of rain
     */
    bool setStormData(const DateTimeFields & stormStart, const DateTimeFields & stormEnd, Rainfall stormRain);

    /**
     * Return whether this storm is ongoing.
     *
     * @return True if the storm is active
     */
    bool isStormActive() const;

    /**
     * Return whether this storm has eneded.
     *
     * @return True if the storm has ended
     */
    bool hasStormEnded() const;

    /**
     * Get the start of the storm.
     *
     * @return The date the storm started
     */
    const DateTimeFields & getStormStart() const;

    /**
     * Get the end of the storm.
     *
     * @return The date the storm ended
     */
    const DateTimeFields & getStormEnd() const;

    /**
     * Get the amount of rain for this storm.
     *
     * @return The storm rainfall
     */
    Rainfall getStormRain() const;

    /**
     * Less than operator.
     *
     * @param other The other storm data to compare this storm to
     * @return True if "this" is less than other
     */
    bool operator<(const StormData & other) const;

    /**
     * OStream operator.
     *
     * @param os The ostream
     * @param sd The StormData to output
     *
     * @return The same ostream passed in
     */
    friend std::ostream & operator<<(std::ostream & os, const StormData & sd);

private:
    StormState     stormState;
    DateTimeFields stormStart;
    DateTimeFields stormEnd;
    Rainfall       stormRain;
};

}

#endif
