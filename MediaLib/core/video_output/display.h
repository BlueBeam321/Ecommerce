/*****************************************************************************
 * display.h: "vout display" management
 *****************************************************************************/

#include <vlc_vout_wrapper.h>

vout_display_t *vout_NewSplitter(vout_thread_t *vout,
                                 const video_format_t *source,
                                 const vout_display_state_t *state,
                                 const char *module,
                                 const char *splitter_module,
                                 mtime_t double_click_timeout,
                                 mtime_t hide_timeout);

/* FIXME should not be there */
void vout_SendDisplayEventMouse(vout_thread_t *, const vlc_mouse_t *);

vout_window_t *vout_NewDisplayWindow(vout_thread_t *, unsigned type);
void vout_DeleteDisplayWindow(vout_thread_t *, vout_window_t *);
void vout_SetDisplayWindowSize(vout_thread_t *, unsigned, unsigned);
int  vout_HideWindowMouse(vout_thread_t *, bool);

void vout_UpdateDisplaySourceProperties(vout_display_t *vd, const video_format_t *);
