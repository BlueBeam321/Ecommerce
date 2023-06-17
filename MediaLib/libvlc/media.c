/*****************************************************************************
 * media.c: Libvlc API media descriptor management
 *****************************************************************************/

#include "libvlc_internal.h"

#include <assert.h>
#include <errno.h>

#include <vlc/libvlc_instance.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>

#include <vlc_input.h>
#include <vlc_meta.h>
#include <vlc_playlist.h>
#include <vlc_url.h>
#include <vlc_network.h>
#include <vlc_charset.h>
#include <vlc_internal.h>

#include <dvbpsi/dvbpsi.h>
#include <dvbpsi/psi.h>
#include <dvbpsi/demux.h>
#include <dvbpsi/pat.h>
#include <dvbpsi/descriptor.h>
#include <dvbpsi/sdt.h>
#include <dvbpsi/dr.h>

#include "media_internal.h"
#include "media_list_internal.h"
#include "media_player_internal.h"
#include "common/core/input/input_internal.h"
#include "common/modules/demux/dvb_text.h"

static const vlc_meta_type_t libvlc_to_vlc_meta[] =
{
    [libvlc_meta_Title]        = vlc_meta_Title,
    [libvlc_meta_Artist]       = vlc_meta_Artist,
    [libvlc_meta_Genre]        = vlc_meta_Genre,
    [libvlc_meta_Copyright]    = vlc_meta_Copyright,
    [libvlc_meta_Album]        = vlc_meta_Album,
    [libvlc_meta_TrackNumber]  = vlc_meta_TrackNumber,
    [libvlc_meta_Description]  = vlc_meta_Description,
    [libvlc_meta_Rating]       = vlc_meta_Rating,
    [libvlc_meta_Date]         = vlc_meta_Date,
    [libvlc_meta_Setting]      = vlc_meta_Setting,
    [libvlc_meta_URL]          = vlc_meta_URL,
    [libvlc_meta_Language]     = vlc_meta_Language,
    [libvlc_meta_NowPlaying]   = vlc_meta_NowPlaying,
    [libvlc_meta_Publisher]    = vlc_meta_Publisher,
    [libvlc_meta_EncodedBy]    = vlc_meta_EncodedBy,
    [libvlc_meta_ArtworkURL]   = vlc_meta_ArtworkURL,
    [libvlc_meta_TrackID]      = vlc_meta_TrackID,
    [libvlc_meta_TrackTotal]   = vlc_meta_TrackTotal,
    [libvlc_meta_Director]     = vlc_meta_Director,
    [libvlc_meta_Season]       = vlc_meta_Season,
    [libvlc_meta_Episode]      = vlc_meta_Episode,
    [libvlc_meta_ShowName]     = vlc_meta_ShowName,
    [libvlc_meta_Actors]       = vlc_meta_Actors,
    [libvlc_meta_AlbumArtist]  = vlc_meta_AlbumArtist,
    [libvlc_meta_DiscNumber]   = vlc_meta_DiscNumber,
    [libvlc_meta_DiscTotal]    = vlc_meta_DiscTotal
};

static const libvlc_meta_t vlc_to_libvlc_meta[] =
{
    [vlc_meta_Title]        = libvlc_meta_Title,
    [vlc_meta_Artist]       = libvlc_meta_Artist,
    [vlc_meta_Genre]        = libvlc_meta_Genre,
    [vlc_meta_Copyright]    = libvlc_meta_Copyright,
    [vlc_meta_Album]        = libvlc_meta_Album,
    [vlc_meta_TrackNumber]  = libvlc_meta_TrackNumber,
    [vlc_meta_Description]  = libvlc_meta_Description,
    [vlc_meta_Rating]       = libvlc_meta_Rating,
    [vlc_meta_Date]         = libvlc_meta_Date,
    [vlc_meta_Setting]      = libvlc_meta_Setting,
    [vlc_meta_URL]          = libvlc_meta_URL,
    [vlc_meta_Language]     = libvlc_meta_Language,
    [vlc_meta_NowPlaying]   = libvlc_meta_NowPlaying,
    [vlc_meta_ESNowPlaying] = libvlc_meta_NowPlaying,
    [vlc_meta_Publisher]    = libvlc_meta_Publisher,
    [vlc_meta_EncodedBy]    = libvlc_meta_EncodedBy,
    [vlc_meta_ArtworkURL]   = libvlc_meta_ArtworkURL,
    [vlc_meta_TrackID]      = libvlc_meta_TrackID,
    [vlc_meta_TrackTotal]   = libvlc_meta_TrackTotal,
    [vlc_meta_Director]     = libvlc_meta_Director,
    [vlc_meta_Season]       = libvlc_meta_Season,
    [vlc_meta_Episode]      = libvlc_meta_Episode,
    [vlc_meta_ShowName]     = libvlc_meta_ShowName,
    [vlc_meta_Actors]       = libvlc_meta_Actors,
    [vlc_meta_AlbumArtist]  = libvlc_meta_AlbumArtist,
    [vlc_meta_DiscNumber]   = libvlc_meta_DiscNumber,
    [vlc_meta_DiscTotal]    = libvlc_meta_DiscTotal
};

static libvlc_media_list_t *media_get_subitems( libvlc_media_t * p_md,
                                                bool b_create )
{
    libvlc_media_list_t *p_subitems = NULL;

    vlc_mutex_lock( &p_md->subitems_lock );
    if( p_md->p_subitems == NULL && b_create )
    {
        p_md->p_subitems = libvlc_media_list_new( p_md->p_libvlc_instance );
        if( p_md->p_subitems != NULL )
        {
            p_md->p_subitems->b_read_only = true;
            p_md->p_subitems->p_internal_md = p_md;
        }
    }
    p_subitems = p_md->p_subitems;
    vlc_mutex_unlock( &p_md->subitems_lock );
    return p_subitems;
}

static libvlc_media_t *input_item_add_subitem( libvlc_media_t *p_md,
                                               input_item_t *item )
{
    libvlc_media_t * p_md_child;
    libvlc_media_list_t *p_subitems;
    libvlc_event_t event;

    p_md_child = libvlc_media_new_from_input_item( p_md->p_libvlc_instance,
                                                   item );

    /* Add this to our media list */
    p_subitems = media_get_subitems( p_md, true );
    if( p_subitems != NULL )
    {
        libvlc_media_list_lock( p_subitems );
        libvlc_media_list_internal_add_media( p_subitems, p_md_child );
        libvlc_media_list_unlock( p_subitems );
    }

    /* Construct the event */
    event.type = libvlc_MediaSubItemAdded;
    event.u.media_subitem_added.new_child = p_md_child;

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );
    return p_md_child;
}

static void input_item_add_subnode( libvlc_media_t *md,
                                    input_item_node_t *node )
{
    for( int i = 0; i < node->i_children; i++ )
    {
        input_item_node_t *child = node->pp_children[i];
        libvlc_media_t *md_child = input_item_add_subitem( md, child->p_item );

        if( md_child != NULL )
        {
            input_item_add_subnode( md_child, child );
            libvlc_media_release( md_child );
        }
    }
}

