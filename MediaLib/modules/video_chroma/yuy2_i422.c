/*****************************************************************************
 * yuy2_i422.c : Packed YUV 4:2:2 to Planar YUV conversion module for vlc
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_picture.h>

#define MODULE_NAME     vchroma_yuy2_i422
#define MODULE_STRING   "vchroma_yuy2_i422"

#define SRC_FOURCC "YUY2,YUNV,YVYU,UYVY,UYNV,Y422"
#define DEST_FOURCC  "I422"

/*****************************************************************************
 * Local and extern prototypes.
 *****************************************************************************/
static int  Activate ( vlc_object_t * );

static void YUY2_I422           ( filter_t *, picture_t *, picture_t * );
static void YVYU_I422           ( filter_t *, picture_t *, picture_t * );
static void UYVY_I422           ( filter_t *, picture_t *, picture_t * );
static picture_t *YUY2_I422_Filter    ( filter_t *, picture_t * );
static picture_t *YVYU_I422_Filter    ( filter_t *, picture_t * );
static picture_t *UYVY_I422_Filter    ( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin ()
    set_description( N_("Conversions from " SRC_FOURCC " to " DEST_FOURCC) )
    set_capability( "video converter", 80 )
    set_callbacks( Activate, NULL )
vlc_module_end ()

/*****************************************************************************
 * Activate: allocate a chroma function
 *****************************************************************************
 * This function allocates and initializes a chroma function
 *****************************************************************************/
static int Activate( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;

    if( p_filter->fmt_in.video.i_width & 1
     || p_filter->fmt_in.video.i_height & 1 )
    {
        return -1;
    }

    if( p_filter->fmt_in.video.i_width != p_filter->fmt_out.video.i_width
     || p_filter->fmt_in.video.i_height != p_filter->fmt_out.video.i_height
     || p_filter->fmt_in.video.orientation != p_filter->fmt_out.video.orientation)
        return -1;

    switch( p_filter->fmt_out.video.i_chroma )
    {
        case VLC_CODEC_I422:
            switch( p_filter->fmt_in.video.i_chroma )
            {
                case VLC_CODEC_YUYV:
                    p_filter->pf_video_filter = YUY2_I422_Filter;
                    break;

                case VLC_CODEC_YVYU:
                    p_filter->pf_video_filter = YVYU_I422_Filter;
                    break;

                case VLC_CODEC_UYVY:
                    p_filter->pf_video_filter = UYVY_I422_Filter;
                    break;

                default:
                    return -1;
            }
            break;

        default:
            return -1;
    }
    return 0;
}

/* Following functions are local */

VIDEO_FILTER_WRAPPER( YUY2_I422 )
VIDEO_FILTER_WRAPPER( YVYU_I422 )
VIDEO_FILTER_WRAPPER( UYVY_I422 )

/*****************************************************************************
 * YUY2_I422: packed YUY2 4:2:2 to planar YUV 4:2:2
 *****************************************************************************/
static void YUY2_I422( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
        {
#define C_YUYV_YUV422( p_line, p_y, p_u, p_v )      \
            *p_y++ = *p_line++; *p_u++ = *p_line++; \
            *p_y++ = *p_line++; *p_v++ = *p_line++
            C_YUYV_YUV422( p_line, p_y, p_u, p_v );
            C_YUYV_YUV422( p_line, p_y, p_u, p_v );
            C_YUYV_YUV422( p_line, p_y, p_u, p_v );
            C_YUYV_YUV422( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_YUYV_YUV422( p_line, p_y, p_u, p_v );
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;
    }
}

/*****************************************************************************
 * YVYU_I422: packed YVYU 4:2:2 to planar YUV 4:2:2
 *****************************************************************************/
static void YVYU_I422( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
        {
#define C_YVYU_YUV422( p_line, p_y, p_u, p_v )      \
            *p_y++ = *p_line++; *p_v++ = *p_line++; \
            *p_y++ = *p_line++; *p_u++ = *p_line++
            C_YVYU_YUV422( p_line, p_y, p_u, p_v );
            C_YVYU_YUV422( p_line, p_y, p_u, p_v );
            C_YVYU_YUV422( p_line, p_y, p_u, p_v );
            C_YVYU_YUV422( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_YVYU_YUV422( p_line, p_y, p_u, p_v );
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;
    }
}

/*****************************************************************************
 * UYVY_I422: packed UYVY 4:2:2 to planar YUV 4:2:2
 *****************************************************************************/
static void UYVY_I422( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
        {
#define C_UYVY_YUV422( p_line, p_y, p_u, p_v )      \
            *p_u++ = *p_line++; *p_y++ = *p_line++; \
            *p_v++ = *p_line++; *p_y++ = *p_line++
            C_UYVY_YUV422( p_line, p_y, p_u, p_v );
            C_UYVY_YUV422( p_line, p_y, p_u, p_v );
            C_UYVY_YUV422( p_line, p_y, p_u, p_v );
            C_UYVY_YUV422( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_UYVY_YUV422( p_line, p_y, p_u, p_v );
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;
    }
}
