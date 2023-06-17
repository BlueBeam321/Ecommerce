/*****************************************************************************
 * libvlc.c: libvlc instances creation and deletion, interfaces handling
 *****************************************************************************/

#include "libvlc_internal.h"
#include <vlc_input.h>
#include <vlc_plugin.h>

#include <stdio.h>                                              /* sprintf() */
#include <string.h>
#include <stdlib.h>                                                /* free() */
#include <errno.h>

#include <vlc_getopt.h>
#include <vlc_playlist.h>
#include <vlc_interface.h>

#include <vlc_actions.h>
#include <vlc_charset.h>
#include <vlc_keystore.h>
#include <vlc_fs.h>
#include <vlc_cpu.h>
#include <vlc_url.h>
#include <vlc_modules.h>
#include <vlc_internal.h>

#include "common/core/modules/modules.h"
#include "common/core/config/configuration.h"
#include "common/core/playlist/preparser.h"
#include "common/core/playlist/playlist_internal.h"
#include "common/core/misc/variables.h"

#include <assert.h>

/**
 * Allocate a blank libvlc instance, also setting the exit handler.
 * Vlc's threading system must have been initialized first
 */
libvlc_int_t * libvlc_InternalCreate( void )
{
    libvlc_int_t *p_libvlc;
    libvlc_priv_t *priv;

    /* Allocate a libvlc instance object */
    p_libvlc = vlc_custom_create(NULL, sizeof(*priv), "libvlc");
    if (p_libvlc == NULL)
        return NULL;

    priv = libvlc_priv(p_libvlc);

    vlc_ExitInit(&priv->exit);

    return p_libvlc;
}

/**
 * Initialize a libvlc instance
 * This function initializes a previously allocated libvlc instance:
 *  - CPU detection
 *  - gettext initialization
 *  - message queue, module bank and playlist initialization
 *  - configuration and commandline parsing
 */
