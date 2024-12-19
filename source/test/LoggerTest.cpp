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

#include "VantageLogger.h"

using namespace std;
using namespace vws;

int
main(int argc, char * argv[]) {

    string prefix = "./logfile";

    VantageLogger::setLogFileParameters(prefix, 20, 1);

    VantageLogger & logger = VantageLogger::getLogger("TestLogger");

    cout << "Logging 100 messages" << endl;

    string message = "Logger message";
    for (int i = 0; i < 1000; i++)
        message += " " + to_string(i);

    for (int i = 0; i < 100000; i++)
        logger.log(VantageLogger::VANTAGE_ERROR) << "Logger message" << endl;
}
