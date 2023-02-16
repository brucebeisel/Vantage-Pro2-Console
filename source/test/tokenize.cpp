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
