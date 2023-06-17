/*****************************************************************************
 * vlc_meta_fetcher.h
 *****************************************************************************/

#ifndef VLC_META_FETCHER_H
#define VLC_META_FETCHER_H 1

typedef enum meta_fetcher_scope_t
{
    FETCHER_SCOPE_LOCAL   = 0x01,
    FETCHER_SCOPE_NETWORK = 0x02,
    FETCHER_SCOPE_ANY     = 0x03
} meta_fetcher_scope_t;

typedef struct meta_fetcher_t
{
    VLC_COMMON_MEMBERS
    input_item_t *p_item;
    meta_fetcher_scope_t e_scope;
} meta_fetcher_t;

#endif