/**************************************************************************
 * input_item_subitemtree_added (Private) (vlc event Callback)
 **************************************************************************/
static void input_item_subitemtree_added( const vlc_event_t * p_event,
                                          void * user_data )
{
    libvlc_media_t * p_md = user_data;
    libvlc_event_t event;
    input_item_node_t *node = p_event->u.input_item_subitem_tree_added.p_root;

    /* FIXME FIXME FIXME
     * Recursive function calls seem much simpler for this. But playlists are
     * untrusted and can be arbitrarily deep (e.g. with XSPF). So recursion can
     * potentially lead to plain old stack overflow. */
    input_item_add_subnode( p_md, node );

    /* Construct the event */
    event.type = libvlc_MediaSubItemTreeAdded;
    event.u.media_subitemtree_added.item = p_md;

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );
}

/**************************************************************************
 * input_item_meta_changed (Private) (vlc event Callback)
 **************************************************************************/
static void input_item_meta_changed( const vlc_event_t *p_event,
                                     void * user_data )
{
    libvlc_media_t * p_md = user_data;
    libvlc_event_t event;

    /* Construct the event */
    event.type = libvlc_MediaMetaChanged;
    event.u.media_meta_changed.meta_type =
        vlc_to_libvlc_meta[p_event->u.input_item_meta_changed.meta_type];

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );
}

/**************************************************************************
 * input_item_duration_changed (Private) (vlc event Callback)
 **************************************************************************/
static void input_item_duration_changed( const vlc_event_t *p_event,
                                         void * user_data )
{
    libvlc_media_t * p_md = user_data;
    libvlc_event_t event;

    /* Construct the event */
    event.type = libvlc_MediaDurationChanged;
    event.u.media_duration_changed.new_duration =
        from_mtime(p_event->u.input_item_duration_changed.new_duration);

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );
}

static void send_parsed_changed( libvlc_media_t *p_md,
                                 libvlc_media_parsed_status_t new_status )
{
    libvlc_event_t event;

    vlc_mutex_lock( &p_md->parsed_lock );
    if( p_md->parsed_status == new_status )
    {
        vlc_mutex_unlock( &p_md->parsed_lock );
        return;
    }

    /* Legacy: notify libvlc_media_parse */
    if( !p_md->is_parsed )
    {
        p_md->is_parsed = true;
        vlc_cond_broadcast( &p_md->parsed_cond );
    }

    p_md->parsed_status = new_status;
    if( p_md->parsed_status == libvlc_media_parsed_status_skipped )
        p_md->has_asked_preparse = false;

    vlc_mutex_unlock( &p_md->parsed_lock );

    /* Construct the event */
    event.type = libvlc_MediaParsedChanged;
    event.u.media_parsed_changed.new_status = new_status;

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );

    libvlc_media_list_t *p_subitems = media_get_subitems( p_md, false );
    if( p_subitems != NULL )
    {
        /* notify the media list */
        libvlc_media_list_lock( p_subitems );
        libvlc_media_list_internal_end_reached( p_subitems );
        libvlc_media_list_unlock( p_subitems );
    }
}

/**************************************************************************
 * input_item_preparse_ended (Private) (vlc event Callback)
 **************************************************************************/
static void input_item_preparse_ended( const vlc_event_t * p_event,
                                       void * user_data )
{
    libvlc_media_t * p_md = user_data;
    libvlc_media_parsed_status_t new_status;

    switch( p_event->u.input_item_preparse_ended.new_status )
    {
        case ITEM_PREPARSE_SKIPPED:
            new_status = libvlc_media_parsed_status_skipped;
            break;
        case ITEM_PREPARSE_FAILED:
            new_status = libvlc_media_parsed_status_failed;
            break;
        case ITEM_PREPARSE_TIMEOUT:
            new_status = libvlc_media_parsed_status_timeout;
            break;
        case ITEM_PREPARSE_DONE:
            new_status = libvlc_media_parsed_status_done;
            break;
        default:
            return;
    }
    send_parsed_changed( p_md, new_status );
}

/**************************************************************************
 * Install event handler (Private)
 **************************************************************************/
static void install_input_item_observer( libvlc_media_t *p_md )
{
    vlc_event_attach( &p_md->p_input_item->event_manager,
                      vlc_InputItemMetaChanged,
                      input_item_meta_changed,
                      p_md );
    vlc_event_attach( &p_md->p_input_item->event_manager,
                      vlc_InputItemDurationChanged,
                      input_item_duration_changed,
                      p_md );
    vlc_event_attach( &p_md->p_input_item->event_manager,
                      vlc_InputItemSubItemTreeAdded,
                      input_item_subitemtree_added,
                      p_md );
    vlc_event_attach( &p_md->p_input_item->event_manager,
                      vlc_InputItemPreparseEnded,
                      input_item_preparse_ended,
                      p_md );
}

/**************************************************************************
 * Uninstall event handler (Private)
 **************************************************************************/
static void uninstall_input_item_observer( libvlc_media_t *p_md )
{
    vlc_event_detach( &p_md->p_input_item->event_manager,
                      vlc_InputItemMetaChanged,
                      input_item_meta_changed,
                      p_md );
    vlc_event_detach( &p_md->p_input_item->event_manager,
                      vlc_InputItemDurationChanged,
                      input_item_duration_changed,
                      p_md );
    vlc_event_detach( &p_md->p_input_item->event_manager,
                      vlc_InputItemSubItemTreeAdded,
                      input_item_subitemtree_added,
                      p_md );
    vlc_event_detach( &p_md->p_input_item->event_manager,
                      vlc_InputItemPreparseEnded,
                      input_item_preparse_ended,
                      p_md );
}

/**************************************************************************
 * Create a new media descriptor object from an input_item
 * (libvlc internal)
 * That's the generic constructor
 **************************************************************************/
libvlc_media_t * libvlc_media_new_from_input_item(
                                   libvlc_instance_t *p_instance,
                                   input_item_t *p_input_item )
{
    libvlc_media_t * p_md;

    if (!p_input_item)
    {
        libvlc_printerr( "No input item given" );
        return NULL;
    }

    p_md = calloc( 1, sizeof(libvlc_media_t) );
    if( !p_md )
    {
        libvlc_printerr( "Not enough memory" );
        return NULL;
    }

    p_md->p_libvlc_instance = p_instance;
    p_md->p_input_item      = p_input_item;
    p_md->i_refcount        = 1;

    vlc_cond_init(&p_md->parsed_cond);
    vlc_mutex_init(&p_md->parsed_lock);
    vlc_mutex_init(&p_md->subitems_lock);

    p_md->state = libvlc_NothingSpecial;

    /* A media descriptor can be a playlist. When you open a playlist
     * It can give a bunch of item to read. */
    p_md->p_subitems        = NULL;

    libvlc_event_manager_init( &p_md->event_manager, p_md );

    input_item_Hold( p_md->p_input_item );

    install_input_item_observer( p_md );

    libvlc_retain( p_instance );
    return p_md;
}

