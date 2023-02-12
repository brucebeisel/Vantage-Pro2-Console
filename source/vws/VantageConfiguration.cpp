/*
 * Copyright (C) 2023 Bruce Beisel
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

#include <cmath>

#include "VantageConfiguration.h"
#include "BitConverter.h"
#include "VantageDecoder.h"
#include "VantageEepromConstants.h"
#include "VantageProtocolConstants.h"
#include "VantageEnums.h"

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
                                                                              rainSeasonStartMonth(ProtocolConstants::Month::JANUARY),
                                                                              secondaryWindCupSize(0),
                                                                              retransmitId(0),
                                                                              logFinalTemperature(false),
                                                                              logger(VantageLogger::getLogger("VantageConfiguration")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageConfiguration::~VantageConfiguration() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updatePosition(const PositionData & position) {
    bool success = false;
    char buffer[4];

    int value = std::lround(position.latitude * LAT_LON_SCALE);
    BitConverter::getBytes(value, buffer, 0, 2);

    value = std::lround(position.longitude * LAT_LON_SCALE);
    BitConverter::getBytes(value, buffer, 2, 2);

    if (station.eepromBinaryWrite(VantageEepromConstants::EE_LATITUDE_ADDRESS, buffer, 4)) {
        if (station.updateElevationAndBarometerOffset(position.elevation, 0.0)) {
            success = true;
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrievePosition(PositionData & position) {
    bool success = false;

    byte buffer[6];

    if (station.eepromBinaryRead(VantageEepromConstants::EE_LATITUDE_ADDRESS, 6, buffer)) {
        decodePosition(buffer, 0, position);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::decodePosition(byte * buffer, int offset, PositionData & position) {
    position.latitude = static_cast<double>(BitConverter::toInt16(buffer, offset + 0)) / LAT_LON_SCALE;
    position.longitude = static_cast<double>(BitConverter::toInt16(buffer, offset + 2)) / LAT_LON_SCALE;
    position.elevation = BitConverter::toInt16(buffer, offset + 4);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateTimeSettings(const TimeSettings & timeSettings) {
    byte buffer[6];

    int tzIndex = 18;

    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        if (TIME_ZONES[i].name == timeSettings.timezoneName) {
            tzIndex = TIME_ZONES[i].index;
            break;
        }
    }

    buffer[0] = tzIndex;
    buffer[1] = timeSettings.manualDaylightSavingsTime ? 1 : 0;
    buffer[2] = timeSettings.manualDaylightSavingsTimeOn ? 1 : 0;
    buffer[5] = timeSettings.useGmtOffset ? 1 : 0;

    int value = (timeSettings.gmtOffsetMinutes / 60 * 100) + (timeSettings.gmtOffsetMinutes % 60);
    BitConverter::getBytes(value, buffer, 3, 2);

    return station.eepromBinaryWrite(VantageEepromConstants::EE_TIME_FIELDS_START_ADDRESS, buffer, 6);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveTimeSettings(TimeSettings & timeSettings) {
    bool success = false;
    byte buffer[6];

    if (station.eepromBinaryRead(VantageEepromConstants::EE_TIME_FIELDS_START_ADDRESS, 6, buffer)) {
        decodeTimeSettings(buffer, 0, timeSettings);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::decodeTimeSettings(const byte * buffer, int offset, TimeSettings & timeSettings) {
    const char * tzName = "";
    int tzIndex = BitConverter::toInt8(buffer, offset);

    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        if (TIME_ZONES[i].index == tzIndex) {
            tzName = TIME_ZONES[i].name;
            break;
        }
    }

    timeSettings.timezoneName = tzName;
    timeSettings.manualDaylightSavingsTime = BitConverter::toInt8(buffer, offset + 1) == 1;
    timeSettings.manualDaylightSavingsTimeOn = BitConverter::toInt8(buffer, offset + 2) == 1;

    int value16 = BitConverter::toInt16(buffer, offset + 3);
    if (value16 < 0)
        value16 = (~value16) + 1;

    //timeSettings.gmtOffsetMinutes = ((value16 / 100) * 60) + (value16 % 100);
    timeSettings.gmtOffsetMinutes = value16;
    timeSettings.useGmtOffset = BitConverter::toInt8(buffer, offset + 5) == 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateUnitsSettings(const UnitsSettings & unitsSettings) {
    bool success = false;
    byte buffer;
    buffer = static_cast<int>(unitsSettings.baroUnits) & 0x3;
    buffer |= (static_cast<int>(unitsSettings.temperatureUnits) & 0x3) << 2;
    buffer |= (static_cast<int>(unitsSettings.elevationUnits) & 0x1) << 4;
    buffer |= (static_cast<int>(unitsSettings.rainUnits) & 0x1) << 5;
    buffer |= (static_cast<int>(unitsSettings.windUnits) & 0x3) << 6;

    byte invertedBuffer = ~buffer;
    if (station.eepromBinaryWrite(VantageEepromConstants::EE_UNIT_BITS_ADDRESS, &buffer, 1) &&
        station.eepromBinaryWrite(VantageEepromConstants::EE_UNIT_BITS_ADDRESS + 1, &invertedBuffer, 1)) {
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveUnitsSettings(UnitsSettings & unitsSettings) {
    bool success = false;
    byte buffer;
    if (station.eepromBinaryRead(VantageEepromConstants::EE_UNIT_BITS_ADDRESS, 1, &buffer)) {
        decodeUnitsSettings(&buffer, 0, unitsSettings);
        success = true;
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::decodeUnitsSettings(const byte * buffer, int offset, UnitsSettings & unitsSettings) {
    unitsSettings.baroUnits = static_cast<BarometerUnits>(buffer[offset] & 0x3);
    unitsSettings.temperatureUnits = static_cast<TemperatureUnits>((buffer[offset] >> 2) & 0x3);
    unitsSettings.elevationUnits = static_cast<ElevationUnits>((buffer[offset] >> 4) & 0x1);
    unitsSettings.rainUnits = static_cast<RainUnits>((buffer[offset] >> 5) & 0x1);
    unitsSettings.windUnits = static_cast<WindUnits>((buffer[offset] >> 6) & 0x3);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::updateSetupBits(const SetupBits & setupBits) {
    bool success = false;
    byte buffer;

    buffer = setupBits.is24HourMode ?  0x1 : 0;
    buffer |= setupBits.isCurrentlyAM ? 0x2 : 0;
    buffer |= setupBits.isDayMonthDisplay ? 0x4 : 0;
    buffer |= setupBits.isWindCupLarge ? 0x8 : 0;
    buffer |= setupBits.isNorthLatitude ? 0x40 : 0;
    buffer |= setupBits.isEastLongitude ? 0x80 : 0;
    buffer |= (static_cast<int>(setupBits.rainCollectorSizeType) & 0x3) << 4;
    if (station.eepromBinaryWrite(VantageEepromConstants::EE_SETUP_BITS_ADDRESS, &buffer, 1)) {
        saveRainCollectorSize(setupBits.rainCollectorSizeType);

        //
        // Per the serial protocol documentation, when the setup bits byte is changed, the
        // console must be reinitialized.
        success = station.initializeSetup();
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageConfiguration::retrieveSetupBits(SetupBits & setupBits) {
    bool success = false;
    char buffer;

    if (station.eepromBinaryRead(VantageEepromConstants::EE_SETUP_BITS_ADDRESS, 1, &buffer)) {
        decodeSetupBits(&buffer, 0, setupBits);
        success = true;
        saveRainCollectorSize(setupBits.rainCollectorSizeType);
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::decodeSetupBits(const byte * buffer, int offset, SetupBits & setupBits) {
        setupBits.is24HourMode = (buffer[offset] & 0x1) != 0;
        setupBits.isCurrentlyAM = (buffer[offset] & 0x2) != 0;
        setupBits.isDayMonthDisplay = (buffer[offset] & 0x4) != 0;
        setupBits.isWindCupLarge = (buffer[offset] & 0x8) != 0;
        setupBits.isNorthLatitude = (buffer[offset] & 0x40) != 0;
        setupBits.isEastLongitude = (buffer[offset] & 0x80) != 0;
        setupBits.rainCollectorSizeType = static_cast<ProtocolConstants::RainCupSizeType>((buffer[offset] >> 4) & 0x3);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
string
VantageConfiguration::retrieveAllConfigurationData() {
    byte buffer[EEPROM_CONFIG_SIZE];
    byte logFinalTemperatureValue;

    //
    // Read the entire configuration section of the EEPROM
    //
    if (station.eepromBinaryRead(1U, sizeof(buffer), buffer) && station.eepromBinaryRead(VantageEepromConstants::EE_LOG_AVG_TEMP_ADDRESS, 1, &logFinalTemperatureValue)) {
        SetupBits setupBits;
        UnitsSettings unitsSettings;
        TimeSettings timeSettings;
        decodeSetupBits(buffer, VantageEepromConstants::EE_SETUP_BITS_ADDRESS - 1, setupBits);
        decodeUnitsSettings(buffer, VantageEepromConstants::EE_UNIT_BITS_ADDRESS - 1, unitsSettings);
        decodeTimeSettings(buffer, VantageEepromConstants::EE_TIME_FIELDS_START_ADDRESS - 1, timeSettings);
        secondaryWindCupSize = buffer[VantageEepromConstants::EE_WIND_CUP_SIZE_ADDRESS - 1] & 0x3;
        rainSeasonStartMonth = static_cast<ProtocolConstants::Month>(buffer[VantageEepromConstants::EE_RAIN_SEASON_START_ADDRESS - 1]);
        retransmitId = buffer[VantageEepromConstants::EE_RETRANSMIT_ID_ADDRESS - 1];
        logFinalTemperature = logFinalTemperatureValue;

        ostringstream oss;
        oss << "{ \"configuration\" : { "
            << "  \"time\" : {"
            << " \"gmtoffsetminutes\" : " << timeSettings.gmtOffsetMinutes
            << ", \"manualdst\" : " << timeSettings.manualDaylightSavingsTime
            << ", \"manualDSTon\" : " << timeSettings.manualDaylightSavingsTimeOn
            << ", \"timezonename\" : \"" << timeSettings.timezoneName << "\""
            << ", \"useGMToffset\" : " << timeSettings.useGmtOffset
            << " }, \"units\" : {"
            << " \"barounits\" : \"" << barometerUnitsEnum.valueToString(unitsSettings.baroUnits) << "\""
            << ", \"elevationunits\" : \"" << elevationUnitsEnum.valueToString(unitsSettings.elevationUnits) << "\""
            << ", \"rainunits\" : \"" << rainUnitsEnum.valueToString(unitsSettings.rainUnits) << "\""
            << ", \"temperatureunits\" : \"" << temperatureUnitsEnum.valueToString(unitsSettings.temperatureUnits) << "\""
            << ", \"windunits\" : \"" << windUnitsEnum.valueToString(unitsSettings.windUnits) << "\""
            << " }, \"setupBits\" : {"
            << " \"24hourmode\" : " << setupBits.is24HourMode
            << ", \"currentlyAM\" : " << setupBits.isCurrentlyAM
            << ", \"daymonthdisplay\" : " << setupBits.isDayMonthDisplay
            << ", \"eastlongitude\" : " << setupBits.isEastLongitude
            << ", \"northlatitude\" : " << setupBits.isNorthLatitude
            << ", \"windcuplarge\" : " << setupBits.isWindCupLarge
            << ", \"raincollectorsize\" : " << static_cast<int>(setupBits.rainCollectorSizeType)
            << " }, \"misc\" : {"
            << " \"2ndwindcupsize\" : " << secondaryWindCupSize
            << ", \"rainseasonstartmonth\" : " << static_cast<int>(rainSeasonStartMonth)
            << ", \"retransmitid\" : " << retransmitId
            << ", \"logfinaltemperature\" : " << logFinalTemperature
            << "} } }";
        return oss.str();
    }
    else
        return "";
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::saveRainCollectorSize(RainCupSizeType rainCupType) {
    Rainfall rainCollectorSize = 0.01;

    switch (rainCupType) {
        case RainCupSizeType::POINT_01_INCH:
            rainCollectorSize = ProtocolConstants::POINT_01_INCH_SIZE;
            break;
        case ProtocolConstants::RainCupSizeType::POINT_2_MM:
            rainCollectorSize = ProtocolConstants::POINT_2_MM_SIZE;
            break;
        case RainCupSizeType::POINT_1_MM:
            rainCollectorSize = ProtocolConstants::POINT_1_MM_SIZE;
            break;
        default:
            logger.log(VantageLogger::VANTAGE_WARNING) << "Rain collector size type not valid. Using .01 inches as default" << endl;
            break;
    }

    VantageDecoder::setRainCollectorSize(rainCollectorSize);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageConfiguration::retrieveTimeZoneOptions(std::vector<std::string> & timezoneList) {
    for (int i = 0; i < NUM_TIME_ZONES; i++) {
        timezoneList.push_back(string(TIME_ZONES[i].name));
    }
}

}
