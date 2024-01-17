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
#include "CalibrationAdjustmentsPacket.h"

#include <sstream>
#include "BitConverter.h"
#include "VantageLogger.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CalibrationAdjustmentsPacket::CalibrationAdjustmentsPacket() : insideTemperatureAdjustment(0.0),
                                                               outsideTemperatureAdjustment(0.0),
                                                               insideHumidityAdjustment(0),
                                                               outsideHumidityAdjustment(0),
                                                               windDirectionAdjustment(0),
                                                               logger(VantageLogger::getLogger("CalibrationAdjustmentsPacket")) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CalibrationAdjustmentsPacket::~CalibrationAdjustmentsPacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// TODO Add sizeof buffer argument to prevent potential buffer overflows
void
CalibrationAdjustmentsPacket::decodePacket(const byte buffer[]) {
    int value8 = BitConverter::toInt8(buffer, INSIDE_TEMPERATURE_ADJUSTMENT_OFFSET);
    insideTemperatureAdjustment = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;


    value8 = BitConverter::toInt8(buffer, OUTSIDE_TEMPERATURE_ADJUSTMENT_OFFSET);
    outsideTemperatureAdjustment = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;

    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        value8 = BitConverter::toInt8(buffer, EXTRA_TEMPERATURE_ADJUSTMENTS_OFFSET + i);
        extraTemperatureAdjustments[i] = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;
    }

    for (int i = 0; i < MAX_SOIL_TEMPERATURES; i++) {
        value8 = BitConverter::toInt8(buffer, SOIL_TEMPERATURE_ADJUSTMENTS_OFFSET + i);
        soilTemperatureAdjustments[i] = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;
    }

    for (int i = 0; i < MAX_LEAF_TEMPERATURES; i++) {
        value8 = BitConverter::toInt8(buffer, LEAF_TEMPERATURE_ADJUSTMENTS_OFFSET + i);
        leafTemperatureAdjustments[i] = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;
    }

    insideHumidityAdjustment = BitConverter::toInt8(buffer, INSIDE_HUMIDITY_ADJUSTMENT_OFFSET);
    outsideHumidityAdjustment = BitConverter::toInt8(buffer, OUTSIDE_HUMIDITY_ADJUSTMENT_OFFSET);

    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++)
        extraHumidityAdjustments[i] = BitConverter::toInt8(buffer, EXTRA_HUMIDITY_ADJUSTMENTS_OFFSET + i);

    windDirectionAdjustment = BitConverter::toInt16(buffer, WIND_DIRECTION_ADJUSTMENT_OFFSET);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// TODO Add sizeof buffer argument to prevent potential buffer overflows
