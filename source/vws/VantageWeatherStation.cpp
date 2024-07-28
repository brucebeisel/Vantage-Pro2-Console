/* 
 * Copyright (C) 2024 Bruce Beisel
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

#include "VantageWeatherStation.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <time.h>
#include <math.h>
#include <string.h>
#include <sstream>
#include <cstring>
#include <regex>
#include <iterator>
#include "VantageProtocolConstants.h"
#include "VantageEepromConstants.h"
#include "HiLowPacket.h"
#include "LoopPacket.h"
#include "Loop2Packet.h"
#include "CalibrationAdjustmentsPacket.h"
#include "VantageCRC.h"
#include "BitConverter.h"
#include "SerialPort.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "Weather.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

static constexpr int protectedEepromBytes[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0x2d};
static constexpr int NUM_PROTECTED_EEPROM_BYTES = sizeof(protectedEepromBytes) / sizeof(protectedEepromBytes[0]);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::VantageWeatherStation(SerialPort & serialPort) : serialPort(serialPort),
                                                                        archivePeriodMinutes(0),
                                                                        consoleType(VANTAGE_PRO_2),
                                                                        logger(VantageLogger::getLogger("VantageWeatherStation")) {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::~VantageWeatherStation() {
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::addLoopPacketListener(LoopPacketListener & listener) {
    loopPacketListenerList.push_back(&listener);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::removeLoopPacketListener(LoopPacketListener & listener) {
    for (LoopPacketListenerList::iterator it = loopPacketListenerList.begin();
         it != loopPacketListenerList.end();
         ++it) {
        if (*it == &listener) {
            loopPacketListenerList.erase(it);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::openStation() {
    return serialPort.open();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::closeStation() {
    serialPort.close();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::wakeupStation() {
    bool awake = false;

    for (int i = 0; i < WAKEUP_TRIES && !awake; i++) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_DEBUG1) << "Attempting to wakeup console" << endl;
        serialPort.write(WAKEUP_COMMAND);
      
        //
        // After sending the wakeup command the console will respond with <LF><CR>
        //
        if (serialPort.readBytes(buffer, 2) && buffer[0] == WAKEUP_RESPONSE[0] && buffer[1] == WAKEUP_RESPONSE[1]) {
            awake = true;
            logger.log(VantageLogger::VantageLogger::VANTAGE_INFO) << "Console is awake" << endl;
        }
        else {
            serialPort.discardInBuffer();
            Weather::sleep(WAKEUP_WAIT);
        }
    }

    return awake;
}

//
// Testing Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendTestCommand() {
    serialPort.write(TEST_CMD);
    serialPort.write(COMMAND_TERMINATOR);

    if (serialPort.readBytes(buffer, TEST_RESPONSE.length())) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "sendTestCommand() read failed while waiting for test response" << endl;
        return false;
    }

    if (TEST_RESPONSE != buffer) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "sendTestCommand() received unexpected test response: '" << buffer << "'" << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveConsoleDiagnosticsReport(ConsoleDiagnosticReport & report) {
    string response;

    if (!sendStringValueCommand(RECEIVE_CHECK_CMD, response))
        return false;

    if (sscanf(response.c_str(), "%d %d %d %d %d", &report.packetCount,
                                                   &report.missedPacketCount,
                                                   &report.syncCount,
                                                   &report.maxPacketSequence,
                                                   &report.crcErrorCount) != 5) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Console diagnostic report did not receive 5 tokens. Response: " << response << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveConsoleType(string * consoleTypeString) {
    const char command[] = {STATION_TYPE_CMD[0], STATION_TYPE_CMD[1], STATION_TYPE_CMD[2], WRD_BYTE1, WRD_BYTE2, '\0'};

    if (!sendAckedCommand(command))
        return false;

    logger.log(VantageLogger::VANTAGE_INFO) << "Reading console type" << endl;

    byte stationTypeByte;
    if (!serialPort.readBytes(&stationTypeByte, 1)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read console type" << endl;
        return false;
    }

    consoleType = static_cast<ConsoleType>(stationTypeByte);

    if (consoleTypeString != NULL)
        *consoleTypeString = consoleTypeEnum.valueToString(consoleType);

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieved console type of " << consoleTypeEnum.valueToString(consoleType) << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::performReceiveTest() {
    //
    // There is no documentation in the serial protocol document regarding this
    // command. It does not state if a command terminator must be added or if
    // there is any response.
    //
    serialPort.write(RXTEST_CMD);
    serialPort.write(COMMAND_TERMINATOR);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveFirmwareVersion(std::string & firmwareVersion) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving firmware version" << endl;
    return sendStringValueCommand(FIRMWARE_VERSION_CMD, firmwareVersion);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveReceiverList(std::vector<StationId> & sensorStations) {

    if (!sendOKedCommand(RECEIVER_LIST_CMD))
       return false;

    byte stations;
    if (!serialPort.readBytes(&stations, 1))
        return false;

    sensorStations.clear();

    for (int i = 0; i < ProtocolConstants::MAX_STATION_ID; i++) {
        if ((stations & (1 << i)) != 0)
            sensorStations.push_back(i + 1);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveFirmwareDate(std::string & firmwareDate) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving firmware date" << endl;

    return sendStringValueCommand(FIRMWARE_DATE_CMD, firmwareDate);
}

//
// End of Testing Commands
//

//
// Current Data Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::currentValuesLoop(int records) {
    LoopPacket loopPacket;
    Loop2Packet loop2Packet;
    bool terminateLoop = false;
    bool resetNeeded = false;

    ostringstream command;
    command << LPS_CMD << " " << (records * 2);

    if (!sendAckedCommand(command.str()))
        return;

    for (int i = 0; i < records && !terminateLoop && !resetNeeded; i++) {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading LOOP and LOOP2 Packets ---------------------------------" << endl;
        //
        // Loop packet comes first
        //
        if (readLoopPacket(loopPacket)) {
            //
            // Send the LOOP packet to each listener
            //
            for (LoopPacketListenerList::iterator it = loopPacketListenerList.begin();
                                                  it != loopPacketListenerList.end();
                                                ++it) {
                terminateLoop = terminateLoop || !(*it)->processLoopPacket(loopPacket);
            }

            if (!terminateLoop && readLoop2Packet(loop2Packet)) {
                //
                // Send the LOOP2 packet to each listener
                //
                for (LoopPacketListenerList::iterator it = loopPacketListenerList.begin();
                                                      it != loopPacketListenerList.end();
                                                    ++it) {
                    terminateLoop = terminateLoop || !(*it)->processLoop2Packet(loop2Packet);
                }
            }
            else
                resetNeeded = true;
        }
        else
            resetNeeded = true;
    }

    //
    // If the callback wants to terminated the loop early or there was a problem use the Wakeup sequence to terminate the loop
    // See the LPS command in the Vantage Pro2, Vue Serial Protocol document
    //
    if (terminateLoop || resetNeeded)
        wakeupStation();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveLoopPacket(LoopPacket & loopPacket) {
    ostringstream command;
    command << LOOP_CMD << " 1";

    if (!sendAckedCommand(command.str()))
        return false;

    if (!readLoopPacket(loopPacket)) {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveHiLowValues(HiLowPacket & packet) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Retrieving Hi/Low packet" << endl;

    if (!sendAckedCommand(HIGH_LOW_CMD))
        return false;

    if (!serialPort.readBytes(buffer, HILOW_PACKET_SIZE + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read response to HILOWS command" << endl;
        return false;
    }

    packet.decodeHiLowPacket(buffer);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::putYearlyRain(Rainfall rain) {
    ostringstream ss;

    int argument = round(rain * 100.0);

    ss << PUT_YEARLY_RAIN_CMD << " " << argument;

    return sendAckedCommand(ss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::putYearlyET(Evapotranspiration et) {
    ostringstream ss;

    int argument = round(et * MONTH_YEAR_ET_SCALE);

    ss << PUT_YEARLY_ET_CMD << " " << argument;

    return sendAckedCommand(ss.str());
}
//
// End Current Data Commands
//

//
// Download Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::dump(vector<ArchivePacket> & list) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Dumping archive..." << endl;
    list.clear();
    list.reserve(NUM_ARCHIVE_RECORDS);

    if (sendAckedCommand(DUMP_ARCHIVE_CMD)) {
        DateTimeFields zeroTime;
        for (int i = 0; i < NUM_ARCHIVE_PAGES; i++) {
            readNextArchivePage(list, 0, zeroTime);
            if (!serialPort.write(DMP_SEND_NEXT_PAGE)) {
                break;
            }
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::dumpAfter(const DateTimeFields & time, vector<ArchivePacket> & list) {
    int year, month, monthDay, hour, minute;

    //
    // If the provided time is not a valid time, then just use 2000-01-01 00:00
    //
    if (time.isDateTimeValid()) {
        year = time.getYear();
        month = time.getMonth();
        monthDay = time.getMonthDay();
        hour = time.getHour();
        minute = time.getMinute();
    }
    else {
        year = 2000;
        month = 1;
        monthDay = 1;
        hour = 0;
        minute = 0;
    }


    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dumping archive after " << time << endl;
    list.clear();

    //
    // First send the dump after command and get an ACK back
    //
    if (!sendAckedCommand(DUMP_AFTER_CMD))
        return false;

    //
    // Next send the date with a checksum
    //
    int datestamp = monthDay + (month * 32) + ((year - VANTAGE_YEAR_OFFSET) * 512);
    int timestamp = (hour * 100) + minute;

    byte dateTimeBytes[TIME_LENGTH + CRC_BYTES];
    BitConverter::getBytes(datestamp, dateTimeBytes, 0, 2);
    BitConverter::getBytes(timestamp, dateTimeBytes, 2, 2);

    int crc = VantageCRC::calculateCRC(dateTimeBytes, TIME_LENGTH);
    BitConverter::getBytes(crc, dateTimeBytes, TIME_LENGTH, CRC_BYTES, false);

    if (!serialPort.write(dateTimeBytes, TIME_LENGTH + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Canceling DMPAFT due to port write failure" << endl;
        return false;
    }

    //
    // Another ACK
    //
    if (!consumeAck()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Canceling DMPAFT due to CRC failure" << endl
                                        << Weather::dumpBuffer(dateTimeBytes, sizeof(dateTimeBytes));
        return false;
    }

    //
    // Now the console sends 4 bytes indicating the number of pages to be
    // dumped and which record in the first page is valid for the date specified
    //
    if (!serialPort.readBytes(buffer, DUMP_AFTER_RESPONSE_LENGTH + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read response to DMPAFT time data command" << endl;
        return false;
    }

    if (!VantageCRC::checkCRC(buffer, DUMP_AFTER_RESPONSE_LENGTH)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "DMPAFT response to time data failed CRC check" << endl;
        return false;
    }

    serialPort.write(string(1, ProtocolConstants::ACK));

    int numPages = BitConverter::toInt16(buffer, 0);
    int firstRecord = BitConverter::toInt16(buffer, 2);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dumping " << numPages << " archive pages. First record in page with new data = " << firstRecord << endl;

    if (numPages == 0)
        return true;

    return readAfterArchivePages(time, list, firstRecord, numPages);
}

//
// EEPROM Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromReadDataBlock(byte buffer[]) {
    if (!sendAckedCommand(DUMP_EEPROM_CMD))
        return false;

    if (!serialPort.readBytes(buffer, EEPROM_DATA_BLOCK_SIZE + CRC_BYTES) || !VantageCRC::checkCRC(buffer, EEPROM_DATA_BLOCK_SIZE))
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromRead(unsigned address, unsigned count) {
    ostringstream command;
    command << READ_EEPROM_CMD << " " << uppercase << hex << address << " " << count << nouppercase;
    if (!sendOKedCommand(command.str()))
        return false;

    //
    // Read four bytes at a time, each read will contain a 2 digit hex code and a <LF><CR> sequence.
    //
    for (int i = 0; i < count; i++) {
        if (serialPort.readBytes(this->buffer, EEPROM_READ_LINE_LENGTH) &&
            this->buffer[EEPROM_READ_LINE_LENGTH - 2] == ProtocolConstants::LINE_FEED &&
            this->buffer[EEPROM_READ_LINE_LENGTH - 1] == ProtocolConstants::CARRIAGE_RETURN) {

            int value = strtol(this->buffer, nullptr, 16);
            buffer[i] = value & BitConverter::ONE_BYTE_MASK;
        }
        else
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromBinaryRead(unsigned address, unsigned count, char * output) {
    ostringstream command;
    command << READ_EEPROM_AS_BINARY_CMD << " " << uppercase << hex << address << " " << count << nouppercase;

    if (!sendAckedCommand(command.str()))
        return false;

    if (!serialPort.readBytes(buffer, count + CRC_BYTES) || !VantageCRC::checkCRC(buffer, count))
        return false;

    if (output != nullptr)
        memcpy(output, buffer, count);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromWriteByte(unsigned address, byte value) {
    for (int i = 0; i < NUM_PROTECTED_EEPROM_BYTES; i++) {
        if (protectedEepromBytes[i] == address) {
            logger.log(VantageLogger::VANTAGE_ERROR) << "Skipping write to EEPROM address " << address << " because it is a protected byte" << endl;
            return false;
        }
    }

    ostringstream command;
    command << WRITE_EEPROM_CMD << " " << hex << address << " " << value;
    return sendOKedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool 
VantageWeatherStation::eepromBinaryWrite(unsigned address, const byte data[], unsigned count) {
    //
    // Check the address range against the protected bytes
    //
    for (int i = 0; i < count; i++) {
        unsigned addressToCheck = address + i;
        for (int i = 0; i < NUM_PROTECTED_EEPROM_BYTES; i++) {
            if (protectedEepromBytes[i] == addressToCheck) {
                logger.log(VantageLogger::VANTAGE_ERROR) << "Skipping write to EEPROM address " << address
                                                         << " with size " << count << " because overlaps at least one protected byte" << endl;
                return false;
            }
        }
    }

    ostringstream command;
    command << WRITE_EEPROM_AS_BINARY_CMD << " " << uppercase << hex << address << " " << count << nouppercase;

    if (!sendAckedCommand(command.str()))
        return false;

    byte writeBuffer[EEPROM_DATA_BLOCK_SIZE + CRC_BYTES];
    memcpy(writeBuffer, data, count);

    int crc = VantageCRC::calculateCRC(data, count);

    BitConverter::getBytes(crc, writeBuffer, count, CRC_BYTES, false);

    return serialPort.write(writeBuffer, count + CRC_BYTES);
}

//
// Calibration Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveCalibrationAdjustments(CalibrationAdjustmentsPacket & calibrationPacket) {
    if (!eepromBinaryRead(VantageEepromConstants::EE_INSIDE_TEMP_CAL_ADDRESS, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE, buffer))
        return false;

    return calibrationPacket.decodePacket(buffer, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateCalibrationAdjustments(const CalibrationAdjustmentsPacket & calibrationAdjustments) {
    byte buffer[CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE];

    if (!calibrationAdjustments.encodePacket(buffer, sizeof(buffer))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Calibration adjustment encoding failed" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Calibration Adjustment buffer: " << Weather::dumpBuffer(buffer, sizeof(buffer)) << endl;

    if (!eepromBinaryWrite(VantageEepromConstants::EE_INSIDE_TEMP_CAL_ADDRESS, buffer, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE))
        return false;
    else
        return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateBarometerReadingAndElevation(Pressure baroReadingInHg, int elevationFeet) {
    ostringstream command;
    command << SET_BAROMETRIC_DATA_CMD << static_cast<int>(baroReadingInHg * BAROMETER_SCALE) << " " << elevationFeet;

    //
    // This is the one "OK" response command that can also receive a NACK response.
    // If a NACK response is received sendOKedCommand() will return false because it did not
    // the proper OK response. Not sure what would be done differently if the NACK was handled
    // specifically.
    //
    return sendOKedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveBarometerCalibrationParameters(BarometerCalibrationParameters & baroCalParams) {
    static constexpr int NUM_LINES = 9;

    if (!sendOKedCommand(GET_BAROMETRIC_CAL_DATA_CMD))
        return false;

    //
    // This command returns a long string that contains 9 <CR><LF> sequences with preceding text.
    // Continue reading from the serial port until there is no more data.
    //
    byte buffer[1024];
    int offset = 0;
    int bytesRead;
    do {
        bytesRead = serialPort.read(buffer, offset, sizeof(buffer), 500);
        offset += bytesRead;
    }
    while(bytesRead > 0);

    buffer[offset] = '\0';
    string response(buffer);

    regex lineRegex("[^\\n]+\n\r");

    auto linesBegin = sregex_iterator(response.begin(), response.end(), lineRegex);
    auto linesEnd = sregex_iterator();

    int numLines = distance(linesBegin, linesEnd);
    if (numLines != NUM_LINES) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Received the wrong number of line from BARDATA command. Expected: " << NUM_LINES << " Got: " << numLines << endl;
        return false;
    }

    int linesProcessed = 0;
    for (std::sregex_iterator it = linesBegin; it != linesEnd; ++it) {
        string lineString = (*it).str();
        const char *line = lineString.c_str();
        if (strncmp(line, "BAR ", strlen("BAR ")) == 0) {
            linesProcessed++;
            baroCalParams.recentMeasurement = atoi(&line[strlen("BAR ")]);
        }
        else if (strncmp(line, "ELEVATION", strlen("ELEVATION")) == 0) {
            linesProcessed++;
            baroCalParams.elevation = atoi(&line[strlen("ELEVATION")]);
        }
        else if (strncmp(line, "DEW POINT", strlen("DEW POINT")) == 0) {
            linesProcessed++;
            baroCalParams.dewPoint = atoi(&line[strlen("DEW POINT")]);
        }
        else if (strncmp(line, "VIRTUAL TEMP", strlen("VIRTUAL TEMP")) == 0) {
            linesProcessed++;
            baroCalParams.avgTemperature12Hour = atoi(&line[strlen("VIRTUAL TEMP")]);
        }
        else if (strncmp(line, "C", strlen("C")) == 0) {
            linesProcessed++;
            baroCalParams.humidityCorrectionFactor = atoi(&line[1]);
        }
        else if (strncmp(line, "R", strlen("R")) == 0) {
            linesProcessed++;
            baroCalParams.correctionRatio = atoi(&line[1]);
        }
        else if (strncmp(line, "BARCAL", strlen("BARCAL")) == 0) {
            linesProcessed++;
            baroCalParams.offsetCorrectionFactor = atoi(&line[strlen("BARCAL")]);
        }
        else if (strncmp(line, "GAIN", strlen("GAIN")) == 0) {
            linesProcessed++;
            baroCalParams.fixedGain = atoi(&line[strlen("GAIN")]);
        }
        else if (strncmp(line, "OFFSET", strlen("OFFSET")) == 0) {
            linesProcessed++;
            baroCalParams.fixedOffset = atoi(&line[strlen("OFFSET")]);
        }
        else {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Received invalid token in response to BARDATA: " << line << endl;
            return false;
        }
    }

    if (linesProcessed != NUM_LINES) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Processed the wrong number of lines from BARDATA command. Expected: " << NUM_LINES << " Got: " << linesProcessed << endl;
        return false;
    }

    return true;
}

//
// Clearing commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearArchive() {
    return sendAckedCommand(CLEAR_ARCHIVE_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearAlarmThresholds() {
    return sendOKedWithDoneCommand(CLEAR_ALARM_THRESHOLDS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets() {
    // The Vantage protocol document indicates that the leading <LF><CR>
    // is not sent in response to the CLRCAL command. This needs to be tested
    // to determine if this is an error in the document or an inconsistency
    // with the protocol
    // TODO This is commented out for now to avoid clearing offset needed for the weather station to run properly
    //return sendOKedWithDoneCommand(CLEAR_TEMP_HUMID_CAL_CMD);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearGraphPoints() {
    // See the comment in clearTemperatureHumidityCalibrationOffsets()
    return sendOKedWithDoneCommand(CLEAR_GRAPH_POINTS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearCumulativeValue(ProtocolConstants::CumulativeValue cumValue) {
    ostringstream ss;

    ss << CLEAR_CUMULATIVE_VALUE_CMD << " " << static_cast<int>(cumValue);

    return sendAckedCommand(ss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearHighValues(ProtocolConstants::ExtremePeriod period) {
    ostringstream command;
    command << CLEAR_HIGH_VALUES_CMD << " " << static_cast<int>(period);

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearLowValues(ProtocolConstants::ExtremePeriod period) {
    ostringstream command;
    command << CLEAR_LOW_VALUES_CMD << " " << static_cast<int>(period);

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearActiveAlarms() {
    return sendAckedCommand(CLEAR_ACTIVE_ALARMS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearCurrentData() {
    return sendAckedCommand(CLEAR_CURRENT_DATA_VALUES_CMD);
}

//
// End of Clearing Commands
//

//
// Configuration Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateBaudRate(BaudRate baudRate) {
    ostringstream command;
    command << SET_BAUD_RATE_CMD << " " << static_cast<int>(baudRate);

    //
    // First set the console's baud rate, then reopen the serial port with the
    // new baud rate
    // TBD The console can respond with a "NO" to indicate failure
    //
    if (sendOKedCommand(CLEAR_CURRENT_DATA_VALUES_CMD)) {
        serialPort.close();
        serialPort.setBaudRate(static_cast<int>(baudRate));
        serialPort.open();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateConsoleTime() {

    //
    // If the console time is close to the actual time, then don't set the time.
    // Note that the console has an undocumented feature, where setting the console's time
    // will reset the diagnostics counters. So the time delta check is meant to keep the
    // diagnostics counters from being reset.
    //
    time_t now = time(0);
    DateTimeFields currentStationTime;
    if (retrieveConsoleTime(currentStationTime)) {
        DateTime delta = abs(now - currentStationTime.getEpochDateTime());
        logger.log(VantageLogger::VANTAGE_INFO) << "Console time delta to actual time: " << delta << endl;
        if (delta < CONSOLE_TIME_DELTA_THRESHOLD_SECONDS) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Not setting console time because it is close to actual time" << endl;
            return true;
        }
    }

    struct tm tm;
    Weather::localtime(now, tm);
    logger.log(VantageLogger::VantageLogger::VANTAGE_INFO) << "Setting console time to " << Weather::formatDateTime(now) << endl;
    int n = 0;
    buffer[n++] = static_cast<byte>(tm.tm_sec);
    buffer[n++] = static_cast<byte>(tm.tm_min);
    buffer[n++] = static_cast<byte>(tm.tm_hour);
    buffer[n++] = static_cast<byte>(tm.tm_mday);
    buffer[n++] = static_cast<byte>(tm.tm_mon + 1);
    buffer[n] = static_cast<byte>(tm.tm_year);

    int crc = VantageCRC::calculateCRC(buffer, SET_TIME_LENGTH);
    BitConverter::getBytes(crc, buffer, SET_TIME_LENGTH, CRC_BYTES, false);

    if (!sendAckedCommand(SET_TIME_CMD))
        return false;

    bool success = false;
    if (serialPort.write(buffer, SET_TIME_LENGTH + CRC_BYTES)) {
        success = consumeAck();
    }
    else {
        success = false;
        wakeupStation();
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveConsoleTime(DateTimeFields & stationTime) {
    bool success = true;
    stationTime.resetDateTimeFields();

    if (!sendAckedCommand(GET_TIME_CMD))
        return false;

    if (serialPort.readBytes(buffer, TIME_RESPONSE_LENGTH + CRC_BYTES)) {
        if (VantageCRC::checkCRC(buffer, TIME_RESPONSE_LENGTH)) {
            int second = buffer[0];
            int minute = buffer[1];
            int hour = buffer[2];
            int mday = buffer[3];
            int month = buffer[4];
            int year = buffer[5] + Weather::TIME_STRUCT_YEAR_OFFSET;
            stationTime.setDateTime(year, month, mday, hour, minute, second);
        }
        else {
            logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "Received time failed CRC check" << endl;
            success = false;
        }
    }
    else {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "Failed to read time from console" << endl;
        success = false;
    }

    if (!success)
        wakeupStation();

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateArchivePeriod(ArchivePeriod period) {
    ostringstream command;
    command << SET_ARCHIVE_PERIOD_CMD << " " << static_cast<int>(period);
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating archive period to: " << static_cast<int>(period) << endl;

    //
    // Note that the Vantage protocol document claims this is an ACKed command, but it is really an OKed command
    //
    if (sendOKedCommand(command.str())) {
        archivePeriodMinutes = static_cast<int>(period);
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveArchivePeriod(ArchivePeriod & period) {

    if (!eepromBinaryRead(VantageEepromConstants::EE_ARCHIVE_PERIOD_ADDRESS, 1))
        return false;

    int archivePeriodValue = BitConverter::toUint8(buffer, 0);
    period = static_cast<ArchivePeriod>(archivePeriodValue);
    archivePeriodMinutes = archivePeriodValue;

    logger.log(VantageLogger::VantageLogger::VANTAGE_DEBUG1) <<  " Archive Period: " << archivePeriodValue << endl;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
VantageWeatherStation::getArchivePeriod() const {
    return archivePeriodMinutes;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::startArchiving() {
    //
    // Note that the Vantage serial protocol document does specify the command sequence
    // of the START command. The OK sequence was determined by trial and error.
    //
    logger.log(VantageLogger::VANTAGE_INFO) << "Starting to archive" << endl;
    return sendOKedCommand(START_ARCHIVING_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::stopArchiving() {
    //
    // Note that the Vantage serial protocol document does specify the command sequence
    // of the STOP command. The OK sequence was determined by trial and error.
    //
    logger.log(VantageLogger::VANTAGE_INFO) << "Stopping archiving" << endl;
    return sendOKedCommand(STOP_ARCHIVING_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::initializeSetup() {
    //
    // Note that an "R" will appear in the lower right corner of the console display to
    // indicate the console is initializing
    //
    logger.log(VantageLogger::VANTAGE_INFO) << "**************************" << endl;
    logger.log(VantageLogger::VANTAGE_INFO) << "* Reinitializing console *" << endl;
    logger.log(VantageLogger::VANTAGE_INFO) << "**************************" << endl;
    return sendAckedCommand(REINITIALIZE_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::controlConsoleLamp(bool on) {
    ostringstream command;
    command << CONTROL_LAMP_CMD << " " << (on ? "1" : "0");

    logger.log(VantageLogger::VANTAGE_INFO) << "Sending lamp command: " << (on ? "On" : "Off") << endl;
    return sendOKedCommand(command.str());
}

//
// EEPROM retrieval commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ConsoleType
VantageWeatherStation::getConsoleType() const {
    return consoleType;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readLoopPacket(LoopPacket & loopPacket) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading LOOP Packet" << endl;

    //
    // Read and decode the LOOP packet
    //
    if (!serialPort.readBytes(buffer, LoopPacket::LOOP_PACKET_SIZE))
        return false;

    if (!loopPacket.decodeLoopPacket(buffer))
        return false;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "LOOP packet read successfully" << endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readLoop2Packet(Loop2Packet & loop2Packet) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading LOOP2 Packet" << endl;

    //
    // Read and decode LOOP2 packet
    //
    if (!serialPort.readBytes(buffer, Loop2Packet::LOOP2_PACKET_SIZE))
        return false;

    if (!loop2Packet.decodeLoop2Packet(buffer))
        return false;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "LOOP2 packet read successfully" << endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readAfterArchivePages(const DateTimeFields & afterTime, vector<ArchivePacket> & list, int firstRecordInFirstPageToProcess, int numPages) {
    DateTimeFields newestPacketTime = afterTime;
    int firstRecordInPageToProcess = firstRecordInFirstPageToProcess;

    bool success = true;
    for (int i = 0; i < numPages; i++) {
        //
        // Process a single page. This will return 1 - 5 packets
        //
        if (!readNextArchivePage(list, firstRecordInPageToProcess, newestPacketTime)) {
            serialPort.write(DMP_CANCEL_DOWNLOAD);
            success = false;
            break;
        }

        //
        // Sometimes the last page of the dump contains zero records. We don't
        // need to save the newest time because we are at the end of the dump
        // anyway.
        //
        if (list.size() > 0)
            newestPacketTime = list.at(list.size() - 1).getDateTimeFields();

        if (!serialPort.write(DMP_SEND_NEXT_PAGE)) {
            success = false;
            break;
        }

        //
        // After the first page, the first valid record in each page is the first record
        //
        firstRecordInPageToProcess = 0;
    }

    if (success)
        logger.log(VantageLogger::VANTAGE_INFO) << "Received " << list.size() << " records from DMPAFT " << afterTime << endl;
    else {
        logger.log(VantageLogger::VANTAGE_WARNING) << "DMPAFT " << afterTime << " failed" << endl;
        wakeupStation();
    }

    return success;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readNextArchivePage(vector<ArchivePacket> & list, int firstRecordInPageToProcess, const DateTimeFields & newestPacketTime) {
    bool success = true;
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Processing archive page. Newest packet time = " << newestPacketTime << endl;

    //
    // Try to read the page. Will attempt 3 tries to correct CRC errors.
    //
    for (int i = 0; i < ARCHIVE_PAGE_READ_RETRIES; i++) {
        if (serialPort.readBytes(buffer, ARCHIVE_PAGE_SIZE + CRC_BYTES)) {
            if (VantageCRC::checkCRC(buffer, ARCHIVE_PAGE_SIZE)) {
                decodeArchivePage(list, buffer, firstRecordInPageToProcess, newestPacketTime);
                success = true;
                break;
            }
            else {
                logger.log(VantageLogger::VANTAGE_WARNING) << "CRC check failed on archive page. Try # " << (i + 1) << endl;
                serialPort.write(DMP_RESEND_PAGE);
                success = false;
            }
        }
        else {
            serialPort.write(DMP_CANCEL_DOWNLOAD);
            success = false;
            break;
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::decodeArchivePage(vector<ArchivePacket> & list, const byte * buffer, int firstRecordInPageToProcess, const DateTimeFields & newestPacketTime) {
    int recordCount = 0;

    //
    // Which page this is in a DMP or DMPAFT command
    //
    int pageSequence = BitConverter::toUint8(buffer, 0);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Decoding archive page " << pageSequence
                                              << ". Newest packet time = " << newestPacketTime << endl;

    //
    // The first record value may not be zero in the case of a dump after command. The first record after the specified time may not be at the
    // beginning of a page so the others must be skipped.
    //
    for (int i = firstRecordInPageToProcess; i < RECORDS_PER_ARCHIVE_PAGE; i++) {
        //
        // The record offset accounts for the page sequence byte and the previous records in the page
        //
        int recordOffset = 1 + (ArchivePacket::BYTES_PER_ARCHIVE_PACKET * i);
        if (ArchivePacket::archivePacketContainsData(buffer, recordOffset)) {
            ArchivePacket packet(buffer, recordOffset);
           
            //
            // In the case of a dump after command the last page may contain packets from the beginning of the circular
            // archive buffer. In this case the packets will have earlier dates than the last packet of the previous page
            // or the time of the DMPAFT command.
            //
            if (packet.getDateTimeFields() > newestPacketTime) {
                list.push_back(packet);
                recordCount++;
            }
            else
                logger.log(VantageLogger::VANTAGE_DEBUG1) << "Skipping archive record " << i << " in page " << pageSequence
                                                          << " with date " << packet.getPacketDateTimeString() << endl;
        }
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Page " << pageSequence << " contained " << recordCount << " records" << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendOKedCommand(const string & command) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Sending command '" << command << "' that expects an OK response" << endl;
    bool success = false;

    for (int i = 0; i < COMMAND_RETRIES && !success; i++) {
        if (!serialPort.write(command)) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to write command: '" << command << "'" << endl;
            success = false;
        }
        else if (!serialPort.write(COMMAND_TERMINATOR)) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to write command terminator" << endl;
            success = false;
        }
        else if (!serialPort.readBytes(buffer, COMMAND_RECOGNIZED_RESPONSE.length()))
            success = false;
        else {
            buffer[COMMAND_RECOGNIZED_RESPONSE.length()] = '\0';
            if (COMMAND_RECOGNIZED_RESPONSE != buffer)
                success = false;
            else
                success = true;
        }

        if (!success)
            wakeupStation();
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Command " << command << " status is " << success << endl;
    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendOKedWithDoneCommand(const string & command) {
    bool success = false;

    if (!sendOKedCommand(command))
        return false;

    //
    // The commands that receive "DONE" in the end can have a very long delay.
    // Tell the serial port to wait up to a minute for the "DONE" response.
    // This is acceptable because the console will not repond to any other
    // command until the command that requires the DONE response is complete.
    //
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Waiting for 'DONE' to complete the command" << endl;
    if (!serialPort.readBytes(buffer, DONE_RESPONSE.length(), 60000))
        success = false;
    else if (DONE_RESPONSE != buffer)
        success = false;
    else
        success = true;

    if (!success)
        wakeupStation();

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Command " << command << " final status is " << success << endl;
    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendAckedCommand(const string & command) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Sending command '" << command << "' that expects and ACK response" << endl;
    bool success = false;

    //
    // Try multiple times for completeness. If an ACK is not received then wake up the console and
    // try again.
    //
    for (int i = 0; i < COMMAND_RETRIES && !success; i++) {
        if (!serialPort.write(command)) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to write command: '" << command << "'" << endl;
            success = false;
        }
        else if (!serialPort.write(COMMAND_TERMINATOR)) {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to write command terminator" << endl;
            success = false;
            break;
        }
        else
            success = consumeAck();

        if (!success)
            wakeupStation();
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Command " << command << " status is " << success << endl;
    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendStringValueCommand(const string & command, string & results) {
    bool success = false;

    results.clear();

    if (!sendOKedCommand(command))
        return false;

    //
    // Read 1 byte at a time, appending to the string value until a CR or LF is detected.
    // The CR is the indicator that the command is complete.
    //
    byte b;
    while (serialPort.readBytes(&b, 1)) {
        if (b != ProtocolConstants::LINE_FEED && b != ProtocolConstants::CARRIAGE_RETURN)
            results.append(1, b);
        else if (b == ProtocolConstants::LINE_FEED) {
            if (serialPort.readBytes(&b, 1) && b == ProtocolConstants::CARRIAGE_RETURN) {
                success = true;
                break;
            }
        }
    }

    if (!success)
        wakeupStation();
    else
        logger.log(VantageLogger::VANTAGE_INFO) << "String Value command read string '" << results << "' " << endl;

    return success;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::consumeAck() {
    bool rv = true;
    byte b;

    if (!serialPort.readBytes(&b, 1)) {
        logger.log(VantageLogger::VANTAGE_INFO) << "consumeACK() read failed while consuming ACK" << endl;
        rv = false;
    }
    else if (b == ProtocolConstants::CRC_FAILURE) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() received a CRC failure response" << endl;
        rv = false;
    }
    else if (b == ProtocolConstants::NACK) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() received a NACK response" << endl;
        rv = false;
    }
    else if (b != ProtocolConstants::ACK) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() read 0x" << hex << static_cast<int>(b) << dec << " not an ACK" << endl;
        rv = false;
    }

    return rv;
}

}