/**************************************************************************
 * Create a new media descriptor object
 **************************************************************************/
libvlc_media_t *libvlc_media_new_location( libvlc_instance_t *p_instance,
                                           const char * psz_mrl )
{
    input_item_t * p_input_item;
    libvlc_media_t * p_md;

    p_input_item = input_item_New( psz_mrl, NULL );

    if (!p_input_item)
    {
        libvlc_printerr( "Not enough memory" );
        return NULL;
    }

    p_md = libvlc_media_new_from_input_item( p_instance, p_input_item );

    /* The p_input_item is retained in libvlc_media_new_from_input_item */
    input_item_Release( p_input_item );

    return p_md;
}

libvlc_media_t *libvlc_media_new_path( libvlc_instance_t *p_instance,
                                       const char *path )
{
    char *mrl = vlc_path2uri( path, NULL );
    if( unlikely(mrl == NULL) )
    {
        libvlc_printerr( "%s", vlc_strerror_c(errno) );
        return NULL;
    }

    libvlc_media_t *m = libvlc_media_new_location( p_instance, mrl );
    free( mrl );
    return m;
}

libvlc_media_t *libvlc_media_new_fd( libvlc_instance_t *p_instance, int fd )
{
    char mrl[16];
    snprintf( mrl, sizeof(mrl), "fd://%d", fd );

    return libvlc_media_new_location( p_instance, mrl );
}

libvlc_media_t *libvlc_media_new_callbacks(libvlc_instance_t *p_instance,
                                           libvlc_media_open_cb open_cb,
                                           libvlc_media_read_cb read_cb,
                                           libvlc_media_seek_cb seek_cb,
                                           libvlc_media_close_cb close_cb,
                                           void *opaque)
{
    libvlc_media_t *m = libvlc_media_new_location(p_instance, "imem://");
    if (unlikely(m == NULL))
        return NULL;

    assert(read_cb != NULL);
    input_item_AddOpaque(m->p_input_item, "imem-data", opaque);
    input_item_AddOpaque(m->p_input_item, "imem-open", open_cb);
    input_item_AddOpaque(m->p_input_item, "imem-read", read_cb);
    input_item_AddOpaque(m->p_input_item, "imem-seek", seek_cb);
    input_item_AddOpaque(m->p_input_item, "imem-close", close_cb);
    return m;
}

/**************************************************************************
 * Add an option to the media descriptor,
 * that will be used to determine how the media_player will read the
 * media. This allow to use VLC advanced reading/streaming
 * options in a per-media basis
 *
 * The options are detailed in vlc --long-help, for instance "--sout-all"
 **************************************************************************/
void libvlc_media_add_option( libvlc_media_t * p_md,
                              const char * psz_option )
{
    char *p;
#if CRYPT_ENABLE
    p = sinjong_decrypt_text(psz_option);
#else
    p = psz_option;
#endif
    libvlc_media_add_option_flag(p_md, p,
                          VLC_INPUT_OPTION_UNIQUE|VLC_INPUT_OPTION_TRUSTED );
}

/**************************************************************************
 * Same as libvlc_media_add_option but with configurable flags.
 **************************************************************************/
void libvlc_media_add_option_flag( libvlc_media_t * p_md,
                                   const char * ppsz_option,
                                   unsigned i_flags )
{
    input_item_AddOption( p_md->p_input_item, ppsz_option, i_flags );
}

/**************************************************************************
 * Delete a media descriptor object
 **************************************************************************/
void libvlc_media_release( libvlc_media_t *p_md )
{
    if (!p_md)
        return;

    p_md->i_refcount--;

    if( p_md->i_refcount > 0 )
        return;

    uninstall_input_item_observer( p_md );

    /* Cancel asynchronous parsing (if any) */
    libvlc_MetadataCancel( p_md->p_libvlc_instance->p_libvlc_int, p_md );

    if( p_md->p_subitems )
        libvlc_media_list_release( p_md->p_subitems );

    input_item_Release( p_md->p_input_item );

    vlc_cond_destroy( &p_md->parsed_cond );
    vlc_mutex_destroy( &p_md->parsed_lock );
    vlc_mutex_destroy( &p_md->subitems_lock );

    /* Construct the event */
    libvlc_event_t event;
    event.type = libvlc_MediaFreed;
    event.u.media_freed.md = p_md;

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );

    libvlc_event_manager_destroy( &p_md->event_manager );
    libvlc_release( p_md->p_libvlc_instance );
    free( p_md );
}

/**************************************************************************
 * Retain a media descriptor object
 **************************************************************************/
void libvlc_media_retain( libvlc_media_t *p_md )
{
    assert (p_md);
    p_md->i_refcount++;
}

/**************************************************************************
 * Duplicate a media descriptor object
 **************************************************************************/
libvlc_media_t *
libvlc_media_duplicate( libvlc_media_t *p_md_orig )
{
    return libvlc_media_new_from_input_item(
        p_md_orig->p_libvlc_instance, p_md_orig->p_input_item );
}

/**************************************************************************
 * Get mrl from a media descriptor object
 **************************************************************************/
char *
libvlc_media_get_mrl( libvlc_media_t * p_md )
{
    assert( p_md );
    return input_item_GetURI( p_md->p_input_item );
}

/**************************************************************************
 * Getter for meta information
 **************************************************************************/

char *libvlc_media_get_meta( libvlc_media_t *p_md, libvlc_meta_t e_meta )
{
    char *psz_meta = NULL;

    if( e_meta == libvlc_meta_NowPlaying )
    {
        psz_meta = input_item_GetNowPlayingFb( p_md->p_input_item );
    }
    else
    {
        psz_meta = input_item_GetMeta( p_md->p_input_item,
                                             libvlc_to_vlc_meta[e_meta] );
        /* Should be integrated in core */
        if( psz_meta == NULL && e_meta == libvlc_meta_Title
         && p_md->p_input_item->psz_name != NULL )
            psz_meta = strdup( p_md->p_input_item->psz_name );
    }
    return psz_meta;
}

/**************************************************************************
 * Setter for meta information
 **************************************************************************/

void libvlc_media_set_meta( libvlc_media_t *p_md, libvlc_meta_t e_meta, const char *psz_value )
{
    assert( p_md );
    input_item_SetMeta( p_md->p_input_item, libvlc_to_vlc_meta[e_meta], psz_value );
}

int libvlc_media_save_meta( libvlc_media_t *p_md )
{
    assert( p_md );
    vlc_object_t *p_obj = VLC_OBJECT(p_md->p_libvlc_instance->p_libvlc_int);
    return input_item_WriteMeta( p_obj, p_md->p_input_item ) == VLC_SUCCESS;
}

/**************************************************************************
 * Getter for state information
 * Can be error, playing, buffering, NothingSpecial.
 **************************************************************************/

libvlc_state_t
libvlc_media_get_state( libvlc_media_t *p_md )
{
    assert( p_md );
    return p_md->state;
}

