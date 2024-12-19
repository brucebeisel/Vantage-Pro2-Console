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
#include <iostream>
#include "StormData.h"

using namespace std;
using namespace vws;

int
main(int argc, char *argv[]) {
    cout << "StormData Tests" << endl;

    StormData sd1;

    if (!sd1.isStormActive())
        cout << "PASSED: Constructor did not activate storm" << endl;
    else
        cout << "FAILED: Constructor incorrectly activate storm" << endl;

    sd1.setStormStart(2024, 2, 3, 1.0);

    if (sd1.getStormStart().formatDate() == "2024-02-03" && sd1.getStormRain() == 1.0)
        cout << "PASSED: setStormStart(y,m,d,r)" << endl;
    else
        cout << "FAILED: setStormStart(y,m,d,r): " << sd1 << endl;

    if (sd1.isStormActive())
        cout << "PASSED: setStormStart() activated storm" << endl;
    else
        cout << "FAILED: setStormStart did not activate storm" << endl;

    sd1.setStormEnd(2024, 2, 4);

    if (sd1.getStormEnd().formatDate() == "2024-02-04" && sd1.hasStormEnded())
        cout << "PASSED: Setting storm end date ended the storm" << endl;
    else
        cout << "FAILED: Setting storm end date did not end the storm" << endl;


    sd1.resetStormData();

    if (!sd1.getStormStart().isDateTimeValid() && !sd1.getStormEnd().isDateTimeValid() && sd1.getStormRain() == 0.0)
        cout << "PASSED: resetStormData() succeeded" << endl;
    else
        cout << "FAILED: resetStormData() did not succeed" << endl;
}
