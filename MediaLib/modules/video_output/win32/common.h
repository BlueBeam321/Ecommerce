/*****************************************************************************
 * common.h: Windows video output header file
 *****************************************************************************/
#include "events.h"

/*****************************************************************************
 * vout_sys_t: video output method descriptor
 *****************************************************************************
 * This structure is part of the video output thread descriptor.
 * It describes the module specific properties of an output thread.
 *****************************************************************************/
typedef struct vout_display_sys_win32_t
{
    /* */
    event_thread_t *event;

    /* */
    HWND                 hwnd;                  /* Handle of the main window */
    HWND                 hvideownd;        /* Handle of the video sub-window */
    struct vout_window_t *parent_window;         /* Parent window VLC object */
    HWND                 hparent;             /* Handle of the parent window */
    HWND                 hfswnd;          /* Handle of the fullscreen window */

    /* size of the display */
    RECT         rect_display;

    /* size of the overall window (including black bands) */
    RECT         rect_parent;

# if !defined(NDEBUG) && defined(HAVE_DXGIDEBUG_H)
    HINSTANCE     dxgidebug_dll;
# endif

    unsigned changes;        /* changes made to the video display */

    /* Misc */
    bool is_first_display;
    bool is_on_top;

    /* Coordinates of src and dest images (used when blitting to display) */
    RECT         rect_src;
    RECT         rect_src_clipped;
    RECT         rect_dest;
    RECT         rect_dest_clipped;

    picture_pool_t *pool;

    bool use_desktop;     /* show video on desktop window ? */

    bool use_overlay;     /* Are we using an overlay surface */
    /* Overlay alignment restrictions */
    int  i_align_src_boundary;
    int  i_align_src_size;
    int  i_align_dest_boundary;
    int  i_align_dest_size;

    bool (*pf_GetRect)(const struct vout_display_sys_win32_t *p_sys, RECT *out);
    unsigned int (*pf_GetPictureWidth) (const vout_display_t *);
    unsigned int (*pf_GetPictureHeight)(const vout_display_t *);
} vout_display_sys_win32_t;


/*****************************************************************************
 * Prototypes from common.c
 *****************************************************************************/
int  CommonInit(vout_display_t *);
void CommonClean(vout_display_t *);
void CommonManage(vout_display_t *);
int  CommonControl(vout_display_t *, int , va_list );
void CommonDisplay(vout_display_t *);
int  CommonUpdatePicture(picture_t *, picture_t **fallback, uint8_t *plane, unsigned pitch);

void UpdateRects (vout_display_t *,
                  const vout_display_cfg_t *,
                  bool is_forced);
void AlignRect(RECT *, int align_boundary, int align_size);

picture_pool_t *CommonPool(vout_display_t *, unsigned);

/*****************************************************************************
 * Constants
 *****************************************************************************/
#define IDM_TOGGLE_ON_TOP WM_USER + 1
#define DX_POSITION_CHANGE 0x1000
