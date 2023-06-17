#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_mtime.h>
#include <vlc_fourcc.h>
#include <vlc_es.h>

#include "MxVODNetStream.h"
#include "MxArrayBase.h"
#include "MxErrCodes.h"
#include "MxBitStream.h"
//#include "MxBlockQueue.h"

#include "VODNetLib.h"
#include "VOBDHeader_SJIS.h"
#define	Malloc(addr, size, type)	{addr = (type)::malloc(size); ::memset(addr, 0, size);}

#ifndef _WIN32
#include <arpa/inet.h>
#define _strnicmp	strncmp
#endif

#define CHECK_FIRST_GROUP(group_name, buffer, buffer_end) \
	uint32_t i_left_size_##group_name = buffer_end - buffer; \
	uint32_t i_total_size_##group_name = GetDWBE(buffer); buffer += 4; \
	if(i_left_size_##group_name + 4 < i_total_size_##group_name) \
		return mxErr_Generic;

#define CHECK_GROUP(group_name, buffer, buffer_end) \
	buffer_t p_buffer_##group_name = buffer; \
	uint32_t i_left_size_##group_name = buffer_end - buffer; \
	uint32_t i_total_size_##group_name = GetDWBE(buffer); buffer += 4; \
	if(i_left_size_##group_name < i_total_size_##group_name) \
		return mxErr_Generic;

#define SKIP_GROUP(group_name, buffer) \
	buffer = p_buffer_##group_name + i_total_size_##group_name;


#define READ_16(buffer) \
	GetWBE(buffer); buffer += 2;

#define READ_32(buffer) \
	GetDWBE(buffer); buffer += 4;

#define READ_64(buffer) \
	GetQWBE(buffer); buffer += 8;

static inline char* GetString16(buffer_t& p_buffer)
{
    char* p_str = NULL;
    int32_t i_len = GetWBE(p_buffer); p_buffer += 2;

    if (i_len)
    {
        p_str = (char*)malloc(i_len + 1);
        memcpy(p_str, p_buffer, i_len);
        p_str[i_len] = 0;

        p_buffer += i_len;
    }

    return p_str;
}
#define _USE_RTPEXT_FPOS_	1		//ver2.63 video_pointer_docment ref

#define	TS_PACKET_LENGTH		188
#define	DATA_PACK_SIZE			7*TS_PACKET_LENGTH

#define MX_TIME_1_MILISEC       1000
#ifndef MAXDWORD
#	define MAXDWORD				0xFFFFFFFF
#endif
#define DELTADWORD(a,b) ((a >= b) ? (a-b) : (a+(MAXDWORD-b)+1))
#define DELTATICK(a,b) DELTADWORD(a,b)


#define kLV_Msg_StreamReady_Str		"Stream Ready"
#define kLV_Msg_StreamPlay_Str		"Stream Playing"
#define kLV_Msg_StreamStop_Str		"Stream Stop"
#define kLV_Msg_StreamErrStop_Str	"Stream Error Stop"

#define Msg_Clear_Str				""
#define Msg_Connetting_Str			"Connecting"
#define Msg_Negotiating_Str			"Negotiating"
#define Msg_RequestedData_Str		"requested data"
#define Msg_Buffering_Str			"Buffering..."
#define Msg_ConnectionFaild_Str		"Connection Failed"
#define kLV_Msg_ConnectSuccessStr	"Net Stream Connect Success"

#	define MAX_NETDIPTH_LIMIT		1000000
#	define MAX_NETDIPTH				20000
#	define NET_BUFFERING_TIME		1000000		// 1s


//#define USE_TS_SAVE		1

struct _MxVODNetStream_t
{
	VODINSTANCE m_VODInstance;

	int32_t m_Linequality;

	bool m_bQuit;
	bool m_FirstConnect;
	bool m_isTsStream;
	bool m_isConnect;
	bool m_FillBuff;

	bool m_Continue;
	bool m_PreFilling;

	int32_t m_errorCode;
    float m_StartScrTime;
    float m_AllPlayTime;
    uint32_t m_FirstRTPTime;
    uint32_t m_LastRTPTime;

	float m_seekTime;
    int32_t m_ChapterNum;
    int64_t m_seekFilepos;
    int64_t m_seekStartCr;

    uint8_t* m_VODHeader;
    uint32_t m_VODHeaderSize;
    uint8_t* m_AttachmentHeader;			//MKV
    uint32_t m_AttachmentHeaderSize;

    uint32_t m_FileType;
    uint32_t m_StreamType;
    uint16_t m_Width;
    uint16_t m_Height;
	float m_FPS;

	MxBlockQueue* m_pPktInfoQueue;
	vlc_mutex_t m_hMutexForQueue;
    vlc_mutex_t m_mutex;
	CVODNetReadThread* m_ReadThread;
    uint32_t m_OldTicktime;
	int32_t m_bStreamEof;

	bool m_bCapture;
	MxBlockQueue* m_pCaptureQueue;
	uint32_t m_CaptureFirstRTPTime;

	bool m_DownloadProcessFlag; //MKV Ref
	bool m_bAuthStreamMode;
    uint32_t m_TitleNumber;
};

//----------------------------------------------
//*MxVODNetStream
//----------------------------------------------
MxVODNetStream::MxVODNetStream(char* inUrl, int32_t inRtpTransport, int32_t inAuthMode)
{
    m_VODNetMembers = new _MxVODNetStream_t;
    memset(m_VODNetMembers, 0, sizeof(_MxVODNetStream_t));
    memset(&m_UrlInfo, 0, sizeof(mxVODNetUrl_Info));
    strcpy(m_UrlInfo.url, inUrl);
    m_UrlInfo.rtpTransport = inRtpTransport;
    m_UrlInfo.authMode = inAuthMode;
    m_UrlInfo.startTime = -1;
    m_UrlInfo.startCr = -1;
    m_UrlInfo.startPos = -1;

	m_VODNetMembers->m_pPktInfoQueue = new MxBlockQueue(TS_PACKET_LENGTH * MAX_NETDIPTH, TS_PACKET_LENGTH);

    vlc_mutex_init(&m_VODNetMembers->m_hMutexForQueue);
    vlc_mutex_init(&m_VODNetMembers->m_mutex);
	m_VODNetMembers->m_seekTime = -1.0f;

	m_VODNetMembers->m_seekStartCr = -1;
	m_VODNetMembers->m_seekFilepos = -1;
	m_VODNetMembers->m_bStreamEof = false;

	m_VODNetMembers->m_pCaptureQueue = NULL;
	m_VODNetMembers->m_DownloadProcessFlag = false;
	m_VODNetMembers->m_bAuthStreamMode = 0;
}

MxVODNetStream::~MxVODNetStream()
{
	DisposeVODInstance();

	if (m_VODNetMembers->m_pPktInfoQueue)
		delete m_VODNetMembers->m_pPktInfoQueue;
	m_VODNetMembers->m_pPktInfoQueue = NULL;

    vlc_mutex_destroy(&m_VODNetMembers->m_hMutexForQueue);
    vlc_mutex_destroy(&m_VODNetMembers->m_mutex);

	if (m_VODNetMembers->m_pCaptureQueue)
		delete m_VODNetMembers->m_pCaptureQueue;
	m_VODNetMembers->m_pCaptureQueue = NULL;

    if (m_VODNetMembers->m_VODHeader)
	    free(m_VODNetMembers->m_VODHeader);
    if (m_VODNetMembers->m_AttachmentHeader)
	    free(m_VODNetMembers->m_AttachmentHeader);
	delete (m_VODNetMembers);
}

static void GetVODErrorFunc(char* inProgramID, long inErrCode, const char* inErrStr, long inErrStrSize, void* inParams)
{
	if (inErrCode == kVODNet_Error_NoErr)
		return;
	
	MxVODNetStream* pNetStream;
	pNetStream = (MxVODNetStream*)inParams;
	if (pNetStream)
	{
		if (inErrCode == 11000805)
			pNetStream->MessageProc(inErrCode,(char*) inErrStr);
        else
			printf(" ErrCode = %d, ErrStr = %s", inErrCode, inErrStr);
	}
}

static bool IsStopingFunc(void* inParams)
{
	MxVODNetStream* pObj;
	pObj = (MxVODNetStream*)inParams;
	if (pObj)
		return pObj->IsRecvStopCmd();
	return false;
}

void MxVODNetStream::CreateVODInstance()
{
	unsigned long SSRC;
	
	srand((unsigned int)time(NULL));
	SSRC = rand();
	
	int32_t appKind = 0;
	//if (m_VODNetMembers->m_pMediaPlayer)
	//	m_VODNetMembers->m_pMediaPlayer->GetVideoAttr(&appKind, NULL);
    m_VODNetMembers->m_VODInstance = (void*)VODNet_CreateClient(SSRC,
        GetVODErrorFunc, IsStopingFunc,
        (void*)this,
        /*appKind == ATTR_STB ? true :*/ false, m_UrlInfo.authMode);

	m_VODNetMembers->m_isConnect = false;
	m_VODNetMembers->m_FirstConnect = true;
	m_VODNetMembers->m_bStreamEof = false;
	VODNet_SetRTPStreamMode(m_VODNetMembers->m_VODInstance, m_UrlInfo.rtpTransport);
	
}

void MxVODNetStream::DisposeVODInstance()
{
	StopStream();
	
	if (m_VODNetMembers->m_VODInstance)
	{
		long nDBTime = MxGetVODServerTime();
		
		if (m_VODNetMembers->m_isConnect)
		{
			LogProc("VODNet_SendTeardown");
			VODNet_SendTeardown(m_VODNetMembers->m_VODInstance, nDBTime);
		}
		
		LogProc("VODNet_DisposeClient");
		VODNet_DisposeClient(m_VODNetMembers->m_VODInstance);
	}
	
	m_VODNetMembers->m_VODInstance = NULL;
	m_VODNetMembers->m_FirstConnect = false;
	m_VODNetMembers->m_isConnect = false;

	
}

//void MxVODNetStream::SetUrlInfo(mxVODNetUrl_Info* urlInfo)
//{
//	if (urlInfo)
//		memcpy(&m_UrlInfo, urlInfo, sizeof(mxVODNetUrl_Info));
//}
//
//mxVODNetUrl_Info* MxVODNetStream::GetUrlInfo()
//{
//	return &m_UrlInfo;
//}

static inline char HexStrToValue(char inChar)
{
	char value = 0;
	if ('0' <= inChar && inChar <= '9')
		value = inChar - '0';
	else if ('a' <= inChar && inChar <= 'f')
        value = inChar - 'a' + 10;
    else if ('A' <= inChar && inChar <= 'F')
		value = inChar - 'A' + 10;
	else
        value = 0;
    return value;
}

//----------------------------------------------
//*Open:
//----------------------------------------------
int32_t MxVODNetStream::Open(int titleNumber)
{
	int32_t err;

    m_VODNetMembers->m_TitleNumber = titleNumber;

	err = ConnectStream();
	if (err)
	{
		msg_Err((vlc_object_t*)NULL, "%s(%ld)", kLV_Msg_StreamErrStop_Str, err);
        
		//MessageProc(kLV_Msg_StreamErrStop, errStr);

        msg_Err((vlc_object_t*)NULL, "%s(%ld)", Msg_ConnectionFaild_Str, err);
		//MessageProc(kLV_Msg_ConnectionFaild, errStr);
		//MessageProc(err, "");

		return -1;
	}

	return 0;
}

int32_t MxVODNetStream::ConnectStream()
{
    int32_t iNetSpeedValue = 0;
	int32_t iAttachmentSize = 0;
    
	m_VODNetMembers->m_ChapterNum = -1;

	//<- epg_ref
	m_VODNetMembers->m_seekTime = m_UrlInfo.startTime;
	m_VODNetMembers->m_seekFilepos = m_UrlInfo.startPos;
	m_VODNetMembers->m_seekStartCr = m_UrlInfo.startCr;
	// -> epg_ref

	m_VODNetMembers->m_errorCode = 0;
	m_VODNetMembers->m_isConnect = 0;
	m_VODNetMembers->m_bQuit = false;
	m_VODNetMembers->m_bStreamEof = false;
	m_VODNetMembers->m_bAuthStreamMode = m_UrlInfo.authMode;

	vlc_mutex_lock(&m_VODNetMembers->m_mutex);
	
	if (m_VODNetMembers->m_VODInstance == NULL)
		CreateVODInstance();

	if (m_VODNetMembers->m_VODInstance)
	{
		int32_t nDBTime = MxGetVODServerTime();
		char* strDVDInfo = NULL;

		//MessageProc(L_LVideo::kLV_Msg_StreamReady , kLV_Msg_StreamReady_Str);
		//MessageProc(L_LVideo::kLV_Msg_Connetting , Msg_Connetting_Str);
		
		m_VODNetMembers->m_FileType = 0;
		m_VODNetMembers->m_Width = 0;
		m_VODNetMembers->m_Height = 0;
		m_VODNetMembers->m_FPS = 0.0f;

		if (m_VODNetMembers->m_FirstConnect)
		{
			m_VODNetMembers->m_StreamType = 0;
			
			unsigned short portNum;
			char RedirectSrvAddr[128];
			char theURL[1024];
			unsigned short iThumbPort = 0;
            char strThumnbAddr[64] = { 0, };
            char strThumbInfoStr[64];
            char strThumbIdStr[64];
            bool bMainServer = true;
            memset(strThumbInfoStr, 0, sizeof(strThumbInfoStr));
            memset(strThumbIdStr, 0, sizeof(strThumbIdStr));

            memset(theURL, 0, sizeof(theURL));
            memset(RedirectSrvAddr, 0, sizeof(RedirectSrvAddr));
            portNum = 9113;

			strcpy(theURL, m_UrlInfo.url);

	        //iNetSpeedValue = GetNetSpeedEnvValue();//ver2.61 by KCR 2016.3.3.
			do
			{
				long specialnum = 0,redirectfalg = 0;
				

                m_VODNetMembers->m_errorCode = VODNet_SendDescribe(//DESCRIBE
                    m_VODNetMembers->m_VODInstance,
                    theURL,
                    (short)specialnum,
                    redirectfalg,
                    m_VODNetMembers->m_Width,
                    m_VODNetMembers->m_Height,
                    m_VODNetMembers->m_FPS,
                    m_VODNetMembers->m_FileType,
                    m_VODNetMembers->m_StreamType,
                    &strDVDInfo,
                    portNum,
                    RedirectSrvAddr,
                    &iThumbPort,
                    strThumnbAddr,
                    strThumbIdStr,
                    iNetSpeedValue,
                    (int*)&iAttachmentSize);

				if (bMainServer)
				{
					if (iThumbPort != 0 && strlen(strThumnbAddr) > 0 && strlen(strThumbIdStr)>0)
					{
						sprintf(strThumbInfoStr, "%s %d %s",strThumnbAddr, (int)iThumbPort, strThumbIdStr);
						//MessageProc(kLV_Msg_ThumbInfo, strThumbInfoStr);
					}
				}
				
				bMainServer = false;
				
				//if (m_VODNetMembers->m_pMediaPlayer)
				//	m_VODNetMembers->m_pMediaPlayer->m_StreamType = m_VODNetMembers->m_StreamType;
				
				if (m_VODNetMembers->m_errorCode != kVODNet_Error_RedirectError)
					break;

				//if (m_VODNetMembers->m_pMediaPlayer)
				//	m_VODNetMembers->m_pMediaPlayer->ResetTitleTimeForRedirect();

				MakeURLForRedirect(theURL, RedirectSrvAddr, portNum);
			} while (1);
		}
		else
		{
			long specialnum = 0;
			//if (m_VODNetMembers->m_pMediaPlayer)
			//	specialnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;

            specialnum = m_VODNetMembers->m_TitleNumber;
            m_VODNetMembers->m_errorCode = VODNet_SendGetTitleParam(
                m_VODNetMembers->m_VODInstance,
                nDBTime,
                (short)specialnum,
                m_VODNetMembers->m_Width,
                m_VODNetMembers->m_Height,
                m_VODNetMembers->m_FPS,
                m_VODNetMembers->m_FileType,
                m_VODNetMembers->m_StartScrTime,
                m_VODNetMembers->m_AllPlayTime,
                &strDVDInfo,
                (int*)&iAttachmentSize);
		}
		
		if (m_VODNetMembers->m_errorCode != kVODNet_Error_NoErr)
		{	
			DisposeVODInstance();
            vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
            return m_VODNetMembers->m_errorCode + 100;
		}

		if (m_VODNetMembers->m_Height > 700) // HD
		{
			int32_t LineQuality = 0;
			int32_t SpeedLan = 1;
			bool bIsPlayable = true;

			int32_t outAttr = 0;
			char outAppName[256];
			
			//if (m_VODNetMembers->m_pMediaPlayer)
			//	m_VODNetMembers->m_pMediaPlayer->GetVideoAttr(&outAttr, outAppName);
		}	

		if (strDVDInfo)
		{
			long		len,size,i;
			char*		pBuff = NULL;
			
			len = strlen(strDVDInfo);
			size = len / 2 + 10;

			free(m_VODNetMembers->m_VODHeader);

			m_VODNetMembers->m_VODHeader = (uint8_t*)malloc(size);
			m_VODNetMembers->m_VODHeaderSize = size;

			if (m_VODNetMembers->m_VODHeader)
			{
                for (i = 0; i < len; i += 2)
					m_VODNetMembers->m_VODHeader[i/2] = (HexStrToValue(strDVDInfo[i]) << 4) | HexStrToValue(strDVDInfo[i+1]);
			}
		}

		//MessageProc(kLV_Msg_Negotiating, Msg_Negotiating_Str);

		if (iAttachmentSize>0)
		{
			long speicalnum = 0;
			nDBTime = MxGetVODServerTime();

			//if (m_VODNetMembers->m_pMediaPlayer)
			//	speicalnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;
			//MessageProc(kLV_Msg_FontLoading, "FontLoading...");
            speicalnum = m_VODNetMembers->m_TitleNumber;

			free(m_VODNetMembers->m_AttachmentHeader);

		    m_VODNetMembers->m_AttachmentHeader = (uint8_t*)malloc(iAttachmentSize+10);
			m_VODNetMembers->m_AttachmentHeaderSize = iAttachmentSize;

            m_VODNetMembers->m_errorCode = VODNet_AttachmentReq(//ATTACHMENTREQ
                m_VODNetMembers->m_VODInstance,
                nDBTime,
                (short)speicalnum,
                m_VODNetMembers->m_AttachmentHeader,
                iAttachmentSize);
			if (m_VODNetMembers->m_errorCode != kVODNet_Error_NoErr)
			{
				DisposeVODInstance();
                vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
                return kVODNet_Error_NoMediaData;
			}
		}

		if (m_VODNetMembers->m_FirstConnect)
		{
			long speicalnum = 0;
			//if (m_VODNetMembers->m_pMediaPlayer)
			//	speicalnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;
            speicalnum = m_VODNetMembers->m_TitleNumber;
			nDBTime = MxGetVODServerTime();
            m_VODNetMembers->m_errorCode = VODNet_SendPlay(//SETUP
                m_VODNetMembers->m_VODInstance,
                nDBTime,
                m_VODNetMembers->m_StartScrTime,
                m_VODNetMembers->m_AllPlayTime,
                m_VODNetMembers->m_seekTime,
                (short)speicalnum,
                -1);
			
			if (m_VODNetMembers->m_errorCode == kVODNet_Error_NoErr && m_VODNetMembers->m_AllPlayTime <= 0)
			{
				DisposeVODInstance();
                vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
				return kVODNet_Error_NoMediaData;
			}
			
			if (m_VODNetMembers->m_errorCode == 0)
			{
				long curspcialnum = 0;
				//if (m_VODNetMembers->m_pMediaPlayer)
				//	curspcialnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;
                curspcialnum = m_VODNetMembers->m_TitleNumber;
                m_VODNetMembers->m_errorCode = VODNet_SendPausePlay(
                    m_VODNetMembers->m_VODInstance,
                    nDBTime,
                    m_VODNetMembers->m_seekTime,
                    -1,
                    (short)curspcialnum,
                    m_VODNetMembers->m_ChapterNum,
                    m_VODNetMembers->m_seekFilepos,
                    m_VODNetMembers->m_seekStartCr);
			}
		}
		else
		{
			long speicalnum = 0;

			//if (m_VODNetMembers->m_pMediaPlayer)
			//	speicalnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;
            speicalnum = m_VODNetMembers->m_TitleNumber;
			msleep(MX_TIME_1_MILISEC*200);

			VODNet_ClearDownLoadBuffer(m_VODNetMembers->m_VODInstance);

            m_VODNetMembers->m_errorCode = VODNet_SendPausePlay(
                m_VODNetMembers->m_VODInstance,
                nDBTime,
                m_VODNetMembers->m_seekTime,
                -1,
                (short)speicalnum,
                -1,
                m_VODNetMembers->m_seekFilepos, //epg_ref
                m_VODNetMembers->m_seekStartCr);
		}

		if (m_VODNetMembers->m_errorCode != kVODNet_Error_NoErr)
		{
			DisposeVODInstance();
            vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
			return -3;
		}
	}
    else
    {
        vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
        return -2;
    }
	m_VODNetMembers->m_seekTime = -1.0f;

	m_VODNetMembers->m_FillBuff = false;
	m_VODNetMembers->m_Continue = true;
	m_VODNetMembers->m_isConnect = true;
	m_VODNetMembers->m_FirstConnect = false;

	//if (m_VODNetMembers->m_pMediaPlayer)
	//	m_VODNetMembers->m_pMediaPlayer->m_allTime = (int64_t)m_VODNetMembers->m_AllPlayTime;

	m_VODNetMembers->m_FirstRTPTime = 0;
	m_VODNetMembers->m_LastRTPTime = 0;

	StartStream();
    vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
	return 0;
}

//----------------------------------------------
//*Close:
//----------------------------------------------
int32_t MxVODNetStream::Close()
{
	StopStream();

	return 0;
}

void MxVODNetStream::PreFillBuffer()
{
	mtime_t msgTime = 0;
	uint32_t RTPDeltaTime = 0;

	vlc_mutex_lock(&m_VODNetMembers->m_mutex);

	if (m_VODNetMembers->m_bQuit || m_VODNetMembers->m_isConnect == false)
	{
        vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
		return;
	}

	if (CaptureGetPktCount() > 0)
    {
		RTPDeltaTime = m_VODNetMembers->m_LastRTPTime - m_VODNetMembers->m_FirstRTPTime;
		if (RTPDeltaTime * 100 / 9 >= GetBufferingTimeDipth())
		{
			m_VODNetMembers->m_PreFilling = false;
            vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
            return;
		}
	}
	

	m_VODNetMembers->m_PreFilling = true; //epg_ref
	
	//MessageProc(kLV_Msg_Buffering , Msg_Buffering_Str);

	long ReceiveByte = 0;
	mtime_t startTick;
    mtime_t curTick;

	startTick = mdate();

	do
	{
		RTPDeltaTime = m_VODNetMembers->m_LastRTPTime - m_VODNetMembers->m_FirstRTPTime;
		ReceiveByte = GetPktCount() * TS_PACKET_LENGTH;

        curTick = mdate();
		if (DELTADWORD(curTick, startTick) > 30000000)
		{
			//if (m_VODNetMembers->m_pMediaPlayer)
			//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
			m_VODNetMembers->m_isConnect = false;
			//MessageProc(kLV_Msg_CuttingStream , "Time Out Cutting Stream");
			break;
		}

		if (RTPDeltaTime * 100 / 9 >= GetBufferingTimeDipth())
		{
			//MessageProc(kLV_Msg_DemuxStarted , "Buffering Finished"); epg_ref
			break;
		}
		else
		{
			int32_t is_event;
			int32_t i_EventKind,eventTime,eventParam;

			is_event = GetEvent(&i_EventKind , &eventTime , &eventParam);
			if (is_event)
			{
				if (i_EventKind == kVODNet_Event_LineFailed)
				{
					//if (m_VODNetMembers->m_pMediaPlayer)
					//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
					m_VODNetMembers->m_isConnect = false;
					//MessageProc(kLV_Msg_CuttingStream , "Cutting Stream 1");
					break;
				}
				else if (i_EventKind == kVODNet_Event_ServerBye)
				{
					if (eventParam == kVODNet_ByeReason_ServerDown || 
						eventParam == kVODNet_ByeReason_LineFailed)
					{
						//if (m_VODNetMembers->m_pMediaPlayer)
						//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
						m_VODNetMembers->m_isConnect = false;

						//if (eventParam == kVODNet_ByeReason_LineFailed)
						//	MessageProc(kLV_Msg_CuttingStream , "Cutting Stream 2");
						//else
						//	MessageProc(kLV_Msg_ServerDown , "Server Down");

						break;
					}
				}
				else if (m_VODNetMembers->m_isConnect == false)
				{
					//if (m_VODNetMembers->m_pMediaPlayer)
					//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
					//MessageProc(kLV_Msg_CuttingStream , "Cutting Stream 4");
				}
			}
		}
		
		msleep(MX_TIME_1_MILISEC*200);

		//if (IsRecvStopCmd() == true || (m_VODNetMembers->m_pMediaPlayer && m_VODNetMembers->m_pMediaPlayer->m_bStopFlag)) //at epg_ref : add line.
		//	break;

	} while (m_VODNetMembers->m_bQuit == false && m_VODNetMembers->m_Continue);

	//MessageProc(kLV_Msg_DemuxStarted , "Buffering Finished"); //epg_ref
    vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
	m_VODNetMembers->m_PreFilling = false;
}

void MxVODNetStream::NetStreamEventProcess()
{
	vlc_mutex_lock(&m_VODNetMembers->m_mutex);

    int32_t is_event;
    int32_t i_EventKind, eventTime, eventParam;
	
	is_event = GetEvent(&i_EventKind , &eventTime , &eventParam);
	if (is_event)
	{
		if (i_EventKind == kVODNet_Event_LineFailed)
		{
			//if (m_VODNetMembers->m_pMediaPlayer)
			//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
			m_VODNetMembers->m_isConnect = false;
			//MessageProc(kLV_Msg_CuttingStream , "Cutting Stream 9");
		}
		else if (i_EventKind == kVODNet_Event_ServerBye)
		{
			if (kVODNet_ByeReason_ServerDown == eventParam || kVODNet_ByeReason_LineFailed == eventParam)
			{
				//if (m_VODNetMembers->m_pMediaPlayer)
				//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
				m_VODNetMembers->m_isConnect = false;
				
				//if (kVODNet_ByeReason_LineFailed == eventParam)
				//	MessageProc(kLV_Msg_CuttingStream , "Cutting Stream 10");
				//else
				//	MessageProc(kLV_Msg_ServerDown , "Server Down");
			}
		}
	}
	vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
	
}

int32_t MxVODNetStream::GetEvent(int32_t *outEventKind ,int32_t *eventTime, int32_t *eventParam)
{
	if (m_VODNetMembers->m_VODInstance && m_VODNetMembers->m_isConnect)
	{
		VODNet_Event	Event;
		if (0 == VODNet_GetEvent(m_VODNetMembers->m_VODInstance , &Event , 0))
		{
			if (outEventKind)
                *outEventKind = (int32_t)Event.eventKind;
			if (eventTime)
                *eventTime = (int32_t)Event.eventTime;
			if (eventParam)
                *eventParam = (int32_t)Event.eventParams;
			return 1;
		}
	}
	return 0;
}

void MxVODNetStream::SetSeek(int32_t inTime, int32_t inChapterNo, int64_t inFilepos, int64_t inStartCr)
{
	VODNet_Error err = kVODNet_Error_NoErr;
	
	if (m_VODNetMembers->m_isConnect == false)
		return;
	
	
	vlc_mutex_lock(&m_VODNetMembers->m_mutex);

	m_VODNetMembers->m_seekTime = (float)inTime;
	m_VODNetMembers->m_ChapterNum = inChapterNo;
	m_VODNetMembers->m_seekFilepos = inFilepos;
	m_VODNetMembers->m_seekStartCr = inStartCr;

	if (m_VODNetMembers->m_VODInstance)
	{
		err = VODNet_ClearDownLoadBuffer(m_VODNetMembers->m_VODInstance);
		if (err != kVODNet_Error_NoErr)
		{
			m_VODNetMembers->m_isConnect = false;
			//MessageProc(kLV_Msg_CuttingStream, "Cutting Stream 7");
		}
	}
	
	m_VODNetMembers->m_FillBuff = false;
	EmptyQueue();
	vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
}

void MxVODNetStream::DoPause()
{
	VODNet_Error	err = kVODNet_Error_NoErr;

	if (m_VODNetMembers->m_Continue == false)
		return;

	m_VODNetMembers->m_Continue = false;
	vlc_mutex_lock(&m_VODNetMembers->m_mutex); //epg_ref	
	
	if (m_VODNetMembers->m_VODInstance && m_VODNetMembers->m_isConnect)
	{
		long nDBTime = MxGetVODServerTime();
		err = VODNet_SendPause(m_VODNetMembers->m_VODInstance, nDBTime);
		if (err != kVODNet_Error_NoErr)
		{
			m_VODNetMembers->m_isConnect = false;
			//MessageProc(kLV_Msg_CuttingStream, "Cutting Stream 5");
		}
	}

	if (m_VODNetMembers->m_ReadThread)
		m_VODNetMembers->m_ReadThread->DoPause();

	m_VODNetMembers->m_ChapterNum = -1;
	m_VODNetMembers->m_seekFilepos = -1;
	m_VODNetMembers->m_seekStartCr = -1;
	m_VODNetMembers->m_seekTime = -1.0f;	//2016.5.2 ver 2.62
	vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
}

void MxVODNetStream::DoContinue()
{
	VODNet_Error	err = kVODNet_Error_NoErr;
	
	if (m_VODNetMembers->m_Continue)
		return;

	vlc_mutex_lock(&m_VODNetMembers->m_mutex); //epg_ref

	if (m_VODNetMembers->m_VODInstance && m_VODNetMembers->m_isConnect)
	{
		long nDBTime = MxGetVODServerTime();
		long	curspcialnum = 0;
		//if (m_VODNetMembers->m_pMediaPlayer)
		//	curspcialnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;

        err = VODNet_SendPausePlay(
            m_VODNetMembers->m_VODInstance,
            nDBTime,
            m_VODNetMembers->m_seekTime,
            -1,
            (short)curspcialnum,
            m_VODNetMembers->m_ChapterNum,
            m_VODNetMembers->m_seekFilepos,
            m_VODNetMembers->m_seekStartCr);

		if (err != kVODNet_Error_NoErr)
		{
			m_VODNetMembers->m_isConnect = false;
			//MessageProc(kLV_Msg_CuttingStream,"Cutting Stream 6");
            vlc_mutex_unlock(&m_VODNetMembers->m_mutex); //epg_ref

			return;
		}
		m_VODNetMembers->m_seekTime = -1.0f;
	}

	uint32_t RTPDeltaTime = m_VODNetMembers->m_LastRTPTime - m_VODNetMembers->m_FirstRTPTime;
	
	if (RTPDeltaTime * 100 / 9 < GetBufferingTimeDipth())
		m_VODNetMembers->m_FillBuff = false;
	
	m_VODNetMembers->m_Continue = true;

	if (m_VODNetMembers->m_ReadThread)
		m_VODNetMembers->m_ReadThread->DoContinue();
    vlc_mutex_unlock(&m_VODNetMembers->m_mutex);
}

void MxVODNetStream::CaptureStream(bool bCapture)
{
	m_VODNetMembers->m_bCapture = bCapture;
}

int32_t MxVODNetStream::DownloadDataProcess(int32_t iReqTsCount, uint32_t* oRTPTime)
{
    long i_reqsize = iReqTsCount * 188, i_revSize, i_pos;
	int32_t	nDBTime;
	long theResult;
    char* rtpData = new char[i_reqsize];
    char* readPos;
    char* endPos;
    char* oneRtpData;
    char* oneRtpEnd;
    char* rtpExtData;
	unsigned remExtSize;
	unsigned remExtFlag;
	unsigned firstRTPTime = 0;
	int32_t i_err = 0;

	unsigned short i_rtplen;
	i_revSize = i_reqsize;

	if (m_VODNetMembers->m_isConnect == false)
		return -1;

	nDBTime = MxGetVODServerTime();
    m_VODNetMembers->m_errorCode = VODNet_DownloadData(GetVODInstance(), nDBTime, rtpData, i_revSize);
	if (m_VODNetMembers->m_errorCode == kVODNet_Error_NoErr)
	{
		if (i_revSize > 0)
		{
			i_pos = 0;
			readPos = rtpData;
			endPos = rtpData + i_revSize;
			while (readPos < endPos)
			{
				//check '$'
				if ((*readPos) != '$')
					break;
				readPos ++;

				//skip chnlID
				readPos ++;

				//read rtp_len
				i_rtplen = ntohs(*((unsigned short*)readPos));
				readPos += 2;

				//Parse RTP data
				{//Parse RTP
					oneRtpData = readPos;
					oneRtpEnd = readPos + i_rtplen;
					rtpExtData = NULL;
					remExtSize = 0;
					remExtFlag = 0;

					unsigned rtpHdr = ntohl(*(unsigned*)(oneRtpData)); oneRtpData += 4;
					bool rtpMarkerBit = bool(((rtpHdr&0x00800000) >> 23)==1);
					unsigned rtpTimestamp = ntohl(*(unsigned*)(oneRtpData));oneRtpData += 4;
                    unsigned rtpSSRC = ntohl(*(unsigned*)(oneRtpData)); oneRtpData += 4;
					firstRTPTime = rtpTimestamp;

                    if ((rtpHdr & 0xC0000000) != 0x80000000)
                        break;	//err
					unsigned cc = (rtpHdr>>24)&0xF;

					oneRtpData += cc*4;

					// check RTP_header_ext
                    if (rtpHdr & 0x10000000)
					{//Extension
                        unsigned extHdr = ntohl(*(unsigned*)(oneRtpData));
                        oneRtpData += 4;
                        remExtSize = 4 * (extHdr & 0xFFFF);
						remExtFlag = (extHdr >> 16);
						rtpExtData = oneRtpData;//bPacket->SetExtData(remExtFlag,bPacket->data(),remExtSize);
						oneRtpData += remExtSize;
					}

					//TsData : oneRtpData ~ oneRtpEnd
                    SendDataSync((uint8_t*)oneRtpData, oneRtpEnd - oneRtpData, rtpTimestamp, remExtFlag, (uint8_t*)rtpExtData, remExtSize);
					//if (gs_SaveFile)
					//	fwrite(oneRtpData,1,oneRtpEnd - oneRtpData,gs_SaveFile);
				}
                readPos += i_rtplen;
				//next rtp data.
			}
		}
        else
		{
            i_err = -2;
		}
	}
    else
	{
        m_VODNetMembers->m_isConnect = false;
        //MessageProc(kLV_Msg_CuttingStream, "Cutting Stream (rtsp com error: downloaddata)");
        i_err = -1;
	}

    if (oRTPTime)
        *oRTPTime = firstRTPTime;

    delete[] rtpData;
	return i_err;
}

// 3.24
bool MxVODNetStream::MakeURLForRedirect(char* url, char* address, unsigned short& portNum) 
{
	// Parse the URL as "rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t"
	char const* prefix = "rtsp://";
	unsigned const prefixLength = 7;
	if (_strnicmp(url, prefix, prefixLength) != 0) 
	{
		return false;
	}

	unsigned const parseBufferSize = 100;
	char parseBuffer[parseBufferSize];
	char const* from = &url[prefixLength];
	char* to = &parseBuffer[0];
	unsigned i;
	char theServerAddrStr[parseBufferSize];

	to = &theServerAddrStr[0];
	for (i = 0; i < parseBufferSize; ++i) 
	{
		if (*from == '\0' || *from == ':' || *from == '/') 
		{
			// parse address is end.
			*to = '\0';
			break;
		}
		*to++ = *from++;
	}

	if (i == parseBufferSize) 
	{
		return false;
	}

	portNum = 9113;//554; // default value
	char nextChar = *from;
	if (nextChar == ':') 
	{
		int portNumInt;
		if (sscanf(++from, "%d", &portNumInt) != 1)
			return false;
		if (portNumInt < 1 || portNumInt > 65535)
			return false;
		portNum = portNumInt;
		while (*from >= '0' && *from <= '9') ++from; // step port num 
	}

	char theURL[2048];
	
	memset(theURL, 0, sizeof(theURL));
	sprintf(theURL, "rtsp://%s:%d", address, portNum);
	
	strncat(theURL, from, 2048);
	strcpy(url, theURL);

	return true;
}

bool MxVODNetStream::IsConncect()
{
	return m_VODNetMembers->m_isConnect;
}

bool MxVODNetStream::IsEOF()
{
    return m_VODNetMembers->m_bStreamEof ? true : false;
}

bool MxVODNetStream::IsPrefilling()
{
	return m_VODNetMembers->m_PreFilling;
}

void* MxVODNetStream::GetVODInstance()
{
	return m_VODNetMembers->m_VODInstance;
}

bool MxVODNetStream::GetRecordTime(long* outTime)
{
	bool rslt = false;
	vlc_mutex_lock(&m_VODNetMembers->m_mutex);
	
	*outTime = -1;
	
	if (m_VODNetMembers->m_VODInstance && !m_VODNetMembers->m_DownloadProcessFlag)
	{
		long thetime;
		
		m_VODNetMembers->m_errorCode = VODNet_SendRecordParam(m_VODNetMembers->m_VODInstance, thetime);
		if (m_VODNetMembers->m_errorCode == kVODNet_Error_NoErr)
		{
			*outTime = thetime;
			rslt = true;
		}
	}
    vlc_mutex_unlock(&m_VODNetMembers->m_mutex);

	return rslt;
}

//----------------------------------------------
//*Read:
//----------------------------------------------
int32_t MxVODNetStream::Read(void* inBuffer, int32_t inReadSize, int32_t* outRead)
{
	long ReceiveByte;
	int32_t pktCnt;
    //static FILE *dump_fp = 0;

    //if (!dump_fp)
    //    dump_fp = fopen("VODTSDump.ts", "wb");
	NetStreamEventProcess();

	if (inBuffer == NULL || inReadSize == 0)
		return -1;
	
	if (m_VODNetMembers->m_bQuit || m_VODNetMembers->m_VODInstance == NULL || m_VODNetMembers->m_isConnect == false)
		return -1;

	if (m_VODNetMembers->m_DownloadProcessFlag == true) //ver2.64 video doc
	{
		pktCnt = GetPktCount();
		if (pktCnt < 10)
		{
			//DownloadData. req 350ts
			if (DownloadDataProcess(350) == 0)
			{
				pktCnt = GetPktCount();
				if (pktCnt < 6)
				{
					if (outRead)
						*outRead = 0;
					return -1;
				}
			}
            else
				return -1;
		}
	}
    else if (m_VODNetMembers->m_bCapture)
	{
		uint32_t startTick;
		uint32_t curTick;

		startTick = mdate();
		while (1)
		{
			curTick = mdate();
			pktCnt = GetPktCount();
			ReceiveByte = pktCnt * TS_PACKET_LENGTH;
			if (ReceiveByte >= inReadSize)
				break;

			if (DELTADWORD(curTick, startTick) > 30000000)
			{
				//if (m_VODNetMembers->m_pMediaPlayer)
				//	m_VODNetMembers->m_pMediaPlayer->m_isplaystart = 0;
				m_VODNetMembers->m_isConnect = false;
				//MessageProc(kLV_Msg_CuttingStream , "Time Out Cutting Stream");
				break;
			}

			//if (IsRecvStopCmd() == true || (m_VODNetMembers->m_pMediaPlayer && m_VODNetMembers->m_pMediaPlayer->m_bStopFlag)) //at epg_ref : add line.
			//	break;
			msleep(1000);
		}

		if (m_VODNetMembers->m_isConnect == false)
		{
			if (outRead)
				*outRead = 0;
			return -1;
		}

	}
    else if (m_VODNetMembers->m_FillBuff == false)
	{
		PreFillBuffer();
		
		if (m_VODNetMembers->m_Continue == false)
		{
			if (outRead)
				*outRead = 0;
			return -1;
		}
		
		m_VODNetMembers->m_FillBuff = true;
		
		if (m_VODNetMembers->m_isConnect == false)
			return -1;
		
		//if (m_VODNetMembers->m_bQuit == false /*&& m_input->IsSequence*/)
		//	MessageProc(kLV_Msg_StreamPlay , "StreamPlay ");
	}
	
	int32_t size = 0;

	if (m_VODNetMembers->m_bCapture == false)
	{
		pktCnt = CaptureGetPktCount();
		if (pktCnt > 0)
		{
			ReceiveByte = pktCnt * TS_PACKET_LENGTH;
			if (ReceiveByte < inReadSize)
			{
				inReadSize = ReceiveByte;
			}

			size = inReadSize / TS_PACKET_LENGTH;
			CapturePop((uint8_t*)inBuffer, size);
			size = size * TS_PACKET_LENGTH;
			if (outRead)
				*outRead = size;

			return 0;
		}
	}

	//uint32_t rtptime;
	pktCnt = GetPktCount();
	
	ReceiveByte = pktCnt * TS_PACKET_LENGTH;

	// IF "not read stream" THEN goto buffering...
	if (ReceiveByte < inReadSize || pktCnt > MAX_NETDIPTH_LIMIT)
	{
		if (pktCnt > MAX_NETDIPTH_LIMIT)
		{
			m_VODNetMembers->m_errorCode = VODNet_ClearDownLoadBuffer(m_VODNetMembers->m_VODInstance);
			MessageProc(m_VODNetMembers->m_errorCode , "ClearDownLoadBuffer");
		}
		
		if (pktCnt > MAX_NETDIPTH_LIMIT) //add 2019.12.30 by KTI
			EmptyQueue();
		else
		{
			m_VODNetMembers->m_FirstRTPTime = 0;
			m_VODNetMembers->m_LastRTPTime = 0;
		}

		m_VODNetMembers->m_FillBuff = false;
		PreFillBuffer();
		//if (m_VODNetMembers->m_bQuit == false && m_VODNetMembers->m_isConnect)
		//	MessageProc(kLV_Msg_StreamPlay , "StreamPlay ");
		m_VODNetMembers->m_FillBuff = true;

		if (outRead)
			*outRead = 0;
		return -1;
	}
	
	size = inReadSize / TS_PACKET_LENGTH;
    
	Pop((uint8_t*)inBuffer, size);
	if (size == 0)
		return -1;


	if (m_VODNetMembers->m_bCapture)
		CapturePush((uint8_t*)inBuffer, size);

	size = size * TS_PACKET_LENGTH;
	if (outRead)
		*outRead = size;
    //fwrite(inBuffer, 1, size, dump_fp);
	pktCnt = GetPktCount();
	ReceiveByte = pktCnt * TS_PACKET_LENGTH;
	
    uint32_t curTick;
    curTick = mdate();
    if (DELTATICK(curTick, m_VODNetMembers->m_OldTicktime) > 10000000)
    {
        m_VODNetMembers->m_OldTicktime = mdate();
    }
    
	return 0;
}
//
int32_t MxVODNetStream::Control(int32_t inMsgCode, ...)
{
    int32_t	ret;
    va_list	args;

    va_start(args, inMsgCode);
    ret = ControlStream(inMsgCode, args);
    va_end(args);

    return ret;
}

int32_t MxVODNetStream::ControlStream(int32_t inMsgCode, va_list inParamsN)
{
    int32_t ret = 0;
    int32_t i_param1, i_param2;
    int32_t nDBTime;

	switch (inMsgCode)
	{
		case mxStreamCtrlMsg_VODNetStreamPlayTime:
		{
			long playTime;
			int32_t* pPlayTime = va_arg(inParamsN, int32_t*);
			
			if (GetRecordTime(&playTime))
				*pPlayTime = playTime;
			else
				ret = -1;
			break;
		}
		case mxStreamCtrlMsg_VODNetStreamInfo:
		{
			stream_info_t* p_save_stream = va_arg(inParamsN, stream_info_t*);
			int32_t* pTsKind = va_arg(inParamsN, int32_t*);
			uint8_t** pVodHeader = va_arg(inParamsN, uint8_t**);
			int32_t* pVodHeaderSize = va_arg(inParamsN, int32_t*);
			uint8_t** pAttachment = va_arg(inParamsN, uint8_t**);
			int32_t* pAttachmentSize = va_arg(inParamsN, int32_t*);
			
			if (p_save_stream)
			{
				p_save_stream->kind = m_VODNetMembers->m_FileType;
				p_save_stream->playtime = (int)m_VODNetMembers->m_AllPlayTime;
				//p_save_stream->video_track.first_pts = (mtime_t)(m_VODNetMembers->m_StartScrTime * 90000);
			}
			
			if (pTsKind)
				*pTsKind = m_VODNetMembers->m_StreamType;
			
			if (pVodHeader)
				*pVodHeader = m_VODNetMembers->m_VODHeader;
			if (pVodHeaderSize)
				*pVodHeaderSize = m_VODNetMembers->m_VODHeaderSize;
			if (pAttachment)
				*pAttachment = m_VODNetMembers->m_AttachmentHeader;
			if (pAttachmentSize)
				*pAttachmentSize = m_VODNetMembers->m_AttachmentHeaderSize;

			break;
		}

		case mxStreamCtrlMsg_VODNetStreamSeek:
		{
			m_VODNetMembers->m_bStreamEof = false;
			break;
		}

		case mxStreamCtrlMsg_VODNetStreamEnd:
		{
			m_VODNetMembers->m_bStreamEof = true;
			break;
		}

		case mxStreamCtrlMsg_VODNetStreamSeekExStart:
			{
				int	curspcialnum = 0;
				vlc_mutex_lock(&m_VODNetMembers->m_mutex);
				stream_pos_ex_t* p_pos = va_arg(inParamsN, stream_pos_ex_t*);

				//MessageProc(kLV_Msg_Buffering , Msg_Buffering_Str);

				//Send Download_start to Server
				m_VODNetMembers->m_DownloadProcessFlag = true;

				//Pause Process.
				m_VODNetMembers->m_ChapterNum = -1;
				m_VODNetMembers->m_seekFilepos = -1;
				m_VODNetMembers->m_seekStartCr = -1;
				m_VODNetMembers->m_seekTime = -1.0f;	//2016.5.2 ver 2.62

				//if (m_VODNetMembers->m_pMediaPlayer)
				//	curspcialnum = m_VODNetMembers->m_pMediaPlayer->m_CurSpecialNum;

				nDBTime = MxGetVODServerTime();
				m_VODNetMembers->m_errorCode = VODNet_DownloadStart(GetVODInstance(),
                    nDBTime,
                    (char*)p_pos,
                    sizeof(stream_pos_ex_t));

				VODNet_ClearDownLoadBuffer(GetVODInstance());
				EmptyQueue();

				if (m_VODNetMembers->m_errorCode != kVODNet_Error_NoErr)
				{
					m_VODNetMembers->m_isConnect = false;
					//MessageProc(L_LVideo::kLV_Msg_CuttingStream , "Rtsp com error(downloadstart)");
				}
				vlc_mutex_lock(&m_VODNetMembers->m_mutex);
			}
			break;
		case mxStreamCtrlMsg_VODNetStreamSeekExEnd:
			{
				//Send Download_end to Server
				//Fill Net Buffer
				int32_t RTPDeltaTime = 0;
				vlc_mutex_lock(&m_VODNetMembers->m_mutex);
				if (m_VODNetMembers->m_isConnect == true)
				{
					uint32_t lastRTPTime = m_VODNetMembers->m_LastRTPTime;
					uint32_t baseRTPTime = m_VODNetMembers->m_LastRTPTime;
					do 
					{
						RTPDeltaTime = lastRTPTime - baseRTPTime;
						if (RTPDeltaTime * 100 / 9 >= GetBufferingTimeDipth())
							break;

						if (DownloadDataProcess(350, &lastRTPTime) != 0)
							break;

						RTPDeltaTime = m_VODNetMembers->m_LastRTPTime - m_VODNetMembers->m_FirstRTPTime;
						if (m_VODNetMembers->m_isConnect == false)
							break;
					} while (true);
					//--

					nDBTime = MxGetVODServerTime();
					m_VODNetMembers->m_errorCode = VODNet_DownloadEnd(GetVODInstance(),nDBTime);
					if (m_VODNetMembers->m_errorCode != kVODNet_Error_NoErr)
					{
						m_VODNetMembers->m_isConnect = false;
//						MessageProc(L_LVideo::kLV_Msg_CuttingStream , "Rtsp com error(downloadend)");
					}
					else
					{
//						MessageProc(kLV_Msg_DemuxStarted , "Buffering Finished"); //epg_ref
//						MessageProc(kLV_Msg_StreamPlay , "StreamPlay ");

						m_VODNetMembers->m_FillBuff = true;
					}
				}
				m_VODNetMembers->m_DownloadProcessFlag = false;
				vlc_mutex_lock(&m_VODNetMembers->m_mutex);
			}
			break;
		case mxStreamCtrlMsg_VODNetStreamPause:
			DoPause();
			break;

		case mxStreamCtrlMsg_VODNetStreamContinue:
			DoContinue();
			break;
		default:
			ret = -1;
			break;
	}

	return ret;
}

int32_t MxVODNetStream::GetCodecType()
{
    return -1;// MX_CODEC_MP2V;
}

//--------------------------------------------------------------------
// StartStream
//--------------------------------------------------------------------
void MxVODNetStream::StartStream()
{
	m_VODNetMembers->m_ReadThread = new CVODNetReadThread(this);
	m_VODNetMembers->m_ReadThread->StartThread();
}

//--------------------------------------------------------------------
// StopStream
//--------------------------------------------------------------------
void MxVODNetStream::StopStream()
{
	m_VODNetMembers->m_bQuit = true;
	if (m_VODNetMembers->m_ReadThread)
	{
		m_VODNetMembers->m_ReadThread->StopThread();
		delete m_VODNetMembers->m_ReadThread;
	}
	m_VODNetMembers->m_ReadThread = NULL;

	if (m_VODNetMembers->m_VODInstance && m_VODNetMembers->m_isConnect)
		VODNet_ClearDownLoadBuffer(m_VODNetMembers->m_VODInstance);

	EmptyQueue();

}

//void swap_vod(char *buf,long size)
//{
//	long		i;
//	char		tmp;
//	for (i=0;i<size/2;i++)
//	{
//		tmp = buf[i];
//		buf[i] = buf[size-i-1];
//		buf[size-i-1]=tmp;
//	}
//}
//------------------------------------------------------
//*SendDataSync : 
//------------------------------------------------------
void MxVODNetStream::SendDataSync(uint8_t* inData, int32_t inSize, uint32_t inRtpTime, unsigned short inRtpExtflag, unsigned char* inRtpExtdata, long inRtpExtsize)
{
	if (inRtpTime)
	{
		vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);

		if (m_VODNetMembers->m_FirstRTPTime == 0)
		{
			m_VODNetMembers->m_FirstRTPTime = inRtpTime;
			m_VODNetMembers->m_LastRTPTime = inRtpTime;
			m_VODNetMembers->m_CaptureFirstRTPTime = inRtpTime;
		}
		else
		{
			m_VODNetMembers->m_LastRTPTime = inRtpTime;

			if (m_VODNetMembers->m_LastRTPTime < m_VODNetMembers->m_FirstRTPTime) // TimeLoop
			{
				m_VODNetMembers->m_FirstRTPTime = 1;
				m_VODNetMembers->m_LastRTPTime = 1;
			}
		}
		vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
	}

	int32_t i, theTsCnt = inSize/TS_PACKET_LENGTH;
	uint8_t* pTSData = NULL;
    int64_t* pFpos = (int64_t*)inRtpExtdata;
    uint8_t fposTsData[188 + 4];
    int32_t fposTsSize = 188;

    if (inRtpExtflag && inRtpExtsize)
    {
        for (i = 0; i < theTsCnt; i++)
        {
            if (inRtpExtflag & (1 << i))
            {
                //Make pos ts
                char* p_buffer = (char*)&fposTsData[0];
                int32_t i_buffer = 188;

                int64_t i_fpos;

                i_fpos = GetQWBE(pFpos);
                //swap_vod((char*)&i_fpos,8);
                //MxSplitterUtils::MakeRTPStreamPosPacket((char**)fposTsData,&fposTsSize,*pFpos);
                Push(fposTsData, 1);
                pFpos++;
            }
            pTSData = &inData[i * TS_PACKET_LENGTH];
            Push((uint8_t*)pTSData, 1);
        }
    }
    else
        Push(inData, inSize / TS_PACKET_LENGTH);
}

//------------------------------------------------------
// Queue
//------------------------------------------------------
// Queue
void MxVODNetStream::Push(uint8_t* pData, int32_t count)
{
	vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);
    if (pData && count > 0)
        m_VODNetMembers->m_pPktInfoQueue->Push((uint8_t*)pData, count * TS_PACKET_LENGTH);
    vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);

}

