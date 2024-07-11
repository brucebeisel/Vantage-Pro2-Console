#include <iostream>
#include "DateTimeFields.h"
#include "Weather.h"

using namespace std;
using namespace vws;

void
checkLessAndGreaterThan(const DateTimeFields & d1, const DateTimeFields & d2) {
    if (d1 < d2)
        cout << "PASSED: operator<()" << endl;
    else
        cout << "FAILED: operator<()" << endl;

    if (d1 > d2)
        cout << "FAILED: operator>()" << endl;
    else
        cout << "PASSED: operator>()" << endl;

    if (d1 <= d2)
        cout << "PASSED: operator<=()" << endl;
    else
        cout << "FAILED: operator<=()" << endl;

    if (d1 >= d2)
        cout << "FAILED: operator>=()" << endl;
    else
        cout << "PASSED: operator>=()" << endl;
}

int
main(int argc, char *argv[]) {
    DateTimeFields dtf1, dtf2;

    if (!dtf1.isDateTimeValid())
        cout << "PASSED: Constructor with no arguments created an invalid object" << endl;
    else
        cout << "FAILED: Constructor with no arguments did NOT create an invalid object: " << dtf1.formatDateTime() << endl;

    dtf1.setYear(2024);
    dtf1.setMonth(2);

    string s = dtf1.formatDateTime();
    if (s == "2024-02-01 00:00")
        cout << "PASSED: formatDateTime()" << endl;
    else
        cout << "FAILED: formatDateTime(). Result = '" << s << "' not '2024-02-01 00:00'" << endl;

    s = dtf1.formatDate();

    if (s == "2024-02-01")
        cout << "PASSED: formatDate()" << endl;
    else
        cout << "FAILED: formatDate(). Result = '" << s << "' not '2024-02-01'" << endl;

    s = dtf1.formatTime();

    if (s == "00:00")
        cout << "PASSED: formatTime()" << endl;
    else
        cout << "FAILED: formatTime(). Result = '" << s << "' not '00:00'" << endl;



    if (dtf1 != dtf2)
        cout << "PASSED: operator!=()" << endl;
    else
        cout << "FAILED: operator!=()" << endl;

    dtf2.setYear(2024);
    dtf2.setMonth(2);

    if (dtf1 == dtf2)
        cout << "PASSED: operator==()" << endl;
    else
        cout << "FAILED: operator==()" << endl;

    dtf2.setMinute(dtf2.getMinute() + 1);
    cout << "Checking minute inequality/greater/less than" << endl;
    checkLessAndGreaterThan(dtf1, dtf2);

    dtf2.setMinute(dtf1.getMinute() + 1);
    dtf2.setHour(dtf1.getHour() + 1);
    cout << "Checking hour inequality/greater/less than" << endl;
    checkLessAndGreaterThan(dtf1, dtf2);

    dtf2.setHour(dtf1.getHour() + 1);
    dtf2.setMonthDay(dtf1.getMonthDay() + 1);
    cout << "Checking day of month inequality/greater/less than" << endl;
    checkLessAndGreaterThan(dtf1, dtf2);

    dtf2.setMonthDay(dtf1.getMonthDay());
    dtf2.setMonth(dtf2.getMonth() + 1);
    cout << "Checking month inequality/greater/less than" << endl;
    checkLessAndGreaterThan(dtf1, dtf2);

    dtf2.setMonth(dtf1.getMonth());
    dtf2.setYear(dtf2.getYear() + 1);;
    cout << "Checking year inequality/greater/less than" << endl;
    checkLessAndGreaterThan(dtf1, dtf2);

    cout << endl << "Epoch test" << endl;
    time_t t = dtf1.getEpochDateTime();

    struct tm tm;
    Weather::localtime(t, tm);

    if (tm.tm_year + 1900 == dtf1.getYear())
        cout << "PASSED: getEpochDateTime()" << endl;
    else
        cout << "FAILED: getEpochDateTime()" << endl;

    t += 3600;
    DateTimeFields fromEpoch;
    fromEpoch.setFromEpoch(t);

    cout << "From epoch. Source: " << dtf1.formatDateTime() << " 1 hour later: " << fromEpoch.formatDateTime(true) << endl;


    cout << endl << "Validity test" << endl;
    if (dtf1.isDateTimeValid())
        cout << "PASSED: Existing time is valid" << endl;
    else
        cout << "FAILED: Existing time is NOT valid" << endl;

    dtf1.resetDateTimeFields();
    if (dtf1.isDateTimeValid())
        cout << "FAILED: Existing time is valid after reset" << endl;
    else
        cout << "PASSED: Existing time is NOT valid after reset" << endl;

    DateTimeFields dtf3;

    if (dtf3.isDateTimeValid())
        cout << "FAILED: Existing time is valid after construction" << endl;
    else
        cout << "PASSED: Existing time is NOT valid after construction" << endl;

    cout << endl << "Parse tests" << endl;
    DateTimeFields dtf4;
    cout << "Parsing date 2024-6-10" << endl;
    dtf4.parseDate("2024-6-10");
    if (dtf4.getYear() == 2024 && dtf4.getMonth() == 6 && dtf4.getMonthDay() == 10)
        cout << "PASSED: Parsed date is correct" << endl;
    else
        cout << "FAILED: Parsed date is NOT correct: " << dtf4.formatDate() << endl;

    cout << "Parsing date/time 2024-10-20 12:59:23" << endl;
    dtf4.parseDateTime("2024-10-20 12:59:23");
    if (dtf4.getYear() == 2024 && dtf4.getMonth() == 10 && dtf4.getMonthDay() == 20 && dtf4.getHour() == 12 && dtf4.getMinute() == 59 && dtf4.getSecond() == 23)
        cout << "PASSED: Parsed date is correct" << endl;
    else
        cout << "FAILED: Parsed date is NOT correct: " << dtf4.formatDateTime(true) << endl;

    cout << "Parsing date/time 2024-10-20T12:59:23" << endl;
    dtf4.parseDateTime("2024-10-20T12:59:23");
    if (dtf4.getYear() == 2024 && dtf4.getMonth() == 10 && dtf4.getMonthDay() == 20 && dtf4.getHour() == 12 && dtf4.getMinute() == 59 && dtf4.getSecond() == 23)
        cout << "PASSED: Parsed date is correct" << endl;
    else
        cout << "FAILED: Parsed date is NOT correct: " << dtf4.formatDateTime(true) << endl;

    cout << endl << "Copy constructor tests" << endl;
    DateTimeFields dtf5(dtf4);

    if (dtf4 == dtf5)
        cout << "PASSED: Copy constructor" << endl;
    else
        cout << "FAILED: Copy constructor. Source: " << dtf4.formatDateTime() << " Destination: " << dtf5.formatDateTime() << endl;

    DateTimeFields dtf6;
    dtf6 = dtf5;

    if (dtf5 == dtf6)
        cout << "PASSED: operator=()" << endl;
    else
        cout << "FAILED: operator=()" << endl;

    cout << endl << "Constructor Tests" << endl;
    DateTimeFields df7(2024, 10, 9);

    if (df7.getYear() == 2024 && df7.getMonth() == 10 && df7.getMonthDay() == 9 && df7.getHour() == 0 && df7.getMinute() == 0 && df7.getSecond() == 0)
        cout << "PASSED: DateTimeField(year, month, day) constructor" << endl;
    else
        cout << "FAILED: DateTimeField(year, month, day) constructor created: " << df7.formatDateTime()  << endl;

    DateTimeFields df8(2024, 10, 9, 1, 2, 3);

    if (df8.getYear() == 2024 && df8.getMonth() == 10 && df8.getMonthDay() == 9 && df8.getHour() == 1 && df8.getMinute() == 2 && df8.getSecond() == 3)
        cout << "PASSED: DateTimeField(year, month, day, hour, minute, second) constructor" << endl;
    else
        cout << "FAILED: DateTimeField(year, month, day) constructor created: " << df8.formatDateTime()  << endl;

    DateTimeFields df9("2024-05-06 02:03:04");

    if (df9.getYear() == 2024 && df9.getMonth() == 5 && df9.getMonthDay() == 6 && df9.getHour() == 2 && df9.getMinute() == 3 && df9.getSecond() == 4)
        cout << "PASSED: DateTimeField(Date String) constructor" << endl;
    else
        cout << "FAILED: DateTimeField(Date String) constructor created: " << df9.formatDateTime()  << endl;

    DateTimeFields df10(1720397350);

    if (df10.getYear() == 2024 && df10.getMonth() == 7 && df10.getMonthDay() == 7 && df10.getHour() == 20 && df10.getMinute() == 9 && df10.getSecond() == 10)
        cout << "PASSED: DateTimeField(Epoch time) constructor" << endl;
    else
        cout << "FAILED: DateTimeField(Epoch time) constructor created: " << df10.formatDateTime()  << endl;

    tm.tm_year = 124;
    tm.tm_mon = 6;
    tm.tm_mday = 7;
    tm.tm_hour = 5;
    tm.tm_min = 6;
    tm.tm_sec = 7;

    DateTimeFields df11(tm);

    if (df11.getYear() == 2024 && df11.getMonth() == 7 && df11.getMonthDay() == 7 && df11.getHour() == 5 && df11.getMinute() == 6 && df11.getSecond() == 7)
        cout << "PASSED: DateTimeField(struct tm) constructor" << endl;
    else
        cout << "FAILED: DateTimeField(struct tm) constructor created: " << df11.formatDateTime()  << endl;

    return 0;
}
