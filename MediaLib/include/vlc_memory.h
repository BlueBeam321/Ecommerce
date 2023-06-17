/*****************************************************************************
 * vlc_memory.h: Memory functions
 *****************************************************************************/

#ifndef VLC_MEMORY_H
#define VLC_MEMORY_H 1

#include <stdlib.h>

/**
 * \defgroup memory Memory
 * @{
 * \file
 * Memory fixups
 */

/**
 * This wrapper around realloc() will free the input pointer when
 * realloc() returns NULL. The use case ptr = realloc(ptr, newsize) will
 * cause a memory leak when ptr pointed to a heap allocation before,
 * leaving the buffer allocated but unreferenced. vlc_realloc() is a
 * drop-in replacement for that use case (and only that use case).
 */
static inline void *realloc_or_free( void *p, size_t sz )
{
    void *n = realloc(p,sz);
    if( !n )
        free(p);
    return n;
}

/**
 * @}
 */

#endif
