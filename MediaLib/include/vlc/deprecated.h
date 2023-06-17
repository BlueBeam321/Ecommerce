/*****************************************************************************
 * deprecated.h:  libvlc deprecated API
 *****************************************************************************/

#ifndef LIBVLC_DEPRECATED_H
#define LIBVLC_DEPRECATED_H 1

# ifdef __cplusplus
extern "C" {
# endif

/**
 * \ingroup libvlc libvlc_media_player
 * @{
 */

/**
 * Get movie fps rate
 *
 * This function is provided for backward compatibility. It cannot deal with
 * multiple video tracks. In LibVLC versions prior to 3.0, it would also fail
 * if the file format did not convey the frame rate explicitly.
 *
 * \deprecated Consider using libvlc_media_tracks_get() instead.
 *
 * \param p_mi the Media Player
 * \return frames per second (fps) for this playing movie, or 0 if unspecified
 */
LIBVLC_DEPRECATED
LIBVLC_API float libvlc_media_player_get_fps( libvlc_media_player_t *p_mi );

/** end bug */

/**
 * \deprecated Use libvlc_media_player_set_nsobject() instead
 */
LIBVLC_DEPRECATED
LIBVLC_API void libvlc_media_player_set_agl ( libvlc_media_player_t *p_mi, uint32_t drawable );

/**
 * \deprecated Use libvlc_media_player_get_nsobject() instead
 */
LIBVLC_DEPRECATED
LIBVLC_API uint32_t libvlc_media_player_get_agl ( libvlc_media_player_t *p_mi );

/**
 * \deprecated Use libvlc_track_description_list_release() instead
 */
LIBVLC_DEPRECATED LIBVLC_API
void libvlc_track_description_release( libvlc_track_description_t *p_track_description );

/** @}*/

/**
 * \ingroup libvlc libvlc_video
 * @{
 */

/**
 * Get the description of available titles.
 *
 * \param p_mi the media player
 * \return list containing description of available titles.
 * It must be freed with libvlc_track_description_list_release()
 */
LIBVLC_DEPRECATED LIBVLC_API libvlc_track_description_t *
        libvlc_video_get_title_description( libvlc_media_player_t *p_mi );

/**
 * Get the description of available chapters for specific title.
 *
 * \param p_mi the media player
 * \param i_title selected title
 * \return list containing description of available chapter for title i_title.
 * It must be freed with libvlc_track_description_list_release()
 */
LIBVLC_DEPRECATED LIBVLC_API libvlc_track_description_t *
        libvlc_video_get_chapter_description( libvlc_media_player_t *p_mi, int i_title );

/**
 * Set new video subtitle file.
 * \deprecated Use libvlc_media_player_add_slave() instead.
 *
 * \param p_mi the media player
 * \param psz_subtitle new video subtitle file
 * \return the success status (boolean)
 */
LIBVLC_DEPRECATED LIBVLC_API int
libvlc_video_set_subtitle_file( libvlc_media_player_t *p_mi, const char *psz_subtitle );

/**
 * Toggle teletext transparent status on video output.
 * \deprecated use libvlc_video_set_teletext() instead.
 *
 * \param p_mi the media player
 */
LIBVLC_DEPRECATED LIBVLC_API void
libvlc_toggle_teletext( libvlc_media_player_t *p_mi );

/** @}*/

/**
 * \ingroup libvlc libvlc_audio
 * @{
 */

/**
 * \ingroup libvlc libvlc_media
 * @{
 */

/**
 * Parse a media.
 *
 * This fetches (local) art, meta data and tracks information.
 * The method is synchronous.
 *
 * \deprecated This function could block indefinitely.
 *             Use libvlc_media_parse_with_options() instead
 *
 * \see libvlc_media_parse_with_options
 * \see libvlc_media_get_meta
 * \see libvlc_media_get_tracks_info
 *
 * \param p_md media descriptor object
 */
LIBVLC_DEPRECATED LIBVLC_API void
libvlc_media_parse( libvlc_media_t *p_md );

/**
 * Parse a media.
 *
 * This fetches (local) art, meta data and tracks information.
 * The method is the asynchronous of libvlc_media_parse().
 *
 * To track when this is over you can listen to libvlc_MediaParsedChanged
 * event. However if the media was already parsed you will not receive this
 * event.
 *
 * \deprecated You can't be sure to receive the libvlc_MediaParsedChanged
 *             event (you can wait indefinitely for this event).
 *             Use libvlc_media_parse_with_options() instead
 *
 * \see libvlc_media_parse
 * \see libvlc_MediaParsedChanged
 * \see libvlc_media_get_meta
 * \see libvlc_media_get_tracks_info
 *
 * \param p_md media descriptor object
 */
LIBVLC_DEPRECATED LIBVLC_API void
libvlc_media_parse_async( libvlc_media_t *p_md );

/**
 * Return true is the media descriptor object is parsed
 *
 * \deprecated This can return true in case of failure.
 *             Use libvlc_media_get_parsed_status() instead
 *
 * \see libvlc_MediaParsedChanged
 *
 * \param p_md media descriptor object
 * \return true if media object has been parsed otherwise it returns false
 *
 * \libvlc_return_bool
 */
LIBVLC_DEPRECATED LIBVLC_API int
   libvlc_media_is_parsed( libvlc_media_t *p_md );

/**
 * Get media descriptor's elementary streams description
 *
 * Note, you need to call libvlc_media_parse() or play the media at least once
 * before calling this function.
 * Not doing this will result in an empty array.
 *
 * \deprecated Use libvlc_media_tracks_get() instead
 *
 * \param p_md media descriptor object
 * \param tracks address to store an allocated array of Elementary Streams
 *        descriptions (must be freed by the caller) [OUT]
 *
 * \return the number of Elementary Streams
 */
LIBVLC_DEPRECATED LIBVLC_API
int libvlc_media_get_tracks_info( libvlc_media_t *p_md,
                                  libvlc_media_track_info_t **tracks );

/** @}*/

/**
 * \ingroup libvlc libvlc_core
 * @{
 */

/** This structure is opaque. It represents a libvlc log iterator */
typedef struct libvlc_log_iterator_t libvlc_log_iterator_t;

typedef struct libvlc_log_message_t
{
    int         i_severity;   /* 0=INFO, 1=ERR, 2=WARN, 3=DBG */
    const char *psz_type;     /* module type */
    const char *psz_name;     /* module name */
    const char *psz_header;   /* optional header */
    const char *psz_message;  /* message */
} libvlc_log_message_t;

/**
 * Always returns minus one.
 * This function is only provided for backward compatibility.
 *
 * \param p_instance ignored
 * \return always -1
 */
LIBVLC_DEPRECATED LIBVLC_API
unsigned libvlc_get_log_verbosity( const libvlc_instance_t *p_instance );

/**
 * This function does nothing.
 * It is only provided for backward compatibility.
 *
 * \param p_instance ignored
 * \param level ignored
 */
LIBVLC_DEPRECATED LIBVLC_API
void libvlc_set_log_verbosity( libvlc_instance_t *p_instance, unsigned level );

/**
 * This function does nothing useful.
 * It is only provided for backward compatibility.
 *
 * \param p_instance libvlc instance
 * \return an unique pointer or NULL on error
 */
LIBVLC_DEPRECATED LIBVLC_API
libvlc_log_t *libvlc_log_open( libvlc_instance_t *p_instance );

/**
 * Frees memory allocated by libvlc_log_open().
 *
 * \param p_log libvlc log instance or NULL
 */
LIBVLC_DEPRECATED LIBVLC_API
void libvlc_log_close( libvlc_log_t *p_log );

/**
 * This function does nothing useful.
 * It is only provided for backward compatibility.
 *
 * \param p_log ignored
 * \return an unique pointer or NULL on error or if the parameter was NULL
 */
LIBVLC_DEPRECATED LIBVLC_API
libvlc_log_iterator_t *libvlc_log_get_iterator( const libvlc_log_t *p_log );

/**
 * Frees memory allocated by libvlc_log_get_iterator().
 *
 * \param p_iter libvlc log iterator or NULL
 */
LIBVLC_DEPRECATED LIBVLC_API
void libvlc_log_iterator_free( libvlc_log_iterator_t *p_iter );

# ifdef __cplusplus
}
# endif

#endif /* _LIBVLC_DEPRECATED_H */