/**************************************************************************
 * Setter for state information (LibVLC Internal)
 **************************************************************************/

void
libvlc_media_set_state( libvlc_media_t *p_md,
                                   libvlc_state_t state )
{
    libvlc_event_t event;

    p_md->state = state;

    /* Construct the event */
    event.type = libvlc_MediaStateChanged;
    event.u.media_state_changed.new_state = state;

    /* Send the event */
    libvlc_event_send( &p_md->event_manager, &event );
}

/**************************************************************************
* subitems
**************************************************************************/
libvlc_media_list_t *
libvlc_media_subitems(libvlc_media_t * p_md)
{
    libvlc_media_list_t *p_subitems = media_get_subitems(p_md, true);
    if (p_subitems)
        libvlc_media_list_retain(p_subitems);
    return p_subitems;
}

/**************************************************************************
 * Getter for statistics information
 **************************************************************************/
int libvlc_media_get_stats( libvlc_media_t *p_md,
                            libvlc_media_stats_t *p_stats )
{
    input_item_t *item = p_md->p_input_item;
    libvlc_media_player_t* p_mi = (libvlc_media_player_t*)p_md->p_user_data;
    if (!p_md->p_input_item || !p_mi)
        return false;

    vlc_mutex_lock( &item->lock );
    
    input_thread_t* p_input = libvlc_get_input_thread(p_mi);
    stats_ComputeInputStats(p_input, input_priv(p_input)->p_item->p_stats);
    vlc_object_release(p_input);

    input_stats_t *p_itm_stats = p_md->p_input_item->p_stats;
    if( p_itm_stats == NULL )
    {
        vlc_mutex_unlock( &item->lock );
        return false;
    }

    vlc_mutex_lock( &p_itm_stats->lock );
    p_stats->i_read_bytes = p_itm_stats->i_read_bytes;
    p_stats->f_input_bitrate = p_itm_stats->f_input_bitrate;

    p_stats->i_demux_read_bytes = p_itm_stats->i_demux_read_bytes;
    p_stats->f_demux_bitrate = p_itm_stats->f_demux_bitrate;
    p_stats->i_demux_corrupted = p_itm_stats->i_demux_corrupted;
    p_stats->i_demux_discontinuity = p_itm_stats->i_demux_discontinuity;

    p_stats->i_decoded_video = p_itm_stats->i_decoded_video;
    p_stats->i_decoded_audio = p_itm_stats->i_decoded_audio;

    p_stats->i_displayed_pictures = p_itm_stats->i_displayed_pictures;
    p_stats->i_lost_pictures = p_itm_stats->i_lost_pictures;

    p_stats->i_played_abuffers = p_itm_stats->i_played_abuffers;
    p_stats->i_lost_abuffers = p_itm_stats->i_lost_abuffers;

    p_stats->i_sent_packets = p_itm_stats->i_sent_packets;
    p_stats->i_sent_bytes = p_itm_stats->i_sent_bytes;
    p_stats->f_send_bitrate = p_itm_stats->f_send_bitrate;
    vlc_mutex_unlock( &p_itm_stats->lock );
    vlc_mutex_unlock( &item->lock );
    return true;
}

/**************************************************************************
 * event_manager
 **************************************************************************/
libvlc_event_manager_t *
libvlc_media_event_manager( libvlc_media_t * p_md )
{
    assert( p_md );

    return &p_md->event_manager;
}

/**************************************************************************
 * Get duration of media object (in ms)
 **************************************************************************/
int64_t
libvlc_media_get_duration( libvlc_media_t * p_md )
{
    assert( p_md );

    if( !p_md->p_input_item )
    {
        libvlc_printerr( "No input item" );
        return -1;
    }

    if (!input_item_IsPreparsed( p_md->p_input_item ))
        return -1;

    return from_mtime(input_item_GetDuration( p_md->p_input_item ));
}

static int media_parse(libvlc_media_t *media, bool b_async,
                       libvlc_media_parse_flag_t parse_flag, int timeout)
{
    bool needed;

    vlc_mutex_lock(&media->parsed_lock);
    needed = !media->has_asked_preparse;
    media->has_asked_preparse = true;
    if (needed)
        media->is_parsed = false;
    vlc_mutex_unlock(&media->parsed_lock);

    if (needed)
    {
        libvlc_int_t *libvlc = media->p_libvlc_instance->p_libvlc_int;
        input_item_t *item = media->p_input_item;
        input_item_meta_request_option_t parse_scope = META_REQUEST_OPTION_SCOPE_LOCAL;
        int ret;

        /* Ignore libvlc_media_fetch_local flag since local art will be fetched
         * by libvlc_MetadataRequest */
        if (parse_flag & libvlc_media_fetch_network)
        {
            ret = libvlc_ArtRequest(libvlc, item,
                                    META_REQUEST_OPTION_SCOPE_NETWORK);
            if (ret != VLC_SUCCESS)
                return ret;
        }

        if (parse_flag & libvlc_media_parse_network)
            parse_scope |= META_REQUEST_OPTION_SCOPE_NETWORK;
        if (parse_flag & libvlc_media_do_interact)
            parse_scope |= META_REQUEST_OPTION_DO_INTERACT;
        ret = libvlc_MetadataRequest(libvlc, item, parse_scope, timeout, media);
        if (ret != VLC_SUCCESS)
            return ret;
    }
    else
        return VLC_EGENERIC;

    if (!b_async)
    {
        vlc_mutex_lock(&media->parsed_lock);
        while (!media->is_parsed)
            vlc_cond_wait(&media->parsed_cond, &media->parsed_lock);
        vlc_mutex_unlock(&media->parsed_lock);
    }
    return VLC_SUCCESS;
}

/**************************************************************************
 * Parse the media and wait.
 **************************************************************************/
void
libvlc_media_parse(libvlc_media_t *media)
{
    media_parse( media, false, libvlc_media_fetch_local, -1 );
}

/**************************************************************************
 * Parse the media but do not wait.
 **************************************************************************/
void
libvlc_media_parse_async(libvlc_media_t *media)
{
    media_parse( media, true, libvlc_media_fetch_local, -1 );
}

/**************************************************************************
 * Parse the media asynchronously with options.
 **************************************************************************/
int
libvlc_media_parse_with_options( libvlc_media_t *media,
                                 libvlc_media_parse_flag_t parse_flag,
                                 int timeout )
{
    return media_parse( media, true, parse_flag, timeout ) == VLC_SUCCESS ? 0 : -1;
}

void
libvlc_media_parse_stop( libvlc_media_t *media )
{
    libvlc_MetadataCancel( media->p_libvlc_instance->p_libvlc_int, media );
}

/**************************************************************************
 * Get parsed status for media object.
 **************************************************************************/
int
libvlc_media_is_parsed(libvlc_media_t *media)
{
    bool parsed;

    vlc_mutex_lock(&media->parsed_lock);
    parsed = media->is_parsed;
    vlc_mutex_unlock(&media->parsed_lock);
    return parsed;
}

