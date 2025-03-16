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

#include "VantageConfiguration.h"

#include <cmath>

#include "BitConverter.h"
#include "VantageDecoder.h"
#include "VantageEepromConstants.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "VantageProtocolConstants.h"
#include "Loop2Packet.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

struct TimeZoneData {
    int index;
    int offsetMinutes;
    const char * name;
};

static const TimeZoneData TIME_ZONES[] = {
    0, -1200, "(GMT-12:00) Eniwetok, Kwajalein",
    1, -1100, "(GMT-11:00) Midway Island, Samoa",
    2, -1000, "(GMT-10:00) Hawaii",
    3,  -900, "(GMT-09:00) Alaska",
    4,  -800, "(GMT-08:00) Pacific Time, Tijuana",
    5,  -700, "(GMT-07:00) Mountain Time",
    6,  -600, "(GMT-06:00) Central Time",
    7,  -600, "(GMT-06:00) Mexico City",
    8,  -600, "(GMT-06:00) Central America",
    9,  -500, "(GMT-05.00) Bogota, Lima, Quito",
    10, -500, "(GMT-05:00) Eastern Time",
    11, -400, "(GMT-04:00) Atlantic Time",
    12, -400, "(GMT-04.00) Caracas, La Paz, Santiago",
    13, -330, "(GMT-03.30) Newfoundland",
    14, -300, "(GMT-03.00) Brasilia",
    15, -300, "(GMT-03.00) Buenos Aires, Georgetown, Greenland",
    16, -200, "(GMT-02.00) Mid-Atlantic",
    17, -100, "(GMT-01:00) Azores, Cape Verde Island",
    18,    0, "(GMT) Greenwich Mean Time, Dublin, Edinburgh, Lisbon, London",
    19,    0, "(GMT) Monrovia, Casablanca",
    20,  100, "(GMT+01.00) Berlin, Rome, Amsterdam, Bern, Stockholm, Vienna",
    21,  100, "(GMT+01.00) Paris, Madrid, Brussels, Copenhagen, W Central Africa",
    22,  100, "(GMT+01.00) Prague, Belgrade, Bratislava, Budapest, Ljubljana",
    23,  200, "(GMT+02.00) Athens, Helsinki, Istanbul, Minsk, Riga, Tallinn",
    24,  200, "(GMT+02:00) Cairo",
    25,  200, "(GMT+02.00) Eastern Europe, Bucharest",
    26,  200, "(GMT+02:00) Harare, Pretoria",
    27,  200, "(GMT+02.00) Israel, Jerusalem",
    28,  300, "(GMT+03:00) Baghdad, Kuwait, Nairobi, Riyadh",
    29,  300, "(GMT+03.00) Moscow, St. Petersburg, Volgograd",
    30,  330, "(GMT+03:30) Tehran",
    31,  400, "(GMT+04:00) Abu Dhabi, Muscat, Baku, Tblisi, Yerevan, Kazan",
    32,  430, "(GMT+04:30) Kabul",
    33,  500, "(GMT+05:00) Islamabad, Karachi, Ekaterinburg, Tashkent",
    34,  530, "(GMT+05:30) Bombay, Calcutta, Madras, New Delhi, Chennai",
    35,  600, "(GMT+06:00) Almaty, Dhaka, Colombo, Novosibirsk, Astana",
    36,  700, "(GMT+07:00) Bangkok, Jakarta, Hanoi, Krasnoyarsk",
    37,  800, "(GMT+08:00) Beijing, Chongqing, Urumqi, Irkutsk, Ulaan Bataar",
    38,  800, "(GMT+08:00) Hong Kong, Perth, Singapore, Taipei, Kuala Lumpur",
    39,  900, "(GMT+09:00) Tokyo, Osaka, Sapporo, Seoul, Yakutsk",
    40,  930, "(GMT+09:30) Adelaide",
    41,  930, "(GMT+09:30) Darwin",
    42, 1000, "(GMT+10:00) Brisbane, Melbourne, Sydney, Canberra",
    43, 1000, "(GMT+10.00) Hobart, Guam, Port Moresby, Vladivostok",
    44, 1100, "(GMT+11:00) Magadan, Solomon Is, New Caledonia",
    45, 1200, "(GMT+12:00) Fiji, Kamchatka, Marshall Is.",
    46, 1200, "(GMT+12:00) Wellington, Auckland"
};

