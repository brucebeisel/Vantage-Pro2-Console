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
#ifndef DATE_TIME_FIELDS_H
#define DATE_TIME_FIELDS_H

#include <iosfwd>
#include "WeatherTypes.h"

namespace vws {

//
// Fields for the standard date and time. Note that the values in this structure do
// not apply any offsets. That is, the year is the actual year (e.g. 2021)
//
struct DateTimeFields {
    DateTimeFields();
    int year;
    int month;
    int monthDay;
    int hour;
    int minute;

    DateTime getEpochDateTime() const;

    bool operator==(const DateTimeFields & other) const;
    bool operator<(const DateTimeFields & other) const;
    bool operator>(const DateTimeFields & other) const;

    friend std::ostream & operator<<(std::ostream & os, const DateTimeFields & fields);
};

}

#endif
