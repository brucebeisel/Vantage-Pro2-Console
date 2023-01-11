#include "BitConverter.h"
#include "Eeprom.h"
#include "VantageConfiguration.h"
#include "VantageConstants.h"

namespace vws {

VantageConfiguration::VantageConfiguration() {

}

VantageConfiguration::~VantageConfiguration() {
}

void
VantageConfiguration::decodeData(const byte buffer[]) {
    issLatitude = static_cast<double>(BitConverter::toInt16(buffer, 11)) / 10.0;
    issLongitude = static_cast<double>(BitConverter::toInt16(buffer, 13)) / 10.0;
    consoleElevation = BitConverter::toInt16(buffer, 15);
    timezoneIndex = static_cast<char>(buffer[17]) - '0';
    manualDaylightSavingsTime = BitConverter::toInt8(buffer, 18) == 1;
    manualDaylightSavingsTimeOn = BitConverter::toInt8(buffer, 19) == 1;
    int value16 = BitConverter::toInt16(buffer, 20);
    gmtOffsetMinutes = (value16 / 100) + ((value16 % 100) * 60 / 100);
    useGmtOffset = BitConverter::toInt8(buffer, 22) == 0;
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
}

bool
VantageConfiguration::retrieveConfigurationParameters() {
    byte buffer[EepromManager::EEPROM_NON_GRAPH_DATA_SIZE];

    if (station.eepromBinaryRead(0, EepromManager::EEPROM_NON_GRAPH_DATA_SIZE, buffer)) {
        decodeData(buffer);
        return true;
    }
    else
        return false;
}

}
