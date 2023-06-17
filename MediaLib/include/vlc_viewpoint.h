/*****************************************************************************
 * vlc_viewpoint.h: viewpoint struct and helpers
 *****************************************************************************/

#ifndef VLC_VIEWPOINT_H_
#define VLC_VIEWPOINT_H_ 1

#include <vlc_common.h>

#include <math.h>

/**
 * \defgroup output Output
 * \ingroup output
 *
 * @{
 * \file
 * Video and audio viewpoint struct and helpers
 */

#define FIELD_OF_VIEW_DEGREES_DEFAULT  80.f
#define FIELD_OF_VIEW_DEGREES_MAX 150.f
#define FIELD_OF_VIEW_DEGREES_MIN 20.f

/**
 * Viewpoints
 */
struct vlc_viewpoint_t {
    float yaw;   /* yaw in degrees */
    float pitch; /* pitch in degrees */
    float roll;  /* roll in degrees */
    float fov;   /* field of view in degrees */
};

static inline void vlc_viewpoint_init( vlc_viewpoint_t *p_vp )
{
    p_vp->yaw = p_vp->pitch = p_vp->roll = 0.0f;
    p_vp->fov = FIELD_OF_VIEW_DEGREES_DEFAULT;
}

static inline void vlc_viewpoint_clip( vlc_viewpoint_t *p_vp )
{
    p_vp->yaw = fmodf( p_vp->yaw, 360.f );
    p_vp->pitch = fmodf( p_vp->pitch, 360.f );
    p_vp->roll = fmodf( p_vp->roll, 360.f );
    p_vp->fov = VLC_CLIP( p_vp->fov, FIELD_OF_VIEW_DEGREES_MIN,
                          FIELD_OF_VIEW_DEGREES_MAX );
}

/**@}*/

#endif /* VLC_VIEWPOINT_H_ */
