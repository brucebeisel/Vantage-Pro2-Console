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

#ifndef VANTAGE_WEATHER_STATION_H
#define VANTAGE_WEATHER_STATION_H

#include <string>
#include <vector>

#include "ArchivePacket.h"
#include "BitConverter.h"
#include "VantageProtocolConstants.h"

using namespace vws::ProtocolConstants;

namespace vws {
class HiLowPacket;
class LoopPacket;
class Loop2Packet;
class CalibrationAdjustmentsPacket;
class SerialPort;
class VantageLogger;

/**
 * Class that handles the command protocols with the Vantage console.
 */
class VantageWeatherStation {
public:
    class LoopPacketListener {
    public:
        virtual ~LoopPacketListener() {}
        virtual bool processLoopPacket(const LoopPacket & packet) = 0;
        virtual bool processLoop2Packet(const Loop2Packet & packet) = 0;
    };

    struct ConsoleDiagnosticReport {
        int packetCount;
        int missedPacketCount;
        int syncCount;
        int maxPacketSequence;
        int crcErrorCount;
    };

    struct BarometerCalibrationParameters {
        int recentMeasurement;         // In 1/1000 of an inch
        int elevation;                 // In feet
        int dewPoint;                  // Fahrenheit
        int avgTemperature12Hour;      // Fahrenheit
        int humidityCorrectionFactor;  // %
        int correctionRatio;           // Unknown units
        int offsetCorrectionFactor;    // Unknown units
        int fixedGain;                 // Unknown units, fixed per console
        int fixedOffset;               // Unknown units, fixed per console
    };


    /**
     * Constructor.
     * 
     * @param serialPort The serial port to use to communicate with the console
     */
    VantageWeatherStation(SerialPort & serialPort);

    /**
     * Destructor.
     */
    virtual ~VantageWeatherStation();

    /**
     * Add a loop packet listener.
     *
     * @param listener The listener to be added
     */
    void addLoopPacketListener(LoopPacketListener & listener);

    /**
     * Remove a loop packet listener.
     *
     * @param listener The listener to be removed
     */
    void removeLoopPacketListener(LoopPacketListener & listener);

    /**
     * Open the Vantage console.
     * 
     * @return True if the console was opened
     */
    bool openStation();

    /**
     * Close the Vantage console.
     */
    void closeStation();

    /**
     * Wake up the console.
     * 
     * @return True of the console is awake
     */
    bool wakeupStation();

    //
    // The following methods correspond to the commands in section VIII of the Vantage Serial Protocol Document, version 2.6.1
    //

    /////////////////////////////////////////////////////////////////////////////////
    // Testing Commands
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Send a test command to the console that expects the response "TEST<LF><CR>". This command
     * is used to test connectivity with the console.
     *
     * @return True if the correct response was received
     */
    bool sendTestCommand();

    /**
     * Retrieve the console diagnostic report.
     *
     * @param report The structure into which the report parameters are written
     *
     * @return True if the report was successfully retrieved
     */
    bool retrieveConsoleDiagnosticsReport(ConsoleDiagnosticReport & report);

    /**
     * Retrieve the type of this console.
     *
     * @param consoleType An optional string to return the console type. NULL pointer is allowed.
     * @return True if the station type was retrieved successfully
     */
    bool retrieveConsoleType(std::string * consoleType = NULL);

    /**
     * Get the console type that was retrieved from the console at initialization.
     * Note: This method does not talk to the console.
     *
     * @return The console type
     */
    ConsoleType getConsoleType() const;

    /**
     * Move the console from the "Receiving from..." screen to the current condition screen and reset
     * counters in the console diagnostic report.
     */
    bool performReceiveTest();

    /**
     * Retrieve the date of the console firmware.
     *
     * @param firmwareDate The string into which the firmware date will be copied
     *
     * @return True if the date was retrieved successfully
     */
    bool retrieveFirmwareDate(std::string & firmwareDate);

    /**
     * Retrieve the list of receivers the console can hear. Note that this is not the set of stations that the console is
     * reading from. There can be other sensor stations in the area that do not belong to this Vantage station.
     *
     * @param sensorStations The vector into which the list of receiver that can be heard is copied.
     *
     * @return True if the list of sensor stations was retrieved
     */
    bool retrieveReceiverList(std::vector<StationId> & sensorStations);

    /**
     * Retrieve the version of the console firmware.
     *
     * @param firmwareDate The string into which the firmware version will be copied
     *
     * @return True if the version was retrieved successfully
     */
    bool retrieveFirmwareVersion(std::string & firmwareVersion);

