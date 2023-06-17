/**
 * @file rtp.h
 * @brief RTP demux module shared declarations
 */

typedef struct rtp_pt_t rtp_pt_t;
typedef struct rtp_session_t rtp_session_t;

struct vlc_demux_chained_t;

/** @section RTP payload format */
struct rtp_pt_t
{
    void   *(*init) (demux_t *);
    void    (*destroy) (demux_t *, void *);
    void    (*header) (demux_t *, void *, block_t *);
    void    (*decode) (demux_t *, void *, block_t *);
    uint32_t  frequency; /* RTP clock rate (Hz) */
    uint8_t   number;
};
void rtp_autodetect(demux_t *, rtp_session_t *, const block_t *);

static inline uint8_t rtp_ptype (const block_t *block)
{
    return block->p_buffer[1] & 0x7F;
}

void *codec_init (demux_t *demux, es_format_t *fmt);
void codec_destroy (demux_t *demux, void *data);
void codec_decode (demux_t *demux, void *data, block_t *block);

void *theora_init (demux_t *demux);
void xiph_destroy (demux_t *demux, void *data);
void xiph_decode (demux_t *demux, void *data, block_t *block);

/** @section RTP session */
rtp_session_t *rtp_session_create (demux_t *);
void rtp_session_destroy (demux_t *, rtp_session_t *);
void rtp_queue (demux_t *, rtp_session_t *, block_t *);
bool rtp_dequeue (demux_t *, const rtp_session_t *, mtime_t *);
void rtp_dequeue_force (demux_t *, const rtp_session_t *);
int rtp_add_type (demux_t *demux, rtp_session_t *ses, const rtp_pt_t *pt);

void *rtp_dgram_thread (void *data);
void *rtp_stream_thread (void *data);

/* Global data */
struct demux_sys_t
{
    rtp_session_t *session;
    struct vlc_demux_chained_t *chained_demux;
#ifdef HAVE_SRTP
    struct srtp_session_t *srtp;
#endif
    int           fd;
    int           rtcp_fd;
#ifdef MANUAL_IGMP
    int           igmp_fd; // Added by K.C.Y
#endif
    vlc_thread_t  thread;

#ifdef MANUAL_IGMP
    uint32_t      host; // Added by K.C.Y
    int           port; // Added by K.C.Y
#endif
    mtime_t       timeout;
    uint16_t      max_dropout; /**< Max packet forward mis-ordering */
    uint16_t      max_misorder; /**< Max packet backward mis-ordering */
    uint8_t       max_src; /**< Max simultaneous RTP sources */
    bool          thread_ready;
    bool          autodetect; /**< Payload type auto-detection pending */
};

