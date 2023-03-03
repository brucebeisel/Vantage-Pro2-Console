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
#ifndef BIT_CONVERTER_H
#define BIT_CONVERTER_H

#include "WeatherTypes.h"

namespace vws {
/**
 * Class to convert bytes to various size of integers
 */
class BitConverter {
public:
    static constexpr int ONE_BYTE_MASK = 0xFF;
    static constexpr int BITS_PER_BYTE = 8;
    static constexpr char SIGN_BIT = 0x80;

    virtual ~BitConverter();

    /**
     * Convert one byte to a signed or unsigned integer.
     * 
     * @param buffer The buffer from which to do the conversion
     * @param index  The index within the buffer to do the conversion
     * @return The converted integer
     */
    static int toInt8(const byte buffer[], int index);
    static uint8 toUint8(const byte buffer[], int index);

    /**
     * Convert two bytes to a signed or unsigned integer.
     * 
     * @param buffer       The buffer from which to do the conversion
     * @param index        The index within the buffer to do the conversion
     * @param littleEndian True if the buffer holds the integer in little endian format
     * @return The converted integer
     */
    static int16 toInt16(const byte buffer[], int index, bool littleEndian = true);
    static uint16 toUint16(const byte buffer[], int index, bool littleEndian = true);

    /**
     * Convert four bytes to a signed or unsigned integer.
     * 
     * @param buffer       The buffer from which to do the conversion
     * @param index        The index within the buffer to do the conversion
     * @param littleEndian True if the buffer holds the integer in little endian format
     * @return The converted integer
     */
    static int32 toInt32(const byte buffer[], int index, bool littleEndian = true);
    static uint32 toUint32(const byte buffer[], int index, bool littleEndian = true);

    /**
     * Convert an integer into bytes.
     * 
     * @param value The value to convert
     * @param buffer The buffer into which to store the integer
     * @param index The index within the buffer to store the integer
     * @param nbytes The number of bytes to convert
     * @param littleEndian True if the integer is to be store as little endian
     */
    static void getBytes(int value, byte buffer[], int index, int nbytes, bool littleEndian = true);

    /**
     * Get the upper nibble of the provided byte.
     *
     * @param value The value from which to extract the upper nibble
     * @return The value of the upper nibble
     */
    static int getUpperNibble(byte value);

    /**
     * Get the lower nibble of the provided byte.
     *
     * @param value The value from which to extract the lower nibble
     * @return The value of the lower nibble
     */
    static int getLowerNibble(byte value);

private:
    static const byte UPPER_NIBBLE_MASK = 0xF0;
    static const byte LOWER_NIBBLE_MASK = 0x0F;
    static const int  NIBBLE_BITS = 4;

    /**
     * Constructor for which there is no implementation.
     */
    BitConverter();

    /**
     * Utility template function to convert bytes in a buffer to any form of integer.
     * Note that "typename T" must be some form of integer.
     * 
     * @param bits         The buffer containing the integer to be converted
     * @param littleEndian Whether the integer is stored in the buffer as little endian
     * @param isSigned     Whether the input should be treated as signed or unsigned
     * @return The resulting integer.
     */
    template <typename T> static T bitsToInt(const byte bits[], bool littleEndian, bool isSigned);
};
}
#endif
