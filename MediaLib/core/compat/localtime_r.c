/*****************************************************************************
 * localtime_r.c: POSIX localtime_r() replacement
 *****************************************************************************/

#if defined(__STDC_LIB_EXT1__) && (__STDC_LIB_EXT1__ >= 20112L)
# define __STDC_WANT_LIB_EXT1__ 1
#else
# define __STDC_WANT_LIB_EXT1__ 0
#endif

#include <errno.h>
#include <time.h>

struct tm *localtime_r (const time_t *timep, struct tm *result)
{
#if (__STDC_WANT_LIB_EXT1__)
    return localtime_s(timep, result);
#elif defined (_WIN32)
    return ((errno = localtime_s(result, timep)) == 0) ? result : NULL;
#else
# warning localtime_r() not implemented!
    return gmtime_r(timep, result);
#endif
}
