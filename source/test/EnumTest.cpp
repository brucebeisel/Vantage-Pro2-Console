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
