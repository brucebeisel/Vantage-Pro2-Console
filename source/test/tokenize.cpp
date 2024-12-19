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
#include <regex>
#include <iterator>
#include <iostream>
#include <iomanip>

using namespace std;

int
main(int argc, char *argv[]) {

    string response("Hello World\n\rLine 2\n\rLine 3\n\r");

    regex line_regex("[^\\n]+\n\r");

    auto lines_begin = sregex_iterator(response.begin(), response.end(), line_regex);
    auto lines_end = sregex_iterator();

    cout << "Lines: " << distance(lines_begin, lines_end) << endl;

    for (std::sregex_iterator it = lines_begin; it != lines_end; ++it) {
        smatch match = *it;
        string match_str = match.str();
        cout << match_str << endl;
        const char * buf = match.str().c_str();

        for (int i = 0; i < strlen(buf); i++) {
            cout << hex << (int)buf[i] << " ";
        }
        cout << endl;
    }
}
