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

    return 0;
}
