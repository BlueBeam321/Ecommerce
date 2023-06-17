/*****************************************************************************
 * mms.c: MMS over tcp, udp and http access plug-in
 *****************************************************************************/

#include <vlc_common.h>
#include <vlc_fixups.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_access.h>

#include "mms.h"

#define MODULE_NAME     access_mms
#define MODULE_STRING   "access_mms"

/****************************************************************************
 * NOTES:
 *  MMSProtocole documentation found at http://get.to/sdp
 ****************************************************************************/

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

#define ALL_TEXT N_("Force selection of all streams")
#define ALL_LONGTEXT N_( \
    "MMS streams can contain several elementary streams, with different " \
    "bitrates. You can choose to select all of them." )

#define BITRATE_TEXT N_( "Maximum bitrate" )
#define BITRATE_LONGTEXT N_( \
    "Select the stream with the maximum bitrate under that limit."  )

#define TIMEOUT_TEXT N_("TCP/UDP timeout (ms)")
#define TIMEOUT_LONGTEXT N_("Amount of time (in ms) to wait before aborting network reception of data. Note that there will be 10 retries before completely giving up.")

vlc_module_begin ()
    set_shortname( "MMS" )
    set_description( N_("Microsoft Media Server (MMS) input") )
    set_capability( "access", -1 )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_ACCESS )

    add_integer( "mms-timeout", 5000, TIMEOUT_TEXT, TIMEOUT_LONGTEXT,
                 true )

    add_bool( "mms-all", false, ALL_TEXT, ALL_LONGTEXT, true )
    add_integer( "mms-maxbitrate", 0, BITRATE_TEXT, BITRATE_LONGTEXT ,
                 false )
    add_obsolete_string( "mmsh-proxy" ) /* since 3.0.0 */

    add_shortcut( "mms", "mmsu", "mmst", "mmsh" )
    set_callbacks( Open, Close )
vlc_module_end ()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
struct access_sys_t
{
    int i_proto;
};

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    stream_t *p_access = (stream_t*)p_this;

    /* use specified method */
    if( !strncmp( p_access->psz_name, "mmsu", 4 ) )
        return  MMSTUOpen ( p_access );
    else if( !strncmp( p_access->psz_name, "mmst", 4 ) )
        return  MMSTUOpen ( p_access );
    else if( !strncmp( p_access->psz_name, "mmsh", 4 ) )
        return  MMSHOpen ( p_access );

    if( MMSTUOpen ( p_access ) )
    {   /* try mmsh if mmstu failed */
        return  MMSHOpen ( p_access );
    }
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: free unused data structures
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    stream_t     *p_access = (stream_t*)p_this;
    access_sys_t *p_sys = p_access->p_sys;

    if( ( p_sys->i_proto == MMS_PROTO_TCP ) ||
        ( p_sys->i_proto == MMS_PROTO_UDP ) )
    {
         MMSTUClose ( p_access );
    }
    else if( p_sys->i_proto == MMS_PROTO_HTTP )
    {
         MMSHClose ( p_access );
    }
}
