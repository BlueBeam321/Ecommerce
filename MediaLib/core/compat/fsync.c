/*****************************************************************************
 * fsync.c: POSIX fsync() replacement
 *****************************************************************************/

#ifdef _WIN32
#include <io.h>

int fsync (int fd)
{
    /* WinCE can use FlushFileBuffers() but it operates on file handles */
    return _commit (fd);
}
#endif
