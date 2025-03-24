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
#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#ifdef __CYGWIN__
#include <windows.h>
#include <winbase.h>
#else
typedef int HANDLE;
#endif
#include <string>
#include "WeatherTypes.h"
#include "BaudRate.h"

namespace vws {
class VantageLogger;

/**
 * Class to communicate with the Vantage console using a serial port interface.
 */
class SerialPort {
public:
    static constexpr int DEFAULT_TIMEOUT_MILLIS = 2500;

    /**
     * Constructor.
     * 
     * @param device The serial device to open
     * @param baudRate The baud rate at which to communicate with the console
     */
    SerialPort(const std::string & device, vws::BaudRate baudRate);

    /**
     * Destructor, that will close the serial port.
     */
    ~SerialPort();

    /**
     * Open the serial port.
     * 
     * @return True if successful
     */
    bool open();

    /**
     * Close the serial port.
     */
    void close();

    /**
     * Read from the serial port into the specified location of a buffer.
     * 
     * @param buffer        The buffer into which the data will be read
     * @param index         The index into the buffer where bytes read will be stored
     * @param requiredBytes The number of bytes that are required by this call
     * @param timeoutMillis The number of milliseconds to wait for data to be available
     *
     * @return The number of bytes actually read
     */
    int read(byte buffer[], int index, int requiredBytes, int timeoutMillis);

    /**
     * Read from the serial port into the beginning of a buffer.
     * 
     * @param buffer        The buffer into which the data will be read
     * @param requiredBytes The number of bytes that are required by this call
     * @param timeoutMillis The number of milliseconds to wait for the expected bytes
     *
     * @return True if the number of bytes read is equal to the required number of bytes
     */
    bool readBytes(byte * buffer, int requiredBytes, int timeoutMillis = DEFAULT_TIMEOUT_MILLIS);

    /**
     * Write a string to the serial port.
     * 
     * @param s The string to be converted to bytes then written to the serial port
     * @return True if all bytes were successfully written
     */
    bool write(const std::string & s);

    /**
     * Write to the serial port.
     * 
     * @param buffer The bytes to write to the serial port
     * @param nbytes The number of bytes to write
     * @return True if all bytes were successfully written
     *
     */
    bool write(const void * buffer, int nbytes);

    /**
     * Discard any bytes in the read buffer.
     */
    void discardInBuffer();

    /**
     * Set the baud rate. Note the serial port must be closed or it must be closed then opened for this to take effect.
     *
     * @param rate The new baud rate
     */
    void setBaudRate(vws::BaudRate rate);

    /**
     * Check if the serial port is open.
     *
     * @return True if the port is open
     */
    bool isOpen() const;

private:
    /**
     * The number of time read() will be called to read the number of required bytes
     */
    static constexpr int READ_TRIES = 3;

    HANDLE          commPort; // The file descriptor of the open port
    std::string     device;   // The name of the serial port to be opened
    vws::BaudRate   baudRate; // The baud rate used to communicate over the serial port
    VantageLogger & logger;
};
}
#endif
