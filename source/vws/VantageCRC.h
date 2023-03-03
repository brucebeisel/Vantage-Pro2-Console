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
#ifndef VANTAGE_CRC_H
#define VANTAGE_CRC_H

#include "WeatherTypes.h"

namespace vws {
/**
 * Class to calculate and check the CRCs sent over the console's serial port.
 */
class VantageCRC {
public:
    /**
     * Calculate a CRC for a buffer.
     * 
     * @param buffer The buffer from which to calculate the CRC
     * @param length The length of the buffer to calculate the CRC
     * @return The CRC
     */
    static int calculateCRC(const byte *buffer, int length);

    /**
     * Check the CRC for a buffer where the CRC is stored in the last 2 bytes of the buffer.
     * 
     * @param buffer The buffer where the CRC will be checked
     * @param length The length of the buffer will be checked
     * @return True if the CRC in the buffer is correct
     */
    static bool checkCRC(const byte * buffer, int length);

private:
    /**
     * Constructor with no implementation.
     */
    VantageCRC() = delete;

    /**
     * Destructor with no implementation.
     */
    virtual ~VantageCRC() = delete;
};
}
#endif /* VANTAGE_CRC_H */
