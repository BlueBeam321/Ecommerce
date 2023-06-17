/*****************************************************************************
 * events.h: Windows video output header file
 *****************************************************************************/

#include <vlc_vout_window.h>

#define MODULE_NAME_IS_direct3d9        1
/**
 * HWNDs manager.
 */
typedef struct event_thread_t event_thread_t;

typedef struct {
    bool use_desktop; /* direct3d */
    bool use_overlay; /* directdraw */
    int x;
    int y;
    unsigned width;
    unsigned height;
} event_cfg_t;

typedef struct {
    vout_window_t *parent_window;
    HWND hparent;
    HWND hwnd;
    HWND hvideownd;
    HWND hfswnd;
} event_hwnd_t;

event_thread_t *EventThreadCreate( vout_display_t *);
void            EventThreadDestroy( event_thread_t * );
int             EventThreadStart( event_thread_t *, event_hwnd_t *, const event_cfg_t * );
void            EventThreadStop( event_thread_t * );

void            EventThreadUpdateTitle( event_thread_t *, const char *psz_fallback );
int             EventThreadGetWindowStyle( event_thread_t * );
void            EventThreadUpdateWindowPosition( event_thread_t *, bool *pb_moved, bool *pb_resized,
                                                 int x, int y, unsigned w, unsigned h );
void            EventThreadUpdateSourceAndPlace( event_thread_t *p_event,
                                                 const video_format_t *p_source,
                                                 const vout_display_place_t *p_place );
void            EventThreadUseOverlay( event_thread_t *, bool b_used );
bool            EventThreadGetAndResetHasMoved( event_thread_t * );

# ifdef __cplusplus
extern "C" {
# endif
void* HookWindowsSensors(vout_display_t*, HWND);
void UnhookWindowsSensors(void*);
# ifdef __cplusplus
}
# endif

