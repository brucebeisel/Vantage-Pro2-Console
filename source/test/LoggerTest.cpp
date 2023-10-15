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
