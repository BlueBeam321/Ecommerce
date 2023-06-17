#ifndef _VOBDHeader_h_
#define _VOBDHeader_h_
// ---------------------------------------------------------------

#if !defined(ATTRIBUTE_PACKED)
	#define ATTRIBUTE_PACKED
	#define PRAGMA_PACK 1
#endif

#if PRAGMA_PACK
	#pragma pack(push, 1)
#endif

// ---------------------------------------------------------------

#define MAX_ASCNT				8
#define MAX_SPSCNT				32
#define MAX_ASATTR				192
#define SUBPICTURE_PALETTE_NUM	16

#define VOBHeader_PackSize		2*1024


//////////////////////////////////////////////////////////////////////////////////////////////
enum /*practical_type_enum*/{
	VTS_CAT_GENERAL				= 0,
	VTS_CAT_KARAOK				= 1,
};

enum /*video_attr_enum*/{
	V_ATR_MPEG_1				= 0,
	V_ATR_MPEG_2				= 1,
	
	V_ATR_FORMAT_NTSC			= 0,
	V_ATR_FORMAT_PAL			= 1,
	
	V_ATR_ASPECT_4_3			= 0,
	V_ATR_ASPECT_16_9			= 3,
	
	V_ATR_PERMITTED_PAN_LET		= 0,
	V_ATR_PERMITTED_PANSCAN		= 1,
	V_ATR_PERMITTED_LETTERBOX	= 2,
	
	V_ATR_PICT_WIDTH_720		= 0,	//720 X 480(NTSC), 720 X 576(PAL)
	V_ATR_PICT_WIDTH_704		= 1,	//704 X 480(NTSC), 704 X 576(PAL)
	V_ATR_PICT_WIDTH_352_BIG	= 2,	//352 X 480(NTSC), 352 X 576(PAL)
	V_ATR_PICT_WIDTH_352		= 3,	//352 X 240(NTSC), 352 X 288(PAL)
	
	V_ATR_LETTERBOXED_NO		= 0,
	V_ATR_LETTERBOXED_YES		= 1,
	
	V_ATR_FILM_MODE_CAMERA		= 0,
	V_ATR_FILM_MODE_FILM		= 1,
};

enum /*audio_attr_enum*/{
	AST_ATRT_FORMAT_AC3			= 0,
	AST_ATRT_FORMAT_MPEG		= 2,
	AST_ATRT_FORMAT_MPEG_EX		= 3,
	AST_ATRT_FORMAT_PCM			= 4,
	AST_ATRT_FORMAT_DTS			= 6,
	AST_ATRT_FORMAT_SDDS		= 7,
	
	AST_ATTR_APP_MODE_GENERAL	= 0,
	AST_ATTR_APP_MODE_KARAOK	= 1,
	AST_ATTR_APP_MODE_SEROUND	= 2,
	
	AST_ATTR_FREQUENCY_48		= 0,
	AST_ATTR_FREQUENCY_96		= 1,
};

enum /*subp_attr_t*/
{
	SPST_ATRT_CODEC_2BITRLE		= 0,
	SPST_ATRT_CODEC_4BITRLE		= 4,
};

//////////////////////////////////////////////////////////////////////////////////
//  Macro
#define	VOBD_ID		VLC_FOURCC('D','B','O','V')

//////////////////////////////////////////////////////////////////////////////////
/**
* VTS Practical Type
*/

typedef struct {
#if defined(_MX_BIG_ENDIAN_)
	uint8_t reserver[3];					// [b31 - b4] reserved
	uint8_t reserver1				: 4;	//
	uint8_t practicaltype			: 4;	// [b3  - b0] VTS_CAT_
#else
	uint8_t reserver[3];
	uint8_t practicaltype			: 4;
	uint8_t reserver1				: 4;
#endif
} ATTRIBUTE_PACKED practical_type_t;

/**
 * Video Attributes.
 */
