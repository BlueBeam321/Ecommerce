/*****************************************************************************
 * libvlc_internal.h : Definition of opaque structures for libvlc exported API
 * Also contains some internal utility functions
 *****************************************************************************/

#ifndef _LIBVLC_INTERNAL_H
#define _LIBVLC_INTERNAL_H 1

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include <vlc/libvlc_instance.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_list.h>
#include <vlc/libvlc_events.h>

/* Note well: this header is included from LibVLC core.
 * Therefore, static inline functions MUST NOT call LibVLC functions here
 * (this can cause linkage failure on some platforms). */

/***************************************************************************
 * Internal creation and destruction functions
 ***************************************************************************/
VLC_API libvlc_int_t *libvlc_InternalCreate(void);
VLC_API int libvlc_InternalInit(libvlc_int_t *);
VLC_API void libvlc_InternalCleanup(libvlc_int_t *);
VLC_API void libvlc_InternalDestroy(libvlc_int_t *);

VLC_API void libvlc_SetExitHandler(libvlc_int_t *, void (*)(void *), void *);

/***************************************************************************
 * Opaque structures for libvlc API
 ***************************************************************************/

struct libvlc_instance_t
{
    libvlc_int_t *p_libvlc_int;
    unsigned      ref_count;
    vlc_mutex_t   instance_lock;
    struct libvlc_callback_entry_list_t *p_callback_list;
    struct
    {
        void (*cb) (void *, int, const libvlc_log_t *, const char *, va_list);
        void *data;
    } log;
};

struct libvlc_event_manager_t
{
    void * p_obj;
    vlc_array_t listeners;
    vlc_mutex_t lock;
};

/***************************************************************************
 * Other internal functions
 ***************************************************************************/

/* Thread context */
void libvlc_threads_init (void);
void libvlc_threads_deinit (void);

/* Events */
void libvlc_event_manager_init(libvlc_event_manager_t *, void *);
void libvlc_event_manager_destroy(libvlc_event_manager_t *);

void libvlc_event_send(
        libvlc_event_manager_t * p_em,
        libvlc_event_t * p_event );

static inline libvlc_time_t from_mtime(mtime_t time)
{
    return (time + 500ULL)/ 1000ULL;
}

static inline mtime_t to_mtime(libvlc_time_t time)
{
    return time * 1000ULL;
}

#endif