void MxVODNetStream::Pop(uint8_t* pData, int32_t& count)
{
    vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);
    m_VODNetMembers->m_pPktInfoQueue->Pop((uint8_t*)pData, count * TS_PACKET_LENGTH);
    vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
}

int32_t MxVODNetStream::GetPktCount()
{
    int ret = -1;
    vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);
    ret = m_VODNetMembers->m_pPktInfoQueue->GetSize() / TS_PACKET_LENGTH;
    vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
    return ret;
}

int32_t MxVODNetStream::GetTimestampDelta(int32_t start, int32_t end)
{
    int ret = -1;

    vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);
    int32_t	count = m_VODNetMembers->m_pPktInfoQueue->GetSize() / TS_PACKET_LENGTH;
    if (count == 0)
    {
        vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
        return 0;
    }
    if (end == -1)
        end = count-1;
    int8_t* head = NULL;
    m_VODNetMembers->m_pPktInfoQueue->ReadFirst((char*)head, TS_PACKET_LENGTH);
    int8_t* tail = NULL;
    m_VODNetMembers->m_pPktInfoQueue->ReadLast((char*)tail, TS_PACKET_LENGTH);
    if (head == 0 || tail == 0)
    {
        vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
        return 0;
    }

    ret = DELTADWORD(*((uint32_t*)tail), *((uint32_t*)head));
    vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);
    return ret;

}