    /////////////////////////////////////////////////////////////////////////////////
    // End Testing Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Current Data Commands
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Retrieve the current weather by reading the LOOP and LOOP2 packet in a loop.
     * 
     * @param records The number of times to execute the loop before returning
     */
    void currentValuesLoop(int records);

    /**
     * Get a single LOOP packet.
     *
     * @param loopPacket Will contain the LOOP packet data if successful
     * @return True if the LOOP packet was read successfully
     */
    bool retrieveLoopPacket(LoopPacket & loop);

    /**
     * Retrieve the current high/low packet from the console.
     *
     * @param packet The packet into which the high/low packet will be written
     *
     * @return True if successful
     */
    bool retrieveHiLowValues(HiLowPacket &packet);

    /**
     * Write the specified rainfall amount as the current yearly accumulated rainfall.
     * This can be used when a weather station is installed or reset mid-year.
     *
     * @param rain The rainfall amount to write to the console
     *
     * @return True if successful
     */
    bool putYearlyRain(Rainfall rain);

    /**
     * Write the specified ET as the current yearly accumulated ET.
     * This can be used when a weather station is installed or reset mid-year.
     *
     * @param et The ET to write to the console
     *
     * @return True if successful
     */
    bool putYearlyET(Evapotranspiration et);

    /////////////////////////////////////////////////////////////////////////////////
    // End Current Data Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Download Commands
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Dump the entire archive.
     *
     * @param list The list that will contain the entire archive in sequential order
     */
    void dump(std::vector<ArchivePacket> & list);

    /**
     * Perform a dump of the archive after the specified time.
     * 
     * @param time    The time after which to dump the archive
     * @param archive The vector into which the dumped archive packets will be returned sorted by time
     *
     * @return True if successful
     */
    bool dumpAfter(DateTime time, std::vector<ArchivePacket> & archive);

    /////////////////////////////////////////////////////////////////////////////////
    // End Download Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // EEPROM Commands
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Read the entire EEPROM data block.
     *
     * @return True if the read was successful
     */
    bool eepromReadDataBlock(byte buffer[]);

    /**
     * Read part of the EEPROM memory.
     *
     * @param address The EEPROM address at which the reading will begin
     * @param count   The number of bytes to read
     *
     * @return True if the read is successful
     */
    bool eepromRead(unsigned address, unsigned count);

    /**
     * Read part of the EEPROM memory.
     *
     * @param address The EEPROM address at which the reading will begin
     * @param count   The number of bytes to read
     * @param output  Option point to which the results of the read will be copied
     *
     * @return True if the read is successful
     */
    bool eepromBinaryRead(unsigned address, unsigned count, char * output = nullptr);

    /**
     * Write a single byte to the specified EEPROM address.
     *
     * @param address The address within the EEPROM memory
     * @param value   The value to write to the specified address
     *
     * @return True if successful
     */
    bool eepromWriteByte(unsigned address, byte value);

    /**
     * Write a series of bytes to the EEPROM.
     *
     * @param address The address within the EEPROM at which the write will start
     * @param buffer  The buffer to write to the EEPROM
     * @param count   The number of bytes to write to the EEPROM
     *
     * @return bool True if successful
     */
    bool eepromBinaryWrite(unsigned address, const byte buffer[], unsigned count);

    /////////////////////////////////////////////////////////////////////////////////
    // End EEPROM Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Calibration Commands
    /////////////////////////////////////////////////////////////////////////////////
    bool retrieveCalibrationAdjustments(CalibrationAdjustmentsPacket & calibrationData);

    bool updateCalibrationAdjustments(const CalibrationAdjustmentsPacket & calibrationData);

    bool updateBarometerReadingAndElevation(Pressure baroReadingInHg, int elevationFeet);

    bool retrieveBarometerCalibrationParameters(BarometerCalibrationParameters & baroCalParams);

    /////////////////////////////////////////////////////////////////////////////////
    // End Calibration Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Clearing Commands
    /////////////////////////////////////////////////////////////////////////////////
    bool clearArchive();
    bool clearAlarmThresholds();
    bool clearTemperatureHumidityCalibrationOffsets();
    bool clearGraphPoints();
    bool clearCumulativeValue(ProtocolConstants::CumulativeValue cumValue);
    bool clearHighValues(ProtocolConstants::ExtremePeriod period);
    bool clearLowValues(ProtocolConstants::ExtremePeriod period);
    bool clearActiveAlarms();
    bool clearCurrentData();

    /////////////////////////////////////////////////////////////////////////////////
    // End Clearing Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Configuration Commands
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Change the baud rate for communicating with the console.
     *
     * @param rate The new baud rate
     *
     * @return True if successful
     */
    bool updateBaudRate(BaudRate rate);
    
    /**
     * Update the console's time.
     * 
     * @return True if successful
     */
    bool updateConsoleTime();

