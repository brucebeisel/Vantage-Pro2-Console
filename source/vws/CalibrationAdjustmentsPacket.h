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

#ifndef CALIBRATION_ADJUSTMENTS_PACKET_H
#define CALIBRATION_ADJUSTMENTS_PACKET_H

#include "Weather.h"
#include "VantageProtocolConstants.h"

namespace vws {

class CalibrationAdjustmentsPacket {
public:
    static constexpr int CALIBRATION_DATA_BLOCK_SIZE = 29;

    CalibrationAdjustmentsPacket();
    virtual ~CalibrationAdjustmentsPacket();

    bool decodePacket(const byte buffer[]);
    void encodePacket(byte buffer[]) const;

    std::string formatJSON() const;

    bool parseJSON(std::string & s);

private:

    static constexpr int INSIDE_TEMPERATURE_ADJUSTMENT_OFFSET = 0;
    static constexpr int INSIDE_TEMPERATURE_ADJUSTMENT_1S_COMPLIMENT_OFFSET = 1;
    static constexpr int OUTSIDE_TEMPERATURE_ADJUSTMENT_OFFSET = 2;
    static constexpr int EXTRA_TEMPERATURE_ADJUSTMENTS_OFFSET = 3;
    static constexpr int SOIL_TEMPERATURE_ADJUSTMENTS_OFFSET = 10;
    static constexpr int LEAF_TEMPERATURE_ADJUSTMENTS_OFFSET = 14;
    static constexpr int INSIDE_HUMIDITY_ADJUSTMENT_OFFSET = 18;
    static constexpr int OUTSIDE_HUMIDITY_ADJUSTMENT_OFFSET = 19;
    static constexpr int EXTRA_HUMIDITY_ADJUSTMENTS_OFFSET = 20;
    static constexpr int WIND_DIRECTION_ADJUSTMENT_OFFSET = 27;

    static constexpr Temperature TEMPERATURE_ADJUSTMENT_SCALE = 10.0;

    Temperature insideTemperatureAdjustment;
    Temperature outsideTemperatureAdjustment;
    Temperature extraTemperatureAdjustments[ProtocolConstants::MAX_EXTRA_TEMPERATURES];
    Temperature soilTemperatureAdjustments[ProtocolConstants::MAX_SOIL_TEMPERATURES];
    Temperature leafTemperatureAdjustments[ProtocolConstants::MAX_LEAF_TEMPERATURES];
    int16       insideHumidityAdjustment;
    int16       outsideHumidityAdjustment;
    int16       extraHumidityAdjustments[ProtocolConstants::MAX_EXTRA_HUMIDITIES];
    int16       windDirectionAdjustment;
};

} /* namespace vws */

#endif
