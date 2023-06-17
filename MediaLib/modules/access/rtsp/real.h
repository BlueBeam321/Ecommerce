/*****************************************************************************
 * real.h: rtsp input
 *****************************************************************************/
#ifndef HAVE_REAL_H
#define HAVE_REAL_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "rtsp.h"
#include "real_rmff.h"
#include "real_sdpplin.h"

#if 0 /*def REALDEBUG*/
#   define lprintf(...) fprintf (stderr, __VA_ARGS__)
#else
    static inline void lprintf( const char *dummy, ... ) { (void)dummy; }
#endif

int real_get_rdt_chunk_header(rtsp_client_t *, rmff_pheader_t *);
int real_get_rdt_chunk(rtsp_client_t *, rmff_pheader_t *, unsigned char **);
rmff_header_t *real_setup_and_get_header(rtsp_client_t *, int bandwidth);

int asmrp_match(const char *rules, int bandwidth, int *matches, int matchsize) ;

#endif
