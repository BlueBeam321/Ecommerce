/*****************************************************************************
 * vlc_vout_osd.h: vout OSD
 *****************************************************************************/

#ifndef VLC_VOUT_OSD_H
#define VLC_VOUT_OSD_H 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup osd On-screen display
 * \ingroup spu
 * @{
 * \file
 * Overlay text and widgets
 */

/**
 * OSD menu position and picture type defines
 */
enum
{
    /* Icons */
    OSD_PLAY_ICON = 1,
    OSD_PAUSE_ICON,
    OSD_SPEAKER_ICON,
    OSD_MUTE_ICON,
    /* Sliders */
    OSD_HOR_SLIDER,
    OSD_VERT_SLIDER,
};

VLC_API int vout_OSDEpg( vout_thread_t *, input_item_t * );

/**
 * \brief Write an informative message if the OSD option is enabled.
 * \param vout The vout on which the message will be displayed
 * \param channel Subpicture channel
 * \param position Position of the text
 * \param duration Duration of the text being displayed
 * \param text Text to be displayed
 */
VLC_API void vout_OSDText( vout_thread_t *vout, int channel, int position, mtime_t duration, const char *text );

/**
 * \brief Write an informative message at the default location,
 *        for the default duration and only if the OSD option is enabled.
 * \param vout The vout on which the message will be displayed
 * \param channel Subpicture channel
 * \param format printf style formatting
 *
 * Provided for convenience.
 */
VLC_API void vout_OSDMessage( vout_thread_t *, int, const char *, ... ) VLC_FORMAT( 3, 4 );

/**
 * Display a slider on the video output.
 * \param p_this    The object that called the function.
 * \param i_channel Subpicture channel
 * \param i_postion Current position in the slider
 * \param i_type    Types are: OSD_HOR_SLIDER and OSD_VERT_SLIDER.
 */
VLC_API void vout_OSDSlider( vout_thread_t *, int, int , short );

/**
 * Display an Icon on the video output.
 * \param p_this    The object that called the function.
 * \param i_channel Subpicture channel
 * \param i_type    Types are: OSD_PLAY_ICON, OSD_PAUSE_ICON, OSD_SPEAKER_ICON, OSD_MUTE_ICON
 */
VLC_API void vout_OSDIcon( vout_thread_t *, int, short );

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* VLC_VOUT_OSD_H */

