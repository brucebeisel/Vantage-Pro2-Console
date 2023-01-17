
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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <time.h>
#include <math.h>
#include <sstream>
#include <cstring>
#include "VantageConstants.h"
#include "HiLowPacket.h"
#include "LoopPacket.h"
#include "Loop2Packet.h"
#include "VantageCRC.h"
#include "BitConverter.h"
#include "ProtocolException.h"
#include "VantageWeatherStation.h"

using namespace std;

namespace vws {
static constexpr int protectedEepromBytes[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0x2d};
static constexpr int NUM_PROTECTED_EEPROM_BYTES = sizeof(protectedEepromBytes) / sizeof(protectedEepromBytes[0]);
//
// Wakeup command/response
//
static const std::string WAKEUP_COMMAND = std::string(1, VantageConstants::LINE_FEED);
static const std::string WAKEUP_RESPONSE = std::string(1, VantageConstants::LINE_FEED) + std::string(1, VantageConstants::CARRIAGE_RETURN);

//
// Testing Commands
//
static const std::string TEST_CMD = "TEST";                          // Sends the string "TEST\n" back
static const byte WRD_BYTE1 = 0x12;
static const byte WRD_BYTE2 = 0x4D;
static const std::string STATION_TYPE_CMD = "WRD";                   // Responds with the weather station type. This command is backward compatible with earlier Davis weather products.
static const std::string RECEIVE_CHECK_CMD = "RXCHECK";              // Console diagnostics report
static const std::string RXTEST_CMD = "RXTEST";                      // Move console to main current conditions screen
static const std::string FIRMWARE_DATE_CMD = "VER";                  // Firmware date
static const std::string RECEIVER_LIST_CMD = "RECEIVERS";            // Get the list of receivers as a bitmap, bit 0 represents station ID 1
static const std::string FIRMWARE_VERSION_CMD = "NVER";              // Get the firmware version

//
// Current Data Commands
//
static const std::string LOOP_CMD = "LOOP";                          // Get the current data values, alarms, battery status, etc. through the LOOP packet. Note that the LPS command renders this superfluous.
static const std::string LPS_CMD = "LPS 3";                          // Get the current values through both the LOOP and LOOP2 packets
static const std::string HIGH_LOW_CMD = "HILOWS";                    // Get the high and low that includes daily, monthly and yearly
static const std::string PUT_YEARLY_RAIN_CMD = "PUTRAIN";            // Set the yearly rainfall
static const std::string PUT_YEARLY_ET_CMD = "PUTET";                // Set the yearly ET

//
// Download Commands
//
static const std::string DUMP_ARCHIVE_CMD = "DMP";                   // Dump the entire archive
static const std::string DUMP_AFTER_CMD = "DMPAFT";                  // Dump the archive after a given date/time

//
// EEPROM Commands
//
static const std::string DUMP_EEPROM_CMD = "GETEE";                  // Read the entire EEPROM data block
static const std::string WRITE_EEPROM_CMD = "EEWR";                  // Write a single byte to EEPROM as hex strings
static const std::string READ_EEPROM_CMD = "EERD";                   // Read EEPROM address as hex strings
static const std::string WRITE_EEPROM_AS_BINARY_CMD = "EEBWR";       // Write to EEPROM as binary
static const std::string READ_EEPROM_AS_BINARY_CMD = "EEBRD";        // Read EEPROM address as binary

//
// Calibration Commands
//
//static const std::string CALIBRATE_TEMPERATURE_HUMIDITY = "CALED";   // Send temperature and humidity calibration values
//static const std::string CALIBRATE_TEMPERATURE_HUMIDITY2 = "CALFIX"; // Updates the display when calibration numbers have changed
static const std::string SET_BAROMETRIC_DATA_CMD = "BAR=";            // Sets barometric offset using local reading and/or elevation
//static const std::string SET_BAROMETRIC_CAL_DATA_CMD = "BARDATA";    // Get the current barometer calibration parameters

//
// Clearing Commands
//
static const std::string CLEAR_ARCHIVE_CMD = "CLRLOG";               // Clear the archived data
static const std::string CLEAR_ALARM_THRESHOLDS_CMD = "CLRALM";      // Clear the alarm thresholds
static const std::string CLEAR_TEMP_HUMID_CAL_CMD = "CLRCAL";        // Set temperature and humidity calibration offsets to zero
static const std::string CLEAR_GRAPH_POINTS_CMD = "CLRGRA";          // Clear the graph points
static const std::string CLEAR_CUMULATIVE_VALUE_CMD = "CLRVAR";      // Clear the specified cumulative value
static const std::string CLEAR_HIGH_VALUES_CMD = "CLRHIGHS";         // Clear the daily, monthly or yearly high values
static const std::string CLEAR_LOW_VALUES_CMD = "CLRLOWS";           // Clear the daily, monthly or yearly low values
static const std::string CLEAR_ACTIVE_ALARMS_CMD = "CLRBITS";        // Clear active alarms
static const std::string CLEAR_CURRENT_DATA_VALUES_CMD = "CLRDATA";  // Clear all current data values

//
// Configuration Commands
//
static const std::string SET_BAUD_RATE_CMD = "BAUD";                 // Sets the console to a new baud rate. Valid values are 1200, 2400, 4800, 9600, 14400 and 19200
static const std::string SET_TIME_CMD = "SETTIME";                   // Sets the time and date on the console
static const std::string GET_TIME_CMD = "GETTIME";                   // Retrieves the current time and date on the Vantage console. Data is sent in binary format
static const std::string SET_ARCHIVE_PERIOD_CMD = "SETPER";          // Set how often the console saves an archive record
static const std::string STOP_ARCHIVING_CMD = "STOP";                // Disables the creation of archive records
static const std::string START_ARCHIVING_CMD = "START";              // Enables the create of archive records
static const std::string REINITIALIZE_CMD = "NEWSETUP";              // Reinitialize the console after making any significant changes to the console's configuration
static const std::string CONTROL_LAMP_CMD = "LAMPS";                 // Turn on/off the console's light

//
// Dump/Dump After responses
//
static const std::string DMP_SEND_NEXT_PAGE = std::string(1, VantageConstants::ACK);
static const std::string DMP_CANCEL_DOWNLOAD = std::string(1, VantageConstants::ESCAPE);
static const std::string DMP_RESEND_PAGE = std::string(1, VantageConstants::NACK);

//
// Generic strings for various command protocols
//
static const std::string COMMAND_TERMINATOR = std::string(1, VantageConstants::LINE_FEED);
static const std::string RESPONSE_FRAME = std::string(1, VantageConstants::LINE_FEED) + std::string(1, VantageConstants::CARRIAGE_RETURN);
static const std::string COMMAND_RECOGNIZED_RESPONSE = RESPONSE_FRAME + "OK" + RESPONSE_FRAME;
static const std::string DONE_RESPONSE = "DONE" + std::string(1, VantageConstants::LINE_FEED) + std::string(1, VantageConstants::CARRIAGE_RETURN);
static const std::string TEST_RESPONSE = "TEST" + std::string(1, VantageConstants::LINE_FEED) + std::string(1, VantageConstants::CARRIAGE_RETURN);

const string STATION_TYPE_STRINGS[] = { "Vantage Pro2", "Vantage Vue", "Unknown" };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::VantageWeatherStation(const string & portName, int baudRate) :
                                            serialPort(portName, baudRate),
                                            stationType(VANTAGE_PRO_2),
                                            consoleBatteryVoltage(0.0),
                                            baudRate(baudRate),
                                            archivePeriod(0),
                                            windSensorStationId(0),
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
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Attempting to wakeup console" << endl;
        serialPort.write(WAKEUP_COMMAND);
      
