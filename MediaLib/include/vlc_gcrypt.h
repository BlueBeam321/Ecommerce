/*****************************************************************************
 * vlc_gcrypt.h: VLC thread support for gcrypt
 *****************************************************************************/

/**
 * \file
 * This file implements gcrypt support functions in vlc
 */

#include <errno.h>

static inline void vlc_gcrypt_init (void)
{
    /* This would need a process-wide static mutex with all libraries linking
     * to a given instance of libgcrypt. We cannot do this as we have different
     * plugins linking with gcrypt, and some underlying libraries may use it
     * behind our back. Only way is to always link gcrypt statically (ouch!) or
     * have upstream gcrypt provide one shared object per threading system. */
    static bool done = false;

    vlc_global_lock (VLC_GCRYPT_MUTEX);
    if (!done)
    {
        /* The suggested way for an application to make sure that global_init
         * has been called is by using gcry_check_version. (see global_init
         * comments in gcrypt sources) */
        gcry_check_version(NULL);
        done = true;
    }
    vlc_global_unlock (VLC_GCRYPT_MUTEX);
}
