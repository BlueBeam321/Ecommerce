/*****************************************************************************
 * avformat.c: demuxer and muxer using libavformat library
 *****************************************************************************/

#ifndef MERGE_FFMPEG

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>

#include "avformat.h"
#include "common/modules/codec/avcodec/avcommon.h"

#define MODULE_NAME     demux_ffmpeg
#define MODULE_STRING   "demux_ffmpeg"

vlc_module_begin ()
#endif /* MERGE_FFMPEG */
    add_shortcut( "ffmpeg", "avformat" )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_DEMUX )
    set_description( N_("Avformat demuxer" ) )
    set_shortname( N_("Avformat") )
    set_capability( "demux", 2 )
    set_callbacks( avformat_OpenDemux, avformat_CloseDemux )
    set_section( N_("Demuxer"), NULL )
    add_string( "avformat-format", NULL, FORMAT_TEXT, FORMAT_LONGTEXT, true )
    add_obsolete_string("ffmpeg-format") /* removed since 2.1.0 */
    add_string( "avformat-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true )

#ifdef ENABLE_SOUT
    /* mux submodule */
    add_submodule ()
    add_shortcut( "ffmpeg", "avformat" )
    set_description( N_("Avformat muxer" ) )
    set_capability( "sout mux", 2 )
    set_section( N_("Muxer"), NULL )
    add_string( "sout-avformat-mux", NULL, MUX_TEXT, MUX_LONGTEXT, true )
    add_obsolete_string("ffmpeg-mux") /* removed since 2.1.0 */
    add_string( "sout-avformat-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true )
    set_callbacks( avformat_OpenMux, avformat_CloseMux )
#endif
#ifndef MERGE_FFMPEG
vlc_module_end ()
#endif
