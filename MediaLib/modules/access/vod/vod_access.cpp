/*****************************************************************************
* vod.cpp : Vod Streaming Media support based MediaCodec
*****************************************************************************/

#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_demux.h>
#include <vlc_url.h>
#include <vlc_strings.h>
#include <vlc_interrupt.h>
#include <vlc_keystore.h>

#include "MxVODNetStream.h"
#include "MxErrCodes.h"

#define MODULE_NAME     access_demux_vod
#define MODULE_STRING   "access_demux_vod"

/*****************************************************************************
* Module descriptor
*****************************************************************************/
static int  Open(vlc_object_t *);
static void Close(vlc_object_t *);


vlc_module_begin ()
    set_description( N_("vod demuxer (based MediaCodec)" ) )
    set_capability( "demux", 5 )
    set_shortname( "vod")
    set_callbacks( Open, Close )
    add_shortcut( "vod", "vodstream" )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_DEMUX )

    add_submodule ()
        set_description( N_("vod stream access and demux") )
        add_shortcut( "vod")
        set_capability( "access_demux", 0 )
        set_callbacks( Open, Close )
vlc_module_end ()

struct demux_sys_t
{
    MxVODNetStream              *p_input;
    stream_info_t               *p_stream_info;
    bool                        b_vod_proxy_mode;
    int32_t				        i_vod_fmts;
    vod_es_format_t**		    pp_vod_fmts;

    bool                        b_open;
    uint32_t                    i_running;
    vlc_thread_t                thread;
    vlc_demux_chained_t         *chained_demux;

    uint32_t                    i_current_title;
    uint32_t                    i_titles;
    input_title_t               **pp_tititles;
};

static int Demux(demux_t *);
static int Control(demux_t *, int, va_list);

static void*    DataFillThread(void *opaque);
static void     ParseTitleInfo(demux_sys_t *p_sys);
static int      VodSeek(demux_sys_t *p_sys, float postion);
static int      VodTitleOpen(demux_sys_t *p_sys, int i_title);
/*****************************************************************************
* DemuxOpen:
*****************************************************************************/
static int  Open(vlc_object_t *p_this)
{
    demux_t     *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = NULL;
    char *psz_url;
    int i_return;
    int i_error = VLC_EGENERIC;

    if (!strstr(p_demux->psz_access, "vod"))
        return VLC_EGENERIC;

    p_demux->pf_demux = NULL;
    p_demux->pf_control = Control;
    p_demux->p_sys = p_sys = (demux_sys_t *)calloc(1, sizeof(demux_sys_t));
    if (!p_sys) return VLC_ENOMEM;
    if (asprintf(&psz_url, "rtsp://%s", p_demux->psz_location) == -1)
        return VLC_EGENERIC;

    p_sys->p_input = new MxVODNetStream(psz_url, 0, 0);
    free(psz_url);
    if (p_sys->p_input->Open())
        return VLC_EGENERIC;
    p_sys->p_stream_info = (stream_info_t*)malloc(sizeof(stream_info_t));
    memset(p_sys->p_stream_info, 0, sizeof(stream_info_t));

    char* p_vod = NULL;
    int32_t i_vod_size = 0, i_ts_kind = 0;
    char* p_attach = NULL;
    int32_t i_attach_size = 0;

    p_sys->i_running = 1;
    p_sys->b_open = true;
    p_sys->i_titles = var_InheritInteger(p_demux, "vod-title");
    p_sys->i_current_title = 0;
    if (p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamInfo, p_sys->p_stream_info, &i_ts_kind,
        &p_vod, &i_vod_size, &p_attach, &i_attach_size) == mxErr_None)
    {
        int ret;
        if (p_sys->p_input->ParseVODHeader(p_vod, i_vod_size, p_sys->p_stream_info->video_track.first_pts, p_sys->p_stream_info) == mxErr_None)
            ret = VLC_SUCCESS;
        else
            ret = p_sys->p_input->ParseAttachData(p_attach, i_attach_size, &p_sys->pp_vod_fmts, &p_sys->i_vod_fmts, p_sys->p_stream_info, &p_sys->b_vod_proxy_mode);

        if (ret == VLC_SUCCESS)
            ParseTitleInfo(p_sys);
        else
            p_sys->i_titles = 0;
    }
    else
        p_sys->i_titles = 0;
    if (vlc_clone(&p_sys->thread, DataFillThread, p_demux, "access_vod_fill", VLC_THREAD_PRIORITY_INPUT))
        return VLC_EGENERIC;
    return VLC_SUCCESS;

