/*****************************************************************************
 * mta_holder.c: Hold a MTA from another thread
 *****************************************************************************/

#ifndef MTA_HOLDER_H
# define MTA_HOLDER_H

#include <assert.h>
#include <windows.h>
#include <objbase.h>

typedef struct vlc_mta_holder
{
    vlc_thread_t thread;
    int          i_refcount;
    vlc_sem_t    ready_sem;
    vlc_sem_t    release_sem;
} vlc_mta_holder;

static inline void* MtaMainLoop( void* opaque )
{
    vlc_mta_holder* p_mta = (vlc_mta_holder*)opaque;
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    vlc_sem_post( &p_mta->ready_sem );

    vlc_sem_wait( &p_mta->release_sem );

    CoUninitialize();
    return NULL;
}

/**
 * Ensure an MTA context will be available until vlc_mta_release gets called.
 *
 * In the background, this will create a thread that does nothing but to keep the MTA
 * refcount greater than 0.
 *
 * This is usefull in order not to commit a thread to a specific concurrency model.
 * This function is win32 specific.
 */
static inline bool mta_acquire( vlc_object_t *p_parent )
{
    vlc_global_lock( VLC_MTA_MUTEX );
    vlc_mta_holder* p_mta = (vlc_mta_holder*)var_CreateGetAddress( p_parent->obj.libvlc, "mta-holder" );
    if ( p_mta == NULL )
    {
        p_mta = (vlc_mta_holder*)malloc( sizeof( *p_mta ) );
        if ( unlikely( p_mta == NULL ) )
        {
            vlc_global_unlock( VLC_MTA_MUTEX );
            return false;
        }
        vlc_sem_init( &p_mta->ready_sem, 0 );
        vlc_sem_init( &p_mta->release_sem, 0 );
        p_mta->i_refcount = 1;
        if ( vlc_clone( &p_mta->thread, MtaMainLoop, p_mta, "MtaMainLoop", VLC_THREAD_PRIORITY_LOW ) )
        {
            vlc_sem_destroy( &p_mta->release_sem );
            vlc_sem_destroy( &p_mta->ready_sem );
            free( p_mta );
            p_mta = NULL;
            vlc_global_unlock( VLC_MTA_MUTEX );
            return false;
        }
        var_SetAddress( p_parent->obj.libvlc, "mta-holder", p_mta );
        vlc_sem_wait( &p_mta->ready_sem );
    }
    else
        ++p_mta->i_refcount;
    vlc_global_unlock( VLC_MTA_MUTEX );
    return true;
}

/**
 * Releases a reference to the MTA holder.
 *
 * When its refcount reaches 0, the thread created by
 */
static inline void mta_release( vlc_object_t* p_parent )
{
    vlc_global_lock( VLC_MTA_MUTEX );
    vlc_mta_holder *p_mta = (vlc_mta_holder*)var_InheritAddress( p_parent->obj.libvlc, "mta-holder" );
    assert( p_mta != NULL );
    int i_refcount = --p_mta->i_refcount;
    if ( i_refcount == 0 )
        var_SetAddress( p_parent->obj.libvlc, "mta-holder", NULL );
    vlc_global_unlock( VLC_MTA_MUTEX );
    if ( i_refcount == 0 )
    {
        vlc_sem_post( &p_mta->release_sem );

        vlc_join( p_mta->thread, NULL );

        vlc_sem_destroy( &p_mta->release_sem );
        vlc_sem_destroy( &p_mta->ready_sem );
        free( p_mta );
    }
}

#endif
