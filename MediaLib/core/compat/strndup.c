/*****************************************************************************
 * strndup.c: POSIX strndup() replacement
 *****************************************************************************/

#ifdef _WIN32
#include <vlc_fixups.h>
#include <string.h>
#include <stdlib.h>

char *strndup (const char *str, size_t max)
{
    size_t len = strnlen (str, max);
    char *res = malloc (len + 1);
    if (res)
    {
        memcpy (res, str, len);
        res[len] = '\0';
    }
    return res;
}
#endif