typedef struct {
#if defined(_MX_BIG_ENDIAN_)
	uint8_t mpeg_version			: 2;	// [b15b14]	:	00: mpeg1,	01: mpeg2,	10: h.264	V_ATR_MPEG_
	uint8_t video_format			: 2;	// [b13b12] :	00: NTSC	01: PAL					V_ATR_FORMAT_ 
	uint8_t display_aspect_ratio	: 2;	// [b11b10] :	00: 4:3		11: 16:9				V_ATR_ASPECT_
	uint8_t permitted_df			: 2;	// [b9b8]   :	00: (16:9)Pan-scan & Letterbox		V_ATR_PERMITTED_
											//				01: (16:9)Pan-scan
											//				10: (16:9)Letterbox
	
	uint8_t line21_cc_1				: 1;	// [b7b6]   :	reserved
	uint8_t line21_cc_2				: 1;	
	uint8_t picture_size			: 3;	// [b5-b3]  :	000 - 720 X 480(NTSC), 720 X 576(PAL)	V_ATR_PICT_WIDTH_
											//				001 - 704 X 480(NTSC), 704 X 576(PAL)
											//				010 - 352 X 480(NTSC), 352 X 576(PAL)
											//				011 - 352 X 240(NTSC), 352 X 288(PAL)
	uint8_t letterboxed				: 1;	// [b2]		:	V_ATR_LETTERBOXED_
	uint8_t unknown1				: 1;
	uint8_t film_mode				: 1;	// [b0]		:	0: Camera,	1: Film					V_ATR_FILM_MODE_
#else
	uint8_t permitted_df			: 2;
	uint8_t display_aspect_ratio	: 2;
	uint8_t video_format			: 2;
	uint8_t mpeg_version			: 2;
	
	uint8_t film_mode				: 1;
	uint8_t unknown1				: 1;
	uint8_t letterboxed				: 1;
	uint8_t picture_size			: 3;
	uint8_t line21_cc_2				: 1;
	uint8_t line21_cc_1				: 1;
#endif
} ATTRIBUTE_PACKED video_attr_t;

/**
 * Audio Attributes.
 */
typedef struct {
#if defined(_MX_BIG_ENDIAN_)
  uint8_t audio_format				: 3;	// [b63-b61]	AST_ATRT_FORMAT_
						  					//				000 - Dolby AC-3
											//				010 - MPEG-1 / MPEG-2 without extension bitstream
											//				011 - MPEG-2 with extension bitstream
											//				100 - LPCM
											//				110 - DTS
											//				111 - SDDS

  uint8_t multichannel_extension	: 1;	// [b60]		1 - Karaoke / Surround mode
  uint8_t lang_type					: 2;	// [b59b58]		00 : no meaning,	01 : Language,	other : reserved
  uint8_t application_mode			: 2;	// [b57b56]		00 : no meaning,	01 : karaoke,	10 : Surround,	11 : reserved	AST_ATTR_APP_MODE_
  
  uint8_t quantization				: 2;	// [b55b54]		Quantization/DRC
  uint8_t sample_frequency			: 2;	// [b53b52]		00 : 48kHz,			01 : 96kHz,		other : reserved				AST_ATTR_FREQUENCY_
  uint8_t unknown1					: 1;	// [b51]		reserved
  uint8_t channels					: 3;	// [b50b49b48]	audio channel count + 1
#else
  uint8_t application_mode			: 2;
  uint8_t lang_type					: 2;
  uint8_t multichannel_extension	: 1;
  uint8_t audio_format				: 3;
  
  uint8_t channels					: 3;
  uint8_t unknown1					: 1;
  uint8_t sample_frequency			: 2;
  uint8_t quantization				: 2;
#endif
  uint8_t lang_code[2];						// language code
  uint8_t lang_extension;					// 
  uint8_t code_extension;					// 
  uint8_t blfe;								// 1 if LFE exist, 0 otherwise
  union {
    struct {
#if defined(_MX_BIG_ENDIAN_)
      uint8_t unknown4				: 1;
      uint8_t channel_assignment	: 3;
      uint8_t version				: 2;
      uint8_t mc_intro				: 1;	/* probably 0: true, 1:false */
      uint8_t mode					: 1;	/* Karaoke mode 0: solo 1: duet */
#else
      uint8_t mode					: 1;
      uint8_t mc_intro				: 1;
      uint8_t version				: 2;
      uint8_t channel_assignment	: 3;
      uint8_t unknown4				: 1;
#endif
    } ATTRIBUTE_PACKED karaoke;
    struct {
#if defined(_MX_BIG_ENDIAN_)
      uint8_t unknown5				: 4;
      uint8_t dolby_encoded			: 1;	/* suitable for surround decoding */
      uint8_t unknown6				: 3;
#else
      uint8_t unknown6				: 3;
      uint8_t dolby_encoded			: 1;
      uint8_t unknown5				: 4;
#endif
    } ATTRIBUTE_PACKED surround;
  } app_info;
} ATTRIBUTE_PACKED audio_attr_t;

/**
 * Subpicture Attributes.
 */
typedef struct {
  /*
   * type: 0 not specified
   *       1 language
   *       2 other
   * coding mode: 0 run length
   *              1 extended
   *              2 other
   * language: indicates language if type == 1
   * lang extension: if type == 1 contains the lang extension
   */
#if defined(_MX_BIG_ENDIAN_)
  uint8_t code_mode				: 3;	// [b47-b45] - sps encoding mode
  uint8_t zero1					: 3;	// [b44-b42] - reserved(3bit)
  uint8_t type					: 2;	// [b41b40] - sps type
#else
  uint8_t type					: 2;
  uint8_t zero1					: 3;
  uint8_t code_mode				: 3;
#endif
  uint8_t zero2;
  uint8_t lang_code[2];
  uint8_t lang_extension;
  uint8_t code_extension;
} ATTRIBUTE_PACKED subp_attr_t;