void
CalibrationAdjustmentsPacket::encodePacket(byte buffer[]) const {
    int8 value = static_cast<int>(insideTemperatureAdjustment * TEMPERATURE_ADJUSTMENT_SCALE);
    BitConverter::getBytes(value, buffer, INSIDE_TEMPERATURE_ADJUSTMENT_OFFSET, 1);
    BitConverter::getBytes(~value, buffer, INSIDE_TEMPERATURE_ADJUSTMENT_1S_COMPLIMENT_OFFSET, 1);

    value = static_cast<int>(outsideTemperatureAdjustment * TEMPERATURE_ADJUSTMENT_SCALE);
    BitConverter::getBytes(value, buffer, OUTSIDE_TEMPERATURE_ADJUSTMENT_OFFSET, 1);

    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        value = static_cast<int>(extraTemperatureAdjustments[i] * TEMPERATURE_ADJUSTMENT_SCALE);
        BitConverter::getBytes(value, buffer, EXTRA_TEMPERATURE_ADJUSTMENTS_OFFSET + i, 1);
    }

    for (int i = 0; i < MAX_SOIL_TEMPERATURES; i++) {
        value = static_cast<int>(soilTemperatureAdjustments[i] * TEMPERATURE_ADJUSTMENT_SCALE);
        BitConverter::getBytes(value, buffer, SOIL_TEMPERATURE_ADJUSTMENTS_OFFSET + i, 1);
    }

    for (int i = 0; i < MAX_LEAF_TEMPERATURES; i++) {
        value = static_cast<int>(leafTemperatureAdjustments[i] * TEMPERATURE_ADJUSTMENT_SCALE);
        BitConverter::getBytes(value, buffer, LEAF_TEMPERATURE_ADJUSTMENTS_OFFSET + i, 1);
    }

    BitConverter::getBytes(insideHumidityAdjustment, buffer, INSIDE_HUMIDITY_ADJUSTMENT_OFFSET, 1);
    BitConverter::getBytes(outsideHumidityAdjustment, buffer, OUTSIDE_HUMIDITY_ADJUSTMENT_OFFSET, 1);

    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++)
        BitConverter::getBytes(extraHumidityAdjustments[i], buffer, EXTRA_HUMIDITY_ADJUSTMENTS_OFFSET + i, 1);

    BitConverter::getBytes(windDirectionAdjustment, buffer, WIND_DIRECTION_ADJUSTMENT_OFFSET, 2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
CalibrationAdjustmentsPacket::formatJSON() const {
    ostringstream oss;

    oss << "{ \"calibrationAdjustments\" : { "
        << " \"insideTemperatureAdjustment\" : " << insideTemperatureAdjustment << ", "
        << " \"outsideTemperatureAdjustment\" : " << outsideTemperatureAdjustment << ", "
        << " \"extraTemperatureAdjustments\" : [";

    bool first = true;
    for (int i = 0; i < MAX_EXTRA_TEMPERATURES; i++) {
        if (!first) oss << ", "; else first = false;
        oss << extraTemperatureAdjustments[i];
    }
    oss << " ], "
        << "\"soilTemperatureAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_SOIL_TEMPERATURES; i++) {
        if (!first) oss << ", "; else first = false;
        oss << soilTemperatureAdjustments[i];
    }

    oss << " ], "
        << "\"leafTemperatureAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_LEAF_TEMPERATURES; i++) {
        if (!first) oss << ", "; else first = false;
        oss << leafTemperatureAdjustments[i];
    }
    oss << " ], "
        << "\"insideHumidityAdjustment\" : " << insideHumidityAdjustment << ", "
        << "\"outsideHumidityAdjustment\" : " << outsideHumidityAdjustment << ", "
        << "\"extraHumidityAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        if (!first) oss << ", "; else first = false;
        oss << extraHumidityAdjustments[i];
    }

    oss << " ], ";
    oss << "\"windDirectionAdjustment\" : " << windDirectionAdjustment;

    oss << " } }";

    return oss.str();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
bool
CalibrationAdjustmentsPacket::findJsonValue(json root, const string & name, T & value) {
    bool success = false;
    auto valuePtr = root.find(name);
    if (valuePtr != root.end()) {
        value = *valuePtr;
        success = true;
    }
    else
        logger.log(VantageLogger::VANTAGE_WARNING) << "Missing JSON element: " << name << endl;

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<typename T>
bool
CalibrationAdjustmentsPacket::findJsonArray(json root, const string & name, T & array, int size) {
    bool success = false;
    auto jlist = root.find(name);
    if (jlist != root.end()) {
        if ((*jlist).size() == size) {
            std::copy((*jlist).begin(), (*jlist).end(), array);
            success = true;
        }
        else {
            logger.log(VantageLogger::VANTAGE_WARNING) << "JSON array has the wrong size. Expecting " << size << " received " << (*jlist).size() << endl;
        }
    }
    else
        logger.log(VantageLogger::VANTAGE_WARNING) << "Missing JSON element: " << name << endl;

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CalibrationAdjustmentsPacket::parseJSON(const std::string & s) {

    json adjustments = json::parse(s.begin(), s.end());

    if (!findJsonValue(adjustments, "insideTemperatureAdjustment", this->insideTemperatureAdjustment))
        return false;

    if (!findJsonValue(adjustments, "outsideTemperatureAdjustment", this->outsideTemperatureAdjustment))
        return false;

    if (!findJsonValue(adjustments, "insideHumidityAdjustment", this->insideHumidityAdjustment))
        return false;

    if (!findJsonValue(adjustments, "outsideHumidityAdjustment", this->outsideHumidityAdjustment))
        return false;

    if (!findJsonArray(adjustments, "extraTemperatureAdjustments", this->extraTemperatureAdjustments, MAX_EXTRA_TEMPERATURES))
        return false;

    if (!findJsonArray(adjustments, "soilTemperatureAdjustments", this->soilTemperatureAdjustments, MAX_SOIL_TEMPERATURES))
        return false;

    if (!findJsonArray(adjustments, "leafTemperatureAdjustments", this->leafTemperatureAdjustments, MAX_LEAF_TEMPERATURES))
        return false;

    if (!findJsonArray(adjustments, "extraHumidityAdjustments", this->extraHumidityAdjustments, MAX_EXTRA_HUMIDITIES))
        return false;

    if (!findJsonValue(adjustments, "windDirectionAdjustment", this->windDirectionAdjustment))
        return false;

    return true;
}

} /* namespace vws */
