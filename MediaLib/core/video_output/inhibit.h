/*****************************************************************************
 * inhibit.h: screen saver inhibition
 *****************************************************************************/

#ifndef LIBVLC_VIDEO_OUTPUT_INHIBIT_H_
#define LIBVLC_VIDEO_OUTPUT_INHIBIT_H_

# include <vlc_inhibit.h>

vlc_inhibit_t *vlc_inhibit_Create (vlc_object_t *);
void vlc_inhibit_Destroy (vlc_inhibit_t *);
#endif
