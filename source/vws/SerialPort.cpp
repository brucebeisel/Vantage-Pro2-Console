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
#include "SerialPort.h"

#ifndef __CYGWIN__
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
static const int INVALID_HANDLE_VALUE = -1;
#else
#include <winsock2.h>
#endif
#include <iostream>
#include <string.h>
#include "VantageLogger.h"
#include "Weather.h"
#include "BaudRate.h"

using namespace std;

namespace vws {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SerialPort::SerialPort(const std::string & device, vws::BaudRate br) : commPort(INVALID_HANDLE_VALUE),
                                                                       device(device),
                                                                       baudRate(br),
                                                                       logger(VantageLogger::getLogger("SerialPort")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SerialPort::~SerialPort(){
    close();
}

#ifdef __CYGWIN__
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::open() {
    commPort = CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (commPort == INVALID_HANDLE_VALUE) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open serial port " << device << " (" << logger.strerror() << ")" << endl;
        return false;
    }

    DCB dcb;
    GetCommState(commPort, &dcb);
    dcb.BaudRate = baudRate.getOsValue();
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(commPort, &dcb);

    COMMTIMEOUTS cto;
    cto.ReadIntervalTimeout = 100;
    cto.ReadTotalTimeoutMultiplier = 10;
    cto.ReadTotalTimeoutConstant = 1000;
    cto.WriteTotalTimeoutConstant = 5000;
    cto.WriteTotalTimeoutMultiplier = 1;
    return SetCommTimeouts(commPort, &cto) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SerialPort::close() {
    CloseHandle(commPort);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::write(const void * buffer, int nbytes) {
    if (!isOpen()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Cannot write to console, serial port not open" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Writing " << nbytes << " bytes" << endl;
    DWORD dwWritten;
    if (!WriteFile(commPort, static_cast<LPCVOID>(buffer), nbytes, &dwWritten, nullptr)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Write to serial port failed (" << logger.strerror() << ")" << endl;
        return false;
    }

    if (dwWritten != nbytes)
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
SerialPort::read(byte buffer[], int index, int nbytes, int timeoutMillis) {
    DWORD dwRead;
    if (!ReadFile(commPort, &buffer[index], nbytes, &dwRead, nullptr)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Read " << dwRead << " bytes from serial port failed (" << logger.strerror() << ")" << endl;
        return -1;
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG3) << "Read " << dwRead << " bytes" << endl;
        return dwRead;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SerialPort::discardInBuffer() {
    PurgeComm(commPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}
#else
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::open() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Opening serial port device " << device << endl;

    commPort = ::open(device.c_str(), O_RDWR);
    if (commPort < 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to open serial port " << device << " (" << logger.strerror() << ")" << endl;
        return false;
    }

    struct termios tio;
    if (tcgetattr(commPort, &tio) != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "tcgetattr() failed (" << logger.strerror() << ")" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Setting serial port attributes, including baud rate of " << baudRate << endl;
    cfsetospeed(&tio, baudRate.getOsValue()); // Baud rate
    cfsetispeed(&tio, B0);                    // B0 baud rate means same as the output baud rate
    cfmakeraw(&tio);                          // Sets the terminal to something like the "raw" mode of the old Version 7 terminal driver
    tio.c_cflag &= ~PARENB;                   // No parity
    tio.c_cflag &= ~CSTOPB;                   // 1 stop bit and 1 start bit
    tio.c_cflag &= ~CSIZE;                    // Clear out the data bits field before setting it
    tio.c_cflag |= CS8;                       // 8 data bits
    int rv = tcsetattr(commPort, 0, &tio);

    if (rv != 0) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "tcsetattr() failed (" << logger.strerror() << ")" << endl;
        return false;
    }

    discardInBuffer();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SerialPort::close() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Closing serial port device " << device << endl;

    ::close(commPort);
    commPort = -1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::write(const void * buffer, int nbytes) {
    if (!isOpen()) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Cannot write to console, serial port not open" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG3) << "Write buffer: " << Weather::dumpBuffer(static_cast<const byte *>(buffer), nbytes);

    ssize_t bytesWritten = ::write(commPort, buffer, nbytes);

    if (bytesWritten == -1) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Write to console failed (" << logger.strerror() << ")" << endl;
        return false;
    }

    if (bytesWritten != nbytes) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Write to console failed. Expected=" << nbytes << " Actual=" << bytesWritten << endl;
        return false;
    }
    else
        return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
SerialPort::read(byte buffer[], int index, int requiredBytes, int timeoutMillis) {
    ssize_t bytesRead = 0;
    struct timeval timeout;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(commPort, &readSet);

    timeout.tv_sec = timeoutMillis / 1000;
    timeout.tv_usec = (timeoutMillis % 1000) * 1000;

    int numFdsSet = select(commPort + 1, &readSet, nullptr, nullptr, &timeout);
    if (numFdsSet < 0) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Select() failed (" << logger.strerror() << ")" << endl;
    }
    else if (numFdsSet == 0) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Select() timed out" << endl;
    }
    else {
        if (FD_ISSET(commPort, &readSet)) {
            bytesRead = ::read(commPort, &buffer[index], requiredBytes);
            logger.log(VantageLogger::VANTAGE_DEBUG2) << "Read " << bytesRead << " bytes" << endl;
        }
    }

    return bytesRead;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SerialPort::discardInBuffer() {
    tcflush(commPort, TCIOFLUSH);
}
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::write(const string & s) {
    return write(s.c_str(), s.length());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::readBytes(byte buffer[], int requiredBytes, int timeoutMillis) {
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Attempting to read " << requiredBytes << " bytes" << endl;
    int readIndex = 0;
    
    //
    // Keep reading until the require number of bytes are read or the number of tries had been reached.
    // Note that the timeout can expire READ_TRIES times, so the total timeout delay can be up to
    // timeoutMillis * READ_TRIES.
    //
    for (int i = 0; i < READ_TRIES && readIndex < requiredBytes; i++) {
        int nbytes = this->read(buffer, readIndex, requiredBytes - readIndex, timeoutMillis);
        if (nbytes > 0) {
            readIndex += nbytes;
            logger.log(VantageLogger::VANTAGE_DEBUG2) << "Read " << readIndex << " bytes of " << requiredBytes << " bytes" << endl;
        }
        else if (nbytes < 0) {
            break;
        }
    }

    //
    // After all is done, check to see if the desired number of bytes were read
    //
    if (readIndex < requiredBytes) {
        this->discardInBuffer();
        logger.log(VantageLogger::VANTAGE_INFO) << "Failed to read requested bytes. Required=" << requiredBytes << ", Actual=" << readIndex << endl;
        return false;
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG3) << Weather::dumpBuffer(buffer, requiredBytes);
        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
SerialPort::setBaudRate(vws::BaudRate br) {
    baudRate = br;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
SerialPort::isOpen() const {
    return commPort != INVALID_HANDLE_VALUE;
}

}
