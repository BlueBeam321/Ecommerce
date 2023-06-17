/*****************************************************************************
 * vlc_inhibit.h: VLC screen saver inhibition
 *****************************************************************************/

/**
 * \file
 * This file defines the interface for screen-saver inhibition modules
 */

#ifndef VLC_INHIBIT_H
# define VLC_INHIBIT_H 1

typedef struct vlc_inhibit vlc_inhibit_t;
typedef struct vlc_inhibit_sys vlc_inhibit_sys_t;

enum vlc_inhibit_flags
{
    VLC_INHIBIT_NONE=0 /*< No inhibition */,
    VLC_INHIBIT_SUSPEND=0x1 /*< Processor is in use - do not suspend */,
    VLC_INHIBIT_DISPLAY=0x2 /*< Display is in use - do not blank/lock */,
#define VLC_INHIBIT_AUDIO (VLC_INHIBIT_SUSPEND)
#define VLC_INHIBIT_VIDEO (VLC_INHIBIT_SUSPEND|VLC_INHIBIT_DISPLAY)
};

struct vlc_inhibit
{
    VLC_COMMON_MEMBERS

    vlc_inhibit_sys_t *p_sys;
    void (*inhibit) (vlc_inhibit_t *, unsigned flags);
};

static inline void vlc_inhibit_Set (vlc_inhibit_t *ih, unsigned flags)
{
    ih->inhibit (ih, flags);
}

#endif
