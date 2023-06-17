/*****************************************************************************
 * demux.h: Input demux functions
 *****************************************************************************/

#ifndef LIBVLC_INPUT_DEMUX_H
#define LIBVLC_INPUT_DEMUX_H 1

#include "stream.h"

/* stream_t *s could be null and then it mean a access+demux in one */
demux_t *demux_NewAdvanced( vlc_object_t *p_obj, input_thread_t *p_parent_input,
                            const char *psz_access, const char *psz_demux,
                            const char *psz_path, stream_t *s, es_out_t *out, bool );
#define demux_NewAdvanced( a, b, c, d, e, f, g, h ) demux_NewAdvanced(VLC_OBJECT(a),b,c,d,e,f,g,h)

unsigned demux_TestAndClearFlags( demux_t *, unsigned );
int demux_GetTitle( demux_t * );
int demux_GetSeekpoint( demux_t * );

/**
 * Builds an explicit chain of demux filters.
 *
 * This function creates a chain of filters according to a supplied list.
 *
 * See also stream_FilterChainNew(). Those two functions have identical
 * semantics and ownership rules, except for the use of demux vs stream.
 *
 * @param source input stream around which to build a filter chain
 * @param list colon-separated list of stream filters (upstream first)
 *
 * @return The last demux (filter) in the chain.
 * The return value is always a valid (non-NULL) demux pointer.
 */
demux_t *demux_FilterChainNew( demux_t *source, const char *list ) VLC_USED;

bool demux_FilterEnable( demux_t *p_demux_chain, const char* psz_demux );
bool demux_FilterDisable( demux_t *p_demux_chain, const char* psz_demux );

#endif