error:
    return VLC_EGENERIC;
}

/*****************************************************************************
* DemuxClose:
*****************************************************************************/
static void Close(vlc_object_t *p_this)
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;

    p_sys->b_open = true;
    p_sys->i_running = 0;

    vlc_join(p_sys->thread, NULL);
    p_sys->p_input->Close();
    delete p_sys->p_input;
   
    for (int i = 0; i < p_sys->i_titles; i++)
        vlc_input_title_Delete(p_sys->pp_tititles[i]);
    
    if (p_sys->pp_vod_fmts)
    {
        for (int i = 0; i < p_sys->i_vod_fmts; i++)
        {
            vod_es_format_t* p_fmt = p_sys->pp_vod_fmts[i];
            if (p_fmt->p_extra)
                free(p_fmt->p_extra);
            if (p_fmt->psz_description)
                free(p_fmt->psz_description);
            if (p_fmt->psz_language)
                free(p_fmt->psz_language);
            if (p_fmt->video.p_palette)
                free(p_fmt->video.p_palette);
            free(p_fmt);
        }
        free(p_sys->pp_vod_fmts);
    }
    free(p_sys->p_stream_info);
    free(p_sys);
    p_demux->p_sys = 0;
}


/*****************************************************************************
* Demux:
*****************************************************************************/
static int Demux(demux_t *p_demux)
{
    return 0;
}


