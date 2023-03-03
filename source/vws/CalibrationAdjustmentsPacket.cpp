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

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CalibrationAdjustmentsPacket::CalibrationAdjustmentsPacket() :
                                                                    insideTemperatureAdjustment(0.0),
                                                                    outsideTemperatureAdjustment(0.0),
                                                                    insideHumidityAdjustment(0),
                                                                    outsideHumidityAdjustment(0),
                                                                    windDirectionAdjustment(0) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CalibrationAdjustmentsPacket::~CalibrationAdjustmentsPacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CalibrationAdjustmentsPacket::decodePacket(const byte buffer[]) {
    int value8 = BitConverter::toInt8(buffer, INSIDE_TEMPERATURE_ADJUSTMENT_OFFSET);
    insideTemperatureAdjustment = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;


    value8 = BitConverter::toInt8(buffer, OUTSIDE_TEMPERATURE_ADJUSTMENT_OFFSET);
    outsideTemperatureAdjustment = static_cast<Temperature>(value8) / TEMPERATURE_ADJUSTMENT_SCALE;
    cout << "Converted outside temperature adjustment from byte " << hex << (int)buffer[OUTSIDE_TEMPERATURE_ADJUSTMENT_OFFSET] << dec << " to value " << (int)value8 << " and temperature " << outsideTemperatureAdjustment;

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

    return true;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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
        BitConverter::getBytes(extraHumidityAdjustments[i], buffer, EXTRA_HUMIDITY_ADJUSTMENTS_OFFSET, 1);

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
        if (!first)
            oss << ", ";

        oss << extraTemperatureAdjustments[i];
        first = false;
    }
    oss << " ], "
        << "\"soilTemperatureAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_SOIL_TEMPERATURES; i++) {
        if (!first)
            oss << ", ";

        oss << soilTemperatureAdjustments[i];
        first = false;
    }

    oss << " ], "
        << "\"leafTemperatureAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_LEAF_TEMPERATURES; i++) {
        if (!first)
            oss << ", ";

        oss << leafTemperatureAdjustments[i];
        first = false;
    }
    oss << " ], "
        << "\"insideHumidityAdjustment\" : " << insideHumidityAdjustment << ", "
        << "\"outsideHumidityAdjustment\" : " << outsideHumidityAdjustment << ", "
        << "\"extraHumidityAdjustments\" : [ ";

    first = true;
    for (int i = 0; i < MAX_EXTRA_HUMIDITIES; i++) {
        if (!first)
            oss << ", ";

        oss << extraHumidityAdjustments[i];
        first = false;
    }

    oss << " ], ";
    oss << "\"windDirectionAdjustment\" : " << windDirectionAdjustment;

    oss << " } }";

    return oss.str();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
CalibrationAdjustmentsPacket::parseJSON(const std::string & s) {
    return true;

}

} /* namespace vws */
