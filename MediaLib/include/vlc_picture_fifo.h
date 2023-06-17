/*****************************************************************************
 * vlc_picture_fifo.h: picture fifo definitions
 *****************************************************************************/

#ifndef VLC_PICTURE_FIFO_H
#define VLC_PICTURE_FIFO_H 1

/**
 * \file
 * This file defines picture fifo structures and functions in vlc
 */

#include <vlc_picture.h>

/**
 * Picture fifo handle
 *
 * It is thread safe (push/pop).
 */
typedef struct picture_fifo_t picture_fifo_t;

/**
 * It creates an empty picture_fifo_t.
 */
VLC_API picture_fifo_t * picture_fifo_New( void ) VLC_USED;

/**
 * It destroys a fifo created by picture_fifo_New.
 *
 * All pictures inside the fifo will be released by picture_Release.
 */
VLC_API void picture_fifo_Delete( picture_fifo_t * );

/**
 * It retreives a picture_t from the fifo.
 *
 * If the fifo is empty, it return NULL without waiting.
 */
VLC_API picture_t * picture_fifo_Pop( picture_fifo_t * ) VLC_USED;

/**
 * It returns the first picture_t pointer from the fifo but does not
 * remove it. The picture returned has been hold for you so you
 * must call picture_Release on it.
 *
 * If the fifo is empty, it return NULL without waiting.
 */
VLC_API picture_t * picture_fifo_Peek( picture_fifo_t * ) VLC_USED;

/**
 * It saves a picture_t into the fifo.
 */
VLC_API void picture_fifo_Push( picture_fifo_t *, picture_t * );

/**
 * It release all picture inside the fifo that have a lower or equal date
 * if flush_before or higher or equal to if not flush_before than the given one.
 *
 * All pictures inside the fifo will be released by picture_Release.
 */
VLC_API void picture_fifo_Flush( picture_fifo_t *, mtime_t date, bool flush_before );

/**
 * It applies a delta on all the picture timestamp.
 */
VLC_API void picture_fifo_OffsetDate( picture_fifo_t *, mtime_t delta );


#endif /* VLC_PICTURE_FIFO_H */

