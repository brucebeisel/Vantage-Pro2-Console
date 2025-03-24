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
#ifndef BAUD_RATE_H
#define BAUD_RATE_H

#ifdef __CYGWIN__
#include "windows.h"
#include "winbase.h"
typedef DWORD speed_t;
#else
#include <termios.h>
#endif
#include <iosfwd>

namespace vws {

/**
 * Class that represents a baud rate for the serial interface with the Vantage console.
 * Note that the default baud rate for the Vantage console is 19200 and it really should not be changed.
 */
class BaudRate {
public:
    /**
     * The valid baud rates for the Vantage console, but note that Linux does not support the baud rate of 14400.
     */
    static const BaudRate BR_19200;
#ifdef __CYGWIN__
    static const BaudRate BR_14400;
#endif
    static const BaudRate BR_9600;
    static const BaudRate BR_4800;
    static const BaudRate BR_2400;
    static const BaudRate BR_1200;

    /**
     * Find the baud rate object using the vantage speed value.
     *
     * @return The BaudRate object that matches the speed or BR_19200 if there is no match
     */
    static BaudRate findBaudRateBySpeed(int speed);

    /**
     * Get the value needed by operating system to set the baud rate of the serial port.
     *
     * @return The OS specific value
     */
    speed_t getOsValue() const;

    /**
     * Get the baud rate value needed by the Vantage console.
     *
     * @return The baud rate
     */
    int getVantageValue() const;

    /**
     * ostream operator.
     *
     * @param os The ostream
     * @param br The baud rate to output
     * @return The ostream passed in
     */
    friend std::ostream & operator<<(std::ostream & os, const vws::BaudRate & br);

private:
    /**
     * Constructor.
     *
     * @param osBaudRate      The value the operating system uses to configure a serial port
     * @param vantageBaudRate The value the Vantage console uses for baud rate
     */
    BaudRate(speed_t osBaudRate, int vantageBaudRate);

    speed_t osValue;       // Operating system specific value (CBR_##### for windows and B##### for Linux)
    int     vantageValue;  // The value that the Vantage console uses for the BAUD command
};

}

#endif
