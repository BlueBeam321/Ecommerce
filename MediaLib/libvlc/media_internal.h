/*****************************************************************************
 * media_internal.h : Definition of opaque structures for libvlc exported API
 * Also contains some internal utility functions
 *****************************************************************************/

#ifndef _LIBVLC_MEDIA_INTERNAL_H
#define _LIBVLC_MEDIA_INTERNAL_H 1

#include <vlc_input.h>

struct libvlc_media_t
{
    libvlc_event_manager_t event_manager;
    input_item_t *p_input_item;
    int i_refcount;
    libvlc_instance_t *p_libvlc_instance;
    libvlc_state_t state;
    libvlc_media_list_t* p_subitems; /* A media descriptor can have Sub items. This is the only dependency we really have on media_list */
    void *p_user_data;

    vlc_cond_t parsed_cond;
    vlc_mutex_t parsed_lock;
    vlc_mutex_t subitems_lock;

    libvlc_media_parsed_status_t parsed_status;
    bool is_parsed;
    bool has_asked_preparse;
};

/* Media Descriptor */
libvlc_media_t * libvlc_media_new_from_input_item(
        libvlc_instance_t *, input_item_t * );

void libvlc_media_set_state( libvlc_media_t *, libvlc_state_t );

#endif