libvlc_media_parsed_status_t
libvlc_media_get_parsed_status(libvlc_media_t *media)
{
    libvlc_media_parsed_status_t status;

    vlc_mutex_lock(&media->parsed_lock);
    status = media->parsed_status;
    vlc_mutex_unlock(&media->parsed_lock);
    return status;
}

/**************************************************************************
 * Get media descriptor's elementary streams description
 **************************************************************************/
int
libvlc_media_get_tracks_info( libvlc_media_t *p_md, libvlc_media_track_info_t ** pp_es )
{
    assert( p_md );

    input_item_t *p_input_item = p_md->p_input_item;
    vlc_mutex_lock( &p_input_item->lock );

    const int i_es = p_input_item->i_es;
    *pp_es = (i_es > 0) ? vlc_alloc( i_es, sizeof(libvlc_media_track_info_t) ) : NULL;

    if( !*pp_es ) /* no ES, or OOM */
    {
        vlc_mutex_unlock( &p_input_item->lock );
        return 0;
    }

    /* Fill array */
    for( int i = 0; i < i_es; i++ )
    {
        libvlc_media_track_info_t *p_mes = *pp_es+i;
        const es_format_t *p_es = p_input_item->es[i];

        p_mes->i_codec = p_es->i_codec;
        p_mes->i_id = p_es->i_id;

        p_mes->i_profile = p_es->i_profile;
        p_mes->i_level = p_es->i_level;

        switch(p_es->i_cat)
        {
        case UNKNOWN_ES:
        default:
            p_mes->i_type = libvlc_track_unknown;
            break;
        case VIDEO_ES:
            p_mes->i_type = libvlc_track_video;
            p_mes->u.video.i_height = p_es->video.i_visible_height;
            p_mes->u.video.i_width = p_es->video.i_visible_width;
            break;
        case AUDIO_ES:
            p_mes->i_type = libvlc_track_audio;
            p_mes->u.audio.i_channels = p_es->audio.i_channels;
            p_mes->u.audio.i_rate = p_es->audio.i_rate;
            break;
        case SPU_ES:
            p_mes->i_type = libvlc_track_text;
            break;
        }
    }

    vlc_mutex_unlock( &p_input_item->lock );
    return i_es;
}

unsigned
libvlc_media_tracks_get( libvlc_media_t *p_md, libvlc_media_track_t *** pp_es )
{
    assert( p_md );

    input_item_t *p_input_item = p_md->p_input_item;
    vlc_mutex_lock( &p_input_item->lock );

    const int i_es = p_input_item->i_es;
    *pp_es = (i_es > 0) ? calloc( i_es, sizeof(**pp_es) ) : NULL;

    if( !*pp_es ) /* no ES, or OOM */
    {
        vlc_mutex_unlock( &p_input_item->lock );
        return 0;
    }

    /* Fill array */
    for( int i = 0; i < i_es; i++ )
    {
        libvlc_media_track_t *p_mes = calloc( 1, sizeof(*p_mes) );
        if ( p_mes )
        {
            p_mes->audio = malloc( __MAX(__MAX(sizeof(*p_mes->audio),
                                               sizeof(*p_mes->video)),
                                               sizeof(*p_mes->subtitle)) );
        }
        if ( !p_mes || !p_mes->audio )
        {
            libvlc_media_tracks_release( *pp_es, i_es );
            *pp_es = NULL;
            free( p_mes );
            vlc_mutex_unlock( &p_input_item->lock );
            return 0;
        }
        (*pp_es)[i] = p_mes;

        const es_format_t *p_es = p_input_item->es[i];

        p_mes->i_codec = p_es->i_codec;
        p_mes->i_original_fourcc = p_es->i_original_fourcc;
        p_mes->i_id = p_es->i_id;

        p_mes->i_profile = p_es->i_profile;
        p_mes->i_level = p_es->i_level;

        p_mes->i_bitrate = p_es->i_bitrate;
        p_mes->psz_language = p_es->psz_language != NULL ? strdup(p_es->psz_language) : NULL;
        p_mes->psz_description = p_es->psz_description != NULL ? strdup(p_es->psz_description) : NULL;

        switch(p_es->i_cat)
        {
        case UNKNOWN_ES:
        default:
            p_mes->i_type = libvlc_track_unknown;
            break;
        case VIDEO_ES:
            p_mes->i_type = libvlc_track_video;
            p_mes->video->i_height = p_es->video.i_visible_height;
            p_mes->video->i_width = p_es->video.i_visible_width;
            p_mes->video->i_sar_num = p_es->video.i_sar_num;
            p_mes->video->i_sar_den = p_es->video.i_sar_den;
            p_mes->video->i_frame_rate_num = p_es->video.i_frame_rate;
            p_mes->video->i_frame_rate_den = p_es->video.i_frame_rate_base;

            assert( p_es->video.orientation >= ORIENT_TOP_LEFT &&
                    p_es->video.orientation <= ORIENT_RIGHT_BOTTOM );
            p_mes->video->i_orientation = (int) p_es->video.orientation;

            assert( ( p_es->video.projection_mode >= PROJECTION_MODE_RECTANGULAR &&
                    p_es->video.projection_mode <= PROJECTION_MODE_EQUIRECTANGULAR ) ||
                    ( p_es->video.projection_mode == PROJECTION_MODE_CUBEMAP_LAYOUT_STANDARD ) );
            p_mes->video->i_projection = (int) p_es->video.projection_mode;

            p_mes->video->pose.f_yaw = p_es->video.pose.yaw;
            p_mes->video->pose.f_pitch = p_es->video.pose.pitch;
            p_mes->video->pose.f_roll = p_es->video.pose.roll;
            p_mes->video->pose.f_field_of_view = p_es->video.pose.fov;
            break;
        case AUDIO_ES:
            p_mes->i_type = libvlc_track_audio;
            p_mes->audio->i_channels = p_es->audio.i_channels;
            p_mes->audio->i_rate = p_es->audio.i_rate;
            break;
        case SPU_ES:
            p_mes->i_type = libvlc_track_text;
            p_mes->subtitle->psz_encoding = p_es->subs.psz_encoding != NULL ?
                                            strdup(p_es->subs.psz_encoding) : NULL;
            break;
        }
    }

    vlc_mutex_unlock( &p_input_item->lock );
    return i_es;
}

/**************************************************************************
 * Get codec description from media elementary stream
 **************************************************************************/
const char *
libvlc_media_get_codec_description( libvlc_track_type_t i_type,
                                    uint32_t i_codec )
{
    switch( i_type )
    {
        case libvlc_track_audio:
            return vlc_fourcc_GetDescription( AUDIO_ES, i_codec );
        case libvlc_track_video:
            return vlc_fourcc_GetDescription( VIDEO_ES, i_codec );
        case libvlc_track_text:
            return vlc_fourcc_GetDescription( SPU_ES, i_codec );
        case libvlc_track_unknown:
        default:
            return vlc_fourcc_GetDescription( UNKNOWN_ES, i_codec );
    }
}

/**************************************************************************
 * Release media descriptor's elementary streams description array
 **************************************************************************/
