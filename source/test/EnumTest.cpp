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

#include "VantageEnums.h"

using namespace std;
using namespace vws;
using namespace vws::ProtocolConstants;

int
main(int argc, char *argv[]) {

    cout << "Running EnumTest" << endl;

    string s = barometerUnitsEnum.valueToString(BarometerUnits::HPA);

    cout << "String for " << BarometerUnits::HPA << " '" << s << "'" << endl;

    BarometerUnits unit = barometerUnitsEnum.stringToValue(s);

    cout << "Value for string '" << s << "' is " << unit << endl;

    unit = static_cast<BarometerUnits>(40);
    s = barometerUnitsEnum.valueToString(unit);

    cout << "String for " << 40 << " '" << s << "'" << endl;

    try {
        unit = barometerUnitsEnum.stringToValue("Bad value");
        cout << "Value for string 'Bad value' is " << unit << endl;
    }
    catch (std::exception & e) {
        cout << "Caught exception: " << e.what() << endl;
    }
}
