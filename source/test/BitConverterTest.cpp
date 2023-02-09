#include <iostream>
#include "BitConverter.h"
#include "WeatherTypes.h"

using namespace std;
using namespace vws;

int
main(int argc, char *argv[]) {
    vws::byte buffer[10];

    buffer[0] = 0;
    buffer[1] = 0x80;

    int value = BitConverter::toInt16(buffer, 0);
    cout << "Converted 0x00, 0x80 to " << value << endl;

    unsigned int uvalue = BitConverter::toUint16(buffer, 0);
    cout << "Converted 0x00, 0x80 to " << uvalue << endl;

    buffer[0] = 0x80;
    value = BitConverter::toInt8(buffer, 0);
    cout << "Converted 0x80 to " << value << endl;

    uvalue = BitConverter::toUint8(buffer, 0);
    cout << "Converted 0x80 to " << uvalue << endl;

    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0x80;
    value = BitConverter::toInt32(buffer, 0);
    cout << "Converted 0x00, 0x00, 0x00, 0x80 to " << value << endl;

    uvalue = BitConverter::toUint32(buffer, 0);
    cout << "Converted 0x00, 0x00, 0x00, 0x80 to " << uvalue << endl;

}
