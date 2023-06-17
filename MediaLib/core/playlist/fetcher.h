/*****************************************************************************
 * playlist_fetcher.h:
 *****************************************************************************/

#ifndef _PLAYLIST_FETCHER_H
#define _PLAYLIST_FETCHER_H 1

#include <vlc_input_item.h>

/**
 * Fetcher opaque structure.
 *
 * The fetcher object will retrieve the art album data for any given input
 * item in an asynchronous way.
 */
typedef struct playlist_fetcher_t playlist_fetcher_t;

/**
 * This function creates the fetcher object and thread.
 */
playlist_fetcher_t *playlist_fetcher_New( vlc_object_t * );

/**
 * This function enqueues the provided item to be art fetched.
 *
 * The input item is retained until the art fetching is done or until the
 * fetcher object is destroyed.
 */
int playlist_fetcher_Push( playlist_fetcher_t *, input_item_t *,
                           input_item_meta_request_option_t, int );

/**
 * This function destroys the fetcher object and thread.
 *
 * All pending input items will be released.
 */
void playlist_fetcher_Delete( playlist_fetcher_t * );

#endif

