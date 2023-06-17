/*****************************************************************************
 * omxil.h: helper functions
 *****************************************************************************/

#ifdef RPI_OMX
#define OMX_SKIP64BIT
#endif

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "OMX_Core.h"
#include "OMX_Index.h"
#include "OMX_Component.h"
#include "OMX_Video.h"

#include "omxil_utils.h"
#include "omxil_core.h"

#if defined(USE_IOMX)
#include "common/modules/video_output/android/utils.h"
#endif

enum
{
    BUF_STATE_NOT_OWNED = 0,
    BUF_STATE_OWNED,
};

/*****************************************************************************
 * decoder_sys_t : omxil decoder descriptor
 *****************************************************************************/
typedef struct OmxFifo
{
    vlc_mutex_t lock;
    vlc_cond_t  wait;

    OMX_BUFFERHEADERTYPE *p_first;
    OMX_BUFFERHEADERTYPE **pp_last;

    int offset;

} OmxFifo;

typedef struct HwBuffer
{
    vlc_thread_t    dequeue_thread;
    bool            b_run;
    vlc_mutex_t     lock;
    vlc_cond_t      wait;
    picture_sys_t** inflight_picture; /**< stores the inflight picture for each output buffer or NULL */

    unsigned int    i_buffers;
    void            **pp_handles;
    int             *i_states;
    unsigned int    i_max_owned;
    unsigned int    i_owned;

#if defined(USE_IOMX)
    native_window_priv_api_t anwpriv;
    native_window_priv *window_priv;
#endif

} HwBuffer;

typedef struct OmxPort
{
    bool b_valid;
    OMX_U32 i_port_index;
    OMX_HANDLETYPE omx_handle;
    OMX_PARAM_PORTDEFINITIONTYPE definition;
    es_format_t *p_fmt;

    unsigned int i_frame_size;
    unsigned int i_frame_stride;
    unsigned int i_frame_stride_chroma_div;

    unsigned int i_buffers;
    OMX_BUFFERHEADERTYPE **pp_buffers;

    OmxFifo fifo;

    OmxFormatParam format_param;

    OMX_BOOL b_reconfigure;
    OMX_BOOL b_update_def;
    OMX_BOOL b_direct;
    OMX_BOOL b_flushed;

    HwBuffer *p_hwbuf;

} OmxPort;

struct decoder_sys_t
{
    OMX_HANDLETYPE omx_handle;

    bool b_enc;

    char psz_component[OMX_MAX_STRINGNAME_SIZE];
    char ppsz_components[MAX_COMPONENTS_LIST_SIZE][OMX_MAX_STRINGNAME_SIZE];
    unsigned int components;
    int i_quirks;

    OmxEventQueue event_queue;

    OmxPort *p_ports;
    unsigned int ports;
    OmxPort in;
    OmxPort out;

    bool b_error;

    bool b_aspect_ratio_handled;

    date_t end_date;

    uint8_t i_nal_size_length; /* Length of the NAL size field for H264 */
    int b_use_pts;

};
