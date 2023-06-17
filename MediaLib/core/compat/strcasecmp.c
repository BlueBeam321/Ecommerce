/*****************************************************************************
 * strcasecmp.c: POSIX strcasecmp() replacement
 *****************************************************************************/

#ifdef _WIN32
#include <string.h>
#include <ctype.h>
#include <assert.h>

int strcasecmp (const char *s1, const char *s2)
{
#if 1
    return stricmp (s1, s2);
#else
    for (size_t i = 0;; i++)
    {
        unsigned char c1 = s1[i], c2 = s2[i];
        int d = tolower (c1) - tolower (c2);
        if (d || !c1)
            return d;
        assert (c2);
    }
#endif
}
#endif