/*****************************************************************************
* Control:
*****************************************************************************/
static int Control(demux_t *p_demux, int i_query, va_list args)
{
    demux_sys_t *p_sys = p_demux->p_sys;
    demux_query_e e_query = (demux_query_e)i_query;
    switch (e_query)
    {
    case DEMUX_GET_TIME:
    case DEMUX_GET_POSITION:
        if (p_sys->b_open && p_sys->chained_demux)
            vlc_demux_chained_ControlVa(p_sys->chained_demux, i_query, args);
        return VLC_SUCCESS;

    case DEMUX_GET_LENGTH:
    {
        int64_t* pi64 = va_arg(args, int64_t*);
        if (p_sys->b_open)
            *pi64 = p_sys->p_stream_info->playtime * 1000 * 1000;
        else
            *pi64 = 0;
        return VLC_SUCCESS;
    }

    case DEMUX_SET_POSITION:
    {
        double f = va_arg(args, double);
        static double fPrev = 0;
        if (fPrev != f)
        {
            fPrev = f;
            return VodSeek(p_sys, f);
        }
        return VLC_EGENERIC;
    }

    case DEMUX_SET_TIME:
    {
        mtime_t i_time = va_arg(args, mtime_t);
        double f = (double)i_time / (double)(p_sys->p_stream_info->playtime * 1000 * 1000);
        return VodSeek(p_sys, f);
    }
    case DEMUX_CAN_PAUSE:
    case DEMUX_CAN_SEEK:
    {
        bool* pb = va_arg(args, bool*);
        *pb = true;
        return VLC_SUCCESS;
    }

    case DEMUX_CAN_CONTROL_PACE:
        return VLC_SUCCESS;

    case DEMUX_CAN_CONTROL_RATE:
  
        return VLC_SUCCESS;

    case DEMUX_SET_RATE:
        return VLC_SUCCESS;

    case DEMUX_SET_PAUSE_STATE:
        return VLC_SUCCESS;
    case DEMUX_GET_TITLE_INFO:
        if (!p_sys->i_titles || !p_sys->b_open)
            return VLC_EGENERIC;
        else if (p_sys->pp_tititles && p_sys->pp_tititles[0]->i_seekpoint > 0)
        {
            input_title_t ***ppp_title = va_arg(args, input_title_t***);
            int *pi_int = va_arg(args, int*);

            *pi_int = p_sys->i_titles;
            *ppp_title = (input_title_t**)(vlc_alloc(p_sys->i_titles, sizeof(input_title_t*)));

            for (size_t i = 0; i < p_sys->i_titles; i++)
                (*ppp_title)[i] = vlc_input_title_Duplicate(p_sys->pp_tititles[i]);
            return VLC_SUCCESS;
        }
        else
            return VLC_EGENERIC;
    case DEMUX_SET_TITLE:
    {
        int i_title = va_arg(args, int);
        if (i_title != p_sys->i_current_title)
            return VodTitleOpen(p_sys, i_title);
        else
            return VLC_EGENERIC;
    }
    case DEMUX_SET_SEEKPOINT:
    {
        int i_skp = va_arg(args, int);
        if (i_skp < p_sys->pp_tititles[0]->i_seekpoint)
        {
            mtime_t i_pts = p_sys->pp_tititles[0]->seekpoint[i_skp]->i_time_offset;
            return VodSeek(p_sys, (float)i_pts / (float)p_sys->p_stream_info->playtime);
        }
        return VLC_EGENERIC;
    }
    case DEMUX_GET_PTS_DELAY:
        //pi64 = va_arg(args, int64_t *);
        //*pi64 = INT64_C(1000)
        //    * var_InheritInteger(p_demux, "network-caching");
        return VLC_SUCCESS;
    case DEMUX_GET_VOD_FMT_BY_ID:
    {
        int i_id = va_arg(args, int);
        void** pp_fmt = va_arg(args, void**);
        for (int i = 0; i < p_sys->i_vod_fmts; i++)
        {
            if (p_sys->pp_vod_fmts[i]->i_id == i_id)
            {
                *pp_fmt = p_sys->pp_vod_fmts[i];
                return VLC_SUCCESS;
            }
        }
        return VLC_EGENERIC;
    }

    case DEMUX_GET_VOD_FMT_CODEC:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        int* pi_cat = va_arg(args, int*);
        int* pi_codec = va_arg(args, int*);

        *pi_cat = (p_fmt->i_cat == 'ediv') ? VIDEO_ES :
            (p_fmt->i_cat == 'edua') ? AUDIO_ES :
            (p_fmt->i_cat == 'eups') ? SPU_ES :
            UNKNOWN_ES;
        *pi_codec = p_fmt->i_codec;

        if (*pi_codec == MX_CODEC_SPU)
            *pi_codec = VLC_CODEC_SPU;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_EXTRA:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        char** pp_extra = va_arg(args, char**);
        int* pi_extra = va_arg(args, int*);

        *pp_extra = p_fmt->p_extra;
        *pi_extra = p_fmt->i_extra;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_ORG_FOURCC:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        int* pi_original_fourcc = va_arg(args, int*);
        
        *pi_original_fourcc = p_fmt->i_original_fourcc;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_DESC:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        char** pp_description = va_arg(args, char**);
        
        *pp_description = p_fmt->psz_description;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_LANG:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        char** pp_language = va_arg(args, char**);
        
        *pp_language = p_fmt->psz_language;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_VIDEO_PALETTE:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        video_palette_t** pp_palette = va_arg(args, video_palette_t**);
        
        *pp_palette = p_fmt->video.p_palette;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_SPU_PALETTE:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        uint32_t** pp_palette = va_arg(args, uint32_t**);
        
        *pp_palette = p_fmt->subs.spu.palette;
        return VLC_SUCCESS;
    }

    case DEMUX_GET_VOD_FMT_SPU_ORG_SIZE:
    {
        vod_es_format_t* p_fmt = va_arg(args, vod_es_format_t*);
        int* pi_x = va_arg(args, int*);
        int* pi_y = va_arg(args, int*);
        
        *pi_x = p_fmt->subs.spu.i_original_frame_width;
        *pi_y = p_fmt->subs.spu.i_original_frame_height;
        return VLC_SUCCESS;
    }
    default:
        return VLC_EGENERIC;
    }
    return VLC_EGENERIC;
}


static void *stream_init(demux_t *demux, const char *name)
{
    demux_sys_t *p_sys = demux->p_sys;

    if (p_sys->chained_demux != NULL)
        return NULL;
    p_sys->chained_demux = vlc_demux_chained_New(VLC_OBJECT(demux), name,
        demux->out);
    return p_sys->chained_demux;
}


static void stream_destroy(demux_t *demux, void *data)
{
    demux_sys_t *p_sys = demux->p_sys;

    if (data)
    {
        vlc_demux_chained_Delete((vlc_demux_chained_t*)data);
        p_sys->chained_demux = NULL;
    }
}

static void *vod_stream_init(demux_t *demux)
{
    char const* name = demux->psz_demux;

    if (*name == '\0' || !strcasecmp(name, "any"))
        name = NULL;

    return stream_init(demux, name ? name : "ts");
}

