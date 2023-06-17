/*****************************************************************************
 * timespec_get.c: C11 timespec_get() replacement
 *****************************************************************************/

#if defined(_WIN32) || defined(__ANDROID__)
#include <vlc_fixups.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

int timespec_get(struct timespec *ts, int base)
{
    switch (base)
    {
        case TIME_UTC:
#ifndef _WIN32
#if (_POSIX_TIMERS >= 0)
            if (clock_gettime(CLOCK_REALTIME, ts) == 0)
                break;
#endif
#endif
        {
            struct timeval tv;
            if (gettimeofday(&tv, NULL) == 0)
            {
                ts->tv_sec = tv.tv_sec;
                ts->tv_nsec = tv.tv_usec * 1000;
                break;
            }
        }
            /* fall through */
        default:
            return 0;
    }
    return base;
}
#endif
