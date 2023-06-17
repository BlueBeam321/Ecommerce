#ifndef _MX_VODNET_STREAM_H_
#define _MX_VODNET_STREAM_H_

//#include "MyQueue.h"
#include "MxBlockQueue.h"
typedef char* buffer_t;


#define	mxStreamCtrlMsg_VODNetStreamInfo		4000	/* arg1= stream_info_t*outStreamInfo, arg2= int32_t*outTSKind,
* arg3= buffer_t*outVODBuf, arg4= int32_t*outVODSize
* res=can fail */
#define	mxStreamCtrlMsg_VODNetStreamSeek		4001	/* arg1= mtime_t inSeekTime, arg2= mtime_t inStartCr,
* arg3= int64_t inSeekPos, arg4= int32_t inChapterNo
* res=can fail */
#define	mxStreamCtrlMsg_VODNetStreamPlayTime	4002	/* arg1= int32_t outPlayTime(second)
* res=can fail */
#define	mxStreamCtrlMsg_VODNetStreamSeekExStart	4003	/* arg1= stream_pos_ex_t* inSeekPos
* res=can fail */
#define	mxStreamCtrlMsg_VODNetStreamSeekExEnd	4004	/* 
* res=can fail */
#define	mxStreamCtrlMsg_FileEmpty			    4006	/* res=can fail */

#define	mxStreamCtrlMsg_VODNetStreamPause		4007	/* */
#define	mxStreamCtrlMsg_VODNetStreamContinue	4008	/* */

#define	mxStreamCtrlMsg_VODNetStreamEnd			4010	/*  */

    typedef struct
    {
        char url[1024];
        int32_t rtpTransport; //  0:eRTP_UDP, 1:eRTP_TCP; default eRTP_UDP
        int32_t authMode; // 1: auth streaming, 0: raw streaming

        int32_t startTime;
        int64_t startCr;
        int64_t startPos;
    } mxVODNetUrl_Info;

    struct _MxVODNetStream_t;


    // Stream Info
    struct video_track_info_t
    {
        int32_t		kind;			// MX_CODEC_MPGV, MX_CODEC_H264, ...
        int32_t		sub_kind;		// profile

        int32_t		playtime;		// in second
        mtime_t		first_dts;		// 90KHz
        mtime_t		first_pts;		// 90KHz
        int64_t		first_pos;		// stream position

        int32_t		width;			// width of the decoded picture
        int32_t		height;			// height of the decoded picture
        int32_t		visible_width;	// width in the header
        int32_t		visible_height;	// height in the header

        bool		b_progressive;
        int32_t		aspect;			// aspect ratio of integer type. To convert into double value, use picture_CalcAspectRatio()
        int32_t		aspect_mode;	// mxCodecAspectMode_4_3, mxCodecAspectMode_16_9, mxCodecAspectMode_Unknown
        int32_t		mode;			// mxCodecVideoMode_NTSC, mxCodecVideoMode_PAL, ...
        int32_t		fps;
        int32_t		fps_base;
        int32_t		bitrate;
        bool		b_vbr;

        int32_t		track_id;

        char		codec_desc[64];
        char		mode_desc[64];

        int32_t		present_rate;
        int32_t		present_rate_base;

        int32_t		i_extra;
        char*	p_extra;		// do not free by the user

        uint32_t	i_sar_num;
        uint32_t	i_sar_den;

        uint32_t	i_orientation;
        uint32_t	i_seq_aspect_ratio; // or specify the HWAccel mode in stream_play_info_t structure

        char*		description;	// track name

        int32_t     i_sub_track_count; // For MPEG-DASH
        struct video_track_info_t* sub_tracks;
    };

    struct audio_track_info_t
    {
        int32_t		kind;			// MX_CODEC_MPGA, MX_CODEC_DTS, ...
        int32_t		sub_kind;		// layer

        int32_t		playtime;		// in second

        int32_t		channel_count;
        int32_t		channel_layout;
        int32_t		has_lef;
        int32_t		frequency;
        int32_t		bitrate;
        bool		b_vbr;

        int32_t		track_id;

        char		codec_desc[64];
        char		channel_desc[64];
        char		language_kor[64];
        char		language_eng[64];

        int32_t		i_extra;
        char*	p_extra;		// do not free by the user

        uint32_t	i_bytes_per_frame;
        int32_t		i_frame_length;
        uint32_t	i_bitspersample;
        int32_t		i_profile;

        char*		description;	// track name

        bool		b_added;

        int32_t     i_sub_track_count; // For MPEG-DASH
        struct audio_track_info_t* sub_tracks;
    };

#define MX_VIDEO_TRACK_MAX_NUM              10
#define	MX_AUDIO_TRACK_MAX_NUM				50
#define	MX_SPU_TRACK_MAX_NUM				50
    //#define MX_CHAPTER_TRACK_MAX_NUM			100
