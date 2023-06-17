/*****************************************************************************
 * timegm.c: BSD/GNU timegm() replacement
 *****************************************************************************/

#ifdef _WIN32
#include <stdbool.h>
#include <time.h>

static bool is_leap_year(unsigned y)
{
    if (y % 4)
        return false;
    if (y % 100)
        return true;
    if (y % 400)
        return false;
    return true;
}

time_t timegm(struct tm *tm)
{
    static const unsigned ydays[12 + 1] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    if (tm->tm_year < 70 /* FIXME: (negative) dates before Epoch */
     || tm->tm_mon < 0 || tm->tm_mon > 11
     || tm->tm_mday < 1 || tm->tm_mday > 31
     || tm->tm_hour < 0 || tm->tm_hour > 23
     || tm->tm_min < 0 || tm->tm_min > 59
     || tm->tm_sec < 0 || tm->tm_sec > 60 /* mind the leap second */)
        return -1;

    /* Count the number of days */
    unsigned t = 365 * (tm->tm_year - 70)
                 + ydays[tm->tm_mon] + (tm->tm_mday - 1);

    /* TODO: unroll */
    for (int i = 70; i < tm->tm_year; i++)
        t += is_leap_year(1900 + i);

    if (tm->tm_mon > 1)
        t += is_leap_year(1900 + tm->tm_year);

    t *= 24;
    t += tm->tm_hour;
    t *= 60;
    t += tm->tm_min;
    t *= 60;
    t += tm->tm_sec;
    return t;
}
#endif
