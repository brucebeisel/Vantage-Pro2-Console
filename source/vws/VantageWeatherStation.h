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

#ifndef VANTAGE_WEATHER_STATION_H
#define VANTAGE_WEATHER_STATION_H

#include <string>
#include <vector>

#include "ArchivePacket.h"
#include "BitConverter.h"
#include "VantageProtocolConstants.h"
#include "RainCollectorSizeListener.h"
#include "ConsoleConnectionMonitor.h"
#include "BaudRate.h"

using namespace vws::ProtocolConstants;

namespace vws {
class HiLowPacket;
class LoopPacket;
class Loop2Packet;
class CalibrationAdjustmentsPacket;
class SerialPort;
class VantageLogger;
class LoopPacketListener;
class ConsoleDiagnosticReport;

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
 * Class that handles the command protocols with the Vantage console.
 */
class VantageWeatherStation : public RainCollectorSizeListener, public ConsoleConnectionMonitor  {
public:
    using LinkQuality = double;
    static constexpr LinkQuality MAX_LINK_QUALITY = 100.0;

    /**
     * Constructor.
     * 
     * @param serialPort           The serial port to use to communicate with the console
     * @param archivePeriodMinutes The number of minutes per archive record, used for test purposes only (normally retrieved from the console at startup)
     * @param rainCollectorSize    The size of the rain collector bucket, used for test purposes only (normally retrieved from the console at startup)
     */
    VantageWeatherStation(SerialPort & serialPort, int archivePeriodMinutes = 0, Rainfall rainCollectorSize = 0.0);

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
     * Called when the rain collector size changes or at startup.
     *
     * @param bucketSize The size of the rain collector bucket
     */
    virtual void processRainCollectorSizeChange(Rainfall bucketSize);

    /**
     * Called when a connection with the console is established.
     */
    virtual void consoleConnected();

    /**
     * Called when the connection with the console is lost.
     */
    virtual void consoleDisconnected();

    /**
     * Open the Vantage console.
     * 
     * @return True if the console was opened
     */
    bool openStation();

    /**
     * Checks if the console is open.
     *
     * @return True if the console is open
     */
    bool isOpen();

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
     * @param time    The fields that define the date and time after which to dump
     * @param archive The vector into which the dumped archive packets will be returned sorted by time
     *
     * @return True if successful
     */
    bool dumpAfter(const DateTimeFields & time, std::vector<ArchivePacket> & archive);

    /**
     * Calculate link quality for a given station ID.
     *
     * @param archivePeriodSeconds The number of seconds between packets in the archive
     * @param stationId            The ID of the station for which to calculate link quality
     * @param windSamples          The total number of wind samples counted across one or more archive packets
     * @param archiveRecords       The number of archive records from which the wind samples were counted
     * @return The link quality 0 - 100%
     */
    static LinkQuality calculateLinkQuality(int archivePeriodSeconds, int stationId, int windSamples, int archiveRecords);

    /**
     * Calculate link quality for a given station ID.
     *
     * @param stationId            The ID of the station for which to calculate link quality
     * @param windSamples          The total number of wind samples counted across one or more archive packets
     * @param archiveRecords       The number of archive records from which the wind samples were counted
     * @return The link quality 0 - 100%
     */
    LinkQuality calculateLinkQuality(int stationId, int windSamples, int archiveRecords) const;

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
     * Check if the specified address range in the EEPROMs protected list.
     *
     * @param address The start of the address range to check
     * @param count   The number of addresses to be checked
     * @return True if any of the addresses in the range are protected
     */
    bool isEepromAddressProtected(unsigned address, unsigned count) const;

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

    /**
     * Clear the console's archive.
     *
     * @return True if successful
     */
    bool clearArchive();

    /**
     * Clear all of the alarm thresholds, disabling all active alarms.
     *
     * @return True if successful
     */
    bool clearAlarmThresholds();

    /**
     * Clear all the temperature and humidity calibration offsets.
     *
     * @return True if successful
     */
    bool clearTemperatureHumidityCalibrationOffsets();

    /**
     * Clear the graph points stored in the EEPROM.
     *
     * @return True if successful
     */
    bool clearGraphPoints();

    /**
     * Clear the cumulative values for the specified rain or ET measurement.
     *
     * @param cumValue The rain or ET cumulative value to clear
     * @return True if successful
     */
    bool clearCumulativeValue(ProtocolConstants::CumulativeValue cumValue);

