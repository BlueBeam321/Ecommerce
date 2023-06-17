/*****************************************************************************
 * flockfile.c: POSIX unlocked I/O stream stubs
 *****************************************************************************/

#ifdef _WIN32
#include <stdio.h>

#define _lock_file(s) ((void)(s))
#define _unlock_file(s) ((void)(s))
#undef _getc_nolock
#define _getc_nolock(s) getc(s)
#undef _putc_nolock
#define _putc_nolock(s,c) putc(s,c)

void flockfile (FILE *stream)
{
    _lock_file (stream);
}

void funlockfile (FILE *stream)
{
    _unlock_file (stream);
}

int getc_unlocked (FILE *stream)
{
    return _getc_nolock (stream);
}

int putc_unlocked (int c, FILE *stream)
{
    return _putc_nolock (c, stream);
}

int getchar_unlocked (void)
{
    return getc_unlocked (stdin);
}

int putchar_unlocked (int c)
{
    return putc_unlocked (c, stdout);
}
#endif