typedef struct{
#if defined(_MX_BIG_ENDIAN_)
#if 1
	uint8_t		useflag			: 1; 	// [b15]
	uint8_t		zero1			: 4; 	// [b14-b11] - reserved(4bit)
	uint8_t		a_stream_num	: 3;	// audio stream number(3bit)
	
	uint8_t		zero2;
#else
	uint8_t		zero2;

	uint8_t		useflag			: 1;
	uint8_t		zero1			: 4;
	uint8_t		a_stream_num	: 3;
#endif
#else
	uint8_t		a_stream_num	: 3;	
	uint8_t		zero1			: 4; 		
	uint8_t		useflag			: 1; 		
	
	uint8_t		zero2;
#endif
}ATTRIBUTE_PACKED program_audio_ctlt_t;

typedef struct{
#if defined(_MX_BIG_ENDIAN_)
	uint8_t		useflag	: 1;				// [b31]
	uint8_t		zero1	: 2;				// [b30b29] - reserved(2bit)
	uint8_t		s_4_3_stream_num : 5;		// [b28-b24] - sps stream number in 4:3 mode
	
	uint8_t		zero2	: 3;				// [b23-b21] - reserved(3bit)
	uint8_t		s_wide_stream_num : 5;		// [b20-b16] - sps stream number in Wide mode (16:9)
	
	uint8_t		zero3	: 3;				// [b15-b13] - reserved(3bit)
	uint8_t		s_letterbox_stream_num : 5;	// [b12-b8] - sps stream number in Letterbox mode
	
	uint8_t		zero4	: 3;				// [b7-b5] - reserved(3bit)
	uint8_t		s_panscan_stream_num : 5;	// [b4-b0] - sps stream number in Pan-scan mode
#else
	uint8_t		s_4_3_stream_num : 5;
	uint8_t		zero1	: 2;
	uint8_t		useflag	: 1;
	
	uint8_t		s_wide_stream_num : 5;
	uint8_t		zero2	: 3;
	
	uint8_t		s_letterbox_stream_num : 5;
	uint8_t		zero3	: 3;
	
	uint8_t		s_panscan_stream_num : 5;
	uint8_t		zero4	: 3;
#endif
}ATTRIBUTE_PACKED program_subpict_ctlt_t;

typedef  struct{
	uint8_t	reserver;
	uint8_t	y_value;
	uint8_t	cr_value;
	uint8_t	cb_value;
}ATTRIBUTE_PACKED program_subpict_palette_t;

typedef  struct{
	uint32_t			VOB_ID;				// VOBD = VOBD_ID
	uint16_t			VERN;				// VERSION
	practical_type_t	VTS_CAT;			// VTS category
	video_attr_t		V_ATR;				// video property
	
	uint16_t			AST_Ns;				// audio stream count
	audio_attr_t		AST_ATRT[8];		// audio stream property (8*8=64byte)
	
	uint16_t			SPST_Ns;			// sps stream count
	subp_attr_t			SPST_ATRT[32];		// sps stream property (32*6=192byte)
	
	uint8_t				MU_AST_ATR[192];	// multi channel audio property (8*24=192byte)

	uint16_t			PGCI_Ns;			// pgc count
}ATTRIBUTE_PACKED vobd_info_t;

typedef  struct{
#if defined(_MX_BIG_ENDIAN_)
	uint8_t		reserved : 6;		// [b7-b2]
	uint8_t		sp_disp_mode : 2;	// [b1b0] - initial disp mode
									//		[b1] : 0 : no apply, 1 : apply
									//		[b0] : 0 : show, 1 : hide
#else
	uint8_t		sp_disp_mode : 2;
	uint8_t		reserved : 6;
#endif
	uint8_t		SCR_start[7];		// reserved, 0
	int64_t		SCR_end;			// end SCR, 0
	int64_t		PTS_start;			// start PTS, 0
	int64_t		PTS_end;			// end PTS, 0
	int64_t		POS_start;			// start POS, 0
	int64_t		POS_end;			// end POS, 0
	
	program_audio_ctlt_t		PGC_AST_CTLT[8];	// info of 8 audio streams
	program_subpict_ctlt_t		PGC_SPST_CTLT[32];	// info of 32 sps streams
	program_subpict_palette_t	PGC_SP_PLT[16];		// palette of sps (64byte)
}ATTRIBUTE_PACKED program_info_t;