    /**
     * Clear all the high values for the specified period.
     *
     * @param period The period of the high values to clear (daily, monthly, yearly)
     * @return True if successful
     */
    bool clearHighValues(ProtocolConstants::ExtremePeriod period);

    /**
     * Clear all the low values for the specified period.
     *
     * @param period The period of the low values to clear (daily, monthly, yearly)
     * @return True if successful
     */
    bool clearLowValues(ProtocolConstants::ExtremePeriod period);

    /**
     * Clear any active alarms, which will reactivate if the values are still beyond the alarm threshold.
     *
     * @return True if successful
     */
    bool clearActiveAlarms();

    /**
     * Clear all the current value data, changing all of the fields on the console's display to "dashed values".
     *
     * @return True if successful
     */
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
    bool updateBaudRate(vws::BaudRate rate);
    
    /**
     * Update the console's time.
     * Note that this command will also reset the diagnostic counters.
     * 
     * @return True if successful
     */
    bool updateConsoleTime();

    /**
     * Get the clock time on the console.
     * 
     * @param stationTime The time that was retrieved or invalid on failure
     * @return True if the time was retrieved successfully
     */
    bool retrieveConsoleTime(vws::DateTimeFields & stationTime);

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
     * Get the state of archiving.
     *
     * @return True if archiving is active
     */
    bool getArchivingState() const;

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

    static constexpr int NUM_ARCHIVE_PAGES = 512;         // The total number of pages in the console's memory
    static constexpr int ARCHIVE_PAGE_SIZE = 265;         // 1 sequence byte, 5 52 byte records (260 bytes) and 4 spare bytes. 1 + 260 + 4 = 265 bytes
    static constexpr int RECORDS_PER_ARCHIVE_PAGE = 5;    // The number of archive records per archive page
    static constexpr int MAX_ARCHIVE_PAGE_SEQUENCE = 255; // The maximum value for an archive page number, it wraps back to 0 if more pages are dumped
    static constexpr int DUMP_AFTER_RESPONSE_LENGTH = 4;  // The length of the response to the DMPAFT command
    static constexpr int EEPROM_READ_LINE_LENGTH = 4;     // The length of the response to the EEPROM READ command

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
     * @return True if the packet was read successfully
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
    bool readAfterArchivePages(const DateTimeFields & afterTime, std::vector<ArchivePacket> & list, int firstRecordInFirstPageToProcess, int numPages);

    /**
     * Read the next archive page that is part of the one of the dump commands.
     *
     * @param packets                    The vector into which the processed archive packets will be returned
     * @param firstRecordInPageToProcess The first record in the page to process, which may not be zero if this is the first page that is being dumped by a DMPAFT command
     * @param newestPacketTime           The newest packet used to detect if the page contains the end of the ring buffer
     * @param [in/out] lastPageSequence  The page sequence number of the last archive page dumped and returns the page sequence number that was read
     *
     * @return True if the page was read successfully
     */
    bool readNextArchivePage(std::vector<ArchivePacket> & packets, int firstRecordInPageToProcess, const DateTimeFields & newestPacketTime, int & lastPageSequenceNumber);

    /**
     * Decode an archive page that contains up to 5 packets.
     *
     * @param packets                    The vector into which the processed archive packets will be returned
     * @param firstRecordInPageToProcess The first record in the page to process, which may not be zero if this is the first page that is being dumped by a DMPAFT command
     * @param newestPacketTime           The newest packet used to detect if the page contains the end of the ring buffer
     * @return The page sequence of the decoded archive page
     */
    int decodeArchivePage(std::vector<ArchivePacket> &, const byte * buffer, int firstRecordInPageToProcess, const DateTimeFields & newestPacketTime);

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
    /**
     * Determine if archiving is active by dumping the latest record(s) from the archive.
     * The results are stored in the "archiveActive" member variable.
     */
    void determineIfArchivingIsActive();

    typedef std::vector<LoopPacketListener *> LoopPacketListenerList;

    SerialPort &               serialPort;               // The serial port object that communicates with the console
    byte                       buffer[BUFFER_SIZE];      // The buffer used for all reads
    LoopPacketListenerList     loopPacketListenerList;   // The list of Loop Packet listeners
    ConsoleType                consoleType;              // The type of Davis Vantage console this is
    int                        archivePeriodMinutes;     // The number of minutes between archive records
    Rainfall                   rainCollectorSize;        // The amount of rain for each rain bucket tip
    bool                       archivingActive;          // Whether the console is currently archiving
    VantageLogger &            logger;

};
}

#endif /* VANTAGE_WEATHER_STATION_H */
