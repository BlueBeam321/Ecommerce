/*****************************************************************************
 * asfpacket.h :
 *****************************************************************************/
#ifndef VLC_ASF_ASFPACKET_H_
#define VLC_ASF_ASFPACKET_H_

#include <vlc_demux.h>
#include <vlc_es.h>

#define ASFPACKET_PREROLL_FROM_CURRENT -1

typedef struct
{
    block_t *p_frame; /* used to gather complete frame */
    asf_object_stream_properties_t *p_sp;
    asf_object_extended_stream_properties_t *p_esp;
    int i_cat;
} asf_track_info_t;

typedef struct asf_packet_sys_s asf_packet_sys_t;

struct asf_packet_sys_s
{
    demux_t *p_demux;

    /* global stream info */
    uint64_t *pi_preroll;
    int64_t *pi_preroll_start;

    /* callbacks */
    void (*pf_send)(asf_packet_sys_t *, uint8_t, block_t **);
    asf_track_info_t * (*pf_gettrackinfo)(asf_packet_sys_t *, uint8_t);

    /* optional callbacks */
    bool (*pf_doskip)(asf_packet_sys_t *, uint8_t, bool);
    void (*pf_updatesendtime)(asf_packet_sys_t *, mtime_t);
    void (*pf_updatetime)(asf_packet_sys_t *, uint8_t, mtime_t);
    void (*pf_setaspectratio)(asf_packet_sys_t *, uint8_t, uint8_t, uint8_t);
};

int DemuxASFPacket( asf_packet_sys_t *, uint32_t, uint32_t );
#endif