int32_t MxVODNetStream::GetTimestampDepth()
{
    int32_t timestampDepth = GetTimestampDelta(0, -1);
    //int32_t timestampDepth = m_VODNetMembers->m_LastRTPTime - m_VODNetMembers->m_FirstRTPTime;
    return timestampDepth / 90;
}

void MxVODNetStream::EmptyQueue()
{
	vlc_mutex_lock(&m_VODNetMembers->m_hMutexForQueue);
	m_VODNetMembers->m_pPktInfoQueue->Clear();

	m_VODNetMembers->m_FirstRTPTime = 0;
	m_VODNetMembers->m_LastRTPTime = 0;

	CaptureEmptyQueue();
    vlc_mutex_unlock(&m_VODNetMembers->m_hMutexForQueue);

}

//------------------------------------------------------
// Capture Queue
//------------------------------------------------------
void MxVODNetStream::CapturePush(uint8_t* pData, int32_t count)
{
    if (m_VODNetMembers->m_pCaptureQueue == NULL)
        m_VODNetMembers->m_pCaptureQueue = new MxBlockQueue(TS_PACKET_LENGTH * MAX_NETDIPTH, TS_PACKET_LENGTH);

    if (pData && count > 0 && m_VODNetMembers->m_pCaptureQueue)
        m_VODNetMembers->m_pCaptureQueue->Push((uint8_t*)pData, count * TS_PACKET_LENGTH);
}

