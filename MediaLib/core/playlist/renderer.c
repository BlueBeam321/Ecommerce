/*****************************************************************************
 * renderer.c : Manage renderer modules
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_playlist.h>
#include <vlc_renderer_discovery.h>

#include "common/core/playlist/playlist_internal.h"

int playlist_SetRenderer( playlist_t* p_playlist, vlc_renderer_item_t* p_item )
{
    if( p_item )
        vlc_renderer_item_hold( p_item );

    PL_LOCK;

    playlist_private_t *p_priv = pl_priv( p_playlist );
    vlc_renderer_item_t *p_prev_renderer = p_priv->p_renderer;
    p_priv->p_renderer = p_item;
    if( p_priv->p_input )
        input_Control( p_priv->p_input, INPUT_SET_RENDERER, p_item );

    PL_UNLOCK;

    if( p_prev_renderer )
        vlc_renderer_item_release( p_prev_renderer );
    return VLC_SUCCESS;
}