    /**
     * Get the clock time on the console.
     * 
     * @param stationTime The time that was retrieved or 0 on failure
     * @return True if the time was retrieved successfully
     */
    bool retrieveConsoleTime(vws::DateTime & stationTime);

    /**
     * Update the archive period to one of the allowed intervals.
     *
     * @param period The interval at which the archive data will saved
     *
     * @return True if successful
     */
    bool updateArchivePeriod(ProtocolConstants::ArchivePeriod period);

    /**
     * Retrieve the archive period.
     *
     * @param period The interval at which the archive data is being saved
     *
     * @return True if successful
     */
    bool retrieveArchivePeriod(ProtocolConstants::ArchivePeriod  & period);

    /**
     * Get the archive period last retrieved.
     *
     * @return The archive period in minutes
     */
    int getArchivePeriod() const;

    /**
     * Start archiving the weather data.
     *
     * @return True if successful
     */
    bool startArchiving();

    /**
     * Stop archiving the weather data.
     *
     * @return True if successful
     */
    bool stopArchiving();

    /**
     * Reinitialize the console after making any of the following changes to the configuration:
     *      1. Lat/Lon
     *      2. Elevation
     *      3. Any value in the EEPROM byte 0x2B (Decimal 43)
     *      4. Any value in the EEPROM byte 0x29 (Decimal 41)
     *      5. Other EEPROM bytes that have not been determined yet
     * Note that an "R" will appear in the lower right corner of the console display to
     * indicate the console is performing the setup. Even though the console will return control
     * back to the caller quickly, the R will remain on the screen for quite a while. The amount
     * of time the R remains on the console is not consistent as well as the time it takes for the
     * display units to change.
     *
     * @return True if successful
     */
    bool initializeSetup();

    /**
     * Turn the console lamp on or off.
     *
     * @param on Turn the lamp on if true
     *
     * @return True if successful
     */
    bool controlConsoleLamp(bool on);

    /////////////////////////////////////////////////////////////////////////////////
    // End Configuration Commands
    /////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////
    // Configuration Commands for data stored in the EEPROM
    /////////////////////////////////////////////////////////////////////////////////

    /**
     * Update the rain season start month.
     * The console will automatically clear the yearly rain total on the 1st of the specified month.
     *
     * @param month The month in which the rain season starts
     * @return True if successful
     */
    bool updateRainSeasonStart(ProtocolConstants::Month month);

    /**
     * Retrieve the rain season start month.
     *
     * @param month The month in which the rain season starts
     * @return True if successful
     */
    bool retrieveRainSeasonStart(ProtocolConstants::Month & month);

    /**
     * Set the whether the console will retransmit the data it receives and the retransmit station ID it will use if on.
     *
     * @param isRetransmitOn True if the console is to retransmit
     * @param retransmitStationId The station ID used for retransmission which is only used if isRetransmitOn is true
     * @return True if the successful
     */
    bool updateConsoleRetransmitMode(bool isRetransmitOn, StationId retransmitStationId);

    /**
     * Retrieve the retransmission mode of the console.
     *
     * @param isRetransmitOn True if the console is retransmitting
     * @param retransmitStationId The station ID being used for retransmission which is only valid if isRetransmitOn is true
     * @return True if the successful
     */
    bool retrieveConsoleRetransmitMode(bool & isTransmitOn, StationId & retransmitStationId);

    /////////////////////////////////////////////////////////////////////////////////
    // End Configuration Commands for data stored in the EEPROM
    /////////////////////////////////////////////////////////////////////////////////

private:
    static constexpr int WAKEUP_TRIES = 5;                         // The number of times to try to wake up the console before performing a disconnect/reconnect cycle
    static constexpr int CONSOLE_TIME_DELTA_THRESHOLD_SECONDS = 5; // The number of seconds that the console time can be off before the time will be set
    static constexpr int CRC_BYTES = 2;                            // The number of bytes in the CRC

    static constexpr int NUM_ARCHIVE_PAGES = 512;        // The total number of pages in the console's memory
    static constexpr int ARCHIVE_PAGE_SIZE = 265;        // 1 sequence byte, 5 52 byte records (260 bytes) and 4 spare bytes. 1 + 260 + 4 = 265 bytes
    static constexpr int RECORDS_PER_ARCHIVE_PAGE = 5;   // The number of archive records per archive page
    static constexpr int DUMP_AFTER_RESPONSE_LENGTH = 4; // The length of the response to the DMPAFT command
    static constexpr int EEPROM_READ_LINE_LENGTH = 4;    // The length of the response to the EEPROM READ command