int libvlc_InternalInit(libvlc_int_t *p_libvlc)
{
    libvlc_priv_t* priv = libvlc_priv (p_libvlc);
    char* psz_modules = NULL;
    char* psz_parser = NULL;
    char* psz_control = NULL;
    char* psz_val;
    int i_ret = VLC_EGENERIC;

    /* System specific initialization code */
    system_Init();
    vlc_LogPreinit(p_libvlc);
    module_InitBank();

    vlc_threads_setup(p_libvlc);

    size_t module_count = module_LoadPlugins (p_libvlc);

    vlc_LogInit(p_libvlc);

    if (module_count <= 1)
    {
        msg_Err(p_libvlc, "No plugins found! Check your VLC installation.");
        i_ret = VLC_ENOMOD;
        goto error;
    }

    i_ret = VLC_ENOMEM;
    vlc_CPU_dump(VLC_OBJECT(p_libvlc));

    priv->b_stats = var_InheritBool(p_libvlc, "stats");

    /*
     * Meta data handling
     */
    priv->parser = playlist_preparser_New(VLC_OBJECT(p_libvlc));
    if (!priv->parser)
        goto error;

    /* Create a variable for showing the fullscreen interface */
    var_Create(p_libvlc, "intf-toggle-fscontrol", VLC_VAR_BOOL);
    var_SetBool(p_libvlc, "intf-toggle-fscontrol", true);

    /* Create a variable for showing the right click menu */
    var_Create(p_libvlc, "intf-popupmenu", VLC_VAR_BOOL);

    /* variables for signaling creation of new files */
    var_Create(p_libvlc, "snapshot-file", VLC_VAR_STRING);
    var_Create(p_libvlc, "record-file", VLC_VAR_STRING);

    /* some default internal settings */
    var_Create(p_libvlc, "window", VLC_VAR_STRING);

    var_Create(p_libvlc, "user-agent", VLC_VAR_STRING);
    var_SetString(p_libvlc, "user-agent", "VLC media player (LibVLC 3.0)");
    var_Create(p_libvlc, "http-user-agent", VLC_VAR_STRING);
    var_SetString(p_libvlc, "http-user-agent", "VLC/3.0 LibVLC/3.0");

    /* System specific configuration */
    system_Configure(p_libvlc);

#ifdef __APPLE__
    var_Create(p_libvlc, "drawable-view-top", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-view-left", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-view-bottom", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-view-right", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-clip-top", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-clip-left", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-clip-bottom", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-clip-right", VLC_VAR_INTEGER);
    var_Create(p_libvlc, "drawable-nsobject", VLC_VAR_ADDRESS);
#endif

    return VLC_SUCCESS;

error:
    libvlc_InternalCleanup(p_libvlc);
    return i_ret;
}

/**
 * Cleanup a libvlc instance. The instance is not completely deallocated
 * \param p_libvlc the instance to clean
 */
void libvlc_InternalCleanup( libvlc_int_t *p_libvlc )
{
    libvlc_priv_t *priv = libvlc_priv (p_libvlc);

    if (priv->parser != NULL)
        playlist_preparser_Deactivate(priv->parser);

#if !defined( _WIN32 ) && !defined( __OS2__ )
    char *pidfile = var_InheritString( p_libvlc, "pidfile" );
    if( pidfile != NULL )
    {
        msg_Dbg(p_libvlc, "removing PID file %s", pidfile);
        if (unlink(pidfile))
            msg_Warn(p_libvlc, "cannot remove PID file %s: %s",
            pidfile, vlc_strerror_c(errno));
        free(pidfile);
    }
#endif

    if (priv->parser != NULL)
        playlist_preparser_Delete(priv->parser);

    /* Free module bank. It is refcounted, so we call this each time  */
    vlc_LogDeinit (p_libvlc);
    module_EndBank (true);
#if defined(_WIN32) || defined(__OS2__)
    system_End( );
#endif
}

/**
 * Destroy everything.
 * This function requests the running threads to finish, waits for their
 * termination, and destroys their structure.
 * It stops the thread systems: no instance can run after this has run
 * \param p_libvlc the instance to destroy
 */
void libvlc_InternalDestroy( libvlc_int_t *p_libvlc )
{
    libvlc_priv_t *priv = libvlc_priv( p_libvlc );

    vlc_ExitDestroy( &priv->exit );

    assert( atomic_load(&(vlc_internals(p_libvlc)->refs)) == 1 );
    vlc_object_release( p_libvlc );
}

int vlc_MetadataRequest(libvlc_int_t *libvlc, input_item_t *item,
                        input_item_meta_request_option_t i_options,
                        int timeout, void *id)
{
    libvlc_priv_t *priv = libvlc_priv(libvlc);

    if (unlikely(priv->parser == NULL))
        return VLC_ENOMEM;

    if( i_options & META_REQUEST_OPTION_DO_INTERACT )
    {
        vlc_mutex_lock( &item->lock );
        item->b_preparse_interact = true;
        vlc_mutex_unlock( &item->lock );
    }
    playlist_preparser_Push( priv->parser, item, i_options, timeout, id );
    return VLC_SUCCESS;

}

/**
 * Requests extraction of the meta data for an input item (a.k.a. preparsing).
 * The actual extraction is asynchronous. It can be canceled with
 * libvlc_MetadataCancel()
 */
int libvlc_MetadataRequest(libvlc_int_t *libvlc, input_item_t *item,
                           input_item_meta_request_option_t i_options,
                           int timeout, void *id)
{
    libvlc_priv_t *priv = libvlc_priv(libvlc);

    if (unlikely(priv->parser == NULL))
        return VLC_ENOMEM;

    vlc_mutex_lock( &item->lock );
    if( item->i_preparse_depth == 0 )
        item->i_preparse_depth = 1;
    if( i_options & META_REQUEST_OPTION_DO_INTERACT )
        item->b_preparse_interact = true;
    vlc_mutex_unlock( &item->lock );
    playlist_preparser_Push( priv->parser, item, i_options, timeout, id );
    return VLC_SUCCESS;
}

/**
 * Requests retrieving/downloading art for an input item.
 * The retrieval is performed asynchronously.
 */
int libvlc_ArtRequest(libvlc_int_t *libvlc, input_item_t *item,
                      input_item_meta_request_option_t i_options)
{
    libvlc_priv_t *priv = libvlc_priv(libvlc);

    if (unlikely(priv->parser == NULL))
        return VLC_ENOMEM;

    playlist_preparser_fetcher_Push(priv->parser, item, i_options);
    return VLC_SUCCESS;
}

/**
 * Cancels extraction of the meta data for an input item.
 *
 * This does nothing if the input item is already processed or if it was not
 * added with libvlc_MetadataRequest()
 */
void libvlc_MetadataCancel(libvlc_int_t *libvlc, void *id)
{
    libvlc_priv_t *priv = libvlc_priv(libvlc);

    if (unlikely(priv->parser == NULL))
        return;

    playlist_preparser_Cancel(priv->parser, id);
}
