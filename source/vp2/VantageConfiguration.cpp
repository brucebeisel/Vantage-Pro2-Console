#include <cmath>
#include "BitConverter.h"
#include "Eeprom.h"
#include "VantageConfiguration.h"
#include "VantageConstants.h"

namespace vws {

static const int EEPROM_LATITUDE_ADDRESS = 0x0D;
static const int EEPROM_TIME_ADDRESS = 0x17;
static const int EEPROM_UNIT_BITS_ADDRESS = 0x29;
static const int EEPROM_SETUP_BITS = 0x2B;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::VantageConfiguration(VantageWeatherStation & station) : station(station), log(VantageLogger::getLogger("VantageConfiguration")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::~VantageConfiguration() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updatePosition(double latitude, double longitude, int elevation) {
    bool success = false;
    char buffer[4];

    int value = std::lround(latitude * 10.0);
    BitConverter::getBytes(value, buffer, 0, 2);

    value = std::lround(longitude * 10.0);
    BitConverter::getBytes(value, buffer, 2, 2);

    if (station.eepromBinaryWrite(EEPROM_LATITUDE_ADDRESS, buffer, 4)) {
        if (station.putElevationAndBarometerOffset(elevation, 0.0)) {
            success = true;
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrievePosition(double & latitude, double & longitude, int & consoleElevation) {
    bool success = false;

    byte positionData[6];

    if (station.eepromBinaryRead(EEPROM_LATITUDE_ADDRESS, 6, positionData)) {
        latitude = static_cast<double>(BitConverter::toInt16(positionData, 0)) / 10.0;  // TBD Does BitConverter::toInt16 handle signed values?
        longitude = static_cast<double>(BitConverter::toInt16(positionData, 2)) / 10.0;
        consoleElevation = BitConverter::toInt16(positionData, 4);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateTimeSettings(const TimeSettings & timeSettings) {
    byte buffer[6];

    buffer[0] = '0' + timeSettings.timezoneIndex;
    buffer[1] = timeSettings.manualDaylightSavingsTime ? 1 : 0;
    buffer[2] = timeSettings.manualDaylightSavingsTimeOn ? 1 : 0;
    buffer[5] = timeSettings.useGmtOffset ? 1 : 0;

    int value = (timeSettings.gmtOffsetMinutes / 60 * 100) + (timeSettings.gmtOffsetMinutes % 60);
    BitConverter::getBytes(value, buffer, 3, 2);

    return station.eepromBinaryWrite(EEPROM_TIME_ADDRESS, buffer, 6);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveTimeSettings(TimeSettings & timeSettings) {
    bool success = false;
    byte buffer[6];

    if (station.eepromBinaryRead(EEPROM_TIME_ADDRESS, 6, buffer)) {
        timeSettings.timezoneIndex = buffer[0] - '0';
        timeSettings.manualDaylightSavingsTime = BitConverter::toInt8(buffer, 1) == 1;
        timeSettings.manualDaylightSavingsTimeOn = BitConverter::toInt8(buffer, 2) == 1;

        int value16 = BitConverter::toInt16(buffer, 3);
        timeSettings.gmtOffsetMinutes = ((value16 / 100) * 60) + (value16 % 100);
        timeSettings.useGmtOffset = BitConverter::toInt8(buffer, 5) == 1;
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateUnitsSettings(VantageConstants::BarometerUnits baroUnits,
                                          VantageConstants::TemperatureUnits temperatureUnits,
                                          VantageConstants::ElevationUnits elevationUnits,
                                          VantageConstants::RainUnits rainUnits,
                                          VantageConstants::WindUnits windUnits) {
    bool success = false;
    byte buffer;
    buffer = baroUnits & 0x3;
    buffer |= (temperatureUnits & 0x3) << 2;
    buffer |= (elevationUnits & 0x1) << 4;
    buffer |= (rainUnits & 0x1) << 5;
    buffer |= (windUnits & 0x3) << 6;

    byte invertedBuffer = buffer ^ 0xFF;
    if (station.eepromBinaryWrite(EEPROM_UNIT_BITS_ADDRESS, &buffer, 1) &&
        station.eepromBinaryWrite(EEPROM_UNIT_BITS_ADDRESS + 1, &invertedBuffer, 1)) {
            success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveUnitsSettings(VantageConstants::BarometerUnits & baroUnits,
                                            VantageConstants::TemperatureUnits & temperatureUnits,
                                            VantageConstants::ElevationUnits & elevationUnits,
                                            VantageConstants::RainUnits & rainUnits,
                                            VantageConstants::WindUnits & windUnits) {
    bool success = false;
    byte buffer;
    if (station.eepromBinaryRead(EEPROM_UNIT_BITS_ADDRESS, 1, &buffer)) {
        baroUnits = static_cast<VantageConstants::BarometerUnits>(buffer & 0x3);
        temperatureUnits = static_cast<VantageConstants::TemperatureUnits>((buffer >> 2) & 0x3);
        elevationUnits = static_cast<VantageConstants::ElevationUnits>((buffer >> 4) & 0x1);
        rainUnits = static_cast<VantageConstants::RainUnits>((buffer >> 5) & 0x1);
        windUnits = static_cast<VantageConstants::WindUnits>((buffer >> 6) & 0x3);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateSetupBits() {
    /*
    int setupBits = BitConverter::toInt8(buffer, 43);
    amPmMode = (setupBits & 0x1) == 0;
    isAM = (setupBits & 0x2) == 1;
    dayMonthFormat = (setupBits & 0x4) == 1;
    windCupLarge = (setupBits & 0x8) == 1;
    isLatitudeNorth = (setupBits & 0x40) == 1;
    isLongitudeEast = (setupBits & 0x80) == 1;
    VantageConstants::RainCupSizeType type = static_cast<VantageConstants::RainCupSizeType>((setupBits & 0x30) >> 4);

    switch (type) {
        case VantageConstants::RainCupSizeType::POINT_01_INCH:
            rainCollectorSize = VantageConstants::POINT_01_INCH_SIZE;
            break;
        case VantageConstants::RainCupSizeType::POINT_2_MM:
            rainCollectorSize = VantageConstants::POINT_2_MM_SIZE;
            break;
        case VantageConstants::RainCupSizeType::POINT_1_MM:
            rainCollectorSize = VantageConstants::POINT_1_MM_SIZE;
            break;
    }
    */
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveSetupBits() {
    return true;
}

}
