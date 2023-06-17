/*****************************************************************************
 * aout.c: audio output controls for the VLC playlist
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_aout.h>
#include <vlc_playlist.h>

#include <math.h>

#include "common/core/audio_output/aout_internal.h"
#include "common/core/playlist/playlist_internal.h"

audio_output_t *playlist_GetAout(playlist_t *pl)
{
    /* NOTE: it is assumed that the input resource exists. In practice,
     * the playlist must have been activated. This is automatic when calling
     * pl_Get(). FIXME: input resources are deleted at deactivation, this can
     * be too early. */
    playlist_private_t *sys = pl_priv(pl);
    return input_resource_HoldAout(sys->p_input_resource);
}

float playlist_VolumeGet (playlist_t *pl)
{
    float volume = -1.f;

    audio_output_t *aout = playlist_GetAout (pl);
    if (aout != NULL)
    {
        volume = aout_VolumeGet (aout);
        vlc_object_release (aout);
    }
    return volume;
}

int playlist_VolumeSet (playlist_t *pl, float vol)
{
    int ret = -1;

    audio_output_t *aout = playlist_GetAout (pl);
    if (aout != NULL)
    {
        ret = aout_VolumeSet (aout, vol);
        vlc_object_release (aout);
    }
    return ret;
}

/**
 * Raises the volume.
 * \param value how much to increase (> 0) or decrease (< 0) the volume
 * \param volp if non-NULL, will contain contain the resulting volume
 */
int playlist_VolumeUp (playlist_t *pl, int value, float *volp)
{
    int ret = -1;

    audio_output_t *aout = playlist_GetAout (pl);
    if (aout != NULL)
    {
        ret = aout_VolumeUpdate (aout, value, volp);
        vlc_object_release (aout);
    }
    return ret;
}

int playlist_MuteGet (playlist_t *pl)
{
    int mute = -1;

    audio_output_t *aout = playlist_GetAout (pl);
    if (aout != NULL)
    {
        mute = aout_MuteGet (aout);
        vlc_object_release (aout);
    }
    return mute;
}

int playlist_MuteSet (playlist_t *pl, bool mute)
{
    int ret = -1;

    audio_output_t *aout = playlist_GetAout (pl);
    if (aout != NULL)
    {
        ret = aout_MuteSet (aout, mute);
        vlc_object_release (aout);
    }
    return ret;
}

void playlist_EnableAudioFilter (playlist_t *pl, const char *name, bool add)
{
    audio_output_t *aout = playlist_GetAout (pl);

    aout_ChangeFilterString (VLC_OBJECT(pl), aout ? VLC_OBJECT(aout) : NULL,
                             "audio-filter", name, add);
    if (aout != NULL)
        vlc_object_release (aout);
}
