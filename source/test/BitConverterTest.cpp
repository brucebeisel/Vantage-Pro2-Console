#include <iostream>
#include <vector>
#include "BitConverter.h"
#include "WeatherTypes.h"

using namespace std;
using namespace vws;

struct Foo {
    int a;
    int b;
};

int
main(int argc, char *argv[]) {
    char x = 0xf7;
    int x2 = (~x & 0x7f) + 1;

    if (x & 0x80)
        x2 = x2 * -1;

    cout << "Value of 0xf7 : " << x2 << endl;

    std::vector<Foo> v;
    Foo f = {1,2};
    v.push_back(f);
    v[0].a = 5;
    cout << "A after: " << v[0].a << endl;

    Foo & o = v[0];
    o.a = 10;
    cout << "A after 2: " << v[0].a << endl;

    vws::byte buffer[10];

    buffer[0] = 0;
    buffer[1] = 0x80;

    int16 value16 = BitConverter::toInt16(buffer, 0);
    cout << "int16: Converted 0x00, 0x80 to " << value16 << endl;

    buffer[0] = 0xc;
    buffer[1] = 0xfe;

    value16 = BitConverter::toInt16(buffer, 0);
    int value2s = ~(value16 & 0xFFFF) + 1;
    cout << "int16: Converted 0x0c, 0xfe to " << value16 << " 2's compliment: " << value2s << endl;

    buffer[0] = 0x10;
    buffer[1] = 0xfd;

    value16 = BitConverter::toInt16(buffer, 0);
    value2s = ~(value16 & 0xFFFF) + 1;
    cout << "int16: Converted 0x10, 0xfd to " << value16 << " 2's compliment: " << value2s << endl;

    uint16 uvalue = BitConverter::toUint16(buffer, 0);
    cout << "uint16: Converted 0x00, 0x80 to " << uvalue << endl;

    buffer[0] = 0xf7;
    int value8 = BitConverter::toInt8(buffer, 0);
    cout << "int8: Converted 0x" << hex << (int)buffer[0] << " to " << dec << (int)value8 << endl;

    buffer[0] = 0;
    value8 = BitConverter::toInt8(buffer, 0);
    cout << "int8: Converted 0x" << hex << (int)buffer[0] << " to " << dec << (int)value8 << endl;

    uint8 uvalue8 = BitConverter::toUint8(buffer, 0);
    cout << "uint8: Converted 0x" << hex << (int)buffer[0] << " to " << dec << (unsigned int)uvalue8 << endl;

    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0x80;
    int32 value32 = BitConverter::toInt32(buffer, 0);
    cout << "int32: Converted 0x00, 0x00, 0x00, 0x80 to " << value32 << endl;

    uint32 uvalue32 = BitConverter::toUint32(buffer, 0);
    cout << "uint32: Converted 0x00, 0x00, 0x00, 0x80 to " << uvalue32 << endl;

    int value = -9;
    BitConverter::getBytes(value, buffer, 0, 1);

    cout << "Bytes: 0x" << hex << (int)buffer[0] << endl;
}
