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
#ifndef FORECAST_RULE_H
#define FORECAST_RULE_H

#include <string>

namespace vws {
/**
 * Class the converts the integer representation of a forecast as reported by the Vantage console into a string.
 */
class ForecastRule {
public:
    /**
     * Return the string representation of this rule's forecast.
     * 
     * @param rule The rule
     * @return  The forecast string or an error string if the rule is out of range
     */
    static std::string forecastString(int rule);

private:
    ForecastRule() = delete;
    /**
     * The array of forecast strings.
     */
    static const std::string FORECAST[];
    static const int NUM_RULES;
};
}

#endif