        //
        // After sending the wakeup command the console will respond with <LF><CR>
        //
        if (serialPort.read(buffer, 2) && buffer[0] == WAKEUP_RESPONSE[0] && buffer[1] == WAKEUP_RESPONSE[1]) {
            awake = true;
            logger.log(VantageLogger::VANTAGE_INFO) << "Console is awake" << endl;
        }
        else {
            serialPort.discardInBuffer();
            Weather::sleep(WAKEUP_WAIT);
        }
    }

    return awake;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveConfigurationData() {
    if (!wakeupStation())
        return false;

    if (!eepromBinaryRead(VantageConstants::EE_ARCHIVE_PERIOD_ADDRESS, 1))
        return false;

    archivePeriod = BitConverter::toInt8(buffer, 0);

    logger.log(VantageLogger::VANTAGE_INFO) << "Configuration Data: " <<  " Archive Period: " << archivePeriod << endl;

    return true;
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

    if (serialPort.read(buffer, TEST_RESPONSE.length())) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "sendTestCommand() read failed while waiting for test response" << endl;
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

    sscanf(response.c_str(), "%d %d %d %d %d", &report.packetCount,
                                               &report.missedPacketCount,
                                               &report.syncCount,
                                               &report.maxPacketSequence,
                                               &report.crcErrorCount);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveStationType() {
    const char command[] = {STATION_TYPE_CMD[0], STATION_TYPE_CMD[1], STATION_TYPE_CMD[2], WRD_BYTE1, WRD_BYTE2, '\0'};

    if (!sendAckedCommand(command))
        return false;

    logger.log(VantageLogger::VANTAGE_INFO) << "Reading station type" << endl;

    byte stationTypeByte;
    if (!serialPort.read(&stationTypeByte, 1)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read station type" << endl;
        return false;
    }

    stationType = static_cast<StationType>(stationTypeByte);

    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieved station type of " << getStationTypeString() << endl;


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
VantageWeatherStation::retrieveFirmwareVersion(std::string * fwVersion) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving firmware version" << endl;
    if (!sendStringValueCommand(FIRMWARE_VERSION_CMD, firmwareVersion))
        return false;

    if (fwVersion != nullptr)
        *fwVersion = firmwareVersion;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveReceiverList(std::vector<StationId> * sensorStations) {

    if (!sendOKedCommand(RECEIVER_LIST_CMD))
       return false;

    byte stations;
    if (!serialPort.read(&stations, 1))
        return false;

    if (sensorStations != nullptr)
        sensorStations->clear();

    stationIds.clear();

    for (int i = 0; i < VantageConstants::MAX_STATION_ID; i++) {
        if (stations & (1 << i) != 0) {
            if (sensorStations != nullptr)
                sensorStations->push_back(i + 1);

            stationIds.push_back(i + 1);
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveFirmwareDate(std::string * fwDate) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving firmware date" << endl;

    if (!sendStringValueCommand(FIRMWARE_DATE_CMD, firmwareDate))
        return false;

    if (fwDate != nullptr)
        *fwDate = firmwareDate;

    return true;
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

    if (stationIds.size() == 0)
        logger.log(VantageLogger::VANTAGE_WARNING) << "Reading current values without any sensor stations connected" << endl;

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
                terminateLoop = !(*it)->processLoopPacket(loopPacket);
            }

            if (!terminateLoop && readLoop2Packet(loop2Packet)) {
                //
                // Send the LOOP2 packet to each listener
                //
                for (LoopPacketListenerList::iterator it = loopPacketListenerList.begin();
                     it != loopPacketListenerList.end();
                     ++it) {
                    terminateLoop = !(*it)->processLoop2Packet(loop2Packet);
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

    if (!serialPort.read(buffer, HILOW_PACKET_SIZE + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read response to HILOWS command" << endl;
        return false;
    }

    packet.decodeHiLowPacket(buffer);
    cout << "Hi/Low packet:" << endl << packet.formatXML() << endl;
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

    int argument = round(et * VantageConstants::MONTH_YEAR_ET_SCALE);

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

    if (sendAckedCommand(DUMP_ARCHIVE_CMD)) {
        for (int i = 0; i < NUM_ARCHIVE_PAGES; i++) {
            readNextArchivePage(list, 0, time(0));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::dumpAfter(DateTime time, vector<ArchivePacket> & list) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dumping archive after " << Weather::formatDateTime(time) << endl;
    list.clear();

    //
    // First send the dump after command and get an ACK back
    //
    if (!sendAckedCommand(DUMP_AFTER_CMD))
        return false;

    //
    // Next send the date with a checksum
    //
    struct tm tm;
    Weather::localtime(time, tm);
    int datestamp = tm.tm_mday + ((tm.tm_mon + 1) * 32) + ((tm.tm_year + 1900 - VANTAGE_YEAR_OFFSET) * 512);
    int timestamp = (tm.tm_hour * 100) + tm.tm_min;

    byte dateTimeBytes[TIME_LENGTH + CRC_BYTES];
    BitConverter::getBytes(datestamp, dateTimeBytes, 0, 2);
    BitConverter::getBytes(timestamp, dateTimeBytes, 2, 2);

    int crc = VantageCRC::calculateCRC(dateTimeBytes, TIME_LENGTH);
    BitConverter::getBytes(crc, dateTimeBytes, TIME_LENGTH, CRC_BYTES, false);

    if (!serialPort.write(dateTimeBytes, TIME_LENGTH + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Canceling DUMPAFT due to port write failure" << endl;
        return false;
    }

    //
    // Another ACK
    //
    if (!consumeAck()) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Canceling DUMPAFT due to CRC failure" << endl
                                        << Weather::dumpBuffer(dateTimeBytes, sizeof(dateTimeBytes));
        return false;
    }

    //
    // Now the console sends 4 bytes indicating the number of pages to be
    // dumped and which record in the first page is valid for the date specified
    //
    if (!serialPort.read(buffer, DUMP_AFTER_RESPONSE_LENGTH + CRC_BYTES)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Failed to read response to DMPAFT time data command" << endl;
        return false;
    }

    if (!VantageCRC::checkCRC(buffer, DUMP_AFTER_RESPONSE_LENGTH)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "DMPAFT response to time data failed CRC check" << endl;
        return false;
    }

    serialPort.write(string(1, VantageConstants::ACK));

    int numPages = BitConverter::toInt16(buffer, 0);
    int firstRecord = BitConverter::toInt16(buffer, 2);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dumping " << numPages << " archive pages. First record in page with new data = " << firstRecord << endl;

    if (numPages == 0)
        return true;

    return readAfterArchivePages(time, list, numPages, firstRecord);
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

    if (!serialPort.read(buffer, EEPROM_DATA_BLOCK_SIZE + CRC_BYTES) || !VantageCRC::checkCRC(buffer, EEPROM_DATA_BLOCK_SIZE))
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
        if (serialPort.read(this->buffer, EEPROM_READ_LINE_LENGTH) &&
            this->buffer[EEPROM_READ_LINE_LENGTH - 2] == VantageConstants::LINE_FEED &&
            this->buffer[EEPROM_READ_LINE_LENGTH - 1] == VantageConstants::CARRIAGE_RETURN) {

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

    if (!serialPort.read(buffer, count + CRC_BYTES) || !VantageCRC::checkCRC(buffer, count))
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
        if (protectedEepromBytes[i] == address)
            return false;
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
    // TODO check protected EEPROM bytes
    //
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
VantageWeatherStation::updateElevationAndBarometerOffset(int elevationFeet, double baroOffsetInHg) {
    ostringstream command;
    command << SET_BAROMETRIC_DATA_CMD << static_cast<int>(baroOffsetInHg * 1000.0) << " " << elevationFeet;

    //
    // TBD This is the one "OK" response command that can also receive a NACK response.
    return sendOKedCommand(command.str());
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
    return sendOKedWithDoneCommand(CLEAR_TEMP_HUMID_CAL_CMD);
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
VantageWeatherStation::clearCumulativeValue(VantageConstants::CumulativeValue cumValue) {
    ostringstream ss;

    ss << CLEAR_CUMULATIVE_VALUE_CMD << " " << static_cast<int>(cumValue);

    return sendAckedCommand(ss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearHighValues(VantageConstants::ExtremePeriod period) {
    ostringstream command;
    command << CLEAR_HIGH_VALUES_CMD << " " << static_cast<int>(period);

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearLowValues(VantageConstants::ExtremePeriod period) {
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
VantageWeatherStation::updateBaudRate(VantageConstants::BaudRate baudRate) {
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
    if (!sendAckedCommand(SET_TIME_CMD))
        return false;

    time_t now = time(0);
    struct tm tm;
    Weather::localtime(now, tm);
    logger.log(VantageLogger::VANTAGE_INFO) << "Setting console time to " << Weather::formatDateTime(now) << endl;
    int n = 0;
    buffer[n++] = static_cast<byte>(tm.tm_sec);
    buffer[n++] = static_cast<byte>(tm.tm_min);
    buffer[n++] = static_cast<byte>(tm.tm_hour);
    buffer[n++] = static_cast<byte>(tm.tm_mday);
    buffer[n++] = static_cast<byte>(tm.tm_mon + 1);
    buffer[n] = static_cast<byte>(tm.tm_year);

    int crc = VantageCRC::calculateCRC(buffer, SET_TIME_LENGTH);
    BitConverter::getBytes(crc, buffer, SET_TIME_LENGTH, CRC_BYTES, false);

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
VantageWeatherStation::retrieveConsoleTime(DateTime & stationTime) {
    bool success = true;
    stationTime = 0;

    if (!sendAckedCommand(GET_TIME_CMD))
        return false;

    if (serialPort.read(buffer, TIME_RESPONSE_LENGTH + CRC_BYTES)) {
        if (VantageCRC::checkCRC(buffer, TIME_RESPONSE_LENGTH)) {
            time_t now = time(0);
            struct tm tm;
            Weather::localtime(now, tm);
            int n = 0;
            tm.tm_sec = buffer[n++];
            tm.tm_min = buffer[n++];
            tm.tm_hour = buffer[n++];
            tm.tm_mday = buffer[n++];
            tm.tm_mon = buffer[n++] - 1;
            tm.tm_year = buffer[n];
            stationTime = mktime(&tm);
        }
        else {
            logger.log(VantageLogger::VANTAGE_WARNING) << "Received time failed CRC check" << endl;
            success = false;
        }
    }
    else {
        logger.log(VantageLogger::VANTAGE_WARNING) << "Failed to read time from console" << endl;
        success = false;
    }

    if (!success)
        wakeupStation();

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateArchivePeriod(VantageConstants::ArchivePeriod period) {
    ostringstream command;
    command << SET_ARCHIVE_PERIOD_CMD << " " << static_cast<int>(period);
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating archive period to: " << period << endl;

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::startArchiving() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Starting to archive" << endl;
    return sendAckedCommand(START_ARCHIVING_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::stopArchiving() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Stopping archiving" << endl;
    return sendAckedCommand(STOP_ARCHIVING_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::initializeSetup() {
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


/*
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const vector<Sensor> &
VantageStation::getSensors() const {
    return sensors;
}
 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
VantageWeatherStation::calculateStationReceptionPercentage(int archivePacketWindSamples) const {
    static const int stationId = 1;

    float archivePeriodSeconds = archivePeriod * 60.0F;
    float stationIndex = stationId - 1.0F;
    int maxPackets = static_cast<int>(archivePeriodSeconds / ((41.0F + stationIndex) / 16.0F));

    int stationReception = (archivePacketWindSamples * 100) / maxPackets;
    if (stationReception > MAX_STATION_RECEPTION)
        stationReception = MAX_STATION_RECEPTION;

    return stationReception;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const std::string &
VantageWeatherStation::getStationTypeString() const {
    std::string stationTypeName;

    if (stationType == VANTAGE_PRO_2)
        return STATION_TYPE_STRINGS[0];
    else if (stationType == VANTAGE_VUE)
        return STATION_TYPE_STRINGS[1];
    else
        return STATION_TYPE_STRINGS[2];
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readLoopPacket(LoopPacket & loopPacket) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading LOOP Packet" << endl;

    //
    // Read and decode the LOOP packet
    //
    if (!serialPort.read(buffer, LoopPacket::LOOP_PACKET_SIZE))
        return false;

    if (!loopPacket.decodeLoopPacket(buffer))
        return false;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "LOOP packet read successfully" << endl;
    return true;

    //
    // First time through determine what sensors are attached to the weather station based on the valid data in
    // the LOOP packet.
    //
    //if (!firstLoopPacketReceived) {
    //    firstLoopPacketReceived = true;
    //    //Sensor::detectSensors(loopPacket, sensors);
    //}

    //
    // Save the battery voltage of the console
    //
    //consoleBatteryVoltage = loopPacket.getConsoleBatteryVoltage();

    //
    // Pull out the battery status for the sensor stations
    //
    //for (vector<SensorStation>::iterator it = sensorStations.begin(); it != sensorStations.end(); ++it) {
    //    it->setBatteryStatus(loopPacket.isTransmitterBatteryGood(it->getSensorIndex()));
    //}

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readLoop2Packet(Loop2Packet & loop2Packet) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Reading LOOP2 Packet" << endl;

    //
    // Read and decode LOOP2 packet
    //
    if (!serialPort.read(buffer, Loop2Packet::LOOP2_PACKET_SIZE))
        return false;

    if (!loop2Packet.decodeLoop2Packet(buffer))
        return false;

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "LOOP2 packet read successfully" << endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readAfterArchivePages(DateTime afterTime, vector<ArchivePacket> & list, int firstRecordInFirstPageToProcess, int numPages) {
    DateTime newestPacketTime = afterTime;
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
            newestPacketTime = list.at(list.size() - 1).getDateTime();

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
        logger.log(VantageLogger::VANTAGE_INFO) << "Received " << list.size() << " records from DMPAFT " << Weather::formatDateTime(afterTime) << endl;
    else {
        logger.log(VantageLogger::VANTAGE_WARNING) << "DMPAFT " << Weather::formatDateTime(afterTime) << " failed" << endl;
        wakeupStation();
    }

    return success;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::readNextArchivePage(vector<ArchivePacket> & list, int firstRecordInPageToProcess, DateTime newestPacketTime) {
    bool success = true;
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Processing archive page. Newest packet time = " << Weather::formatDateTime(newestPacketTime) << endl;

    //
    // Try to read the page. Will attempt 3 tries to correct CRC errors.
    //
    for (int i = 0; i < ARCHIVE_PAGE_READ_RETRIES; i++) {
        if (serialPort.read(buffer, ARCHIVE_PAGE_SIZE + CRC_BYTES)) {
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
VantageWeatherStation::decodeArchivePage(vector<ArchivePacket> & list, const byte * buffer, int firstRecordInPageToProcess, DateTime newestPacketTime) {
    int recordCount = 0;

    //
    // Which page this is in a DMP or DMPAFT command
    //
    int pageSequence = BitConverter::toInt8(buffer, 0);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Decoding archive page " << pageSequence << ". Newest packet time = " << Weather::formatDateTime(newestPacketTime) << endl;

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
            if (packet.getDateTime() > newestPacketTime) {
                list.push_back(packet);
                recordCount++;
            }
            else
                logger.log(VantageLogger::VANTAGE_DEBUG1) << "Skipping archive record " << i << " in page " << pageSequence << " with date " << Weather::formatDateTime(packet.getDateTime()) << endl;
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
        else if (!serialPort.read(buffer, COMMAND_RECOGNIZED_RESPONSE.length()))
            success = false;
        else if (COMMAND_RECOGNIZED_RESPONSE != buffer)
            success = false;
        else
            success = true;

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
    // This may require a loop due to the console's delay between the "OK" and the "DONE".
    // The serial port class only waits for 2 seconds for data before returning an error.
    //
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Waiting for 'DONE' to complete the command" << endl;
    if (!serialPort.read(buffer, DONE_RESPONSE.length()))
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
    while (serialPort.read(&b, 1)) {
        if (b != VantageConstants::LINE_FEED && b != VantageConstants::CARRIAGE_RETURN)
            results.append(1, b);
        else if (b == VantageConstants::LINE_FEED) {
            if (serialPort.read(&b, 1) && b == VantageConstants::CARRIAGE_RETURN) {
                success = true;
                break;
            }
        }
    }

    if (!success)
        wakeupStation();

    return success;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::consumeAck() {
    bool rv = true;
    byte b;

    if (!serialPort.read(&b, 1)) {
        logger.log(VantageLogger::VANTAGE_INFO) << "consumeACK() read failed while consuming ACK" << endl;
        rv = false;
    }
    else if (b == VantageConstants::CRC_FAILURE) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() received a CRC failure response" << endl;
        rv = false;
    }
    else if (b == VantageConstants::NACK) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() received a NACK response" << endl;
        rv = false;
    }
    else if (b != VantageConstants::ACK) {
        logger.log(VantageLogger::VANTAGE_WARNING) << "consumeACK() read " << hex << static_cast<int>(b) << dec << " not an ACK" << endl;
        rv = false;
    }

    return rv;
}

}