void MxVODNetStream::CapturePop(uint8_t* pData, int32_t& count)
{
    if (m_VODNetMembers->m_pCaptureQueue)
        m_VODNetMembers->m_pCaptureQueue->Pop((uint8_t*)pData, count * TS_PACKET_LENGTH);
}

int32_t MxVODNetStream::CaptureGetPktCount()
{
    if (m_VODNetMembers->m_pCaptureQueue)
        return m_VODNetMembers->m_pCaptureQueue->GetSize() / TS_PACKET_LENGTH;
	return 0;
}

void MxVODNetStream::CaptureEmptyQueue()
{
	if (m_VODNetMembers->m_pCaptureQueue)
		m_VODNetMembers->m_pCaptureQueue->Clear();
	m_VODNetMembers->m_CaptureFirstRTPTime = 0;
}


bool MxVODNetStream::IsRecvStopCmd()
{
	//if (m_VODNetMembers->m_pMediaPlayer)
	//{
	//	//if (m_VODNetMembers->m_pMediaPlayer->GetNextState() == L_PlayState_Destroy)
	//	//	return true;
	//}
	return false;
}

void MxVODNetStream::LogProc(char* logStr)
{
	//if (m_VODNetMembers->m_pMediaPlayer)
	//	m_VODNetMembers->m_pMediaPlayer->msg_Reg(logStr);
}

