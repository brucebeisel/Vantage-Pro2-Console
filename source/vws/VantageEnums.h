#ifndef VANTAGE_ENUMS_H
#define VANTAGE_ENUMS_H

#include <vector>
#include <string>
#include <sstream>
#include <utility>
#include <stdexcept>

#include "VantageEepromConstants.h"
#include "VantageProtocolConstants.h"

namespace vws {

static const char INVALID_ENUM_VALUE[] = "Invalid Enum Value";
template <typename T> using NameValuePair = std::pair<std::string,T>;

/**
 * Base class for converting to/from enum values and strings.
 */
template<typename T, int Count>
class VantageEnum {
public:

    /**
     * Constructor.
     */
    VantageEnum() {}

    /**
     * Destructor.
     */
    virtual ~VantageEnum() {};

    /**
     * Abstract method for getting the structures that map between values and strings.
     * Note that the order of the mappings will determine the previous and next values
     * as returned in the previousValue() and nextValue() calls below.
     *
     * @return The array of mappings
     */
    virtual const NameValuePair<T> * getMappings() const = 0;

    /**
     * Return the list of valid enum values for an enumeration.
     *
     * @param value A list into which the values will be copied
     */
    void enumValues(std::vector<T> & values) {
        const NameValuePair<T> * mappings = getMappings();
        values.clear();
        for (int i = 0; i < Count; i++) {
            values.push_back(mappings[i].second);
        }
    }

    /**
     * Return the list of valid enum strings for an enumeration.
     *
     * @param value A list into which the strings will be copied
     */
    void enumStrings(std::vector<std::string> & strings) {
        const NameValuePair<T> * mappings = getMappings();
        strings.clear();
        for (int i = 0; i < Count; i++) {
            strings.push_back(mappings[i].first);
        }
    }

    /**
     * Convert the given enum value to the string representation.
     *
     * @param value The enum value
     * @return The string representation of the enum value, or "Invalid Enum Value"
     */
    std::string valueToString(T value) const {
        const NameValuePair<T> * mappings = getMappings();
        for (int i = 0; i < Count; i++) {
            if (mappings[i].second == value)
                return mappings[i].first;
        }

        std::ostringstream oss;
        oss << INVALID_ENUM_VALUE << " (" << (int)value << ")";
        return oss.str();
    }

    /**
     * Convert the given string to an enum value.
     *
     * @param valueString The string to convert to an enumerated value
     * @return The enumerated value
     * @throw invalid_argument The provided string did not map to an enumerated value
     */
    T stringToValue(const std::string & valueString) const {
        const NameValuePair<T> * mappings = getMappings();
        for (int i = 0; i < Count; i++) {
            if (mappings[i].first == valueString)
                return mappings[i].second;
        }

        throw std::invalid_argument("Invalid enum value string (" + valueString + ")");
    }

    /**
     * Return the previous value in the enum list or the passed in value if it is already the first value.
     *
     * @param value The value from which to find the previous value
     * @return The previous value in the enumerated list or the passed in value
     */
    T previousValue(T value) {
        const NameValuePair<T> * mappings = getMappings();
        for (int i = 0; i < Count; i++) {
            if (mappings[i].second == value && i > 0)
                return mappings[i - 1].second;
        }

        return value;
    }

