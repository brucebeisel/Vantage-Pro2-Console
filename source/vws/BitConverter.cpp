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
#include <iostream>
#include "BitConverter.h"

namespace vws {


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BitConverter::BitConverter() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BitConverter::~BitConverter() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BitConverter::toInt8(const byte buffer[], int index) {
    int result = (static_cast<int>(buffer[index]) & ONE_BYTE_MASK);
    if (buffer[index] & SIGN_BIT)
        result *= -1;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
unsigned int
BitConverter::toUint8(const byte buffer[], int index) {
    return static_cast<unsigned int>(buffer[index]) & ONE_BYTE_MASK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BitConverter::toInt16(const byte buffer[], int index, bool littleEndian) {
    return bitsToInt<short, int>(&buffer[index], littleEndian, true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
unsigned int
BitConverter::toUint16(const byte buffer[], int index, bool littleEndian) {
    return bitsToInt<short, unsigned int>(&buffer[index], littleEndian, false);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BitConverter::toInt32(const byte buffer[], int index, bool littleEndian) {
    return bitsToInt<int, int>(&buffer[index], littleEndian, true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
unsigned int
BitConverter::toUint32(const byte buffer[], int index, bool littleEndian) {
    return bitsToInt<int, unsigned int>(&buffer[index], littleEndian, false);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
BitConverter::getBytes(int value, byte buffer[], int index, int nbytes, bool littleEndian) {
    int bufferIndex;
    for (int i = 0; i < nbytes; i++) {
        if (littleEndian)
            bufferIndex = index + i;
        else
            bufferIndex = index + (nbytes - 1 - i);

        buffer[bufferIndex] = (value >> (BITS_PER_BYTE * i)) & ONE_BYTE_MASK;
    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <typename T, typename R>
R
BitConverter::bitsToInt(const byte * bits, bool littleEndian, bool isSigned) {
    R result = 0;
    bool signBitSet = false;
    if (littleEndian) {
        signBitSet = bits[sizeof(T) - 1] & SIGN_BIT;
        for (int n = sizeof(T) - 1; n >= 0; n--) {
            result = (result << BITS_PER_BYTE) + (static_cast<int>(bits[n]) & ONE_BYTE_MASK);
        }
    }
    else {
        signBitSet = bits[0] & SIGN_BIT;
        for (int n = 0; n < sizeof(T); n++)
            result = (result << BITS_PER_BYTE) + (static_cast<int>(bits[n]) & ONE_BYTE_MASK);
    }

    if (isSigned && signBitSet) {
        result *= -1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BitConverter::getUpperNibble(byte value) {
    return (value & UPPER_NIBBLE_MASK) >> NIBBLE_BITS;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BitConverter::getLowerNibble(byte value) {
    return value & LOWER_NIBBLE_MASK;

}
}