#define MX_ATTACHMENT_MAX_NUM				100
#define	MX_PALETTE_MAX_NUM					16

#define	mxSpuTrackFlags_LetterBox	0x01
#define	mxSpuTrackFlags_PanScan		0x02

    struct spu_track_info_t
    {
        int32_t		kind;			// MX_CODEC_SPU, MX_CODEC_BD_PG, MX_CODEC_SSA, MX_CODEC_SUBT, ...
        int32_t		sub_kind;		// Run-length coding type

        int32_t		track_id;		// main
        int32_t		track_id_lb;	// letter-box
        int32_t		track_id_ps;	// pan-scan

        char		language_kor[64];
        char		language_eng[64];

        int32_t		i_extra;
        char*	    p_extra;		// do not free by the user

        int32_t		i_original_frame_width;		// the width of the original movie the spu was extracted from
        int32_t		i_original_frame_height;	// the height of the original movie the spu was extracted from
        uint32_t*	p_palette;					// do not free by the user, palette_size = sizeof(uint32_t) * MX_PALETTE_MAX_NUM;

        int32_t		i_flags;		// mxSpuTrackFlags_LetterBox, mxSpuTrackFlags_PanScan

        char*		description;	// track name

        int32_t     i_sub_track_count; // For MPEG-DASH
        struct spu_track_info_t* sub_tracks;
    };

    struct chapter_desc_t
    {
        char		language_kor[64];
        char		language_eng[64];

        char*		description;
    };

    struct chapter_t
    {
        int64_t		chapter_pos;
        mtime_t		chapter_sec;
        mtime_t		chapter_pts;

        int32_t		desc_count;
        chapter_desc_t* p_descs;
    };

    struct stream_info_t
    {
        int32_t				kind;			// mxCodecFileType_MPEG1, mxCodecFileType_MPEG2, ...

        int32_t				playtime;		// in second
        int64_t				stream_size;

        video_track_info_t	video_track;

        int32_t				def_audio_no;
        int32_t				cur_audio_no;
        int32_t				audio_track_count;
        int32_t				audio_track_extra_count;
        audio_track_info_t	audio_tracks[MX_AUDIO_TRACK_MAX_NUM];

        int32_t				def_spu_no;
        int32_t				cur_spu_no;
        int32_t				spu_track_count;
        spu_track_info_t	spu_tracks[MX_SPU_TRACK_MAX_NUM];

        uint32_t			palette[12];

        int32_t				chapter_base;
        int32_t				chapter_count;
        chapter_t*			p_chapters;		// do not free by the user

        int32_t				i_header;		// size of the stream header, ex, PMT data in case of TS
        char*			p_header;		// the stream header, do not free by the user

        int32_t				i_extra;		// size of the extra stream data, ex, pack and vod data
        char*			p_extra;		// the extra stream data, do not free by the user

        int32_t				playtime_ms;	// in millisecond
        int32_t				i_pat_header;	// size of the stream header, ex, PAT data in case of TS
        char*			p_pat_header;	// the stream header, do not free by the user

        int32_t				stream_bitrate;
        int32_t				i_flags;		// mxStreamInfoFlags_ShowSubPict, mxStreamInfoFlags_ShowAudioDesc, ...

        int32_t				attachment_count;
        void*	attachments;

    };


    struct stream_pos_ex_t
    {
        mtime_t		millisec;		// negative value(-1) is invalid

        int64_t		stream_pos;		// stream position read from the meta data
        // negative value(-1) is invalid
        // negative value(-2) is special case
        mtime_t		start_cr;		// start clock reference value read from the meta data
        // negative value(-1) is invalid

        int32_t		chapter_no;		// negative value(-1) is invalid

        int32_t		seek_mode;		// MX_SEEK_MODE_DEFAULT, ...

        int32_t		flags;          // mxSeekFlags_ShowBlackPict, ...

        int64_t		ref_I_stream_pos;	// reference I stream position read from the meta data
        // negative value(-1) is invalid
    };

    struct vod_audio_format_t
    {
        uint32_t		i_format;                 /**< audio format fourcc */
        uint32_t		i_rate;                   /**< audio sample-rate */

        /* Describes the channels configuration of the samples (ie. number of
        * channels which are available in the buffer, and positions). */
        uint32_t		i_physical_channels;

        /* Describes from which original channels, before downmixing, the
        * buffer is derived. */
        uint32_t		i_original_channels;

        /* Optional - for A/52, SPDIF and DTS types : */
        /* Bytes used by one compressed frame, depends on bitrate. */
        uint32_t		i_bytes_per_frame;

        /* Number of sampleframes contained in one compressed frame. */
        uint32_t		i_frame_length;
        /* Please note that it may be completely arbitrary - buffers are not
        * obliged to contain a integral number of so-called "frames". It's
        * just here for the division :
        * buffer_size = i_nb_samples * i_bytes_per_frame / i_frame_length
        */

        uint32_t		i_bitrate;                /**< audio bitrate */

        /* FIXME ? (used by the codecs) */
        uint32_t		i_bitspersample;
        uint32_t		i_blockalign;
        uint8_t			i_channels; /* must be <=32 */

        bool			b_vbr;
        int32_t			i_profile;       /**< codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) */
        int32_t			i_level;         /**< codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) */

        /* mp4 editing */
        //	int32_t			i_soun_box;
        //	buffer_t		p_soun_box;
        uint32_t		i_sample_per_packet; // non-zero if qt_version == 1

        /* avi editing */
        int32_t			i_dwscale;
        int32_t			i_dwrate;
        uint32_t		i_dwsamplesize;
    };

    /**
    * video format description
    */
    struct vod_video_format_t
    {
        uint32_t		i_chroma;                 /**< picture chroma */
        uint32_t		i_aspect;                 /**< aspect ratio = i_sar_num/i_sar_den */

        uint32_t		i_width;                  /**< picture width */
        uint32_t		i_height;                 /**< picture height */
        uint32_t		i_x_offset;               /**< start offset of visible area */
        uint32_t		i_y_offset;               /**< start offset of visible area */
        uint32_t		i_visible_width;          /**< width of visible area */
        uint32_t		i_visible_height;         /**< height of visible area */

        uint32_t		i_bits_per_pixel;         /**< number of bits per pixel */

        uint32_t		i_sar_num;                /**< sample/pixel aspect ratio */
        uint32_t		i_sar_den;

        uint32_t		i_frame_rate;             /**< frame rate numerator */
        uint32_t		i_frame_rate_base;        /**< frame rate denominator */
        uint32_t		i_bitrate;

        bool			b_vbr;
        bool			b_progressive;            /**< added by SYH */
        int32_t			i_profile;                /**< codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) */
        int32_t			i_level;                  /**< codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) */

        uint32_t		i_rmask, i_gmask, i_bmask; /**< color masks for RGB chroma */
        int32_t			i_rrshift, i_lrshift;
        int32_t			i_rgshift, i_lgshift;
        int32_t			i_rbshift, i_lbshift;
        video_palette_t* p_palette;                /**< video palette from demuxer */

        uint32_t		i_spu_x_offset;
        uint32_t		i_spu_y_offset;

        int32_t			b_flipped;                 /** in normal case, flip flag
                                                   * in case of ASTB17, half height flag */
        uint32_t		orientation;               /**< picture orientation, video_orientation_t */
        int32_t			field_order;               /**< field coded, display order, 0 : AV_FIELD_PROGRESSIVE, 1 : AV_FIELD_TT, 6 : AV_FIELD_BB, 9 : AV_FIELD_TB, 14 : AV_FIELD_BT */
        uint32_t		seq_aspect_ratio;          /**< pel_aspect_ratio in the sequence header of MPEG video */
    };

    /**
    * subtitles format description
    */
    struct vod_subs_format_t
    {
        /* the character encoding of the text of the subtitle.
        * all gettext recognized shorts can be used */
        char*		psz_encoding;

        int32_t		i_x_origin; /**< x coordinate of the subtitle. 0 = left */
        int32_t		i_y_origin; /**< y coordinate of the subtitle. 0 = top */

        struct
        {
            /*  */
            uint32_t	palette[16 + 1];

            /* the width of the original movie the spu was extracted from */
            int32_t		i_original_frame_width;
            /* the height of the original movie the spu was extracted from */
            int32_t		i_original_frame_height;
        } spu;

        struct
        {
            int32_t	i_id;
        } dvb;
        struct
        {
            int32_t	i_magazine;
            int32_t	i_page;
        } teletext;
    };


    struct attachment_info_t
    {
        char*		file_name;
        char*		mime_type;
        int32_t		data_size;
        buffer_t	p_data;
    };

    struct vod_es_format_t
    {
        int32_t			i_cat;              /**< ES category, MX_ELEMENT_VIDEO, MX_ELEMENT_AUDIO, MX_ELEMENT_SPU */
        uint32_t		i_codec;            /**< FOURCC value as used in MediaCodec */
        uint32_t		i_original_fourcc;  /**< original FOURCC from the container */

        int32_t			i_id;       /**< es identifier, where means
                                    -1: let the core mark the right id
                                    >=0: valid id */
        int32_t			i_group;    /**< group identifier, where means:
                                    -1 : standalone
                                    >= 0 then a "group" (program) is created
                                    for each value */
        int32_t			i_priority; /**< priority, where means:
                                    -2 : mean not selectable by the users
                                    -1 : mean not selected by default even
                                    when no other stream
                                    >=0: priority */

        vod_audio_format_t	audio;     /**< description of audio format */
        vod_video_format_t	video;     /**< description of video format */
        vod_subs_format_t	subs;      /**< description of subtitle format */

        uint32_t		i_bitrate; /**< bitrate of this ES */
        int32_t			i_profile;       /**< codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) */
        int32_t			i_level;         /**< codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) */

        bool			b_packetized;  /**< whether the data is packetized (ie. not truncated) */
        int32_t			i_extra;        /**< length in bytes of extra data pointer */
        buffer_t		p_extra;       /**< extra data needed by some decoders or muxers */

        char*			psz_language;         /**< human readable language name, or attach_filename */
        char*			psz_description;      /**< human readable description of language, or attach_mimetype */
        //int32_t			i_extra_languages;    /**< length in bytes of extra language data pointer */
        //extra_languages_t* p_extra_languages; /**< extra language data needed by some decoders */
    };
    class MxVODNetStream
    {
        friend class CVODNetReadThread;
    public:
        MxVODNetStream(char* inUrl, int32_t inRtpTransport, int32_t inAuthMode);
        virtual ~MxVODNetStream();

        //virtual void SetUrlInfo(mxVODNetUrl_Info* urlInfo);
        //mxVODNetUrl_Info* GetUrlInfo();

        virtual int32_t Open(int titleNumber = 0);
        virtual int32_t Close();

        virtual int32_t Read(void* inBuffer, int32_t inReadSize, int32_t* outRead);
        virtual int32_t ControlStream(int32_t inMsgCode, va_list inParamsN);
        virtual int32_t Control(int32_t inMsgCode, ...);
        virtual int32_t GetCodecType();

        int ParseFormatExtraData(char* inBuffer, int32_t inSize, vod_es_format_t*** outEsFormat,
                                    int32_t* outEsCount, stream_info_t* outInfo, bool* outProxyMode, char** outNextBuffer);
        int32_t ParseAttachData(buffer_t inBuffer, int32_t inSize, vod_es_format_t***	outEsFormat, int32_t* outEsCount,
                                    stream_info_t* outInfo, bool* outProxyMode);
        int32_t ParseVODHeader(buffer_t		inVODBuffer, int32_t inVODSize, mtime_t inFirstPts, stream_info_t*	outInfo);
        void SetSeek(int32_t inTime, int32_t inChapterNo, int64_t inFilePos, int64_t inScrPos);

    private:
        void CreateVODInstance();
        void DisposeVODInstance();
        int32_t ConnectStream();

        void PreFillBuffer();
        void NetStreamEventProcess();
        int32_t GetEvent(int32_t *outEventKind, int32_t *eventTime, int32_t *eventParam);

        void DoPause();
        void DoContinue();

        bool MakeURLForRedirect(char* url, char* address, unsigned short& port);

        bool GetRecordTime(long* outTime);
        bool IsConncect();
        bool IsEOF();
        bool IsPrefilling();
        void* GetVODInstance();
        void LogProc(char* logStr);

        void StartStream();
        void StopStream();

        void SendDataSync(uint8_t* inData, int32_t inSize, 
            uint32_t inRtpTime, unsigned short inRtpExtflag, 
            unsigned char* inRtpExtdata, long inRtpExtsize);

        void SetTcpStreamError();
        void CaptureStream(bool bCapture);
        int32_t DownloadDataProcess(int32_t iReqTsCount, uint32_t* oRTPTime = NULL);

    public:
        void MessageProc(int32_t inMsgCode, char* MsgText, int32_t inTextLen = -1);

    protected:
        //Queue : Reference
        void Push(uint8_t* pData, int32_t count);
        void Pop(uint8_t* pData, int32_t& count);
        int32_t GetPktCount();
        int32_t GetTimestampDepth();
        int32_t GetTimestampDelta(int32_t start, int32_t end);
        void EmptyQueue();

    public:
        bool IsRecvStopCmd();
        int64_t GetBufferingTimeDipth();

    private:
        void CapturePush(uint8_t* pData, int32_t count);
        void CapturePop(uint8_t* pData, int32_t& count);
        int32_t CaptureGetPktCount();
        void CaptureEmptyQueue();

    protected:
        mxVODNetUrl_Info m_UrlInfo;

        _MxVODNetStream_t* m_VODNetMembers;
    };

    class CVODNetReadThread 
    {
    private:
        vlc_thread_t m_thread;
    public:
        CVODNetReadThread(MxVODNetStream* inStream);
        virtual ~CVODNetReadThread();
        //
        virtual void Run();

        void StartThread();
        void StopThread();
        void DoPause();
        void DoContinue();
        void RecvStart();
        void RecvStop();

        bool m_IsPause;
        bool m_bRun;
        MxVODNetStream* m_pStream;
    };

    int32_t MxGetVODServerTime();


#endif //_MX_VODNET_STREAM_H_INCLUDE_