void libvlc_media_tracks_release( libvlc_media_track_t **p_tracks, unsigned i_count )
{
    for( unsigned i = 0; i < i_count; ++i )
    {
        if ( !p_tracks[i] )
            continue;
        free( p_tracks[i]->psz_language );
        free( p_tracks[i]->psz_description );
        switch( p_tracks[i]->i_type )
        {
        case libvlc_track_audio:
            break;
        case libvlc_track_video:
            break;
        case libvlc_track_text:
            free( p_tracks[i]->subtitle->psz_encoding );
            break;
        case libvlc_track_unknown:
        default:
            break;
        }
        free( p_tracks[i]->audio );
        free( p_tracks[i] );
    }
    free( p_tracks );
}

/**************************************************************************
 * Get the media type of the media descriptor object
 **************************************************************************/
libvlc_media_type_t libvlc_media_get_type( libvlc_media_t *p_md )
{
    assert( p_md );

    int i_type;
    input_item_t *p_input_item = p_md->p_input_item;

    vlc_mutex_lock( &p_input_item->lock );
    i_type = p_md->p_input_item->i_type;
    vlc_mutex_unlock( &p_input_item->lock );

    switch( i_type )
    {
    case ITEM_TYPE_FILE:
        return libvlc_media_type_file;
    case ITEM_TYPE_NODE:
    case ITEM_TYPE_DIRECTORY:
        return libvlc_media_type_directory;
    case ITEM_TYPE_DISC:
        return libvlc_media_type_disc;
    case ITEM_TYPE_STREAM:
        return libvlc_media_type_stream;
    case ITEM_TYPE_PLAYLIST:
        return libvlc_media_type_playlist;
    default:
        return libvlc_media_type_unknown;
    }
}

int libvlc_media_slaves_add( libvlc_media_t *p_md,
                             libvlc_media_slave_type_t i_type,
                             unsigned int i_priority,
                             const char *psz_uri )
{
    assert( p_md && psz_uri );
    input_item_t *p_input_item = p_md->p_input_item;

    enum slave_type i_input_slave_type;
    switch( i_type )
    {
    case libvlc_media_slave_type_subtitle:
        i_input_slave_type = SLAVE_TYPE_SPU;
        break;
    case libvlc_media_slave_type_audio:
        i_input_slave_type = SLAVE_TYPE_AUDIO;
        break;
    default:
        vlc_assert_unreachable();
        return -1;
    }

    enum slave_priority i_input_slave_priority;
    switch( i_priority )
    {
    case 0:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_NONE;
        break;
    case 1:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_RIGHT;
        break;
    case 2:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_LEFT;
        break;
    case 3:
        i_input_slave_priority = SLAVE_PRIORITY_MATCH_ALL;
        break;
    default:
    case 4:
        i_input_slave_priority = SLAVE_PRIORITY_USER;
        break;
    }

    input_item_slave_t *p_slave = input_item_slave_New( psz_uri,
                                                      i_input_slave_type,
                                                      i_input_slave_priority );
    if( p_slave == NULL )
        return -1;
    return input_item_AddSlave( p_input_item, p_slave ) == VLC_SUCCESS ? 0 : -1;
}

void libvlc_media_slaves_clear( libvlc_media_t *p_md )
{
    assert( p_md );
    input_item_t *p_input_item = p_md->p_input_item;

    vlc_mutex_lock( &p_input_item->lock );
    for( int i = 0; i < p_input_item->i_slaves; i++ )
        input_item_slave_Delete( p_input_item->pp_slaves[i] );
    TAB_CLEAN( p_input_item->i_slaves, p_input_item->pp_slaves );
    vlc_mutex_unlock( &p_input_item->lock );
}

unsigned int libvlc_media_slaves_get( libvlc_media_t *p_md,
                                      libvlc_media_slave_t ***ppp_slaves )
{
    assert( p_md && ppp_slaves );
    input_item_t *p_input_item = p_md->p_input_item;
    *ppp_slaves = NULL;

    vlc_mutex_lock( &p_input_item->lock );

    int i_count = p_input_item->i_slaves;
    if( i_count <= 0 )
        return vlc_mutex_unlock( &p_input_item->lock ), 0;

    libvlc_media_slave_t **pp_slaves = calloc( i_count, sizeof(*pp_slaves) );
    if( pp_slaves == NULL )
        return vlc_mutex_unlock( &p_input_item->lock ), 0;

    for( int i = 0; i < i_count; ++i )
    {
        input_item_slave_t *p_item_slave = p_input_item->pp_slaves[i];
        assert( p_item_slave->i_priority >= SLAVE_PRIORITY_MATCH_NONE );

        /* also allocate psz_uri buffer at the end of the struct */
        libvlc_media_slave_t *p_slave = malloc( sizeof(*p_slave) +
                                                strlen( p_item_slave->psz_uri )
                                                + 1 );
        if( p_slave == NULL )
        {
            libvlc_media_slaves_release(pp_slaves, i);
            return vlc_mutex_unlock( &p_input_item->lock ), 0;
        }
        p_slave->psz_uri = (char *) ((uint8_t *)p_slave) + sizeof(*p_slave);
        strcpy( p_slave->psz_uri, p_item_slave->psz_uri );

        switch( p_item_slave->i_type )
        {
        case SLAVE_TYPE_SPU:
            p_slave->i_type = libvlc_media_slave_type_subtitle;
            break;
        case SLAVE_TYPE_AUDIO:
            p_slave->i_type = libvlc_media_slave_type_audio;
            break;
        default:
            vlc_assert_unreachable();
        }

        switch( p_item_slave->i_priority )
        {
        case SLAVE_PRIORITY_MATCH_NONE:
            p_slave->i_priority = 0;
            break;
        case SLAVE_PRIORITY_MATCH_RIGHT:
            p_slave->i_priority = 1;
            break;
        case SLAVE_PRIORITY_MATCH_LEFT:
            p_slave->i_priority = 2;
            break;
        case SLAVE_PRIORITY_MATCH_ALL:
            p_slave->i_priority = 3;
            break;
        case SLAVE_PRIORITY_USER:
            p_slave->i_priority = 4;
            break;
        default:
            vlc_assert_unreachable();
        }
        pp_slaves[i] = p_slave;
    }
    vlc_mutex_unlock( &p_input_item->lock );

    *ppp_slaves = pp_slaves;
    return i_count;
}

void libvlc_media_slaves_release(libvlc_media_slave_t **pp_slaves, unsigned int i_count)
{
    if (i_count > 0)
    {
        assert(pp_slaves);
        for (unsigned int i = 0; i < i_count; ++i)
            free(pp_slaves[i]);
    }
    free(pp_slaves);
}

#define DEFAULT_MRU     (1500u - (20 + 8))
typedef struct program_parser_t {
    vlc_thread_t t_thread;
    bool b_thread_ready;
    bool b_completed;
    int i_fd;
    dvbpsi_t* p_pat;
    dvbpsi_t* p_sdt;
    int i_sdt_table_id;
    int i_sdt_extension;
    bool b_broken_charset;
    int i_programs;
    uint16_t* pi_program_ids;
    char** ppsz_program_names;
    mtime_t i_pat_time;
} program_parser_t;

