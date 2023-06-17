/*****************************************************************************
 * hxxx_common.h: AVC/HEVC packetizers shared code
 *****************************************************************************/

#ifndef HXXX_COMMON_H
#define HXXX_COMMON_H

#include <vlc_common.h>

/* */
typedef struct cc_storage_t cc_storage_t;

cc_storage_t * cc_storage_new( void );
void cc_storage_delete( cc_storage_t *p_ccs );

void cc_storage_reset( cc_storage_t *p_ccs );
void cc_storage_append( cc_storage_t *p_ccs, bool b_top_field_first,
                                      const uint8_t *p_buf, size_t i_buf );
void cc_storage_commit( cc_storage_t *p_ccs, block_t *p_pic );

block_t * cc_storage_get_current( cc_storage_t *p_ccs, decoder_cc_desc_t * );

/* */

typedef block_t * (*pf_annexb_nal_packetizer)(decoder_t *, bool *, block_t *);
block_t *PacketizeXXC1( decoder_t *, uint8_t, block_t **, pf_annexb_nal_packetizer );

#endif // HXXX_COMMON_H

