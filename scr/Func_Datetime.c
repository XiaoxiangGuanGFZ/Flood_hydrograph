#include <stdio.h>
#include <time.h>
#include "def_struct.h"
#include "Func_Datetime.h"


// Custom timegm function to convert struct tm to time_t assuming UTC
time_t custom_timegm(struct tm *tm) {
    time_t t = mktime(tm);
    return t - timezone + (tm->tm_isdst > 0 ? 3600 : 0);
}

// Function to convert DATETIME to time_t (UTC)
time_t convert_to_time_t(DATETIME dt) {
    struct tm t = {0};
    t.tm_year = dt.year - 1900; // tm_year is years since 1900
    t.tm_mon = dt.month - 1;    // tm_mon is 0-based
    t.tm_mday = dt.day;
    t.tm_hour = dt.hour;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1; // Automatic daylight saving time adjustment
    return mktime(&t); // Using custom timegm
}

// Function to convert time_t to DATETIME (UTC)
DATETIME convert_to_datetime(time_t t) {
    struct tm *tm_info = gmtime(&t); // Use gmtime for UTC
    DATETIME dt;
    dt.year = tm_info->tm_year + 1900;
    dt.month = tm_info->tm_mon + 1;
    dt.day = tm_info->tm_mday;
    dt.hour = tm_info->tm_hour;
    return dt;
}

char *DateString(
    time_t *tm
)
{
    struct tm *ptm;
    static char buf[30] = {0};
    ptm = localtime(tm);
    ptm->tm_isdst = 0; 
    strftime(buf, 30, "%Y-%m-%d %H:%M:%S", ptm);
    return buf;
}
