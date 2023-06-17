/*****************************************************************************
 * ugly.c : zero-order hold "ugly" resampler
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>
#include <vlc_filter.h>

#define MODULE_NAME     afilter_ugly
#define MODULE_STRING   "afilter_ugly"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int Create (vlc_object_t *);
static int CreateResampler (vlc_object_t *);

static block_t *DoWork( filter_t *, block_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin ()
    set_description( N_("Nearest-neighbor audio resampler") )
    set_capability( "audio converter", 2 )
    set_category( CAT_AUDIO )
    set_subcategory( SUBCAT_AUDIO_RESAMPLER )
    set_callbacks( Create, NULL )

    add_submodule()
    set_capability( "audio resampler", 2 )
    set_callbacks( CreateResampler, NULL )
vlc_module_end ()

/*****************************************************************************
 * Create: allocate ugly resampler
 *****************************************************************************/
static int Create( vlc_object_t *p_this )
{
    filter_t * p_filter = (filter_t *)p_this;

    if( p_filter->fmt_in.audio.i_rate == p_filter->fmt_out.audio.i_rate )
        return VLC_EGENERIC;
    return CreateResampler( p_this );
}

static int CreateResampler( vlc_object_t *p_this )
{
    filter_t * p_filter = (filter_t *)p_this;

    if( p_filter->fmt_in.audio.i_format != p_filter->fmt_out.audio.i_format
     || p_filter->fmt_in.audio.i_channels != p_filter->fmt_out.audio.i_channels
     || !AOUT_FMT_LINEAR( &p_filter->fmt_in.audio ) )
        return VLC_EGENERIC;

    p_filter->pf_audio_filter = DoWork;
    return VLC_SUCCESS;
}

/*****************************************************************************
 * DoWork: convert a buffer
 *****************************************************************************/
static block_t *DoWork( filter_t * p_filter, block_t * p_in_buf )
{
    /* Check if we really need to run the resampler */
    if( p_filter->fmt_out.audio.i_rate == p_filter->fmt_in.audio.i_rate )
        return p_in_buf;

    block_t *p_out_buf = p_in_buf;
    unsigned int i_out_nb = p_in_buf->i_nb_samples
        * p_filter->fmt_out.audio.i_rate / p_filter->fmt_in.audio.i_rate;
    const unsigned framesize = (p_filter->fmt_in.audio.i_bitspersample / 8)
        * aout_FormatNbChannels( &p_filter->fmt_in.audio );

    if( p_filter->fmt_out.audio.i_rate > p_filter->fmt_in.audio.i_rate )
    {
        p_out_buf = block_Alloc( i_out_nb * framesize );
        if( !p_out_buf )
            goto out;
    }

    unsigned char *p_out = p_out_buf->p_buffer;
    unsigned char *p_in = p_in_buf->p_buffer;
    unsigned int i_remainder = 0;

    p_out_buf->i_nb_samples = i_out_nb;
    p_out_buf->i_buffer = i_out_nb * framesize;
    p_out_buf->i_pts = p_in_buf->i_pts;
    p_out_buf->i_length = p_out_buf->i_nb_samples *
        1000000 / p_filter->fmt_out.audio.i_rate;

    while( i_out_nb )
    {
        if( p_out != p_in )
            memcpy( p_out, p_in, framesize );
        p_out += framesize;
        i_out_nb--;

        i_remainder += p_filter->fmt_in.audio.i_rate;
        while( i_remainder >= p_filter->fmt_out.audio.i_rate )
        {
            p_in += framesize;
            i_remainder -= p_filter->fmt_out.audio.i_rate;
        }
    }

    if( p_in_buf != p_out_buf )
out:
        block_Release( p_in_buf );
    return p_out_buf;
}
