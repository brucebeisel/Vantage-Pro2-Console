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
#include "ConsoleDiagnosticReport.h"
#include "VantageCRC.h"
#include "BitConverter.h"
#include "SerialPort.h"
#include "VantageEnums.h"
#include "VantageLogger.h"
#include "Weather.h"
#include "LoopPacketListener.h"

using namespace std;

namespace vws {
using namespace ProtocolConstants;

static constexpr int protectedEepromBytes[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xf, 0x2d};
static constexpr int NUM_PROTECTED_EEPROM_BYTES = sizeof(protectedEepromBytes) / sizeof(protectedEepromBytes[0]);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::VantageWeatherStation(SerialPort & serialPort, int apm, Rainfall rcs) :
                                                                        serialPort(serialPort),
                                                                        archivePeriodMinutes(apm),
                                                                        consoleType(VANTAGE_PRO_2),
                                                                        rainCollectorSize(rcs),
                                                                        archivingActive(false),
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
    LoopPacketListenerList::iterator item = std::find(loopPacketListenerList.begin(), loopPacketListenerList.end(), &listener);

    if (item != loopPacketListenerList.end())
        loopPacketListenerList.erase(item);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::processRainCollectorSizeChange(Rainfall bucketSize) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Received new rain bucket size of " << bucketSize << " inches" << endl;
    rainCollectorSize = bucketSize;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::consoleConnected() {
    ArchivePeriod archivePeriod;
    if (!retrieveArchivePeriod(archivePeriod))
        return;

    determineIfArchivingIsActive();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::consoleDisconnected() {
    // Nothing to do on disconnect
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::openStation() {
    return serialPort.open();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::isOpen() {
    return serialPort.isOpen();
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Attempting to wake up console" << endl;
    bool awake = false;

    for (int i = 0; i < WAKEUP_TRIES && !awake; i++) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_DEBUG1) << "Wake up console attempt " << (i + 1) << " of " << WAKEUP_TRIES << endl;
        if (!serialPort.write(WAKEUP_COMMAND)) {
            logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "Write to console failed while waking up the console, aborting wake up sequence" << endl;
            return false;
        }
      
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending TEST command" << endl;

    if (!serialPort.write(TEST_CMD)) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "sendTestCommand() failed to write the test command" << endl;
        return false;
    }

    if (!serialPort.write(COMMAND_TERMINATOR)) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "sendTestCommand() failed to write the test command terminator" << endl;
        return false;
    }

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending RXCHECK command" << endl;
    string response;

    if (!sendStringValueCommand(RECEIVE_CHECK_CMD, response))
        return false;

    return report.decode(response);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveConsoleType(string * consoleTypeString) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending WRD (Console Type) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending RXTEST command" << endl;

    //
    // There is no documentation in the serial protocol document regarding this
    // command. It does not state if a command terminator must be added or if
    // there is any response.
    //
    if (!serialPort.write(RXTEST_CMD)) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "performReceiverTest() failed to write the receiver test command" << endl;
        return false;
    }

    if (!serialPort.write(COMMAND_TERMINATOR)) {
        logger.log(VantageLogger::VantageLogger::VANTAGE_WARNING) << "performReceiverTest() failed to write the receiver test command terminator" << endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveFirmwareVersion(std::string & firmwareVersion) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending NVER (Firmware version) command" << endl;

    return sendStringValueCommand(FIRMWARE_VERSION_CMD, firmwareVersion);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::retrieveReceiverList(std::vector<StationId> & sensorStations) {
    logger.log(VantageLogger::VantageLogger::VANTAGE_INFO) << "Sending RECEIVERS (Receiver List) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending VER (Firmware Date) command" << endl;

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

    int maxPackets = records * 2;
    ostringstream command;
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending LPS " << maxPackets << " (LOOP packet) command" << endl;
    command << LPS_CMD << " " << maxPackets;

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
            for (auto listener : loopPacketListenerList) {
                terminateLoop = terminateLoop || !listener->processLoopPacket(loopPacket);
            }

            //
            // If all the listeners said to keep going
            //
            if (!terminateLoop) {
                //
                // LOOP2 packet is read second
                //
                if (readLoop2Packet(loop2Packet)) {
                    //
                    // Send the LOOP2 packet to each listener
                    //
                    for (auto listener : loopPacketListenerList) {
                        terminateLoop = terminateLoop || !listener->processLoop2Packet(loop2Packet);
                    }
                }
                else
                    resetNeeded = true;
            }
        }
        else
            resetNeeded = true;
    }

