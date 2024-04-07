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
#include "DateTimeFields.h"

#include <iomanip>

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DateTimeFields::DateTimeFields() :  year(0), month(1), monthDay(1), hour(0), minute(0) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DateTimeFields::operator==(const DateTimeFields & other) const {
    return year ==  other.year && month == other.month && monthDay == other.monthDay && hour == other.hour && minute == other.minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DateTimeFields::operator<(const DateTimeFields & other) const {
    if (year < other.year)
        return true;
    else if (year > other.year)
        return false;

    if (month < other.month)
        return true;
    else if (month > other.month)
        return false;

    if (monthDay < other.monthDay)
        return true;
    else if (monthDay > other.monthDay)
        return false;

    if (hour < other.hour)
        return true;
    else if (hour > other.hour)
        return false;

    return minute < other.minute;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
DateTimeFields::operator>(const DateTimeFields & other) const {
    return !(*this < other || *this == other);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ostream &
operator<<(ostream & os, const DateTimeFields & fields) {
    os << setw(4) << setfill('0') << fields.year << "-" << setw(2) << fields.month << "-" << setw(2) << fields.monthDay
       << " " << setw(2) << fields.hour << ":" << setw(2) << fields.minute << ":00";

    return os;
}

}
