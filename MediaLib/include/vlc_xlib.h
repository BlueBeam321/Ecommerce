/*****************************************************************************
 * vlc_xlib.h: initialization of Xlib
 *****************************************************************************/

#ifndef VLC_XLIB_H
# define VLC_XLIB_H 1

# include <stdio.h>
# include <stdlib.h>
# include <X11/Xlib.h>
# include <X11/Xlibint.h>

static inline bool vlc_xlib_init (vlc_object_t *obj)
{
    if (!var_InheritBool (obj, "xlib"))
        return false;

    bool ok = false;

    /* XInitThreads() can be called multiple times,
     * but it is not reentrant, so we need this global lock. */
    vlc_global_lock (VLC_XLIB_MUTEX);

    if (_Xglobal_lock == NULL && unlikely(_XErrorFunction != NULL))
        /* (_Xglobal_lock == NULL) => Xlib threads not initialized */
        /* (_XErrorFunction != NULL) => Xlib already in use */
        fprintf (stderr, "%s:%u:%s: Xlib not initialized for threads.\n"
                 "This process is probably using LibVLC incorrectly.\n",
                 __FILE__, __LINE__, __func__);
    else if (XInitThreads ())
        ok = true;

    vlc_global_unlock (VLC_XLIB_MUTEX);

    if (!ok)
        msg_Err (obj, "Xlib not initialized for threads");
    return ok;
}

#endif
