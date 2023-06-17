/*****************************************************************************
 * vlc_video_splitter.h: "video splitter" related structures and functions
 *****************************************************************************/

#ifndef VLC_VIDEO_SPLITTER_H
#define VLC_VIDEO_SPLITTER_H 1

#include <vlc_es.h>
#include <vlc_picture.h>
#include <vlc_mouse.h>

/**
 * \file
 * This file defines the structure and types used by video splitter filters.
 */

typedef struct video_splitter_t video_splitter_t;
typedef struct video_splitter_sys_t video_splitter_sys_t;
typedef struct video_splitter_owner_t video_splitter_owner_t;

/** Structure describing a video splitter output properties
 */
typedef struct
{
    /* Video format of the output */
    video_format_t fmt;

    /* Window hints */
    struct
    {
        /* Relative position.
         * (0,0) is equal to the default position.
         */
        int i_x;
        int i_y;

        /* Alignment inside the window
         */
        int i_align;
    } window;

    /* Video output module
     * Use NULL for default
     */
    char *psz_module;

} video_splitter_output_t;

/** Structure describing a video splitter
 */
struct video_splitter_t
{
    VLC_COMMON_MEMBERS

    /* Module properties */
    module_t        *p_module;

    /* configuration */
    config_chain_t  *p_cfg;

    /* Input format
     * It is filled by the creator and cannot be modified.
     */
    video_format_t  fmt;

    /* Output formats
     *
     * It can only be set in the open() function and must remain
     * constant.
     * The module is responsible for the allocation and deallocation.
     */
    int                     i_output;
    video_splitter_output_t *p_output;

    int             (*pf_filter)( video_splitter_t *, picture_t *pp_dst[],
                                  picture_t *p_src );
    int             (*pf_mouse) ( video_splitter_t *, vlc_mouse_t *,
                                  int i_index,
                                  const vlc_mouse_t *p_old, const vlc_mouse_t *p_new );

    video_splitter_sys_t *p_sys;

    /* Buffer allocation */
    int  (*pf_picture_new) ( video_splitter_t *, picture_t *pp_picture[] );
    void (*pf_picture_del) ( video_splitter_t *, picture_t *pp_picture[] );
    video_splitter_owner_t *p_owner;
};

/**
 * It will create an array of pictures suitable as output.
 *
 * You must either returned them through pf_filter or by calling
 * video_splitter_DeletePicture.
 *
 * If VLC_SUCCESS is not returned, pp_picture values are undefined.
 */
static inline int video_splitter_NewPicture( video_splitter_t *p_splitter,
                                             picture_t *pp_picture[] )
{
    int i_ret = p_splitter->pf_picture_new( p_splitter, pp_picture );
    if( i_ret )
        msg_Warn( p_splitter, "can't get output pictures" );
    return i_ret;
}

/**
 * It will release an array of pictures created by video_splitter_NewPicture.
 * Provided for convenience.
 */
static inline void video_splitter_DeletePicture( video_splitter_t *p_splitter,
                                                 picture_t *pp_picture[] )
{
    p_splitter->pf_picture_del( p_splitter, pp_picture );
}

/* */
video_splitter_t * video_splitter_New( vlc_object_t *, const char *psz_name, const video_format_t * );
void video_splitter_Delete( video_splitter_t * );

static inline int video_splitter_Filter( video_splitter_t *p_splitter,
                                         picture_t *pp_dst[], picture_t *p_src )
{
    return p_splitter->pf_filter( p_splitter, pp_dst, p_src );
}
static inline int video_splitter_Mouse( video_splitter_t *p_splitter,
                                        vlc_mouse_t *p_mouse,
                                        int i_index,
                                        const vlc_mouse_t *p_old, const vlc_mouse_t *p_new )
{
    if( !p_splitter->pf_mouse )
    {
        *p_mouse = *p_new;
        return VLC_SUCCESS;
    }
    return p_splitter->pf_mouse( p_splitter, p_mouse, i_index, p_old, p_new );
}

#endif /* VLC_VIDEO_SPLITTER_H */

