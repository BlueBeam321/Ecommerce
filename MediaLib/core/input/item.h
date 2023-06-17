/*****************************************************************************
 * item.h
 *****************************************************************************/

#ifndef LIBVLC_INPUT_ITEM_H
#define LIBVLC_INPUT_ITEM_H 1

#include "common/core/input/input_internal.h"
#include <vlc_atomic.h>

void input_item_SetErrorWhenReading( input_item_t *p_i, bool b_error );
void input_item_UpdateTracksInfo( input_item_t *item, const es_format_t *fmt );
bool input_item_ShouldPreparseSubItems( input_item_t *p_i );

typedef struct input_item_owner
{
    input_item_t item;
    atomic_uint refs;
} input_item_owner_t;

# define item_owner(item) ((struct input_item_owner *)(item))

#endif