static constexpr int NUM_TIME_ZONES = sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::VantageConfiguration(VantageWeatherStation & station) : station(station),
                                                                              lastAtmosphericPressure(0.0),
                                                                              logger(VantageLogger::getLogger("VantageConfiguration")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::~VantageConfiguration() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::processLoopPacket(const LoopPacket & packet) {
    // This class does not have interest in any fields in the LOOP packet
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::processLoop2Packet(const Loop2Packet & packet) {

    lastAtmosphericPressure = packet.getBarometricSensorRawReading();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::consoleConnected() {
    //
    // Get the setup bits first so that the size of the rain bucket is saved before any LOOP packets
    // or archive packets are received.
    //
    SetupBits setupBits;
    if (retrieveSetupBits(setupBits))
        saveRainBucketSize(setupBits.rainBucketSizeType);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::consoleDisconnected() {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updatePosition(const PositionData & position, bool initialize) {
    bool success = false;

    //
    // The latitude and longitude can be written straight to the EEPROM. The
    // elevation requires using the "BAR=" command.
    //
    char buffer[4];
    position.encodeLatLon(buffer, 0);

    if (!station.eepromBinaryWrite(EepromConstants::EE_LATITUDE_ADDRESS, buffer, 4)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to update latitude/longitude in EEPROM" << endl;
        return false;
    }

    //
    // Update the latitude/longitude fields in the setup bits to reflect the provided position
    //
    SetupBits setupBits;
    if (retrieveSetupBits(setupBits)) {
        setupBits.isEastLongitude = position.longitude > 0.0;
        setupBits.isNorthLatitude = position.latitude > 0.0;
        updateSetupBits(setupBits, false);
    }

    /*
    if (station.eepromBinaryRead(EepromConstants::EE_BAR_CAL_ADDRESS, 2, buffer)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read BAR_CAL EEPROM value" << endl;
        return false;
    }

    //
    // Calculate the trusted barometric pressure from the latest raw pressure value and the calibration
    // offset currently stored in the EEPROM
    //
    Pressure trustedBarometerReading = static_cast<Pressure>(BitConverter::toInt16(buffer, 0)) / BAROMETER_SCALE;
    if (lastAtmosphericPressure.isValid())
        trustedBarometerReading += lastAtmosphericPressure.getValue();
    else
        trustedBarometerReading = 0.0;

    logger.log(VantageLogger::VANTAGE_INFO) << "Using " << trustedBarometerReading << " as trusted barometer value when updating position" << endl;
    */

    //
    // Set the elevation using 0 as the reliable barometric reading. The console will calculate the barometer offset
    // from the elevation alone.
    //
    if (station.updateBarometerReadingAndElevation(0.0, position.elevation))
        success = true;

    if (initialize)
        station.initializeSetup();

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrievePosition(PositionData & position) {
    bool success = false;

    byte buffer[6];

    //
    // The latitude, longitude and elevation are in sequential bytes
    //
    if (station.eepromBinaryRead(EepromConstants::EE_LATITUDE_ADDRESS, sizeof(buffer), buffer)) {
        position.decode(buffer, 0);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateTimeSettings(const TimeSettings & timeSettings) {
    byte buffer[6];
    timeSettings.encode(buffer, 0);
    return station.eepromBinaryWrite(EepromConstants::EE_TIME_FIELDS_START_ADDRESS, buffer, 6);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveTimeSettings(TimeSettings & timeSettings) {
    bool success = false;
    byte buffer[6];

    if (station.eepromBinaryRead(EepromConstants::EE_TIME_FIELDS_START_ADDRESS, 6, buffer)) {
        timeSettings.decode(buffer, 0);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateUnitsSettings(const UnitsSettings & unitsSettings, bool initialize) {
    bool success = false;
    byte buffer[2];
    unitsSettings.encode(buffer[0]);

    buffer[1] = ~buffer[0];

    if (!station.eepromBinaryWrite(EepromConstants::EE_UNIT_BITS_ADDRESS, buffer, sizeof(buffer)))
        return false;

    //
    // Though the protocol document does not specifically say to initialize the console when
    // the units are changed, it did not work without it
    //
    if (initialize)
        return station.initializeSetup();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveUnitsSettings(UnitsSettings & unitsSettings) {
    bool success = false;
    byte buffer;
    if (station.eepromBinaryRead(EepromConstants::EE_UNIT_BITS_ADDRESS, 1, &buffer)) {
        unitsSettings.decode(buffer);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateSetupBits(const SetupBits & setupBits, bool initialize) {
    bool success = false;
    byte buffer;

    setupBits.encode(buffer);

    if (!station.eepromBinaryWrite(EepromConstants::EE_SETUP_BITS_ADDRESS, &buffer, 1))
        return false;

    saveRainBucketSize(setupBits.rainBucketSizeType);

    //
    // On VP2 and Vantage Vue this EEPROM address must be in sync with the setup bits
    //
    EepromConstants::SecondaryWindCupSize secondaryWindCupSize;
    if (setupBits.isWindCupLarge)
        secondaryWindCupSize = EepromConstants::SecondaryWindCupSize::LARGE;
    else
        secondaryWindCupSize = EepromConstants::SecondaryWindCupSize::SMALL;

    byte value = static_cast<int>(secondaryWindCupSize);

    if (!station.eepromBinaryWrite(EepromConstants::EE_WIND_CUP_SIZE_ADDRESS, &value, 1))
        return false;

    //
    // Per the serial protocol documentation, when the setup bits byte is changed, the
    // console must be reinitialized.
    //
    if (initialize)
        return station.initializeSetup();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveSetupBits(SetupBits & setupBits) {
    bool success = false;
    byte buffer;

    if (station.eepromBinaryRead(EepromConstants::EE_SETUP_BITS_ADDRESS, 1, &buffer)) {
        setupBits.decode(buffer);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateAllConfigurationData(const ConsoleConfigurationData & configData, bool initializeConsole) {
    bool success = false;

    if (!updatePosition(configData.positionData, false))
        return false;

    if (!updateSetupBits(configData.setupBits, false))
        return false;

    if (!updateUnitsSettings(configData.unitsSettings, false))
        return false;

    if (!updateTimeSettings(configData.timeSettings))
        return false;

    byte value = static_cast<byte>(configData.rainSeasonStartMonth);
    if (!station.eepromWriteByte(EepromConstants::EE_RAIN_SEASON_START_ADDRESS, value))
        return false;

    value = static_cast<byte>(configData.retransmitId);
    if (!station.eepromWriteByte(EepromConstants::EE_RETRANSMIT_ID_ADDRESS, value))
        return false;

    value = static_cast<byte>(configData.logFinalTemperature ? 0 : 1);
    if (!station.eepromWriteByte(EepromConstants::EE_LOG_AVG_TEMP_ADDRESS, value))
        return false;

    value = static_cast<byte>(configData.secondaryWindCupSize);
    if (!station.eepromWriteByte(EepromConstants::EE_WIND_CUP_SIZE_ADDRESS, value))
        return false;

    if (initializeConsole && !station.initializeSetup())
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveAllConfigurationData(ConsoleConfigurationData & configData) {
    byte buffer[EEPROM_CONFIG_SIZE];
    byte logFinalTemperatureValue;
    byte secondaryWindCupSizeValue;

    //
    // Read the entire configuration section of the EEPROM
    //
    if (station.eepromBinaryRead(1U, sizeof(buffer), buffer) &&
        station.eepromBinaryRead(EepromConstants::EE_WIND_CUP_SIZE_ADDRESS, 1, &secondaryWindCupSizeValue) &&
        station.eepromBinaryRead(EepromConstants::EE_LOG_AVG_TEMP_ADDRESS, 1, &logFinalTemperatureValue)) {
        configData.positionData.decode(buffer, EepromConstants::EE_LATITUDE_ADDRESS - 1);
        configData.setupBits.decode(buffer[EepromConstants::EE_SETUP_BITS_ADDRESS - 1]);
        configData.unitsSettings.decode(buffer[EepromConstants::EE_UNIT_BITS_ADDRESS - 1]);
        configData.timeSettings.decode(buffer, EepromConstants::EE_TIME_FIELDS_START_ADDRESS - 1);
        configData.rainSeasonStartMonth = static_cast<ProtocolConstants::Month>(buffer[EepromConstants::EE_RAIN_SEASON_START_ADDRESS - 1]);
        configData.retransmitId = buffer[EepromConstants::EE_RETRANSMIT_ID_ADDRESS - 1];
        configData.logFinalTemperature = logFinalTemperatureValue != 0;
        configData.secondaryWindCupSize = static_cast<EepromConstants::SecondaryWindCupSize>(secondaryWindCupSizeValue & 0x3);
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::saveRainBucketSize(RainBucketSizeType rainBucketType) {

    Rainfall rainBucketSize = rainBucketEnumValueToRain(rainBucketType);
    VantageDecoder::setRainCollectorSize(rainBucketSize);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::getTimeZoneOptions(std::vector<std::string> & timezoneList) {
    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        timezoneList.push_back(string(TIME_ZONES[i].name));
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SetupBits::decode(byte buffer) {
        is24HourMode = (buffer & 0x1) != 0;
        isCurrentlyAM = (buffer & 0x2) != 0;
        isDayMonthDisplay = (buffer & 0x4) != 0;
        isWindCupLarge = (buffer & 0x8) != 0;
        isNorthLatitude = (buffer & 0x40) != 0;
        isEastLongitude = (buffer & 0x80) != 0;
        rainBucketSizeType = static_cast<ProtocolConstants::RainBucketSizeType>((buffer >> 4) & 0x3);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SetupBits::encode(byte & buffer) const {
    buffer = is24HourMode ?  0x1 : 0;
    buffer |= isCurrentlyAM ? 0x2 : 0;
    buffer |= isDayMonthDisplay ? 0x4 : 0;
    buffer |= isWindCupLarge ? 0x8 : 0;
    buffer |= isNorthLatitude ? 0x40 : 0;
    buffer |= isEastLongitude ? 0x80 : 0;
    buffer |= (static_cast<int>(rainBucketSizeType) & 0x3) << 4;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
SetupBits::formatJSON() const {
    ostringstream oss;
    oss << boolalpha
        << " \"setupBits\" : {"
        << " \"clock24hourMode\" : "   << is24HourMode
        << ", \"currentlyAm\" : "      << isCurrentlyAM
        << ", \"dayMonthDisplay\" : "  << isDayMonthDisplay
        << ", \"eastLongitude\" : "    << isEastLongitude
        << ", \"northLatitude\" : "    << isNorthLatitude
        << ", \"windCupLarge\" : "     << isWindCupLarge
        << ", \"rainBucketSize\" : \"" << rainBucketSizeTypeEnum.valueToString(rainBucketSizeType) << "\""
        << "}";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
PositionData::encodeLatLon(byte buffer[], int offset) const {
    bool success = false;

    int16 value16 = std::lround(latitude * LAT_LON_SCALE);
    BitConverter::getBytes(value16, buffer, offset + 0, 2);

    value16 = std::lround(longitude * LAT_LON_SCALE);
    BitConverter::getBytes(value16, buffer, offset + 2, 2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
PositionData::decode(byte buffer[], int offset) {
    latitude = static_cast<double>(BitConverter::toInt16(buffer, offset + 0)) / LAT_LON_SCALE;
    longitude = static_cast<double>(BitConverter::toInt16(buffer, offset + 2)) / LAT_LON_SCALE;
    elevation = BitConverter::toInt16(buffer, offset + 4);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
PositionData::formatJSON() const {
    ostringstream oss;

    oss << " \"position\" : {"
        << " \"latitude\" : " << latitude
        << ", \"longitude\" : " << longitude
        << ", \"elevation\" : " << elevation
        << " } ";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
TimeSettings::encode(byte buffer[], int offset) const {
    int tzIndex = 18;

    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        if (TIME_ZONES[i].name == timezoneName) {
            tzIndex = TIME_ZONES[i].index;
            break;
        }
    }

    buffer[offset + 0] = tzIndex;
    buffer[offset + 1] = manualDaylightSavingsTime ? 1 : 0;
    buffer[offset + 2] = manualDaylightSavingsTimeOn ? 1 : 0;

    //
    // Convert from minutes into 1/100 of hours
    //
    int value = gmtOffsetMinutes * 100 / 60;
    BitConverter::getBytes(value, buffer, offset + 3, 2);

    buffer[offset + 5] = useGmtOffset ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
TimeSettings::decode(byte buffer[], int offset) {
    const char * tzName = "UNKNOWN";
    int tzIndex = BitConverter::toUint8(buffer, offset);

    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        if (TIME_ZONES[i].index == tzIndex) {
            tzName = TIME_ZONES[i].name;
            break;
        }
    }

    timezoneName = tzName;
    manualDaylightSavingsTime = BitConverter::toUint8(buffer, offset + 1) == 1;
    manualDaylightSavingsTimeOn = BitConverter::toUint8(buffer, offset + 2) == 1;

    int16 value16 = BitConverter::toInt16(buffer, offset + 3);

    //
    // Convert from 1/100 of hours to minutes
    //
    gmtOffsetMinutes = value16 * 60 / 100;
    useGmtOffset = BitConverter::toUint8(buffer, offset + 5) == 1;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
TimeSettings::formatJSON() const {
    ostringstream oss;

    oss << boolalpha
        << " \"time\" : {"
        << " \"gmtOffsetMinutes\" : " << gmtOffsetMinutes
        << ", \"manualDst\" : " << manualDaylightSavingsTime
        << ", \"manualDstOn\" : " << manualDaylightSavingsTimeOn
        << ", \"timezoneName\" : \"" << timezoneName << "\""
        << ", \"useGmtOffset\" : " << useGmtOffset
        << " } ";

    return oss.str();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
ConsoleConfigurationData::formatJSON() const {
    ostringstream oss;
    oss << std::boolalpha
        << "{ \"configuration\" : { "
        << positionData.formatJSON()  << ","
        << timeSettings.formatJSON()  << ","
        << unitsSettings.formatJSON() << ","
        << setupBits.formatJSON() << ","
        << " \"miscellaneous\" : {"
        << " \"secondaryWindCupSize\" : " << secondaryWindCupSizeEnum.valueToString(secondaryWindCupSize)
        << ", \"rainSeasonStartMonth\" : \"" << monthEnum.valueToString(rainSeasonStartMonth) << "\""
        << ", \"retransmitId\" : " << retransmitId
        << ", \"logFinalTemperature\" : " << logFinalTemperature
        << "} } }";

    return oss.str();
}
}
