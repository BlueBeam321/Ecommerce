/*****************************************************************************
 * windows_audio_common.h: Windows Audio common code
 *****************************************************************************/
#ifndef DMX_WINDOWS_AUDIO_COMMONS_H
#define DMX_WINDOWS_AUDIO_COMMONS_H

#include <vlc_aout.h>
#include <vlc_codecs.h>

static const uint32_t pi_channels_src[] = { WAVE_SPEAKER_FRONT_LEFT,
    WAVE_SPEAKER_FRONT_RIGHT, WAVE_SPEAKER_FRONT_CENTER,
    WAVE_SPEAKER_LOW_FREQUENCY, WAVE_SPEAKER_BACK_LEFT, WAVE_SPEAKER_BACK_RIGHT,
    WAVE_SPEAKER_BACK_CENTER, WAVE_SPEAKER_SIDE_LEFT, WAVE_SPEAKER_SIDE_RIGHT, 0 };

static const uint32_t pi_channels_aout[] = { AOUT_CHAN_LEFT, AOUT_CHAN_RIGHT,
    AOUT_CHAN_CENTER, AOUT_CHAN_LFE, AOUT_CHAN_REARLEFT, AOUT_CHAN_REARRIGHT,
    AOUT_CHAN_REARCENTER, AOUT_CHAN_MIDDLELEFT, AOUT_CHAN_MIDDLERIGHT, 0 };

static inline unsigned getChannelMask( uint32_t * wvfextChannelMask, int i_channels, int * i_match )
{
    unsigned i_channel_mask = 0;
    *i_match = 0;
    for( unsigned i = 0;
         i < sizeof(pi_channels_src)/sizeof(*pi_channels_src) &&
         *i_match < i_channels; i++ )
    {
        if( *wvfextChannelMask & pi_channels_src[i] )
        {
            if( !( i_channel_mask & pi_channels_aout[i] ) )
                 *i_match += 1;

            *wvfextChannelMask &= ~pi_channels_src[i];
            i_channel_mask |= pi_channels_aout[i];
        }
    }
    return i_channel_mask;
}

#endif /*DMX_WINDOWS_AUDIO_COMMONS_H*/
