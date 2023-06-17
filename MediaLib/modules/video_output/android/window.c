/**
 * @file window.c
 * @brief Android native window provider module for VLC media player
 */

#ifdef __ANDROID__
#include <stdarg.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_vout_window.h>

#include <dlfcn.h>
#include <jni.h>

#include "common/modules/video_output/android/utils.h"

#define MODULE_NAME     vwin_android
#define MODULE_STRING   "vwin_android"

#define THREAD_NAME "Android Window"

static int Open(vout_window_t *, const vout_window_cfg_t *);
static void Close(vout_window_t *);
static int Control(vout_window_t *, int, va_list ap);

/*
 * Module descriptor
 */
vlc_module_begin()
    set_shortname(N_("Android Window"))
    set_description(N_("Android native window"))
    set_category(CAT_VIDEO)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_capability("vout window", 10)
    set_callbacks(Open, Close)
vlc_module_end()


struct vout_window_sys_t
{
    AWindowHandler *p_awh;
};

static void OnNewWindowSize(vout_window_t *wnd, unsigned i_width, unsigned i_height)
{
    vout_window_ReportSize(wnd, i_width, i_height);
}

static void OnNewMouseCoords(vout_window_t *wnd, const struct awh_mouse_coords *coords)
{
    vout_window_ReportMouseMoved(wnd, coords->i_x, coords->i_y);
    switch (coords->i_action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
        vout_window_ReportMousePressed(wnd, coords->i_button);
        break;
    case AMOTION_EVENT_ACTION_UP:
        vout_window_ReportMouseReleased(wnd, coords->i_button);
        break;
    case AMOTION_EVENT_ACTION_MOVE:
        break;
    }
}

/**
 * Create an Android native window.
 */
static int Open(vout_window_t *wnd, const vout_window_cfg_t *cfg)
{
    if (cfg->type != VOUT_WINDOW_TYPE_INVALID && cfg->type != VOUT_WINDOW_TYPE_ANDROID_NATIVE)
        return VLC_EGENERIC;

    vout_window_sys_t *p_sys = malloc(sizeof (*p_sys));
    if (p_sys == NULL)
        return VLC_ENOMEM;
    wnd->sys = p_sys;

    p_sys->p_awh = AWindowHandler_new(wnd, &(awh_events_t) { OnNewWindowSize, OnNewMouseCoords });
    if (!p_sys->p_awh)
        goto error;

    wnd->type = VOUT_WINDOW_TYPE_ANDROID_NATIVE;
    wnd->handle.anativewindow = p_sys->p_awh;
    wnd->control = Control;

    return VLC_SUCCESS;

error:
    Close(wnd);
    return VLC_EGENERIC;
}


/**
 * Destroys the Android native window.
 */
static void Close(vout_window_t *wnd)
{
    vout_window_sys_t *p_sys = wnd->sys;
    if (p_sys->p_awh)
        AWindowHandler_destroy(p_sys->p_awh);
    free(p_sys);
}


/**
 * Window control.
 */
static int Control(vout_window_t *wnd, int cmd, va_list ap)
{
    (void)ap;
    msg_Err(wnd, "request %d not implemented", cmd);
    return VLC_EGENERIC;
}
#endif
