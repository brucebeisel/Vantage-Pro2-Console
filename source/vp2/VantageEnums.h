#ifndef VANTAGE_ENUMS_H_
#define VANTAGE_ENUMS_H_

#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
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

    virtual const NameValuePair<T> * getMappings() const = 0;

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

        return std::string(INVALID_ENUM_VALUE);
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

        throw std::invalid_argument("Invalid enum value string");
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const NameValuePair<ProtocolConstants::ExtremePeriod> epMappings[] = {
    { "Daily Extremes", ProtocolConstants::ExtremePeriod::DAILY },
    { "Monthly Extremes", ProtocolConstants::ExtremePeriod::MONTHLY },
    { "Yearly Extremes", ProtocolConstants::ExtremePeriod::YEARLY }
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::ExtremePeriod value) {
    os << extremePeriodEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::RainUnits value) {
    os << rainUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::BarometerUnits value) {
    os << barometerUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::TemperatureUnits value) {
    os << temperatureUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::WindUnits value) {
    os << windUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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

std::ostream &
operator<<(std::ostream & os, ProtocolConstants::ElevationUnits value) {
    os << elevationUnitsEnum.valueToString(value) << "(" << static_cast<int>(value) << ")";
    return os;
}

}

#endif
