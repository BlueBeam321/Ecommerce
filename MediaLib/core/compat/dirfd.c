/*****************************************************************************
 * dirfd.c: POSIX dirfd replacement
 *****************************************************************************/

#ifdef _WIN32
#include <dirent.h>
#include <errno.h>

int (dirfd) (DIR *dir)
{
#ifdef dirfd
    return dirfd (dir);
#else
    (void) dir;
# ifdef ENOTSUP
    errno = ENOTSUP;
# endif
    return -1;
#endif
}
#endif
