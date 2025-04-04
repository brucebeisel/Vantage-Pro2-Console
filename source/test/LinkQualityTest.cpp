#include <iostream>
#include <math.h>

#include "BaudRate.h"
#include "VantageWeatherStation.h"
#include "VantageStationNetwork.h"
#include "ArchiveManager.h"
#include "SerialPort.h"
#include "VantageLogger.h"
#include "Weather.h"
using namespace vws;
using namespace std;

int
main(int argc, char *argv[]) {
    //VantageLogger::setLogLevel(VantageLogger::VANTAGE_DEBUG3);
    VantageWeatherStation::LinkQuality linkQuality = VantageWeatherStation::calculateLinkQuality(60, 1, 23, 1);
    cout << "Link quality: " << linkQuality << endl;

    linkQuality = VantageWeatherStation::calculateLinkQuality(60, 1, 24, 1);
    cout << "Link quality: " << linkQuality << endl;

    linkQuality = VantageWeatherStation::calculateLinkQuality(300, 1, 117, 1);
    cout << "Link quality: " << linkQuality << endl;

    linkQuality = VantageWeatherStation::calculateLinkQuality(300, 1, 118, 1);
    cout << "Link quality: " << linkQuality << endl;

    linkQuality = VantageWeatherStation::calculateLinkQuality(300, 1, 235, 2);
    cout << "Link quality: " << linkQuality << endl;

    linkQuality = VantageWeatherStation::calculateLinkQuality(300, 1, 234, 2);
    cout << "Link quality: " << linkQuality << endl;

    SerialPort serialPort("/dev/foobar", vws::BaudRate::BR_19200);

    VantageWeatherStation ws(serialPort, 5, .01);

    linkQuality = ws.calculateLinkQuality(1, 234, 2);
    cout << "Link quality: " << linkQuality << endl;

    const char * archiveDir = ".";

    if (argc > 1)
        archiveDir = argv[1];

    ArchiveManager am(archiveDir);
    VantageStationNetwork net(".", ws, am, 1);

    DateTime now = time(0);
    DateTime t = now - (86400 * 30) - 600;
    for (int i = 0; t < now; i++) {
        linkQuality = net.calculateLinkQualityForDay(t);
        cout << Weather::formatDate(t) << ": " << fixed << setprecision(1) << linkQuality << " " << setw(3) << setprecision(0) << round(linkQuality) << endl;
        t += 86400;
    }
}
