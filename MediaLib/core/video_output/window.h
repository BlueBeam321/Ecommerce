/*****************************************************************************
 * window.h: window management for VLC video output
 *****************************************************************************/

vout_window_t *vout_display_window_New(vout_thread_t *, const vout_window_cfg_t *);
void vout_display_window_Attach(vout_window_t *, vout_display_t *);
void vout_display_window_Detach(vout_window_t *);
void vout_display_window_Delete(vout_window_t *);
