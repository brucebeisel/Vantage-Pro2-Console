/*
 * Copyright (C) 2025 Bruce Beisel
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
#include "BaudRate.h"

#include <iostream>

namespace vws {

#ifdef __CYGWIN__
const vws::BaudRate vws::BaudRate::BR_19200(CBR_19200, 19200);
const vws::BaudRate vws::BaudRate::BR_14400(CBR_14400, 14400);
const vws::BaudRate vws::BaudRate::BR_9600(CBR_9600, 9600);
const vws::BaudRate vws::BaudRate::BR_4800(CBR_4800, 4800);
const vws::BaudRate vws::BaudRate::BR_2400(CBR_2400, 2400);
const vws::BaudRate vws::BaudRate::BR_1200(CBR_1200, 1200);
#else
const vws::BaudRate vws::BaudRate::BR_19200(B19200, 19200);
const vws::BaudRate vws::BaudRate::BR_9600(B9600, 9600);
const vws::BaudRate vws::BaudRate::BR_4800(B4800, 4800);
const vws::BaudRate vws::BaudRate::BR_2400(B2400, 2400);
const vws::BaudRate vws::BaudRate::BR_1200(B1200, 1200);
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BaudRate::BaudRate(speed_t osBaudRate, int vantageBaudRate) : osValue(osBaudRate), vantageValue(vantageBaudRate) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BaudRate
BaudRate::findBaudRateBySpeed(int speed) {
    BaudRate rv = BR_19200;

    if (speed == BR_19200.vantageValue)
        rv = BR_19200;
#ifdef __CYGWIN__
    else if (speed == BR_14400.vantageValue)
        rv = BR_14400;
#endif
    else if (speed == BR_9600.vantageValue)
        rv = BR_9600;
    else if (speed == BR_4800.vantageValue)
        rv = BR_4800;
    else if (speed == BR_2400.vantageValue)
        rv = BR_2400;
    else if (speed == BR_1200.vantageValue)
        rv = BR_1200;

    return rv;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
speed_t
BaudRate::getOsValue() const {
    return osValue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
BaudRate::getVantageValue() const {
    return vantageValue;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::ostream &
operator<<(std::ostream & os, const vws::BaudRate & br) {
    os << std::hex << "Baud Rate: " << "OS: 0x" << br.osValue << std::dec << "," << br.osValue << " Vantage: " << br.vantageValue;
    return os;
}
}
