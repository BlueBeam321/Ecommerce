/*****************************************************************************
 * aligned_alloc.c: C11 aligned_alloc() replacement
 *****************************************************************************/

#include <vlc_fixups.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>

void *aligned_alloc(size_t align, size_t size)
{
    if ((align & (align - 1)) || (size & (align - 1)))
    {
        errno = EINVAL;
        return NULL;
    }

#ifdef HAVE_POSIX_MEMALIGN
    if (align < sizeof (void *)) /* POSIX does not allow small alignment */
        align = sizeof (void *);

    void *ptr;
    int err = posix_memalign(&ptr, align, size);
    if (err)
    {
        errno = err;
        ptr = NULL;
    }
    return ptr;

#elif defined(__ANDROID__)
    return memalign(align, size);
#elif defined (_WIN32) && defined(__MINGW32__)
    return __mingw_aligned_malloc(size, align);
#elif defined (_WIN32) && defined(_MSC_VER)
    return _aligned_malloc(size, align);
#endif
}