typedef struct
{
	uint8_t			special_flag;
	uint16_t		file_index;
	uint16_t		chapterbasenum;
	uint16_t		chapter_ns;
}ATTRIBUTE_PACKED chapter_info_t;

typedef struct {
	vobd_info_t		vobdinfo;
	program_info_t	programinfo;
	chapter_info_t	chapterinfo;
} dvm_info_t;

// ---------------------------------------------------------------

#if PRAGMA_PACK
	#pragma pack(pop)
#endif

#if defined(ATTRIBUTE_PACKED)
	#undef	ATTRIBUTE_PACKED
	#undef	PRAGMA_PACK
#endif

// ---------------------------------------------------------------

static inline int32_t GetAudioID(int32_t audiocodec,int32_t audionum)
{
	int32_t 	audioID=0;
	
	switch(audiocodec)
	{
		case AST_ATRT_FORMAT_AC3:
			//audioID = 0x80BD + (audionum << 8);
			audioID = 0xBD80 + audionum;
			break;
		case AST_ATRT_FORMAT_MPEG:
		case AST_ATRT_FORMAT_MPEG_EX:
			audioID = 0x00C0 + audionum;
			break;
		case AST_ATRT_FORMAT_PCM:
			//audioID = 0xA0BD + (audionum << 8);
			audioID = 0xBDA0 + audionum;
			break;
		case AST_ATRT_FORMAT_DTS:
			//audioID = 0x88BD + (audionum << 8);
			audioID = 0xBD88 + audionum;
			break;
		case AST_ATRT_FORMAT_SDDS:
			break;
	}
	return audioID;
}

// 0 : none
// 1 : Audio
// 2 : Subpicture

static inline int32_t GetPESKind(int32_t stream_id, int32_t sub_stream_id, int32_t& out_audio_kind)
{
	out_audio_kind = 0;

	if(stream_id == 0xBD)
	{
		if( (sub_stream_id >= 0x80 && sub_stream_id <= 0x87) ||	// AC3
			(sub_stream_id >= 0xA0 && sub_stream_id <= 0xA7) ||	// PCM
			(sub_stream_id >= 0x88 && sub_stream_id <= 0x8F) )	// DTS
		{
			out_audio_kind = sub_stream_id;
			return 1;
		}

		if( (sub_stream_id >= 0x20 && sub_stream_id < 0x40) ||	// 2BITRLE
			(sub_stream_id >= 0x40 && sub_stream_id < 0x60) )	// 4BITRLE
			return 2;
	}
	else if(stream_id >= 0xC0 && stream_id <= 0xC2)
	{
		out_audio_kind = stream_id;
		return 1;
	}
	return 0;
}

static inline bool IsSubPicture(int32_t stream_id, int32_t sub_stream_id)
{
	if(stream_id == 0xBD)
	{
		if( (sub_stream_id >= 0x20 && sub_stream_id < 0x40) ||	// 2BITRLE
			(sub_stream_id >= 0x40 && sub_stream_id < 0x60) )	// 4BITRLE
			return true;
	}
	return false;
}

static inline void GetAudioCodecName(char*outStr, int32_t audiocodec)
{
	switch(audiocodec)
	{
		case AST_ATRT_FORMAT_AC3:
			strcpy(outStr, "AC3");
			break;
		case AST_ATRT_FORMAT_MPEG:
		case AST_ATRT_FORMAT_MPEG_EX:
			strcpy(outStr, "MPG");
			break;
		case AST_ATRT_FORMAT_PCM:
			strcpy(outStr, "PCM");
			break;
		case AST_ATRT_FORMAT_DTS:
			strcpy(outStr, "DTS");
			break;
		case AST_ATRT_FORMAT_SDDS:
			strcpy(outStr, "SDD");
			break;
		default:
			strcpy(outStr, "");
	}
}

#define DEF_STR_CHNL	""

static inline void GetChannelName(char*outStr,int32_t audiocodec,int32_t channelnum, int32_t isLFE)
{
	if(outStr == NULL)
		return;

	if (isLFE)
		sprintf(outStr, "%d.1%s", channelnum+1, DEF_STR_CHNL);
	else
		sprintf(outStr, "%d%s", channelnum+1, DEF_STR_CHNL);
}

static inline int32_t GetSubPictID(int32_t codec_mode,int32_t spunum)
{
	if(codec_mode == SPST_ATRT_CODEC_2BITRLE)
		//return 0x20BD + (spunum << 8);
		return 0xBD20 + spunum;
	else if(codec_mode == SPST_ATRT_CODEC_4BITRLE)
		//return 0x40BD + (spunum << 8);
		return 0xBD40 + spunum;

	return 0;
}


#endif