/* Send a packet to a chained demuxer */
static void stream_decode(demux_t *demux, void *data, block_t *block)
{
    if (data)
        vlc_demux_chained_Send((vlc_demux_chained_t*)data, block);
    else
        block_Release(block);
    (void)demux;
}

#ifndef TS_PACKET_SIZE
#define TS_PACKET_SIZE 188
#endif

static void* DataFillThread(void*opaque)
{
    demux_t *p_demux = (demux_t *)opaque;
    demux_sys_t *p_sys = p_demux->p_sys;

    vod_stream_init(p_demux);

    int32_t i_read = -1, i_size;
    int32_t ret = -1;

    uint8_t buffer[TS_PACKET_SIZE * 7];

    while (p_sys->i_running)
    {
        ret = p_sys->p_input->Read(buffer, TS_PACKET_SIZE * 7, &i_read);
        if (ret == 0)
        {
            block_t *block = block_Alloc(i_read);
            memcpy(block->p_buffer, buffer, i_read);
            stream_decode(p_demux, p_sys->chained_demux, block);
        }
        else
            msleep(1000);
    }

    stream_destroy(p_demux, p_sys->chained_demux);
    return 0;
}

static void ParseTitleInfo(demux_sys_t *p_sys)
{
    stream_info_t *p_info = p_sys->p_stream_info;
    input_title_t **pp_titles;

    if (p_info->chapter_count && !p_sys->i_titles)
        p_sys->i_titles = 1;
    
    if (p_sys->i_titles)
    {
        pp_titles = (input_title_t**)malloc(sizeof(input_title_t*) * p_sys->i_titles);
        for (int j = 0; j < p_sys->i_titles; j++)
        {
            pp_titles[j] = vlc_input_title_New();
            char *psz_title = (char *)malloc(strlen("Title ") + 4);
            sprintf(psz_title, "Title %d", j + 1);
            pp_titles[j]->psz_name = psz_title;

            if (j == 0 && p_info->chapter_count)
            {
                pp_titles[j]->i_seekpoint = p_info->chapter_count;
                pp_titles[j]->seekpoint = (seekpoint_t **)malloc(sizeof(seekpoint_t*) * p_info->chapter_count);

                for (int i = 0; i < p_info->chapter_count; i++)
                {
                    seekpoint_t * sp = vlc_seekpoint_New();
                    sp->i_time_offset = p_info->p_chapters[i].chapter_sec;
                    sp->psz_name = p_info->p_chapters[i].p_descs ?
                        strdup(p_info->p_chapters[i].p_descs->description) : strdup("Chapter");
                    pp_titles[j]->seekpoint[i] = sp;
                }
            }
        }
        p_sys->pp_tititles = pp_titles;
    }
}

static int VodSeek(demux_sys_t *p_sys, float postion)
{
    int64_t i64;
    int32_t ret = 0;

    i64 = (mtime_t)(postion * p_sys->p_stream_info->playtime);
    
    p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamPause);
    p_sys->p_input->SetSeek(i64, -1, -1, -1);
    p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamSeek);
    p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamContinue);
    
    return VLC_SUCCESS;
}

static int VodTitleOpen(demux_sys_t *p_sys, int i_title)
{
    int ret;

    p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamPause);
    p_sys->p_input->Close();


    ret = p_sys->p_input->Open(i_title);
    if (ret != mxErr_None)
        return VLC_EGENERIC;

    p_sys->i_current_title = i_title;
    
    memset(p_sys->p_stream_info, 0, sizeof(stream_info_t));

    char* p_vod = NULL;
    int32_t i_vod_size = 0, i_ts_kind = 0;
    char* p_attach = NULL;
    int32_t i_attach_size = 0;

    if (p_sys->p_input->Control(mxStreamCtrlMsg_VODNetStreamInfo, p_sys->p_stream_info, &i_ts_kind,
        &p_vod, &i_vod_size, &p_attach, &i_attach_size) == mxErr_None)
    {
        int ret = p_sys->p_input->ParseAttachData(p_attach, i_attach_size, &p_sys->pp_vod_fmts, &p_sys->i_vod_fmts, p_sys->p_stream_info, &p_sys->b_vod_proxy_mode);

        if (ret == mxErr_None)
            ParseTitleInfo(p_sys);
    }

    return VLC_SUCCESS;
}
