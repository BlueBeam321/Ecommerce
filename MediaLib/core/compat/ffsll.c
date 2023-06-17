/*****************************************************************************
 * ffsll.c: GNU ffsll() replacement
 *****************************************************************************/

#if defined(_WIN32) || defined(__ANDROID__)
#include <limits.h>

int ffsll(long long x)
{
    for (unsigned i = 0; i < sizeof (x) * CHAR_BIT; i++)
        if ((x >> i) & 1)
            return i + 1;
    return 0;
}
#endif