    //
    // If the callback wants to terminated the loop early or there was a problem use the wakeup sequence to terminate the loop
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending HILOWS (Request High/Low packet) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending PUTRAIN (Put Yearly Rain) command" << endl;

    if (rainCollectorSize != 0.0) {
        long argument = lround(rain / rainCollectorSize);
        ostringstream ss;
        ss << PUT_YEARLY_RAIN_CMD << " " << argument;
        return sendAckedCommand(ss.str());
    }
    else {
        logger.log(VantageLogger::VANTAGE_WARNING) << "PUTRAIN not being sent because the rain collector size has not been set" << endl;
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::putYearlyET(Evapotranspiration et) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending PUTET (Put Yearly ET) command" << endl;
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending DMP (Dump Archive) command" << endl;
    list.clear();
    list.reserve(NUM_ARCHIVE_RECORDS);

    int lastPageSequenceNumber = MAX_ARCHIVE_PAGE_SEQUENCE;

    if (sendAckedCommand(DUMP_ARCHIVE_CMD)) {
        DateTimeFields zeroTime;
        for (int i = 0; i < NUM_ARCHIVE_PAGES; i++) {
            readNextArchivePage(list, 0, zeroTime, lastPageSequenceNumber);
            if (!serialPort.write(DMP_SEND_NEXT_PAGE)) {
                logger.log(VantageLogger::VANTAGE_WARNING) << "Canceling DMP command due to error writing send-next-page command" << endl;
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


    logger.log(VantageLogger::VANTAGE_INFO) << "Sending DMPAFT " << time << " (Dump After) command" << endl;
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
        logger.log(VantageLogger::VANTAGE_ERROR) << "Canceling DMPAFT due to read failure of time data response" << endl;
        return false;
    }

    if (!VantageCRC::checkCRC(buffer, DUMP_AFTER_RESPONSE_LENGTH)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Canceling DMPAFT due to CRC failure of time data response" << endl;
        return false;
    }

    if (!serialPort.write(string(1, ProtocolConstants::ACK))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Canceling DMPAFT due to failure to write ACK to start dump sequence" << endl;
        return false;
    }

    int numPages = BitConverter::toInt16(buffer, 0);
    int firstRecord = BitConverter::toInt16(buffer, 2);
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Dumping " << numPages << " archive pages. First record in page with new data = " << firstRecord << endl;

    if (numPages == 0)
        return true;

    return readAfterArchivePages(time, list, firstRecord, numPages);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::LinkQuality
VantageWeatherStation::calculateLinkQuality(int stationId, int windSampleCount, int archiveRecordCount) const {
    int archivePeriodSeconds = archivePeriodMinutes * 60;

    LinkQuality linkQuality = calculateLinkQuality(archivePeriodSeconds, stationId, windSampleCount, archiveRecordCount);

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Calculated link quality of " << fixed << setprecision(2) << linkQuality
                                              << " with station ID=" << stationId
                                              << " windSampleCount=" << windSampleCount
                                              << " archivePeriod=" << archivePeriodMinutes
                                              << " archiveRecordCount=" << archiveRecordCount << endl;

    return linkQuality;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
VantageWeatherStation::LinkQuality
VantageWeatherStation::calculateLinkQuality(int archivePeriodSeconds, int stationId, int windSampleCount, int archiveRecordCount) {
    //
    // A packet should be received about every 2.5625 seconds for Station ID of 1 (41 / 16 = 2.5625)
    // That is 23.4 packets per minute or 117.07 packets per 5 minute archive interval.
    // According to Vantage protocol document:
    //     It is possible for the number of wind samples to be larger than the "expected" maximum value.
    //     This is because the maximum value is a long term average, rounded to an integer.
    //
    // The document does not go into details as to when the maximum value is rounded to an integer.
    // Though it does refer to the math coming up with a result of 105% which is 25 packets received
    // divided by 24 packets expected. This would be applicable when the archive interval is 1 minute.
    //
    // Trial and error testing was not able to replicate how the console calculates the link quality for a 5 minute
    // archive interval. As a result, the link quality algorithm will round the maximum samples to the nearest
    // integer, even though it does match the link quality reported on the console.
    //

    //
    // Handle zero case to avoid division by 0
    //
    if (archiveRecordCount == 0)
        return 0.0;

    //
    // The interval at which a radio packet is received by the console
    //
    int stationIndex = stationId - 1;
    double packetIntervalSeconds = (41.0F + static_cast<LinkQuality>(stationIndex)) / 16.0F;

    //
    // The maximum number of wind samples that should be received during an archive packet interval
    //
    long maxWindSamplesPerArchiveRecord = lround(static_cast<LinkQuality>(archivePeriodSeconds) / packetIntervalSeconds);

    //
    // Given the number of archive records used for this calculation, calculate the maximum number of wind samples
    // that could have been received
    //
    long maxWindSamples = maxWindSamplesPerArchiveRecord * archiveRecordCount;

    //
    // Convert to a percentage
    //
    LinkQuality linkQuality = (static_cast<LinkQuality>(windSampleCount) / static_cast<LinkQuality>(maxWindSamples)) * 100.0F;

    //
    // Round the link quality to the nearest 1/10th even though the console only reports whole numbers
    //
    LinkQuality roundedLinkQuality = round(linkQuality * 10.0) / 10.0;

    if (linkQuality > MAX_LINK_QUALITY)
        linkQuality = MAX_LINK_QUALITY;

    return roundedLinkQuality;
}

//
// EEPROM Commands
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromReadDataBlock(byte buffer[]) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending GETEE (Get EEPROM) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending EERD (EEPROM Read) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending EEBRD (EEPROM Binary Read) command" << endl;

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
VantageWeatherStation::isEepromAddressProtected(unsigned address, unsigned count) const {
    for (int i = 0; i < NUM_PROTECTED_EEPROM_BYTES; i++) {
        if (protectedEepromBytes[i] >= address && protectedEepromBytes[i] < address + count)
            return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::eepromWriteByte(unsigned address, byte value) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending EEWR (EEPROM Write) command" << endl;

    if (isEepromAddressProtected(address, 1)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Skipping write to EEPROM address " << address << " because it is a protected byte" << endl;
        return false;
    }

    ostringstream command;
    command << WRITE_EEPROM_CMD << " " << hex << address << " " << static_cast<int>(value);
    return sendOKedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool 
VantageWeatherStation::eepromBinaryWrite(unsigned address, const byte data[], unsigned count) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending EEBWR (EEPROM Binary Write) command" << endl;

    //
    // Check the address range against the protected bytes
    //
    if (isEepromAddressProtected(address, count)) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Skipping write to EEPROM address " << address
                                                 << " with size " << count << " because it overlaps at least one protected byte" << endl;
        return false;
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving Calibration Adjustments from EEPROM" << endl;

    if (!eepromBinaryRead(EepromConstants::EE_INSIDE_TEMP_CAL_ADDRESS, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE, buffer))
        return false;

    return calibrationPacket.decodePacket(buffer, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateCalibrationAdjustments(const CalibrationAdjustmentsPacket & calibrationAdjustments) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Updating Calibration Adjustments in EEPROM" << endl;

    byte buffer[CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE];

    if (!calibrationAdjustments.encodePacket(buffer, sizeof(buffer))) {
        logger.log(VantageLogger::VANTAGE_ERROR) << "Calibration adjustment encoding failed" << endl;
        return false;
    }

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Calibration Adjustment buffer: " << Weather::dumpBuffer(buffer, sizeof(buffer)) << endl;

    if (!eepromBinaryWrite(EepromConstants::EE_INSIDE_TEMP_CAL_ADDRESS, buffer, CalibrationAdjustmentsPacket::CALIBRATION_DATA_BLOCK_SIZE))
        return false;
    else
        return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateBarometerReadingAndElevation(Pressure baroReadingInHg, int elevationFeet) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending BAR (Barometer Reading and Elevation) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending BARDATA (Barometric Calibration Data) command" << endl;
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRLOG (Clear Archive) command" << endl;

    return sendAckedCommand(CLEAR_ARCHIVE_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearAlarmThresholds() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRALM (Clear Alarm Thresholds) command" << endl;

    return sendOKedWithDoneCommand(CLEAR_ALARM_THRESHOLDS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearTemperatureHumidityCalibrationOffsets() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRCAL (Clear Temperature and Humidity Calibration Data) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRGRA (Clear Graph Points) command" << endl;

    // See the comment in clearTemperatureHumidityCalibrationOffsets()
    return sendOKedWithDoneCommand(CLEAR_GRAPH_POINTS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearCumulativeValue(ProtocolConstants::CumulativeValue cumValue) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRVAR (Clear Cumulative Values) command" << endl;

    ostringstream ss;

    ss << CLEAR_CUMULATIVE_VALUE_CMD << " " << static_cast<int>(cumValue);

    return sendAckedCommand(ss.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearHighValues(ProtocolConstants::ExtremePeriod period) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRHIGHS (Clear High Values) command" << endl;

    ostringstream command;
    command << CLEAR_HIGH_VALUES_CMD << " " << static_cast<int>(period);

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearLowValues(ProtocolConstants::ExtremePeriod period) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRLOWS (Clear Low Values) command" << endl;

    ostringstream command;
    command << CLEAR_LOW_VALUES_CMD << " " << static_cast<int>(period);

    return sendAckedCommand(command.str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearActiveAlarms() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRBITS (Clear Active Alarms) command" << endl;

    return sendAckedCommand(CLEAR_ACTIVE_ALARMS_CMD);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::clearCurrentData() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending CLRDATA (Clear Current Data Values) command" << endl;

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
VantageWeatherStation::updateBaudRate(vws::BaudRate baudRate) {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending BAUD (Set Baud Rate) command" << endl;

    ostringstream command;
    command << SET_BAUD_RATE_CMD << " " << baudRate.getVantageValue();

    //
    // First set the console's baud rate, then reopen the serial port with the
    // new baud rate.
    // Per the protocol document the OK will be sent using the new baud rate, but a "NO"
    // response is written at the old baud rate. This creates an issue with this command
    // as you don't know which baud rate to use to read the response. In reality this
    // command should not be used during normal operations.
    //
    // TBD The console can respond with a "NO" to indicate failure
    //
    if (sendOKedCommand(command.str())) {
        serialPort.close();
        serialPort.setBaudRate(baudRate);
        serialPort.open();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::updateConsoleTime() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending SETTIME (Set Console Time) command" << endl;

    //
    // TODO This check did not work when DST ended. The time returned during the second 1 AM hour was
    // still adjusted for DST, therefore it was off by one hour (3600 seconds) causing the console time
    // to be set and triggering strange behavior in the archiving.
    //
    /*
     * In this case the console time was set to 2024-11-03 01:14:01 EST, possibly causing a 20 minute
     * gap in the archive between 1:55:00 EDT and 2:20:00 EST. Due to limitations of the console a
     * 1 hour gap was expected. It is still not clear if setting the time was the cause of this gap
     * or it is the nature of the console. We will not know for sure until a fix for this situation is
     * deployed and DST ends in 2025. Almost a year from now (2024/11/12).
    175690 - 2024-11-03 01:00 2024-11-03 01:00:00
    175691 - 2024-11-03 01:05 2024-11-03 01:05:00
    175692 - 2024-11-03 01:10 2024-11-03 01:10:00
    175693 - 2024-11-03 01:15 2024-11-03 01:15:00
    175694 - 2024-11-03 01:20 2024-11-03 01:20:00
    175695 - 2024-11-03 01:25 2024-11-03 01:25:00
    175696 - 2024-11-03 01:30 2024-11-03 01:30:00
    175697 - 2024-11-03 01:35 2024-11-03 01:35:00
    175698 - 2024-11-03 01:40 2024-11-03 01:40:00
    175699 - 2024-11-03 01:45 2024-11-03 01:45:00
    175700 - 2024-11-03 01:50 2024-11-03 01:50:00
    175701 - 2024-11-03 01:55 2024-11-03 01:55:00
    175702 - 2024-11-03 02:20 2024-11-03 02:20:00
    175703 - 2024-11-03 02:25 2024-11-03 02:25:00
    175704 - 2024-11-03 02:30 2024-11-03 02:30:00
    175705 - 2024-11-03 02:35 2024-11-03 02:35:00
    175706 - 2024-11-03 02:40 2024-11-03 02:40:00
    175707 - 2024-11-03 02:45 2024-11-03 02:45:00
    175708 - 2024-11-03 02:50 2024-11-03 02:50:00
    175709 - 2024-11-03 02:55 2024-11-03 02:55:00
    */


    //
    // If the console time is close to the actual time, then don't set the time.
    // Note that the console has an undocumented feature, where setting the console's time
    // will reset the diagnostics counters. So the time delta check is meant to keep the
    // diagnostics counters from being reset.
    // Setting the time may also cause issues with the archive during the transition from daylight saving time
    // to standard time. As a result the console time will never be set during the 1 AM hour to avoid possible
    // archive issues during the DST transition.
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
        else if (currentStationTime.getHour() == 1) {
            logger.log(VantageLogger::VANTAGE_DEBUG1) << "Not setting console time during the 1 AM hour due to possible DST issues. The console time will be checked during the next hour" << endl;
            return true;
        }
    }
    else {
        logger.log(VantageLogger::VANTAGE_DEBUG1) << "Not setting console time due to error retrieving the current console time" << endl;
        return false;
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending GETTIME (Get Console Time) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending SETPER (Set Archive Period) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Retrieving Archive Period from EEPROM" << endl;

    if (!eepromBinaryRead(EepromConstants::EE_ARCHIVE_PERIOD_ADDRESS, 1))
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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending START (Start Archiving) command" << endl;

    //
    // Note that the Vantage serial protocol document does specify the command sequence
    // of the START command. The OK sequence was determined by trial and error.
    //
    logger.log(VantageLogger::VANTAGE_INFO) << "Starting to archive" << endl;
    if (sendOKedCommand(START_ARCHIVING_CMD)) {
        archivingActive = true;
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::stopArchiving() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending STOP (Stop Archiving) command" << endl;

    //
    // Note that the Vantage serial protocol document does specify the command sequence
    // of the STOP command. The OK sequence was determined by trial and error.
    //
    logger.log(VantageLogger::VANTAGE_INFO) << "Stopping archiving" << endl;
    if (sendOKedCommand(STOP_ARCHIVING_CMD)) {
        archivingActive = false;
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::getArchivingState() const {
    return archivingActive;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
VantageWeatherStation::determineIfArchivingIsActive() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Checking if archiving is currently active" << endl;
    archivingActive = false;

    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Archive period for archiving active check is " << archivePeriodMinutes << " minutes" << endl;

    //
    // Dump after a time before what should be the last packet in the archive
    //
    int archivePeriodSeconds = archivePeriodMinutes * 60;
    DateTime now = time(0) - archivePeriodSeconds;
    DateTime after = now - (now % archivePeriodSeconds);

    DateTimeFields dumpTime(after);
    dumpTime.setSecond(0);

    vector<ArchivePacket> packets;
    logger.log(VantageLogger::VANTAGE_DEBUG2) << "Dumping archive after time " << dumpTime << " to see if archive is up to date" << endl;
    dumpAfter(dumpTime, packets);

    //
    // If the dump after command returns at least one packet, archiving is active
    //
    archivingActive = packets.size() > 0;
    logger.log(VantageLogger::VANTAGE_INFO) << "Archiving active: " << boolalpha << archivingActive << ". Last archive packet time: " << packets[0].getPacketDateTimeString() << endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::initializeSetup() {
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending NEWSETUP (Initialize Console Setup) command" << endl;

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
    logger.log(VantageLogger::VANTAGE_INFO) << "Sending LAMPS (Console Lamps Control) command" << endl;

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
    int lastPageSequenceNumber = MAX_ARCHIVE_PAGE_SEQUENCE;

    bool success = true;
    for (int i = 0; i < numPages; i++) {
        //
        // Process a single page. This will return 1 - 5 packets
        //
        if (!readNextArchivePage(list, firstRecordInPageToProcess, newestPacketTime, lastPageSequenceNumber)) {
            serialPort.write(DMP_CANCEL_DOWNLOAD); // No need to check write() return as this is an abort sequence
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
VantageWeatherStation::readNextArchivePage(vector<ArchivePacket> & list, int firstRecordInPageToProcess, const DateTimeFields & newestPacketTime, int & lastPageSequenceNumber) {
    bool success = true;
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Processing archive page. Newest packet time = " << newestPacketTime << endl;

    int expectedPageSequenceValue = lastPageSequenceNumber;
    if (expectedPageSequenceValue == MAX_ARCHIVE_PAGE_SEQUENCE)
        expectedPageSequenceValue = 0;
    else
        expectedPageSequenceValue++;

    //
    // Try to read the page. Will attempt 3 tries to correct CRC errors.
    //
    for (int i = 0; i < ARCHIVE_PAGE_READ_RETRIES; i++) {
        if (serialPort.readBytes(buffer, ARCHIVE_PAGE_SIZE + CRC_BYTES)) {
            if (VantageCRC::checkCRC(buffer, ARCHIVE_PAGE_SIZE)) {
                int pageSequence = decodeArchivePage(list, buffer, firstRecordInPageToProcess, newestPacketTime);
                if (pageSequence == expectedPageSequenceValue) {
                    success = true;
                    lastPageSequenceNumber = pageSequence;
                }
                else {
                    success = false;
                    logger.log(VantageLogger::VANTAGE_WARNING) << "Aborting archive dump due to page sequence check failure. Expected page sequence " << expectedPageSequenceValue << " received " << pageSequence << endl;
                }
                break;
            }
            else {
                logger.log(VantageLogger::VANTAGE_WARNING) << "CRC check failed on archive page. Try # " << (i + 1) << endl;
                success = false;
                if (!serialPort.write(DMP_RESEND_PAGE)) {
                    logger.log(VantageLogger::VANTAGE_WARNING) << "Aborting attempt to read the next archive page due to write error" << endl;
                    break;
                }
            }
        }
        else {
            serialPort.write(DMP_CANCEL_DOWNLOAD); // No need to check return value of write() as this is an abort path
            success = false;
            break;
        }
    }

    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int
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

    return pageSequence;
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

        if (!success) {
            if (i == COMMAND_RETRIES - 1)
                logger.log(VantageLogger::VANTAGE_WARNING) << "OKed command retries exceeded" << endl;
            else
                wakeupStation();
        }
    }

    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Command " << command << " status is " << success << endl;
    return success;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool
VantageWeatherStation::sendOKedWithDoneCommand(const string & command) {
    logger.log(VantageLogger::VANTAGE_DEBUG1) << "Sending command '" << command << "' that expects an OK response followed by DONE" << endl;
    bool success = false;

    if (!sendOKedCommand(command))
        return false;

    //
    // The commands that receive "DONE" in the end can have a very long delay.
    // Tell the serial port to wait up to a minute for the "DONE" response.
    // This is acceptable because the console will not respond to any other
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

        if (!success) {
            if (i == COMMAND_RETRIES - 1)
                logger.log(VantageLogger::VANTAGE_WARNING) << "ACKed command retries exceeded" << endl;
            else
                wakeupStation();
        }
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
        logger.log(VantageLogger::VANTAGE_INFO) << "consumeACK() read 0x" << hex << static_cast<int>(b) << dec << " not an ACK" << endl;
        rv = false;
    }

    return rv;
}

}