void MxVODNetStream::MessageProc(int32_t inMsgCode, char* MsgText, int32_t inTextLen)
{
	//if (m_pfStateCallback)
	//{
	//	mx_value_t	theMsgValue;
	//	theMsgValue.psz_string = strdup(MsgText);
	//	m_pfStateCallback(inMsgCode, mxVarType_String, theMsgValue,m_pfParam);
	//}
}


int64_t MxVODNetStream::GetBufferingTimeDipth()
{
    long i_streamode = 0;
    if (m_VODNetMembers->m_VODInstance)
        i_streamode = VODNet_GetRTPStreamMode(m_VODNetMembers->m_VODInstance);

	if (i_streamode == 1/*eRTP_TCP*/)
	{
		return (int64_t)((float)NET_BUFFERING_TIME * 1.5);
	}

    return NET_BUFFERING_TIME;
}

void MxVODNetStream::SetTcpStreamError()
{
	m_VODNetMembers->m_isConnect = false;
	//MessageProc(kLV_Msg_CuttingStream , "TcpSock Error.");
}

static inline void GetCodeToLanguge(char *outStr, char *incode, int32_t languge)
{
#if 0
    if (incode)
    {
        int32_t	cnt, i;
        cnt = sizeof(p_languages) / sizeof(iso639_lang_t);
        for (i = 0; i < cnt; i++)
        {
            if (p_languages[i].psz_eng_name != NULL)
            {
                if (strncmp(p_languages[i].psz_iso639_1, incode, 2) == 0)
                {
                    if (languge == 0)
                        strcpy(outStr, p_languages[i].psz_native_name);
                    else
                        strcpy(outStr, p_languages[i].psz_eng_name);
                    return;
                }
            }
        }
    }
    if (languge == 0)
        strcpy(outStr, unknown_language.psz_native_name);
    else
        strcpy(outStr, unknown_language.psz_eng_name);
#endif
}

