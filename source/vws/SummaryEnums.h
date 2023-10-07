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
#ifndef SUMMARY_ENUMS_H
#define SUMMARY_ENUMS_H

namespace vws {

enum class SummaryExtremes {
    NO_EXTREME,
    MAXIMUM_ONLY,
    MINIMUM_ONLY,
    MINIMUM_AND_MAXIMUM
};

enum class SummaryExtremeType {
    HIGH,
    LOW
};

/**
 * The period over which summary records are calculated.
 */
enum class SummaryPeriod {
    DAY,
    WEEK,
    MONTH,
    YEAR
};


}

#endif