static inline uint32_t GetPID(uint8_t *packet)
{
    return ((uint16_t)(packet[1] & 0x1f) << 8) + packet[2];
}

static void MessageCallback(dvbpsi_t *handle, const dvbpsi_msg_level_t level, const char* msg)
{
    switch (level)
    {
    case DVBPSI_MSG_ERROR:
        printf("Error: %s\n", msg);
        break;
    case DVBPSI_MSG_WARN:
        printf("Warn: %s\n", msg);
        break;
    case DVBPSI_MSG_DEBUG:
        printf("Debug: %s\n", msg);
        break;
    default: /* do nothing */
        break;
    }
}

static void SDTCallback(void* p_data, dvbpsi_sdt_t* p_sdt)
{
    program_parser_t* p_parser = (program_parser_t*)p_data;
    dvbpsi_sdt_service_t *p_srv;
    dvbpsi_descriptor_t *p_desc;
    dvbpsi_service_dr_t *p_dr;
    int i, i_service_index;

    if (p_parser->b_completed)
        goto end;

    if (!p_parser->pi_program_ids)
    {
        i = 0;
        p_srv = p_sdt->p_first_service;
        while (p_srv)
        {
            i++;
            p_srv = p_srv->p_next;
        }

        p_parser->i_programs = i;
        p_parser->pi_program_ids = (uint16_t*)malloc(sizeof(uint16_t) * p_parser->i_programs);
        //memset(p_parser->pi_program_ids, 0, sizeof(uint16_t) * p_parser->i_programs);

        i = 0;
        p_srv = p_sdt->p_first_service;
        while (p_srv)
        {
            p_parser->pi_program_ids[i++] = p_srv->i_service_id;
            p_srv = p_srv->p_next;
        }
    }

    if (p_parser->ppsz_program_names)
    {
        for (i = 0; i < p_parser->i_programs; i++)
        {
            if (p_parser->ppsz_program_names[i])
                free(p_parser->ppsz_program_names[i]);
        }
        free(p_parser->ppsz_program_names);
        p_parser->ppsz_program_names = NULL;
    }

    p_parser->ppsz_program_names = (char**)malloc(sizeof(char*) * p_parser->i_programs);
    memset(p_parser->ppsz_program_names, 0, sizeof(char*) * p_parser->i_programs);

    p_srv = p_sdt->p_first_service;
    while (p_srv)
    {
        i_service_index = -1;
        for (i = 0; i < p_parser->i_programs; i++)
        {
            if (p_parser->pi_program_ids[i] == p_srv->i_service_id)
            {
                i_service_index = i;
                break;
            }
        }

        if (i_service_index >= 0)
        {
            p_desc = p_srv->p_first_descriptor;
            while (p_desc)
            {
                if (p_desc->i_tag == 0x48)
                {
                    p_dr = dvbpsi_DecodeServiceDr(p_desc);

                    if (p_sdt->i_network_id == 133)
                        p_parser->b_broken_charset = true;  /* SKY DE & BetaDigital use ISO8859-1 */

                    /* List of providers using ISO8859-1 */
                    const char ppsz_broken_providers[][8] = {
                        "CSAT",     /* CanalSat FR */
                        "GR1",      /* France televisions */
                        "MULTI4",   /* NT1 */
                        "MR5",      /* France 2/M6 HD */
                        ""
                    };
                    for (i = 0; *ppsz_broken_providers[i]; i++)
                    {
                        size_t i_length = strlen(ppsz_broken_providers[i]);
                        if (p_dr->i_service_provider_name_length == i_length && !strncmp((char *)p_dr->i_service_provider_name, ppsz_broken_providers[i], i_length))
                            p_parser->b_broken_charset = true;
                    }

                    if (p_parser->ppsz_program_names[i_service_index])
                        free(p_parser->ppsz_program_names[i_service_index]);

                    if (p_parser->b_broken_charset && (p_dr->i_service_name_length > 0) && (*p_dr->i_service_name > 0x20))
                        p_parser->ppsz_program_names[i_service_index] = FromCharset("ISO_8859-1", p_dr->i_service_name, p_dr->i_service_name_length);
                    else
                        p_parser->ppsz_program_names[i_service_index] = vlc_from_EIT(p_dr->i_service_name, p_dr->i_service_name_length);
                    break;
                }
                p_desc = p_desc->p_next;
            }
        }

        p_srv = p_srv->p_next;
    }

    p_parser->b_completed = true;

end:
    dvbpsi_sdt_delete(p_sdt);
}

static void SITableCallback(dvbpsi_t* p_handle, uint8_t i_table_id, uint16_t i_extension, void* p_data)
{
    program_parser_t* p_parser = (program_parser_t*)p_data;
    if (p_parser->i_sdt_table_id != i_table_id && i_table_id == 0x42)
    {
        p_parser->i_sdt_table_id = i_table_id;
        p_parser->i_sdt_extension = i_extension;
        dvbpsi_sdt_attach(p_handle, i_table_id, i_extension, SDTCallback, p_parser);
    }
}

static void PATCallback(void* p_data, dvbpsi_pat_t* p_pat)
{
    program_parser_t* p_parser = (program_parser_t*)p_data;
    dvbpsi_pat_program_t* p_program;
    int i;

    if (p_parser->b_completed)
        goto end;
    
    if (p_parser->pi_program_ids)
        free(p_parser->pi_program_ids);

    i = 0;
    p_program = p_pat->p_first_program;
    while (p_program)
    {
        if (p_program->i_number != 0)
            i++;
        p_program = p_program->p_next;
    }

    p_parser->i_programs = i;
    p_parser->pi_program_ids = (uint16_t*)malloc(sizeof(uint16_t) * p_parser->i_programs);

    i = 0;
    p_program = p_pat->p_first_program;
    while (p_program)
    {
        if (p_program->i_number != 0)
            p_parser->pi_program_ids[i++] = p_program->i_number;
        p_program = p_program->p_next;
    }

    if (p_parser->i_pat_time == 0)
        p_parser->i_pat_time = mdate();

end:
    dvbpsi_pat_delete(p_pat);
}