static inline void GetCodeToLangugeEx(char *outStr, char *incode, int32_t languge)
{
#if 0
    if (incode)
    {
        int32_t	cnt, i;
        cnt = sizeof(p_languages) / sizeof(iso639_lang_t);
        for (i = 0; i < cnt; i++)
        {
            if (p_languages[i].psz_eng_name != NULL)
            {
                if (strncmp(p_languages[i].psz_iso639_2T, incode, 3) == 0 || strncmp(p_languages[i].psz_iso639_2B, incode, 3) == 0)
                {
                    if (languge == 0)
                        strcpy(outStr, p_languages[i].psz_native_name);
                    else
                        strcpy(outStr, p_languages[i].psz_eng_name);
                    return;
                }
            }
        }
    }

    if (languge == 0)
        strcpy(outStr, "Unknown"/*unknown_language.psz_native_name*/);
    else
        strcpy(outStr, "Unknown"/*unknown_language.psz_eng_name*/);
#endif
}


static inline void GetLanguageToCodeEx(char *inStr, char *outCode)
{
#if 0
    const iso639_lang_t* p_lang;
    int32_t len = 0;

    if (inStr)
    {
        len = strlen(inStr);

        if (!strncmp(inStr, "Unknown", len) || !strncmp(inStr, "unknown", len))
        {
            memcpy(outCode, unknown_language.psz_iso639_2T, 3);
            return;
        }

        for (p_lang = p_languages; p_lang->psz_eng_name; p_lang++)
        {
            if (strlen(p_lang->psz_eng_name) == len && !strncmp(p_lang->psz_eng_name, inStr, len))
            {
                memcpy(outCode, p_lang->psz_iso639_2T, 3);
                return;
            }
        }
    }

    outCode = NULL;
#endif
    return;
}
static void GetLangFromCode(char* inCode, char* inKor, char* inEng)
{
    if (inCode == NULL)
    {
        GetCodeToLangugeEx(inKor, inCode, 0);
        GetCodeToLangugeEx(inEng, inCode, 1);
        return;
    }

    char* p_code = strdup(inCode);

    char* p_code1 = p_code;
    char* p_code2 = NULL;
    char* p_comma = strchr(p_code, ',');

    if (p_comma)
    {
        p_comma[0] = 0;
        p_code2 = p_comma + 1;
    }

    if (strlen(p_code1) == 3)
    {
        GetCodeToLangugeEx(inKor, p_code1, 0);
        GetCodeToLangugeEx(inEng, p_code1, 1);
    }
    else if (strlen(p_code1) == 2) // KCY
    {
        GetCodeToLanguge(inKor, p_code1, 0);
        GetCodeToLanguge(inEng, p_code1, 1);
    }
    else // KCY
    {
        char langCode[8] = { 0 };
        GetLanguageToCodeEx(p_code1, langCode);
        GetCodeToLangugeEx(inKor, langCode, 0);
        GetCodeToLangugeEx(inEng, langCode, 1);
    }

    if (p_code2)
    {
        strcat(inKor, ",");
        strcat(inEng, ",");

        if (strlen(p_code2) == 3)
        {
            GetCodeToLangugeEx(inKor + strlen(inKor), p_code2, 0);
            GetCodeToLangugeEx(inEng + strlen(inEng), p_code2, 1);
        }
        else if (strlen(p_code2) == 2) // KCY
        {
            GetCodeToLanguge(inKor + strlen(inKor), p_code2, 0);
            GetCodeToLanguge(inEng + strlen(inEng), p_code2, 1);
        }
        else // KCY
        {
            char langCode[8] = { 0 };
            GetLanguageToCodeEx(p_code2, langCode);
            GetCodeToLangugeEx(inKor, langCode, 0);
            GetCodeToLangugeEx(inEng, langCode, 1);
        }
    }

    free(p_code);
}

static void SetAudioTrackInfo(audio_track_info_t* p_track,
    int32_t i_codec_id,
    vod_es_format_t* p_es_info,
    int32_t i_track_id)
{
    p_track->kind = i_codec_id;
    p_track->sub_kind = p_es_info->audio.i_profile;

    p_track->channel_count = p_es_info->audio.i_channels;
    p_track->channel_layout = p_es_info->audio.i_physical_channels;
    p_track->has_lef = p_es_info->audio.i_physical_channels & AOUT_CHAN_LFE;
    p_track->frequency = p_es_info->audio.i_rate;
    p_track->bitrate = p_es_info->audio.i_bitrate;
    p_track->b_vbr = p_es_info->audio.b_vbr;

    p_track->track_id = i_track_id;

    p_track->i_bytes_per_frame = p_es_info->audio.i_bytes_per_frame;
    p_track->i_bitspersample = p_es_info->audio.i_bitspersample;
    p_track->i_frame_length = p_es_info->audio.i_frame_length;
    p_track->i_profile = p_es_info->audio.i_profile;

//    MxSplitterUtils::GetCodecDesc(MX_ELEMENT_AUDIO, p_track->kind, p_track->codec_desc);
//    MxSplitterUtils::GetChannelDesc(p_track->channel_count, (p_track->has_lef) ? AOUT_CHAN_LFE : 0, p_track->channel_desc);
}



static bool ReadVodHeader(buffer_t p_buffer,
    int32_t i_size,
    vobd_info_t* out_vob_info,
    program_info_t* out_prog_info,
    chapter_info_t* out_chapter_info,
    chapter_t** out_chapters,
    int32_t* out_start_pos = NULL,
    int32_t* out_packet_len = NULL)
{
    uint8_t		idx;
    int32_t		len, i, offset;
    buffer_t	q;
    vobd_info_t	vob_info;
    program_info_t prog_info;
    MxBitStream	bs(p_buffer, i_size);
    
    if (out_vob_info)
        memset(out_vob_info, 0, sizeof(vobd_info_t));
    if (out_prog_info)
        memset(out_prog_info, 0, sizeof(program_info_t));
    if (out_chapter_info)
        memset(out_chapter_info, 0, sizeof(chapter_info_t));
    if (out_chapters)
        *out_chapters = NULL;
    if (out_start_pos)
        *out_start_pos = 0;

    if (bs.SearchCode32(0x000001BF))
    {
        offset = bs.GetPos();
        q = (buffer_t)bs.GetBuffer(offset);

        if (out_start_pos)
            *out_start_pos = offset;

        len = U16_AT(q + 4);
        idx = U8_AT(q + 6);

        if (offset + len + 6 > i_size)
            return false;

        if (out_packet_len)
            *out_packet_len = len + 6;

        if (idx == 0xFF)
        {
            vob_info = *(vobd_info_t*)(q + 7);
            prog_info = *(program_info_t*)(q + 7 + sizeof(vobd_info_t));

            vob_info.VOB_ID = U32_AT(&vob_info.VOB_ID);
            if (vob_info.VOB_ID != VOBD_ID)
                return false;

            vob_info.VERN = U16_AT(&vob_info.VERN);
            vob_info.AST_Ns = U16_AT(&vob_info.AST_Ns);
            vob_info.SPST_Ns = U16_AT(&vob_info.SPST_Ns);
            vob_info.PGCI_Ns = U16_AT(&vob_info.PGCI_Ns);

            if (out_vob_info)
                *out_vob_info = vob_info;
            if (out_prog_info)
                *out_prog_info = prog_info;

            // chapter info
            chapter_info_t	chapt_info;

            q = q + 7 + sizeof(vobd_info_t) + sizeof(program_info_t);
            chapt_info = *(chapter_info_t*)q;
            chapt_info.chapterbasenum = U16_AT(&chapt_info.chapterbasenum);
            chapt_info.chapter_ns = U16_AT(&chapt_info.chapter_ns);
            if (chapt_info.chapter_ns > 4095)
                chapt_info.chapter_ns = 0;

            if (out_chapter_info)
                *out_chapter_info = chapt_info;

            if (out_chapters)
            {
                chapter_t*	p_chapters = NULL;
                if (chapt_info.chapter_ns)
                    Malloc(p_chapters, sizeof(chapter_t) * chapt_info.chapter_ns, chapter_t*);

                for (i = 0; i < chapt_info.chapter_ns; i++)
                {
                    p_chapters[i].chapter_pos = U64_AT(&q[7 + 16 * i]);
                    p_chapters[i].chapter_pts = U64_AT(&q[7 + 8 + 16 * i]);
                }
                *out_chapters = p_chapters;
            }
        }
        else
            return false;
    }
    else
        return false;

    return true;
}