    /**
     * Return the next value in the enum list or the passed in value if it is already the last value.
     *
     * @param value The value from which to find the next value
     * @return The next value in the enumerated list or the passed in value
     */
    T nextValue(T value) {
        const NameValuePair<T> * mappings = getMappings();
        for (int i = 0; i < Count; i++) {
            if (mappings[i].second == value && i < Count - 1)
                return mappings[i + 1].second;
        }

        return value;
    }
};

/****************************************
 * ExtremePeriod Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::ExtremePeriod> epMappings[] = {
    { "Daily", ProtocolConstants::ExtremePeriod::DAILY },
    { "Monthly", ProtocolConstants::ExtremePeriod::MONTHLY },
    { "Yearly", ProtocolConstants::ExtremePeriod::YEARLY }
};

class ExtremePeriodEnum : public VantageEnum<ProtocolConstants::ExtremePeriod,3>  {
public:
    ExtremePeriodEnum() {}
    virtual ~ExtremePeriodEnum() {};

    virtual const NameValuePair<ProtocolConstants::ExtremePeriod> * getMappings() const {
        return epMappings;
    }
};

static ExtremePeriodEnum extremePeriodEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::ExtremePeriod value) {
    os << extremePeriodEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * RainUnits Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::RainUnits> ruMappings[] = {
    { "inches", ProtocolConstants::RainUnits::INCHES },
    { "millimeters", ProtocolConstants::RainUnits::MILLIMETERS }
};

class RainUnitsEnum : public VantageEnum<ProtocolConstants::RainUnits,sizeof(ruMappings)/sizeof(ruMappings[0])>  {
public:
    RainUnitsEnum() {};
    virtual ~RainUnitsEnum() {};

    virtual const NameValuePair<ProtocolConstants::RainUnits> * getMappings() const {
        return ruMappings;
    }
};

static RainUnitsEnum rainUnitsEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::RainUnits value) {
    os << rainUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * BarometerUnits Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::BarometerUnits> buMappings[] = {
    { "inHg", ProtocolConstants::BarometerUnits::IN_HG },
    { "mm", ProtocolConstants::BarometerUnits::MILLIMETER },
    { "hPa", ProtocolConstants::BarometerUnits::HPA },
    { "mbar", ProtocolConstants::BarometerUnits::MILLIBAR }
};

class BarometerUnitsEnum : public VantageEnum<ProtocolConstants::BarometerUnits,sizeof(buMappings)/sizeof(buMappings[0])>  {
public:
    BarometerUnitsEnum() {};
    virtual ~BarometerUnitsEnum() {};

    virtual const NameValuePair<ProtocolConstants::BarometerUnits> * getMappings() const {
        return buMappings;
    }
};

static BarometerUnitsEnum barometerUnitsEnum;

static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::BarometerUnits value) {
    os << barometerUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * TemperatureUnits Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::TemperatureUnits> tuMappings[] = {
    { "F", ProtocolConstants::TemperatureUnits::FAHRENHEIT },
    { ".1F", ProtocolConstants::TemperatureUnits::TENTH_FAHRENHEIT },
    { "C", ProtocolConstants::TemperatureUnits::CELSIUS },
    { ".1C", ProtocolConstants::TemperatureUnits::TENTH_CELSIUS }
};

class TemperatureUnitsEnum : public VantageEnum<ProtocolConstants::TemperatureUnits,sizeof(tuMappings)/sizeof(tuMappings[0])>  {
public:
    TemperatureUnitsEnum() {};
    virtual ~TemperatureUnitsEnum() {};

    virtual const NameValuePair<ProtocolConstants::TemperatureUnits> * getMappings() const {
        return tuMappings;
    }
};

static TemperatureUnitsEnum temperatureUnitsEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::TemperatureUnits value) {
    os << temperatureUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * WindUnits Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::WindUnits> wuMappings[] = {
    { "mph", ProtocolConstants::WindUnits::MPH },
    { "mps", ProtocolConstants::WindUnits::MPS },
    { "kph", ProtocolConstants::WindUnits::KPH },
    { "kts", ProtocolConstants::WindUnits::KTS }
};

class WindUnitsEnum : public VantageEnum<ProtocolConstants::WindUnits,sizeof(wuMappings)/sizeof(wuMappings[0])>  {
public:
    WindUnitsEnum() {};
    virtual ~WindUnitsEnum() {};

    virtual const NameValuePair<ProtocolConstants::WindUnits> * getMappings() const {
        return wuMappings;
    }
};

static WindUnitsEnum windUnitsEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::WindUnits value) {
    os << windUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * ElevationUnits Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::ElevationUnits> euMappings[] = {
    { "feet", ProtocolConstants::ElevationUnits::FEET },
    { "meters", ProtocolConstants::ElevationUnits::METERS }
};

class ElevationUnitsEnum : public VantageEnum<ProtocolConstants::ElevationUnits,sizeof(euMappings)/sizeof(euMappings[0])>  {
public:
    ElevationUnitsEnum() {};
    virtual ~ElevationUnitsEnum() {};

    virtual const NameValuePair<ProtocolConstants::ElevationUnits> * getMappings() const {
        return euMappings;
    }
};

static ElevationUnitsEnum elevationUnitsEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::ElevationUnits value) {
    os << elevationUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * BarometerTrend Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::BarometerTrend> btMappings[] = {
    { "Steady", ProtocolConstants::BarometerTrend::STEADY },
    { "Rising Slowly", ProtocolConstants::BarometerTrend::RISING_SLOWLY },
    { "Rising Rapidly", ProtocolConstants::BarometerTrend::RISING_RAPIDLY },
    { "Falling Rapidly", ProtocolConstants::BarometerTrend::FALLING_RAPIDLY },
    { "Falling Slowly", ProtocolConstants::BarometerTrend::FALLING_SLOWLY },
    { "Unknown", ProtocolConstants::BarometerTrend::UNKNOWN }
};

class BarometerTrendEnum : public VantageEnum<ProtocolConstants::BarometerTrend,sizeof(btMappings)/sizeof(btMappings[0])>  {
public:
    BarometerTrendEnum() {};
    virtual ~BarometerTrendEnum() {};

    virtual const NameValuePair<ProtocolConstants::BarometerTrend> * getMappings() const {
        return btMappings;
    }
};

static BarometerTrendEnum barometerTrendEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::BarometerTrend value) {
    os << barometerTrendEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * Forecast Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::Forecast> fcMappings[] = {
    { "Sunny", ProtocolConstants::Forecast::SUNNY },
    { "Partly cloudy", ProtocolConstants::Forecast::PARTLY_CLOUDY },
    { "Mostly cloudy", ProtocolConstants::Forecast::MOSTLY_CLOUDY },
    { "Mostly cloudy with rain within 12 hours", ProtocolConstants::Forecast::MOSTLY_CLOUDY_WITH_RAIN },
    { "Mostly cloudy with snow within 12 hours", ProtocolConstants::Forecast::MOSTLY_CLOUDY_WITH_SNOW },
    { "Mostly cloudy with rain or snow within 12 hours", ProtocolConstants::Forecast::MOSTLY_CLOUDY_WITH_RAIN_OR_SNOW },
    { "Partly cloudy with rain within 12 hours", ProtocolConstants::Forecast::PARTLY_CLOUDY_WITH_RAIN_LATER },
    { "Partly cloudy with snow within 12 hours", ProtocolConstants::Forecast::PARTLY_CLOUDY_WITH_SNOW_LATER },
    { "Partly cloudy with rain or snow within 12 hours", ProtocolConstants::Forecast::PARTLY_CLOUDY_WITH_RAIN_OR_SNOW_LATER }
};

class ForecastEnum : public VantageEnum<ProtocolConstants::Forecast,sizeof(fcMappings)/sizeof(fcMappings[0])>  {
public:
    ForecastEnum() {};
    virtual ~ForecastEnum() {};

    virtual const NameValuePair<ProtocolConstants::Forecast> * getMappings() const {
        return fcMappings;
    }
};

static ForecastEnum forecastEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::Forecast value) {
    os << forecastEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * StationType Enumeration
 ****************************************/
static const NameValuePair<VantageEepromConstants::StationType> sstMappings[] = {
    { "Integrated Sensor Station", VantageEepromConstants::StationType::INTEGRATED_SENSOR_STATION },
    { "Temperature Only", VantageEepromConstants::StationType::TEMPERATURE_ONLY_STATION },
    { "Humidity Only", VantageEepromConstants::StationType::HUMIDITY_ONLY_STATION },
    { "Temperature/Humidity", VantageEepromConstants::StationType::TEMPERATURE_HUMIDITY_STATION },
    { "Anemometer", VantageEepromConstants::StationType::ANEMOMETER_STATION },
    { "Rain", VantageEepromConstants::StationType::RAIN_STATION },
    { "Leaf", VantageEepromConstants::StationType::LEAF_STATION },
    { "Soil", VantageEepromConstants::StationType::SOIL_STATION },
    { "Soil/Leaf", VantageEepromConstants::StationType::SOIL_LEAF_STATION },
    { "No Station", VantageEepromConstants::StationType::NO_STATION },
    { "Unknown Station", VantageEepromConstants::StationType::UNKNOWN_STATION }
};

class StationTypeEnum : public VantageEnum<VantageEepromConstants::StationType,sizeof(sstMappings)/sizeof(sstMappings[0])>  {
public:
    StationTypeEnum() {};
    virtual ~StationTypeEnum() {};