static void* ParseProgram(void* arg)
{
    program_parser_t* p_parser = (program_parser_t*)arg;
    int n, pid;
    ssize_t len;
    block_t* block;
#ifdef __linux__
    const int trunc_flag = MSG_TRUNC;
#else
    const int trunc_flag = 0;
#endif
    struct iovec iov = 
    {
        .iov_len = DEFAULT_MRU,
    };
    struct msghdr msg =
    {
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };
    struct pollfd ufd;
    ufd.fd = p_parser->i_fd;
    ufd.events = POLLIN;

    for (;;)
    {
        n = vlc_poll(&ufd, 1, 500);
        if (n == -1)
            continue;

        int canc = vlc_savecancel();
        if (n == 0)
        {
            vlc_restorecancel(canc);
            continue;
        }

        if (ufd.revents)
        {
            if (unlikely(ufd.revents & POLLHUP))
                break;

            block = block_Alloc(iov.iov_len);
            if (unlikely(block == NULL))
            {
                if (iov.iov_len == DEFAULT_MRU)
                    break;
                iov.iov_len = DEFAULT_MRU;
                continue;
            }

            iov.iov_base = block->p_buffer;
            msg.msg_flags = trunc_flag;

            len = recvmsg(p_parser->i_fd, &msg, trunc_flag);
            if (len != -1)
            {
                if (msg.msg_flags & trunc_flag)
                {
                    //msg_Err(NULL, "%d bytes packet truncated (MRU was %d)", len, iov.iov_len);
                    block->i_flags |= BLOCK_FLAG_CORRUPTED;
                    iov.iov_len = len;
                }
                else
                    block->i_buffer = len;

                n = 12;
                while (n < block->i_buffer)
                {
                    pid = GetPID(&block->p_buffer[n]);
                    if (pid == 0x0000) // PAT
                        dvbpsi_packet_push(p_parser->p_pat, &block->p_buffer[n]);
                    else if (pid == 0x0011) // SDT
                        dvbpsi_packet_push(p_parser->p_sdt, &block->p_buffer[n]);
                    n += 188;
                }

                if (!p_parser->b_completed && (p_parser->i_pat_time != 0) && (mdate() - p_parser->i_pat_time > 10000000LL))
                    p_parser->b_completed = true;
            }
            block_Release(block);
        }

        vlc_restorecancel(canc);
    }

    return 0;
}

int libvlc_media_program_parser_start(libvlc_media_t* p_md)
{
    if (!p_md || !p_md->p_input_item || !p_md->p_libvlc_instance)
        return VLC_EGENERIC;

    vlc_url_t url;
    if (vlc_UrlParse(&url, p_md->p_input_item->psz_uri) != VLC_SUCCESS)
        return VLC_EGENERIC;

    program_parser_t* p_parser = NULL;
    int fd = net_OpenDgram(p_md->p_libvlc_instance->p_libvlc_int, url.psz_host, url.i_port, NULL, 0, IPPROTO_UDP);
    if (fd < 0)
        goto error;

    net_SetCSCov(fd, -1, 12);

    p_md->p_user_data = p_parser = (program_parser_t*)malloc(sizeof(program_parser_t));
    memset(p_parser, 0, sizeof(program_parser_t));

    p_parser->i_fd = fd;
    p_parser->p_pat = dvbpsi_new(MessageCallback, DVBPSI_MSG_NONE);
    if (p_parser->p_pat && !dvbpsi_pat_attach(p_parser->p_pat, PATCallback, p_parser))
        goto error;

    p_parser->p_sdt = dvbpsi_new(MessageCallback, DVBPSI_MSG_NONE);
    if (p_parser->p_sdt && !dvbpsi_AttachDemux(p_parser->p_sdt, SITableCallback, p_parser))
        goto error;
    
    if (vlc_clone(&p_parser->t_thread, ParseProgram, p_parser, "parse_program_thread", VLC_THREAD_PRIORITY_INPUT))
        goto error;

    p_parser->b_thread_ready = true;
    vlc_UrlClean(&url);

    return VLC_SUCCESS;

error:
    if (p_parser)
    {
        if (p_parser->p_pat)
        {
            dvbpsi_pat_detach(p_parser->p_pat);
            dvbpsi_delete(p_parser->p_pat);
        }
        if (p_parser->p_sdt)
        {
            if (p_parser->i_sdt_table_id != 0)
                dvbpsi_sdt_detach(p_parser->p_sdt, p_parser->i_sdt_table_id, p_parser->i_sdt_extension);
            dvbpsi_DetachDemux(p_parser->p_sdt);
            dvbpsi_delete(p_parser->p_sdt);
        }
        free(p_parser);
    }
    vlc_UrlClean(&url);

    return VLC_EGENERIC;
}

int libvlc_media_program_parser_stop(libvlc_media_t* p_md)
{
    program_parser_t* p_parser = (program_parser_t*)p_md->p_user_data;
    if (p_parser)
    {
        if (p_parser->b_thread_ready)
        {
            vlc_cancel(p_parser->t_thread);
            vlc_join(p_parser->t_thread, NULL);
        }

        net_Close(p_parser->i_fd);
        if (p_parser->p_pat)
        {
            dvbpsi_pat_detach(p_parser->p_pat);
            dvbpsi_delete(p_parser->p_pat);
        }

        if (p_parser->p_sdt)
        {
            if (p_parser->i_sdt_table_id != 0)
                dvbpsi_sdt_detach(p_parser->p_sdt, p_parser->i_sdt_table_id, p_parser->i_sdt_extension);
            dvbpsi_DetachDemux(p_parser->p_sdt);
            dvbpsi_delete(p_parser->p_sdt);
        }

        if (p_parser->pi_program_ids)
            free(p_parser->pi_program_ids);

        if (p_parser->ppsz_program_names)
        {
            int i;
            for (i = 0; i < p_parser->i_programs; i++)
            {
                if (p_parser->ppsz_program_names[i])
                    free(p_parser->ppsz_program_names[i]);
            }
            free(p_parser->ppsz_program_names);
        }

        free(p_parser);
        p_md->p_user_data = NULL;
    }

    return VLC_SUCCESS;
}

bool libvlc_media_program_parser_is_completed(libvlc_media_t* p_md)
{
    program_parser_t* p_parser = (program_parser_t*)p_md->p_user_data;
    if (p_parser)
        return p_parser->b_completed;
    return false;
}

int libvlc_media_program_parser_get(libvlc_media_t* p_md, libvlc_program_t*** ppp_programs)
{
    program_parser_t* p_parser = (program_parser_t*)p_md->p_user_data;
    if (p_parser)
    {
        if (p_parser->i_programs == 0)
            return 0;

        int i;
        libvlc_program_t** pp_programs = (libvlc_program_t**)malloc(sizeof(libvlc_program_t*) * p_parser->i_programs);
        for (i = 0; i < p_parser->i_programs; i++)
        {
            pp_programs[i] = (libvlc_program_t*)malloc(sizeof(libvlc_program_t));
            pp_programs[i]->i_id = p_parser->pi_program_ids[i];
            if (p_parser->ppsz_program_names && p_parser->ppsz_program_names[i])
                pp_programs[i]->psz_name = strdup(p_parser->ppsz_program_names[i]);
            else
                pp_programs[i]->psz_name = strdup("");
        }
        *ppp_programs = pp_programs;
        return p_parser->i_programs;
    }

    return 0;
}

void libvlc_programs_release(libvlc_program_t** pp_programs, int i_count)
{
    for (int i = 0; i < i_count; i++)
    {
        if (!pp_programs[i])
            continue;

        free(pp_programs[i]->psz_name);
        free(pp_programs[i]);
    }
    free(pp_programs);
}
