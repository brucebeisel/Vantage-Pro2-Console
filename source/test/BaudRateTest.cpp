#include <iostream>
#include "BaudRate.h"


#ifdef __CYGWIN__
speed_t value19200 = CBR_19200;
speed_t value14400 = CBR_14400;
speed_t value9600 = CBR_9600;
speed_t value4800 = CBR_4800;
speed_t value2400 = CBR_2400;
speed_t value1200 = CBR_1200;
#else
speed_t value19200 = B19200;
speed_t value14400 = B14400;
speed_t value9600 = B9600;
speed_t value4800 = B4800;
speed_t value2400 = B2400;
speed_t value1200 = B1200;
#endif

using namespace vws;
using namespace std;

int
main(int argc, char *argv[]) {

    BaudRate br = BaudRate::BR_19200;

    if (br.getOsValue() == value19200 && br.getVantageValue() == 19200)
        cout << "PASSED: Baud rate values are correct at 19200 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 19200 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(1200);
    if (br.getOsValue() == value1200 && br.getVantageValue() == 1200)
        cout << "PASSED: Baud rate values are correct at 1200 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 1200 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(2400);
    if (br.getOsValue() == value2400 && br.getVantageValue() == 2400)
        cout << "PASSED: Baud rate values are correct at 2400 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 2400 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(4800);
    if (br.getOsValue() == value4800 && br.getVantageValue() == 4800)
        cout << "PASSED: Baud rate values are correct at 4800 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 4800 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(9600);
    if (br.getOsValue() == value9600 && br.getVantageValue() == 9600)
        cout << "PASSED: Baud rate values are correct at 9600 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 9600 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(14400);
    if (br.getOsValue() == value14400 && br.getVantageValue() == 14400)
        cout << "PASSED: Baud rate values are correct at 14400 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 14400 (" << br << ")" << endl;

    br = BaudRate::findBaudRateBySpeed(19200);
    if (br.getOsValue() == value19200 && br.getVantageValue() == 19200)
        cout << "PASSED: Baud rate values are correct at 19200 (" << br << ")" << endl;
    else
        cout << "FAILED: Baud rate values are not correct at 19200 (" << br << ")" << endl;

}