    virtual const NameValuePair<VantageEepromConstants::StationType> * getMappings() const {
        return sstMappings;
    }
};

static StationTypeEnum stationTypeEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, VantageEepromConstants::StationType value) {
    os << stationTypeEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * RepeaterId Enumeration
 ****************************************/
static const NameValuePair<VantageEepromConstants::RepeaterId> riMappings[] = {
    { "No Repeater", VantageEepromConstants::RepeaterId::NO_REPEATER },
    { "Repeater A", VantageEepromConstants::RepeaterId::REPEATER_A },
    { "Repeater B", VantageEepromConstants::RepeaterId::REPEATER_B },
    { "Repeater C", VantageEepromConstants::RepeaterId::REPEATER_C },
    { "Repeater D", VantageEepromConstants::RepeaterId::REPEATER_D },
    { "Repeater E", VantageEepromConstants::RepeaterId::REPEATER_E },
    { "Repeater F", VantageEepromConstants::RepeaterId::REPEATER_F },
    { "Repeater G", VantageEepromConstants::RepeaterId::REPEATER_G },
    { "Repeater H", VantageEepromConstants::RepeaterId::REPEATER_H }
};

class RepeaterIdEnum : public VantageEnum<VantageEepromConstants::RepeaterId,sizeof(riMappings)/sizeof(riMappings[0])>  {
public:
    RepeaterIdEnum() {};
    virtual ~RepeaterIdEnum() {};

    virtual const NameValuePair<VantageEepromConstants::RepeaterId> * getMappings() const {
        return riMappings;
    }
};

static RepeaterIdEnum repeaterIdEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, VantageEepromConstants::RepeaterId value) {
    os << repeaterIdEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * CumulativeValue Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::CumulativeValue> cvMappings[] = {
    { "Daily Rain", ProtocolConstants::CumulativeValue::DAILY_RAIN_CUM },
    { "Storm Rain", ProtocolConstants::CumulativeValue::STORM_RAIN_CUM },
    { "Month Rain", ProtocolConstants::CumulativeValue::MONTH_RAIN_CUM },
    { "Year Rain", ProtocolConstants::CumulativeValue::YEAR_RAIN_CUM },
    { "Day ET", ProtocolConstants::CumulativeValue::DAY_ET_CUM },
    { "Month ET", ProtocolConstants::CumulativeValue::MONTH_ET_CUM },
    { "Year ET", ProtocolConstants::CumulativeValue::YEAR_ET_CUM }
};

class CumulativeValueEnum : public VantageEnum<ProtocolConstants::CumulativeValue,sizeof(cvMappings)/sizeof(cvMappings[0])>  {
public:
    CumulativeValueEnum() {};
    virtual ~CumulativeValueEnum() {};

    virtual const NameValuePair<ProtocolConstants::CumulativeValue> * getMappings() const {
        return cvMappings;
    }
};

static CumulativeValueEnum cumulativeValueEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::CumulativeValue value) {
    os << cumulativeValueEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * Month Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::Month> mMappings[] = {
    { "January", ProtocolConstants::Month::JANUARY },
    { "February", ProtocolConstants::Month::FEBRUARY },
    { "March", ProtocolConstants::Month::MARCH },
    { "April", ProtocolConstants::Month::APRIL },
    { "May", ProtocolConstants::Month::MAY },
    { "June", ProtocolConstants::Month::JUNE },
    { "July", ProtocolConstants::Month::JULY },
    { "August", ProtocolConstants::Month::AUGUST },
    { "September", ProtocolConstants::Month::SEPTEMBER },
    { "October", ProtocolConstants::Month::OCTOBER },
    { "November", ProtocolConstants::Month::NOVEMBER },
    { "December", ProtocolConstants::Month::DECEMBER }
};

class MonthEnum : public VantageEnum<ProtocolConstants::Month,sizeof(mMappings)/sizeof(mMappings[0])>  {
public:
    MonthEnum() {};
    virtual ~MonthEnum() {};

    virtual const NameValuePair<ProtocolConstants::Month> * getMappings() const {
        return mMappings;
    }
};

static MonthEnum monthEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::Month value) {
    os << monthEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * Console Type Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::ConsoleType> ctMappings[] = {
    { "Vantage Pro2", ProtocolConstants::ConsoleType::VANTAGE_PRO_2 },
    { "Vantage Vue", ProtocolConstants::ConsoleType::VANTAGE_VUE }
};

class ConsoleTypeEnum : public VantageEnum<ProtocolConstants::ConsoleType,sizeof(ctMappings)/sizeof(ctMappings[0])>  {
public:
    ConsoleTypeEnum() {};
    virtual ~ConsoleTypeEnum() {};

    virtual const NameValuePair<ProtocolConstants::ConsoleType> * getMappings() const {
        return ctMappings;
    }
};

static ConsoleTypeEnum consoleTypeEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::ConsoleType value) {
    os << consoleTypeEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

/****************************************
 * Rain Bucket Type Enumeration
 ****************************************/
static const NameValuePair<ProtocolConstants::RainBucketSizeType> rbtMappings[] = {
    { ".01 inch", ProtocolConstants::RainBucketSizeType::POINT_01_INCH },
    { ".1 mm", ProtocolConstants::RainBucketSizeType::POINT_1_MM },
    { ".2 mm", ProtocolConstants::RainBucketSizeType::POINT_2_MM }
};

class RainBucketSizeTypeEnum : public VantageEnum<ProtocolConstants::RainBucketSizeType,sizeof(rbtMappings)/sizeof(rbtMappings[0])>  {
public:
    RainBucketSizeTypeEnum() {};
    virtual ~RainBucketSizeTypeEnum() {};

    virtual const NameValuePair<ProtocolConstants::RainBucketSizeType> * getMappings() const {
        return rbtMappings;
    }
};

static RainBucketSizeTypeEnum rainBucketSizeTypeEnum;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static std::ostream &
operator<<(std::ostream & os, ProtocolConstants::RainBucketSizeType value) {
    os << rainBucketSizeTypeEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static Rainfall
rainBucketEnumValueToRain(ProtocolConstants::RainBucketSizeType rainBucketType) {
    Rainfall rain = .01;
    switch (rainBucketType) {
        case ProtocolConstants::RainBucketSizeType::POINT_01_INCH:
            rain = ProtocolConstants::POINT_01_INCH_SIZE;
            break;

        case ProtocolConstants::RainBucketSizeType::POINT_1_MM:
            rain = ProtocolConstants::POINT_1_MM_SIZE;
            break;

        case ProtocolConstants::RainBucketSizeType::POINT_2_MM:
            rain = ProtocolConstants::POINT_2_MM_SIZE;
            break;
    }

    return rain;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static ProtocolConstants::RainBucketSizeType
rainToRainBucketEnumValue(Rainfall rain) {
    ProtocolConstants::RainBucketSizeType type = ProtocolConstants::RainBucketSizeType::POINT_01_INCH;

    if (rain == ProtocolConstants::POINT_01_INCH_SIZE)
        type = ProtocolConstants::RainBucketSizeType::POINT_01_INCH;
    else if (rain == ProtocolConstants::POINT_1_MM_SIZE)
        type = ProtocolConstants::RainBucketSizeType::POINT_1_MM;
    else if (rain == ProtocolConstants::POINT_2_MM_SIZE)
        type = ProtocolConstants::RainBucketSizeType::POINT_2_MM;

    return type;
}

}

#endif