    static constexpr int TIME_RESPONSE_LENGTH = 6;
    static constexpr int TIME_LENGTH = 4;
    static constexpr int SET_TIME_LENGTH = 6;
    static constexpr int WAKEUP_WAIT = 1000;
    static constexpr int VANTAGE_YEAR_OFFSET = 2000;
    static constexpr int HILOW_PACKET_SIZE = 436;
    static constexpr int EEPROM_DATA_BLOCK_SIZE = 4096;
    static constexpr int EEPROM_NON_GRAPH_DATA_SIZE = 176;

    static constexpr int ALARM_THRESHOLDS_SIZE = 94; 

    static constexpr int COMMAND_RETRIES = 5;
    static constexpr int ARCHIVE_PAGE_READ_RETRIES = 3;
    static constexpr int BUFFER_SIZE = EEPROM_DATA_BLOCK_SIZE + CRC_BYTES;

    /**
     * Read the LOOP packet and save off a few values for later use.
     *
     * @param loopPacket The LoopPacket into which the data will be copied
     *
     * @return True if the packet was read successfully
     */
    bool readLoopPacket(LoopPacket & loopPacket);

    /**
     * Read the LOOP2 packet.
     *
     * @param loop2Packet The Loop2Packet into which the data will be copied
     *
     * @return True if the packet was read succesfully
     */
    bool readLoop2Packet(Loop2Packet & loop2Packet);

    /**
     * Read the archive pages that occur after the specified time.
     *
     * @param afterTime                       The time after which the archive records will be saved
     * @param list                            The list into which the archive records will be saved
     * @param firstRecordInFirstPageToProcess The first record in the first page that is part of the dump
     * @param numPages                        The number of pages in the archive that are after the specified time
     *
     * @return True if successful
     */
    bool readAfterArchivePages(DateTime afterTime, std::vector<ArchivePacket> & list, int firstRecordInFirstPageToProcess, int numPages);

    /**
     * Read the next archive page that is part of the one of the dump commands.
     *
     * @param packets                    The vector into which the processed archive packets will be returned
     * @param firstRecordInPageToProcess The first record in the page to process, which may not be zero if this is the first page that is being dumped by a DMPAFT command
     * @param newestPacketTime           The newest packet used to detect if the page contains the end of the ring buffer
     *
     * @return True if the page was read successfully
     */
    bool readNextArchivePage(std::vector<ArchivePacket> & packets, int firstRecordInPageToProcess, DateTime newestPacketTime);

    /**
     * Decode an archive page that contains up to 5 packets.
     *
     * @param packets                    The vector into which the processed archive packets will be returned
     * @param firstRecordInPageToProcess The first record in the page to process, which may not be zero if this is the first page that is being dumped by a DMPAFT command
     * @param newestPacketTime           The newest packet used to detect if the page contains the end of the ring buffer
     */
    void decodeArchivePage(std::vector<ArchivePacket> &, const byte * buffer, int firstRecordInPageToProcess, DateTime newestPacketTime);

    /**
     * Checks if an archive packet contains data.
     *
     * @param buffer The buffer containing the packet
     * @param offset The offset within the buffer where the packet starts
     *
     * @return True if the packet contains data
     */
    bool archivePacketContainsData(const byte * buffer, int offset);

    /**
     * Send a command that expects on "OK" response.

     * @param command The command to be sent to the Vantage console
     * @return True if the command was sent successfully
     */
    bool sendOKedCommand(const std::string & command);

    /**
     * Send a command that expects on "OK" response followed by a "DONE" after a period of time.

     * @param command The command to be sent to the Vantage console
     * @return True if the command was sent successfully
     */
    bool sendOKedWithDoneCommand(const std::string & command);

    /**
     * Send a command that expects an ACK response.
     *
     * @param command The command to be sent to the Vantage console
     * @return True if the command was sent successfully
     */
    bool sendAckedCommand(const std::string & command);

    /**
     * Send the command to retrieve a string value.
     *
     * @param command The command to send
     * @param results The string value returned for the given command
     * @return True if successful
     */
    bool sendStringValueCommand(const std::string & command, std::string & results);

    /**
     * Read exactly one byte, checking for an ACK.
     *
     * @return True if one character was read and it was an ACK
     */
    bool consumeAck();

private:
    typedef std::vector<LoopPacketListener *> LoopPacketListenerList;

    SerialPort &               serialPort;               // The serial port object that communicates with the console
    byte                       buffer[BUFFER_SIZE];      // The buffer used for all reads
    LoopPacketListenerList     loopPacketListenerList;   // The list of Loop Packet listeners
    ConsoleType                consoleType;              // The type of Davis Vantage console this is
    int                        archivePeriodMinutes;     // The number of minutes between archive records
    VantageLogger &            logger;

};
}

#endif /* VANTAGE_WEATHER_STATION_H */
