/*****************************************************************************
 * display.c: display stream output module
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_sout.h>
#include <vlc_block.h>

#define MODULE_NAME     sout_display
#define MODULE_STRING   "sout_display"

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define AUDIO_TEXT N_("Enable audio")
#define AUDIO_LONGTEXT N_( "Enable/disable audio rendering." )
#define VIDEO_TEXT N_("Enable video")
#define VIDEO_LONGTEXT N_( "Enable/disable video rendering." )
#define DELAY_TEXT N_("Delay (ms)")
#define DELAY_LONGTEXT N_( "Introduces a delay in the display of the stream." )

static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

#define SOUT_CFG_PREFIX "sout-display-"

vlc_module_begin ()
    set_shortname( N_("Display"))
    set_description( N_("Display stream output") )
    set_capability( "sout stream", 50 )
    add_shortcut( "display" )
    set_category( CAT_SOUT )
    set_subcategory( SUBCAT_SOUT_STREAM )

    add_bool( SOUT_CFG_PREFIX "audio", true, AUDIO_TEXT,
              AUDIO_LONGTEXT, true )
    add_bool( SOUT_CFG_PREFIX "video", true, VIDEO_TEXT,
              VIDEO_LONGTEXT, true )
    add_integer( SOUT_CFG_PREFIX "delay", 100, DELAY_TEXT,
                 DELAY_LONGTEXT, true )
    set_callbacks( Open, Close )
vlc_module_end ()


/*****************************************************************************
 * Exported prototypes
 *****************************************************************************/
static const char *const ppsz_sout_options[] = {
    "audio", "video", "delay", NULL
};

static sout_stream_id_sys_t *Add( sout_stream_t *, const es_format_t * );
static void              Del ( sout_stream_t *, sout_stream_id_sys_t * );
static int               Send( sout_stream_t *, sout_stream_id_sys_t *, block_t* );

struct sout_stream_sys_t
{
    bool     b_audio;
    bool     b_video;

    mtime_t        i_delay;
    input_resource_t *p_resource;
};

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    sout_stream_t     *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys;

    p_sys = malloc( sizeof( sout_stream_sys_t ) );
    if( p_sys == NULL )
        return VLC_ENOMEM;

    p_sys->p_resource = input_resource_New( p_this );
    if( unlikely(p_sys->p_resource == NULL) )
    {
        free( p_sys );
        return VLC_ENOMEM;
    }

    config_ChainParse( p_stream, SOUT_CFG_PREFIX, ppsz_sout_options,
                   p_stream->p_cfg );

    p_sys->b_audio = var_GetBool( p_stream, SOUT_CFG_PREFIX"audio" );
    p_sys->b_video = var_GetBool( p_stream, SOUT_CFG_PREFIX "video" );
    p_sys->i_delay = var_GetInteger( p_stream, SOUT_CFG_PREFIX "delay" );
    p_sys->i_delay = p_sys->i_delay * CLOCK_FREQ / 1000;

    p_stream->pf_add    = Add;
    p_stream->pf_del    = Del;
    p_stream->pf_send   = Send;
    p_stream->p_sys     = p_sys;
    p_stream->pace_nocontrol = true;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close:
 *****************************************************************************/
static void Close( vlc_object_t * p_this )
{
    sout_stream_t     *p_stream = (sout_stream_t*)p_this;
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    input_resource_Terminate( p_sys->p_resource );
    input_resource_Release( p_sys->p_resource );
    free( p_sys );
}

static sout_stream_id_sys_t * Add( sout_stream_t *p_stream, const es_format_t *p_fmt )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    if( ( p_fmt->i_cat == AUDIO_ES && !p_sys->b_audio )||
        ( p_fmt->i_cat == VIDEO_ES && !p_sys->b_video ) )
    {
        return NULL;
    }

    decoder_t *p_dec = input_DecoderCreate( VLC_OBJECT(p_stream), p_fmt,
                                            p_sys->p_resource );
    if( p_dec == NULL )
    {
        msg_Err( p_stream, "cannot create decoder for fcc=`%4.4s'",
                 (char*)&p_fmt->i_codec );
        return NULL;
    }
    return (sout_stream_id_sys_t *)p_dec;
}

static void Del( sout_stream_t *p_stream, sout_stream_id_sys_t *id )
{
    (void) p_stream;
    input_DecoderDelete( (decoder_t *)id );
}

static int Send( sout_stream_t *p_stream, sout_stream_id_sys_t *id,
                 block_t *p_buffer )
{
    sout_stream_sys_t *p_sys = p_stream->p_sys;

    while( p_buffer )
    {
        block_t *p_next = p_buffer->p_next;

        p_buffer->p_next = NULL;

        if( id != NULL && p_buffer->i_buffer > 0 )
        {
            if( p_buffer->i_dts <= VLC_TS_INVALID )
                p_buffer->i_dts = 0;
            else
                p_buffer->i_dts += p_sys->i_delay;

            if( p_buffer->i_pts <= VLC_TS_INVALID )
                p_buffer->i_pts = 0;
            else
                p_buffer->i_pts += p_sys->i_delay;

            input_DecoderDecode( (decoder_t *)id, p_buffer, false );
        }

        p_buffer = p_next;
    }

    return VLC_SUCCESS;
}
