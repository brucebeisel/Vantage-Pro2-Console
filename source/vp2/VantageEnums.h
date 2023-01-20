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

    VantageEnum() {}
    virtual ~VantageEnum() {};

    void enumValues(std::vector<T> & values) {
        const NameValuePair<T> * mappings = getMappings();
        values.clear();
        for (int i = 0; i < Count; i++) {
            values.push_back(mappings[i].second);
        }
    }

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
    const std::string & valueToString(T value) const {
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
    T stringToValue(const std::string & valueString) const { //throw (std::invalid_argument){
        const NameValuePair<T> * mappings = getMappings();
        for (int i = 0; i < Count; i++) {
            if (mappings[i].first == valueString)
                return mappings[i].second;
        }

        throw std::invalid_argument("Invalid enum value");
    }


private:
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const NameValuePair<ProtocolConstants::RainUnits> ruMappings[] = {
    { "inches", RainUnits::INCHES },
    { "millimeters", RainUnits::MILLIMETERS }
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
static const NameValuePair<ProtocolConstants::BarometerUnits> buMappings[] = {
    { "inHg", BarometerUnits::IN_HG },
    { "mm", BarometerUnits::MILLIMETER },
    { "hPa", BarometerUnits::HPA },
    { "mbar", BarometerUnits::MILLIBAR }
};

class BarometerUnitsEnum : public VantageEnum<ProtocolConstants::BarometerUnits,sizeof(ruMappings)/sizeof(ruMappings[0])>  {
public:
    BarometerUnitsEnum() {};
    virtual ~BarometerUnitsEnum() {};

    virtual const NameValuePair<BarometerUnits> * getMappings() const {
        return buMappings;
    }
};

static BarometerUnitsEnum barometerUnitsEnum;

}

#endif
