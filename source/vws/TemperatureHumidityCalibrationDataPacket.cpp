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
#include <sstream>
#include "TemperatureHumidityCalibrationDataPacket.h"
#include "BitConverter.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TemperatureHumidityCalibrationDataPacket::TemperatureHumidityCalibrationDataPacket() :
                                                                    insideTemperatureAdjustment(0.0),
                                                                    outsideTemperatureAdjustment(0.0),
                                                                    insideHumidityAdjustment(0),
                                                                    outsideHumidityAdjustment(0),
                                                                    windDirectionAdjustment(0) {

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TemperatureHumidityCalibrationDataPacket::~TemperatureHumidityCalibrationDataPacket() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
TemperatureHumidityCalibrationDataPacket::decodePacket(const byte buffer[]) {
    uint8 value8 = BitConverter::toInt8(buffer, INSIDE_TEMPERATURE_ADJUSTMENT_OFFSET);
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

    return true;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string
TemperatureHumidityCalibrationDataPacket::formatJSON() const {
    ostringstream oss;

    oss << "{ \"temperatureHumidityCalibrationData\" : { "
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
TemperatureHumidityCalibrationDataPacket::parseJSON(std::string & s) {
    return true;

}

} /* namespace vws */