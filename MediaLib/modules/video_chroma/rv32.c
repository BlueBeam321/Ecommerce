/*****************************************************************************
 * rv32.c: conversion plugin to RV32 format
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_picture.h>

#define MODULE_NAME     vchroma_rv32
#define MODULE_STRING   "vchroma_rv32"

/****************************************************************************
 * Local prototypes
 ****************************************************************************/
static int  OpenFilter ( vlc_object_t * );
static picture_t *Filter( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin ()
    set_description( N_("RV32 conversion filter") )
    set_capability( "video converter", 1 )
    set_callbacks( OpenFilter, NULL )
vlc_module_end ()

/*****************************************************************************
 * OpenFilter: probe the filter and return score
 *****************************************************************************/
static int OpenFilter( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t*)p_this;

    /* XXX Only support RV24 -> RV32 conversion */
    if( p_filter->fmt_in.video.i_chroma != VLC_CODEC_RGB24 ||
        (p_filter->fmt_out.video.i_chroma != VLC_CODEC_RGB32 &&
        p_filter->fmt_out.video.i_chroma != VLC_CODEC_RGBA) )
    {
        return VLC_EGENERIC;
    }

    if( p_filter->fmt_in.video.i_width != p_filter->fmt_out.video.i_width
     || p_filter->fmt_in.video.i_height != p_filter->fmt_out.video.i_height
     || p_filter->fmt_in.video.orientation != p_filter->fmt_out.video.orientation)
        return -1;

    p_filter->pf_video_filter = Filter;

    return VLC_SUCCESS;
}

/****************************************************************************
 * Filter: the whole thing
 ****************************************************************************/
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    picture_t *p_pic_dst;
    int i_plane, i;
    unsigned int j;

    /* Request output picture */
    p_pic_dst = filter_NewPicture( p_filter );
    if( !p_pic_dst )
    {
        picture_Release( p_pic );
        return NULL;
    }

    /* Convert RV24 to RV32 */
    for( i_plane = 0; i_plane < p_pic_dst->i_planes; i_plane++ )
    {
        uint8_t *p_src = p_pic->p[i_plane].p_pixels;
        uint8_t *p_dst = p_pic_dst->p[i_plane].p_pixels;
        unsigned int i_width = p_filter->fmt_out.video.i_width;

        for( i = 0; i < p_pic_dst->p[i_plane].i_lines; i++ )
        {
            for( j = 0; j < i_width; j++ )
            {
                *(p_dst++) = p_src[2];
                *(p_dst++) = p_src[1];
                *(p_dst++) = p_src[0];
                *(p_dst++) = 0xff;  /* Alpha */
                p_src += 3;
            }
            p_src += p_pic->p[i_plane].i_pitch - 3 * i_width;
            p_dst += p_pic_dst->p[i_plane].i_pitch - 4 * i_width;
        }
    }

    picture_CopyProperties( p_pic_dst, p_pic );
    picture_Release( p_pic );

    return p_pic_dst;
}

