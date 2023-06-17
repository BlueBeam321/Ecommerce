/*****************************************************************************
 * picture.h: picture internals
 *****************************************************************************/

#include <vlc_picture.h>
#include <vlc_atomic.h>

typedef struct
{
    picture_t picture;
    struct
    {
        atomic_uintptr_t refs;
        void (*destroy)(picture_t *);
        void *opaque;
    } gc;
} picture_priv_t;