int32_t MxVODNetStream::ParseVODHeader(buffer_t inVODBuffer,
    int32_t inVODSize,
    mtime_t inFirstPts,
    stream_info_t* outInfo)
{
    int32_t i_err = mxErr_Generic;
    dvm_info_t dvm_info;
    chapter_t* p_chapter = NULL;

    if (ReadVodHeader(inVODBuffer, inVODSize, &dvm_info.vobdinfo, &dvm_info.programinfo, &dvm_info.chapterinfo, &p_chapter))
    {
        //outInfo->kind = (dvm_info.vobdinfo.V_ATR.mpeg_version == 0x02) ? mxCodecFileType_DVH : mxCodecFileType_DVM;

        /* chapters */
        outInfo->chapter_base = dvm_info.chapterinfo.chapterbasenum;
        outInfo->chapter_count = dvm_info.chapterinfo.chapter_ns;
        outInfo->p_chapters = p_chapter;

        //CSxUtils::modify_dvm_info(outInfo, &dvm_info);

        i_err = mxErr_None;
    }

    return i_err;
}

int32_t MxVODNetStream::ParseAttachData(buffer_t inBuffer, int32_t inSize, 
    vod_es_format_t*** outEsFormat, int32_t* outEsCount,
    stream_info_t* outInfo, bool* outProxyMode)
{
    if (inBuffer == NULL)
        return mxErr_Generic;

    buffer_t p_buffer = inBuffer;

    ParseFormatExtraData(inBuffer, inSize, outEsFormat, outEsCount, outInfo, outProxyMode, &p_buffer);

    // Mark, 'ATCH'
    if (GetDWBE(p_buffer) != 'ATCH')
        return mxErr_Generic;

    p_buffer += 4;

    // total size
    int32_t i_total_size = GetDWBE(p_buffer); p_buffer += 4;
    if (inSize < i_total_size)
        return mxErr_Generic;

    // attachment count
    int32_t i_attach_count = GetWBE(p_buffer); p_buffer += 2;
    attachment_info_t* p_attachments = NULL;

    if (i_attach_count)
    {
        Malloc(p_attachments, sizeof(attachment_info_t) * i_attach_count, attachment_info_t*);

        for (int32_t i = 0; i < i_attach_count; i++)
        {
            attachment_info_t* p_attach = &p_attachments[i];
            buffer_t p_attach_buf = p_buffer;

            // attachment size
            int32_t i_attach_size = GetDWBE(p_attach_buf); p_attach_buf += 4;

            // attachment info
            p_attach->file_name = GetString16(p_attach_buf);
            p_attach->mime_type = GetString16(p_attach_buf);

            p_attach->data_size = GetDWBE(p_attach_buf); p_attach_buf += 4;
            if (p_attach->data_size)
            {
                p_attach->p_data = (buffer_t)malloc(p_attach->data_size);
                memcpy(p_attach->p_data, p_attach_buf, p_attach->data_size);

                p_attach_buf += p_attach->data_size;
            }

            p_buffer += i_attach_size;
        }
    }

    outInfo->attachment_count = i_attach_count;
    outInfo->attachments = p_attachments;

    return mxErr_None;
}

