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
#ifndef DATE_TIME_FIELDS_H
#define DATE_TIME_FIELDS_H

#include <iosfwd>
#include "WeatherTypes.h"

struct tm;
namespace vws {

//
// Fields for the standard date and time. Note that the values in this structure do
// not apply any offsets. That is, the year is the actual year (e.g. 2021)
//
class DateTimeFields {
public:
    /**
     * Constructor used to set fields to valid ranges.
     */
    DateTimeFields();

    /**
     * Copy constructor.
     *
     * @param rhs The Date/Time to copy
     */
    DateTimeFields(const DateTimeFields & rhs);

    /**
     * Construct that takes a date string, note that an invalid date will leave the object in the
     * same state as the default constructor.
     *
     * @param dateTimeString The date and or time in yyyy-mm-dd[ hh:mm[:ss]] format
     */
    explicit DateTimeFields(const std::string & dateTimeString);

    /**
     * Constructor that sets the date fields only.
     *
     * @param year  The year of the date
     * @param month The month of the date (1-12)
     * @param monthDay The day of the month of this date (1 - 31)
     */
    DateTimeFields(int year, int month, int day);

    /**
     * Constructor that sets the date and time fields
     *
     * @param year  The year of the date
     * @param month The month of the date (1-12)
     * @param monthDay The day of the month of this date (1 - 31)
     * @param hour   The hour of the time (0 - 23)
     * @param minute The minute of the time (0 - 59)
     * @param second Optional argument containing the second of the time (0 - 59)
     */
    DateTimeFields(int year, int month, int day, int hour, int minutes, int seconds = 0);

    /**
     * Constructor that sets the date/time fields from a POSIX time structure.
     *
     * @param tm The POSIX time structure
     */
    explicit DateTimeFields(const struct tm & tm);


    /**
     * Constructor that sets the date/time fields from a POSIX time.
     *
     * @param timeFromEpoch The POSIX time
     */
    explicit DateTimeFields(DateTime timeFromEpoch);

    /**
     * Reset the date/time fields to the defaults, which are not valid.
     */
    void resetDateTimeFields();

    /**
     * Return whether this date/time has been set.
     *
     * @return bool True if it is valid
     */
    bool isDateTimeValid() const;

    /**
     * Set the date fields of this date/time.
     *
     * @param year  The year of the date
     * @param month The month of the date (1-12)
     * @param monthDay The day of the month of this date (1 - 31)
     */
    void setDate(int year, int month, int monthDay);

    /**
     * Set the time fields of this date/time.
     *
     * @param hour   The hour of the time (0 - 23)
     * @param minute The minute of the time (0 - 59)
     * @param second The second of the time (0 - 59)
     */
    void setTime(int hour, int minute, int second);

    /**
     * Set the date and time fields of this date/time.
     *
     * @param year  The year of the date
     * @param month The month of the date (1-12)
     * @param monthDay The day of the month of this date (1 - 31)
     * @param hour   The hour of the time (0 - 23)
     * @param minute The minute of the time (0 - 59)
     * @param second The second of the time (0 - 59)
     */
    void setDateTime(int year, int month, int monthDay, int hour, int minute, int second);

    /**
     * Set the date/time fields from another date/time object.
     *
     * @param other The other date/time object from which to copy the fields
     */
    void setDateTime(const DateTimeFields & other);

    /**
     * Set the date/time fields from a POSIX time structure.
     *
     * @param tm The POSIX time structure
     */
    void setDateTime(const struct tm & tm);

    /**
     * Parse a date and load the date fields.
     *
     * @param dateString The date string in the YYYY-mm-dd format
     *
     * @return True if the date is valid
     */
    bool parseDate(const std::string & dateString);

    /**
     * Parse a date/time and load the date and time fields.
     *
     * @param dateString The date string in the YYYY-mm-dd hh:mm[:ss] or YYYY-mm-ddThh:mm[:ss] format
     *
     * @return True if the date/time is valid
     */
    bool parseDateTime(const std::string & dateTimeString);

    /**
     * Getters and setters.
     */
    void setYear(int year);
    void setMonth(int month);
    void setMonthDay(int monthDay);
    void setHour(int hour);
    void setMinute(int minute);
    void setSecond(int second);

    int getYear() const;
    int getMonth() const;
    int getMonthDay() const;
    int getHour() const;
    int getMinute() const;
    int getSecond() const;

    /**
     * Get the time since the epoch using the system default to adjust for daylight savings time.
     *
     * @return The number of seconds since the epoch
     */
    DateTime getEpochDateTime() const;

    /**
     * Set the date/time fields from an Epoch timestamp.
     *
     * @param epoch The epoch time
     */
    void setFromEpoch(DateTime epoch);

    /**
     * Format the date portion of the Date/Time fields.
     *
     * @return The formatted date in yyyy-mm-dd format
     */
    std::string formatDate() const;

    /**
     * Format the time portion of the Date/Time fields.
     *
     * @param displaySeconds Whether seconds should output in the time string
     *
     * @return The formatted date in hh:mm format
     */
    std::string formatTime(bool displaySeconds = false) const;

    /**
     * Format the date/time fields.
     *
     * @param displaySeconds Whether seconds should output in the time string
     *
     * @return The formatted date and time in yyyy-mm-dd hh:mm format
     */
    std::string formatDateTime(bool displaySeconds = false) const;

    /**
     * Equals operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "other" and "this" are equal.
     */
    bool operator==(const DateTimeFields & other) const;

    /**
     * Not equals operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "other" and "this" are not equal.
     */
    bool operator!=(const DateTimeFields & other) const;

    /**
     * Less than operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "this" is less than "other"
     */
    bool operator<(const DateTimeFields & other) const;

    /**
     * Greater than operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "this" is greater than "other"
     */
    bool operator>(const DateTimeFields & other) const;

    /**
     * Greater than or equal to operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "this" is greater than or equal to "other"
     */
    bool operator>=(const DateTimeFields & other) const;

    /**
     * Less than or equal to operator.
     *
     * @param other The other DateTimeFields object being compared with "this"
     *
     * @return True if "this" is less than or equal to "other"
     */
    bool operator<=(const DateTimeFields & other) const;

    /**
     * Ostream operator.
     *
     * @param os     The output stream
     * @param fields The object to be output on the stream
     *
     * @return The stream provided in the first argument
     */
    friend std::ostream & operator<<(std::ostream & os, const DateTimeFields & fields);

private:
    /**
     * If the year is 0, then the date is not valid.
     */
    static constexpr int INVALID_YEAR = 0;

    int year;            // The actual year, e.g. 2025
    int month;           // The month. Range 1 - 12
    int monthDay;        // The day of the month. Range 1 - 31
    int hour;            // The hour in 24 hour format. Range 0 - 23
    int minute;          // The minute of the hour. Range 0 - 59
    int second;          // The second of the minute. Range 0 - 59

};

}

#endif
