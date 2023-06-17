/*****************************************************************************
 * exit.c: LibVLC termination event
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_interface.h>
#include <vlc_internal.h>

void vlc_ExitInit( vlc_exit_t *exit )
{
    vlc_mutex_init( &exit->lock );
    exit->handler = NULL;
    exit->opaque = NULL;
}

void vlc_ExitDestroy( vlc_exit_t *exit )
{
    vlc_mutex_destroy( &exit->lock );
}


/**
 * Registers a callback for the LibVLC exit event.
 */
void libvlc_SetExitHandler( libvlc_int_t *p_libvlc, void (*handler) (void *),
                            void *opaque )
{
    vlc_exit_t *exit = &libvlc_priv( p_libvlc )->exit;

    vlc_mutex_lock( &exit->lock );
    exit->handler = handler;
    exit->opaque = opaque;
    vlc_mutex_unlock( &exit->lock );
}

/**
 * Posts an exit signal to LibVLC instance.
 * This function should only be called on behalf of the user.
 */
void libvlc_Quit( libvlc_int_t *p_libvlc )
{
    vlc_exit_t *exit = &libvlc_priv( p_libvlc )->exit;

    msg_Dbg( p_libvlc, "exiting" );
    vlc_mutex_lock( &exit->lock );
    if( exit->handler != NULL )
        exit->handler( exit->opaque );
    else
        msg_Dbg( p_libvlc, "no exit handler" );
    vlc_mutex_unlock( &exit->lock );
}