int MxVODNetStream::ParseFormatExtraData(char* inBuffer, int32_t inSize, vod_es_format_t*** outEsFormat,
    int32_t* outEsCount, stream_info_t* outInfo, bool* outProxyMode, char** outNextBuffer)
{
    if (inBuffer == NULL)
        return mxErr_Generic;

    buffer_t p_buffer = inBuffer;
    buffer_t p_buffer_end = inBuffer + inSize;

    // Mark, 'FMT2'
    if (GetDWBE(p_buffer) != 'FMT2')
        return mxErr_Generic;
    p_buffer += 4;

    CHECK_FIRST_GROUP(FMT2_info, p_buffer, p_buffer_end);

    if (outNextBuffer)
        *outNextBuffer = inBuffer + i_total_size_FMT2_info;
    // ----------------------------------------------------------------------------------------------------------------
    CHECK_GROUP(es_format_t_list, p_buffer, p_buffer_end);

    // fmt count
    int16_t i_es_count = READ_16(p_buffer);

    vod_es_format_t** pp_fmt;
    pp_fmt = (vod_es_format_t**)malloc(sizeof(vod_es_format_t*) * i_es_count);

    int32_t i;
    for (i = 0; i < i_es_count; i++)
    {
        vod_es_format_t* p_fmt;
        Malloc(p_fmt, sizeof(vod_es_format_t), vod_es_format_t*);
        pp_fmt[i] = p_fmt;

        CHECK_GROUP(vod_es_format_t, p_buffer, p_buffer_end);

        // fmt info
        p_fmt->i_cat = READ_32(p_buffer);
        p_fmt->i_codec = READ_32(p_buffer);
        p_fmt->i_original_fourcc = READ_32(p_buffer);

#ifdef _MX_BIG_ENDIAN_
        p_fmt->i_cat = GetDWBE(&p_fmt->i_cat);
        p_fmt->i_codec = GetDWBE(&p_fmt->i_codec);
        p_fmt->i_original_fourcc = GetDWBE(&p_fmt->i_original_fourcc);
#endif

        p_fmt->i_id = READ_32(p_buffer);
        p_fmt->i_group = READ_32(p_buffer);
        p_fmt->i_priority = READ_32(p_buffer);

        p_fmt->i_bitrate = READ_32(p_buffer);
        p_fmt->i_profile = READ_32(p_buffer);
        p_fmt->i_level = READ_32(p_buffer);

        p_fmt->b_packetized = READ_32(p_buffer);
        p_fmt->i_extra = READ_32(p_buffer);
        if (p_fmt->i_extra)
        {
            p_fmt->p_extra = (buffer_t)malloc(p_fmt->i_extra);
            memcpy(p_fmt->p_extra, p_buffer, p_fmt->i_extra);

            p_buffer += p_fmt->i_extra;
        }

        p_fmt->psz_language = GetString16(p_buffer);
        p_fmt->psz_description = GetString16(p_buffer);

        if (p_fmt->i_cat == MX_ELEMENT_VIDEO)
        {
            p_fmt->video.i_chroma = READ_32(p_buffer);
#ifdef _MX_BIG_ENDIAN_
            p_fmt->video.i_chroma = GetDWBE(&p_fmt->video.i_chroma);
#endif
            p_fmt->video.i_aspect = READ_32(p_buffer);

            p_fmt->video.i_width = READ_32(p_buffer);
            p_fmt->video.i_height = READ_32(p_buffer);
            p_fmt->video.i_x_offset = READ_32(p_buffer);
            p_fmt->video.i_y_offset = READ_32(p_buffer);
            p_fmt->video.i_visible_width = READ_32(p_buffer);
            p_fmt->video.i_visible_height = READ_32(p_buffer);

            p_fmt->video.i_bits_per_pixel = READ_32(p_buffer);

            p_fmt->video.i_sar_num = READ_32(p_buffer);
            p_fmt->video.i_sar_den = READ_32(p_buffer);

            p_fmt->video.i_frame_rate = READ_32(p_buffer);
            p_fmt->video.i_frame_rate_base = READ_32(p_buffer);
            p_fmt->video.i_bitrate = READ_32(p_buffer);

            p_fmt->video.b_vbr = READ_32(p_buffer);
            p_fmt->video.b_progressive = READ_32(p_buffer);
            p_fmt->video.i_profile = READ_32(p_buffer);
            p_fmt->video.i_level = READ_32(p_buffer);

            p_fmt->video.i_rmask = READ_32(p_buffer);
            p_fmt->video.i_gmask = READ_32(p_buffer);
            p_fmt->video.i_bmask = READ_32(p_buffer);
            p_fmt->video.i_rrshift = READ_32(p_buffer);
            p_fmt->video.i_lrshift = READ_32(p_buffer);
            p_fmt->video.i_rgshift = READ_32(p_buffer);
            p_fmt->video.i_lgshift = READ_32(p_buffer);
            p_fmt->video.i_rbshift = READ_32(p_buffer);
            p_fmt->video.i_lbshift = READ_32(p_buffer);

            uint8_t b_palette = p_buffer[0]; p_buffer += 1;
            if (b_palette)
            {
                p_fmt->video.p_palette = (video_palette_t*)malloc(sizeof(video_palette_t));

                p_fmt->video.p_palette->i_entries = READ_32(p_buffer);
                memcpy(&p_fmt->video.p_palette->palette[0][0], p_buffer, 256 * 4); p_buffer += (256 * 4);
            }

            p_fmt->video.i_spu_x_offset = READ_32(p_buffer);
            p_fmt->video.i_spu_y_offset = READ_32(p_buffer);
            p_fmt->video.b_flipped = READ_32(p_buffer);
            p_fmt->video.orientation = READ_32(p_buffer);
            p_fmt->video.field_order = READ_32(p_buffer);
            p_fmt->video.seq_aspect_ratio = READ_32(p_buffer);
        }
        else if (p_fmt->i_cat == MX_ELEMENT_AUDIO)
        {
            p_fmt->audio.i_format = READ_32(p_buffer);
#ifdef _MX_BIG_ENDIAN_
            p_fmt->audio.i_format = GetDWBE(&p_fmt->audio.i_format);
#endif
            p_fmt->audio.i_rate = READ_32(p_buffer);

            p_fmt->audio.i_physical_channels = READ_32(p_buffer);
            p_fmt->audio.i_original_channels = READ_32(p_buffer);

            p_fmt->audio.i_bytes_per_frame = READ_32(p_buffer);
            p_fmt->audio.i_frame_length = READ_32(p_buffer);

            p_fmt->audio.i_bitspersample = READ_32(p_buffer);
            p_fmt->audio.i_blockalign = READ_32(p_buffer);
            p_fmt->audio.i_channels = READ_32(p_buffer);

            p_fmt->audio.b_vbr = READ_32(p_buffer);
            p_fmt->audio.i_profile = READ_32(p_buffer);
            p_fmt->audio.i_level = READ_32(p_buffer);

            p_fmt->audio.i_sample_per_packet = READ_32(p_buffer);
            p_fmt->audio.i_dwscale = READ_32(p_buffer);
            p_fmt->audio.i_dwrate = READ_32(p_buffer);
            p_fmt->audio.i_dwsamplesize = READ_32(p_buffer);

            if (p_fmt->audio.i_physical_channels == 0)
            {
                p_fmt->audio.i_original_channels =
                    p_fmt->audio.i_physical_channels = 0;// aout_GuessChannelLayout(p_fmt->audio.i_channels);
            }
        }
        else
        {
            p_fmt->subs.psz_encoding = GetString16(p_buffer);

            p_fmt->subs.i_x_origin = READ_32(p_buffer);
            p_fmt->subs.i_y_origin = READ_32(p_buffer);

            memcpy(&p_fmt->subs.spu.palette[0], p_buffer, sizeof(uint32_t) * 17); p_buffer += (sizeof(uint32_t) * 17);
            p_fmt->subs.spu.i_original_frame_width = READ_32(p_buffer);
            p_fmt->subs.spu.i_original_frame_height = READ_32(p_buffer);

            p_fmt->subs.dvb.i_id = READ_32(p_buffer);

            p_fmt->subs.teletext.i_magazine = READ_32(p_buffer);
            p_fmt->subs.teletext.i_page = READ_32(p_buffer);
        }

        SKIP_GROUP(vod_es_format_t, p_buffer);
    }

    SKIP_GROUP(es_format_t_list, p_buffer);

    *outEsCount = i_es_count;
    *outEsFormat = pp_fmt;

    if (outInfo)
    {
        // ----------------------------------------------------------------------------------------------------------------
        CHECK_GROUP(stream_info_t, p_buffer, p_buffer_end);

        //stream_info_header
        {
            CHECK_GROUP(stream_info_header, p_buffer, p_buffer_end);

            // kind
            outInfo->kind = READ_32(p_buffer);
            // playtime
            uint32_t play_time = READ_32(p_buffer);
            if (play_time)
                outInfo->playtime = play_time;
            // playtime_ms
            uint32_t play_time_ms = READ_32(p_buffer);
            if (play_time_ms)
                outInfo->playtime_ms = play_time_ms;

            if (play_time == 0 && play_time_ms == 0 && outProxyMode)
                *outProxyMode = true;

            // flags
            outInfo->i_flags = READ_32(p_buffer);

            SKIP_GROUP(stream_info_header, p_buffer);
        }
        // video_track_info_t
        {
            CHECK_GROUP(video_track_info_t, p_buffer, p_buffer_end);

            // first_dts
            outInfo->video_track.first_dts = READ_64(p_buffer);
            // first_pts
            outInfo->video_track.first_pts = READ_64(p_buffer);
            // first_pos
            outInfo->video_track.first_pos = READ_64(p_buffer);

            SKIP_GROUP(video_track_info_t, p_buffer);
        }
        // audio_track_info_t
        {
            CHECK_GROUP(audio_track_info_t_list, p_buffer, p_buffer_end);

            // def_audio_no
            outInfo->cur_audio_no = outInfo->def_audio_no = READ_32(p_buffer);
            // audio_track_count
            outInfo->audio_track_count = READ_32(p_buffer);

            for (i = 0; i < outInfo->audio_track_count; i++)
            {
                audio_track_info_t* p_atrack = &outInfo->audio_tracks[i];

                CHECK_GROUP(audio_track_info_t, p_buffer, p_buffer_end);

                // track_id
                p_atrack->track_id = READ_32(p_buffer);

                // "Audio" language
                uint8_t i_audio_flag = p_buffer[0]; p_buffer += 1;

                for (int32_t j = 0; j < i_es_count; j++)
                {
                    vod_es_format_t* p_fmt = pp_fmt[j];
                    if (p_fmt->i_cat == MX_ELEMENT_AUDIO && p_fmt->i_id == p_atrack->track_id)
                    {
                        SetAudioTrackInfo(p_atrack, p_fmt->i_codec, p_fmt, p_fmt->i_id);
                        p_atrack->i_extra = p_fmt->i_extra;
                        p_atrack->p_extra = p_fmt->p_extra;

                        if (i_audio_flag == 0)
                        {
                            GetLangFromCode(p_fmt->psz_language, p_atrack->language_kor, p_atrack->language_eng);
                        }
                        else
                        {
                            char audio_desc[64];
                            GetCodeToLanguge(audio_desc, "99", 0);
                            sprintf(p_atrack->language_kor, "%s%d", audio_desc, i + 1);
                            GetCodeToLanguge(audio_desc, "99", 1);
                            sprintf(p_atrack->language_eng, "%s%d", audio_desc, i + 1);
                        }

                        if (p_fmt->psz_description)
                            p_atrack->description = strdup(p_fmt->psz_description);
                        break;
                    }
                }

                SKIP_GROUP(audio_track_info_t, p_buffer);
            }

            SKIP_GROUP(audio_track_info_t_list, p_buffer);
        }
        // spu_track_info_t
        {
            CHECK_GROUP(spu_track_info_t_list, p_buffer, p_buffer_end);

            // def_spu_no
            outInfo->cur_spu_no = outInfo->def_spu_no = READ_32(p_buffer);
            // spu_track_count
            outInfo->spu_track_count = READ_32(p_buffer);

            for (i = 0; i < outInfo->spu_track_count; i++)
            {
                spu_track_info_t* p_strack = &outInfo->spu_tracks[i];

                CHECK_GROUP(spu_track_info_t, p_buffer, p_buffer_end);

                // track_id
                p_strack->track_id = READ_32(p_buffer);

                for (int32_t j = 0; j < i_es_count; j++)
                {
                    vod_es_format_t* p_fmt = pp_fmt[j];
                    if (p_fmt->i_cat == MX_ELEMENT_SPU && p_fmt->i_id == p_strack->track_id)
                    {
                        p_strack->i_extra = p_fmt->i_extra;
                        p_strack->p_extra = p_fmt->p_extra;
                        p_strack->kind = p_fmt->i_codec;
                        p_strack->sub_kind = 0;

                        GetLangFromCode(p_fmt->psz_language, p_strack->language_kor, p_strack->language_eng);

                        if (p_fmt->psz_description)
                            p_strack->description = strdup(p_fmt->psz_description);

                        p_strack->i_original_frame_width = p_fmt->subs.spu.i_original_frame_width;
                        p_strack->i_original_frame_height = p_fmt->subs.spu.i_original_frame_height;

                        if (p_fmt->subs.spu.palette[0] == SPU_PALETTE_DEFINED && p_fmt->i_codec == MX_CODEC_SPU)
                        {
                            p_strack->p_palette = &p_fmt->subs.spu.palette[1];
                            /*if(b_palette_flag == false)
                            {
                            memcpy(&outInfo->palette[0], &p_fmt->subs.spu.palette[1], sizeof(uint32_t) * MX_PALETTE_MAX_NUM);
                            b_palette_flag = true;
                            }*/
                        }
                        break;
                    }
                }

                SKIP_GROUP(spu_track_info_t, p_buffer);
            }

            SKIP_GROUP(spu_track_info_t_list, p_buffer);
        }

        SKIP_GROUP(stream_info_t, p_buffer);
        // ----------------------------------------------------------------------------------------------------------------

        CHECK_GROUP(chapter_t_list, p_buffer, p_buffer_end);

        // chapter pos flag
        uint8_t i_pos_flag = p_buffer[0]; p_buffer += 1;

        // chapter base num, count
        outInfo->chapter_base = READ_16(p_buffer);
        int32_t i_chapt_count = READ_16(p_buffer);
        chapter_t* p_chapters = NULL;

        if (i_chapt_count)
        {
            Malloc(p_chapters, sizeof(chapter_t) * i_chapt_count, chapter_t*);

            for (i = 0; i < i_chapt_count; i++)
            {
                chapter_t* p_chapt = &p_chapters[i];

                CHECK_GROUP(chapter_t, p_buffer, p_buffer_end);

                // chapter info
                p_chapt->chapter_sec = READ_64(p_buffer);
                p_chapt->chapter_pts = READ_64(p_buffer);
                p_chapt->chapter_pos = -1;
                if (i_pos_flag)
                {
                    p_chapt->chapter_pos = READ_64(p_buffer);
                }

                // desc count
                uint8_t i_desc_count = p_buffer[0]; p_buffer += 1;
                if (i_desc_count)
                {
                    Malloc(p_chapt->p_descs, sizeof(chapter_desc_t) * i_desc_count, chapter_desc_t*);
                    for (int32_t j = 0; j < i_desc_count; j++)
                    {
                        chapter_desc_t* p_desc = &p_chapt->p_descs[j];
                        char lang_code[4] = { 0, 0, 0, 0 };

                        CHECK_GROUP(chapter_desc_t, p_buffer, p_buffer_end);

                        // language
                        memcpy(lang_code, p_buffer, 3); p_buffer += 3;
                        GetLangFromCode(lang_code, p_desc->language_kor, p_desc->language_eng);

                        // desc
                        p_desc->description = GetString16(p_buffer);

                        SKIP_GROUP(chapter_desc_t, p_buffer);

                        p_chapt->desc_count++;
                    }
                }

                SKIP_GROUP(chapter_t, p_buffer);
            }
        }

        outInfo->chapter_count = i_chapt_count;
        outInfo->p_chapters = p_chapters;
    }
    return mxErr_None;
}

//////////

CVODNetReadThread::CVODNetReadThread(MxVODNetStream* inStream) 
{
	//SetLogger(inLog);
	m_pStream = inStream;
	m_IsPause = false;
}

CVODNetReadThread::~CVODNetReadThread()
{
	StopThread();
}

void *ThreadRun(void *arg)
{
    CVODNetReadThread *pObj = (CVODNetReadThread *)arg;

    pObj->Run();

    return 0;
}
void CVODNetReadThread::StartThread()
{
    m_bRun = true;
    vlc_clone(&m_thread, ThreadRun, this, "access_vod_read", VLC_THREAD_PRIORITY_INPUT);
}

void CVODNetReadThread::StopThread()
{
    if (m_bRun)
    {
        m_bRun = false;
        vlc_join(m_thread, 0);
    }
}

void CVODNetReadThread::Run()
{
	short theResult = 0;
	long theResultSize = 0;
    long theSize = 64 * 1024;//DATA_PACK_SIZE*2;
	unsigned short theSeqNo;
	unsigned int i_rtptimestemp = 0;
	char logStr[256];
    unsigned char* pBuf = new unsigned char[theSize];
	unsigned short rtp_ext_flag = 0;
	unsigned char* rtp_ext_data = new unsigned char[64];
	long rtp_ext_size = 0;

	VODINSTANCE* pVODInstance = (VODINSTANCE*)m_pStream->GetVODInstance();
	
	RecvStart();
    while (m_bRun)
	{
		if (m_pStream->IsConncect() == false)
		{
			msleep(MX_TIME_1_MILISEC*10);
			continue;
		}

		rtp_ext_size = 64;
        theResult = VODNet_GetTSPacketData(pVODInstance,
            pBuf, theSize, theResultSize, theSeqNo, i_rtptimestemp,
            rtp_ext_flag, rtp_ext_data, rtp_ext_size);
		
		if (theResult != 0)
		{
			if (theResult != -1)
			{
				sprintf(logStr, "Error(%d) in VODNETLIB::VODNet_GetTSPacketData.", theResult);
				m_pStream->LogProc(logStr);
			}
			if (theResult == -100)
			{
				m_pStream->SetTcpStreamError();
				break;
			}
			
			msleep(MX_TIME_1_MILISEC);
			continue;
		}
			
        if (theResultSize)
            m_pStream->SendDataSync(pBuf, theResultSize, i_rtptimestemp, rtp_ext_flag, rtp_ext_data, rtp_ext_size);
	}

	RecvStop();
    
    delete[] pBuf;
	delete[] rtp_ext_data;
	
	m_pStream->LogProc("**** CVODNetReadThread End ****\n");
}
	
void CVODNetReadThread::DoPause()
{
	m_IsPause = true;
}

void CVODNetReadThread::DoContinue()
{
	m_IsPause = false;
}
			
void CVODNetReadThread::RecvStart()
{

}

void CVODNetReadThread::RecvStop()
{

}

//------------------------------------------------------
//*MxGetVODServerTime : Get host computer's time
//------------------------------------------------------
int32_t MxGetVODServerTime()
{
	time_t	dbTime;
	time(&dbTime);
	return (int32_t)dbTime;
}

//5.29 : 331