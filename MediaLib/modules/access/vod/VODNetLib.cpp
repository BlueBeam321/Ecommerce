
#ifdef _WIN32
#include <process.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "VODNetLib.h"
#include "CUsageEnvironment.h"
#include "CRTSPClient.h"
#include "CTsRtpSource.h"

#include "ErrorCode.h"

char* gethostbyname_k(char* hostname);
//namespace L_LVideo {
//    bool lvIsEthernetIp(char* inIpAddr);
//}


#ifdef _WIN32
    typedef unsigned (__stdcall *PTHREAD_START)(void*);
#else
#include <pthread.h>
    typedef void* (*PTHREAD_START)(void*);
#endif

    //const unsigned cMP2TFrameSize = TRANSPORT_PACKET_SIZE * TRANSPORT_PACKETS_PER_NETWORK_PACKET;

    class CClientInstance
    {
    public:
        CClientInstance(unsigned inSSRC, HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, char* inIpAddrStr, void* inHookParams, long inB_Stb, bool inAuthStreamMode = 0);
        virtual ~CClientInstance();

        enum { eMaxInComingBufSize = 1024 * 1024 * 5 };

        operator bool() { return m_ObjectOk; }

        double GetJitter(unsigned inServerSSRC);
        short Dequeue(VODNet_Event* outEvent, unsigned long inTimeoutMiliSecs);

        short GetTSPacketData(unsigned char* outTo, long inMaxSize, long& outSize,
            unsigned short& outSeqNo, uint32_t &outRtpTimeStemp,
            unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize);

        CUsageEnvironment* m_UsageEnvironment;
        CRTSPClient* m_RTSPClient;
        CMediaSession* m_MediaSession;
        CMediaSubsession* m_MediaSubSession;
        CTsRtpSource* m_TsRtpSource;
        long m_RtpStreamMode;    //0: RTP_AUTO, 1:RTP_UDP, 2:RTP_TCP
        bool m_AuthStreamMode;

        unsigned long m_SSRC;
        char* m_SDPLines;

        long m_ObjectOk;
        long m_CmdState;
        char m_IpAddrStr[64];
    };

    CClientInstance::CClientInstance(unsigned inSSRC, HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, char* inIpAddrStr, void* inHookParams, long inB_stb, bool inAuthStreamMode)
    {
        m_ObjectOk = true;

        strcpy(m_IpAddrStr, inIpAddrStr);

        m_CmdState = kVODNet_CmdState_No;

        m_SSRC = inSSRC;
        m_SDPLines = NULL;
        m_MediaSession = NULL;
        m_MediaSubSession = NULL;
        m_TsRtpSource = NULL;
        m_RtpStreamMode = 1; //RTP_UDP
        m_AuthStreamMode = inAuthStreamMode;
        m_UsageEnvironment = CUsageEnvironment::Create(inHookGetVODErrorFunc, inStopingFunc, inHookParams);
        m_RTSPClient = CRTSPClient::createNew(m_UsageEnvironment, inIpAddrStr, "Sobaeksu Client", inB_stb, m_AuthStreamMode);

        strcpy(m_IpAddrStr, inIpAddrStr);
    }

    CClientInstance::~CClientInstance()
    {
        if (m_SDPLines != NULL)
            delete[] m_SDPLines;

        if (m_MediaSession)
        {
            delete m_MediaSession;
            m_MediaSession = NULL;
            m_TsRtpSource = NULL;
        }

        if (m_RTSPClient)
            delete m_RTSPClient;

        if (m_UsageEnvironment)
            delete m_UsageEnvironment;
    }

    short CClientInstance::GetTSPacketData(unsigned char* outTo, long inMaxSize, long& outSize,
        unsigned short& outSeqNo, uint32_t &outRtpTimeStemp,
        unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize)
    {
        short	err = 0;

        outSize = 0;
        if (m_TsRtpSource)
        {
            outSize = m_TsRtpSource->getNextFrame(outTo, inMaxSize, outSeqNo, outRtpTimeStemp, outRtpExtflag, outRtpExtData, ioRtpExtDataSize);
        }

        if (m_TsRtpSource)
        {
            if (m_TsRtpSource->IsTcpSocketError())
                return -100;
        }

        if (outSize <= 0)
            err = -1;
        return err;
    }

    short CClientInstance::Dequeue(VODNet_Event* outEvent, unsigned long inTimeoutMiliSecs)
    {
        if (m_UsageEnvironment == NULL) return -1;

        return m_UsageEnvironment->DequeueEvent(outEvent, inTimeoutMiliSecs);
    }

    double CClientInstance::GetJitter(unsigned inServerSSRC)
    {
        double theJitter = 1000000.0;
        CRTPSource* theRTPSource = NULL;

        if (m_MediaSubSession == NULL)
            return theJitter;

        if (m_TsRtpSource)
        {
            theJitter = m_TsRtpSource->receptionStatsDB().GetJitter(inServerSSRC);
        }

        return theJitter;
    }

    //For Multimedia Client

    VODINSTANCE VODNet_CreateClient(unsigned long inSSRC, HookGetVODError inHookGetVODErrorFunc, HookIsStoping inStopingFunc, void* inHookParams, long inB_Stb, bool  inAuthStreamMode)
    {
        CClientInstance* theClient = NULL;
        char inIpAddrStr[80];

        if (VODNet_GetClientNetworkIP(inIpAddrStr) == 0)
            return NULL;

        theClient = new CClientInstance(inSSRC, inHookGetVODErrorFunc, inStopingFunc, inIpAddrStr, inHookParams, inB_Stb, inAuthStreamMode);
        if (bool(*theClient) == false)
            return NULL;

        return (VODINSTANCE)(theClient);
    }

    VODNet_Error VODNet_SetRTPStreamMode(VODINSTANCE inVODInstance, long inRtpStreamMode) //add ver 2.60
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        theClient->m_RtpStreamMode = inRtpStreamMode;

        return kVODNet_Error_NoErr;
    }

    long VODNet_GetRTPStreamMode(VODINSTANCE inVODInstance)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return 1; //UDP_RTP

        return theClient->m_RtpStreamMode;
    }

    VODNet_Error VODNet_SendDescribe(VODINSTANCE inVODInstance,
        char const* inURL/* For ex:"rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t*/,
        short inTitleIndex, long inRedirect,
        unsigned short& outVideoWidth, unsigned short& outVideoHeight, float& outVideoFPS, unsigned& outVideoFileType, unsigned& outVideoStreamType, char **outDVDInfo,
        unsigned short& outPort, char* outServerAddr,
        unsigned short* outThumbPort, char* outThumbServerAddr, char* outThumbIdStr, int inNetSpeed, int* outAttachmentSize)
    {
        outVideoWidth = 0;
        outVideoHeight = 0;
        outVideoFPS = 0;
        outVideoFileType = 0;

        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        theClient->m_UsageEnvironment->m_bStreamStopFlag = 0;	//gVOD_GlobalStreamStopFlag = 0;

        theClient->m_SDPLines = theClient->m_RTSPClient->describeURL(inURL, inTitleIndex, inRedirect, outPort, outServerAddr, outThumbPort, outThumbServerAddr, outThumbIdStr, inNetSpeed);

        unsigned theErr = theClient->m_RTSPClient->describeStatus();
        if (theErr == 1)
            return kVODNet_Error_ConectFailed;
        if (theErr == 2)
            return kVODNet_Error_UnavailableStream;
        if (theErr == 3)
            return kVODNet_Error_InvalidClientAuthDataFmt;
        if (theErr == 4)
            return kVODNet_Error_NotAcceptableClient;
        if (theErr == 5)
            return kVODNet_Error_NoMediaData;
        if (theErr == 6)
            return kVODNet_Error_OverUsers;
        if (theErr == 7)
            return kVODNet_Error_InternalServerError;
        if (theErr == 100) // Redirect
            return kVODNet_Error_RedirectError;

        if (theClient->m_SDPLines != NULL)
        {
            if (theClient->m_RtpStreamMode == eRTPSTREAM_AUTO)
            {
                //if (L_LVideo::lvIsEthernetIp(theClient->m_RTSPClient->m_IpAddrString) == true)
                //{
                    theClient->m_RtpStreamMode = eRTPSTREAM_UDP;
                //}
                //else
                //{
                //    theClient->m_RtpStreamMode = eRTPSTREAM_TCP;
                //}
            }

            theClient->m_MediaSession = CMediaSession::createNew(theClient->m_UsageEnvironment,
                theClient->m_SDPLines,
                (vodRTPStreamMode)theClient->m_RtpStreamMode,
                theClient->m_AuthStreamMode);
        }
        else
            return kVODNet_Error_InvalidSdpLines;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        CMediaSubsessionIterator theIter(*(theClient->m_MediaSession));
        CMediaSubsession *theSubSession;
        while ((theSubSession = theIter.next()) != NULL)
        {
            if (strcmp(theSubSession->mediumName(), "video") != 0)
                continue;
            else
                break;
        }

        if (theSubSession == NULL)
        {
            delete theClient->m_MediaSession;
            theClient->m_MediaSession = NULL;
            return kVODNet_Error_MediaSubsessionNotValid;
        }

        theClient->m_MediaSubSession = theSubSession;
        bool theResult = theSubSession->initiate(theClient->m_SSRC, -1, theClient->m_RTSPClient);

        if (!theResult)
        {
            delete theClient->m_MediaSession;
            theClient->m_MediaSession = NULL;
            return kVODNet_Error_MediaSubsessionNotInit;
        }

        theClient->m_TsRtpSource = (CTsRtpSource*)theSubSession->rtpSource();

        theClient->m_CmdState = kVODNet_CmdState_Describe;

        outVideoWidth = theSubSession->videoWidth();
        outVideoHeight = theSubSession->videoHeight();
        outVideoFPS = theSubSession->videoFPS();
        outVideoFileType = theSubSession->videoFileType();
        outVideoStreamType = theSubSession->videoStreamType();
        if (outDVDInfo)
            *outDVDInfo = theSubSession->DVDInfo();
        if (outAttachmentSize)
            *outAttachmentSize = theSubSession->GetAttachmentSize();

        theSubSession->GetThumbServerInfo(outThumbPort, outThumbServerAddr, outThumbIdStr);

        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_AttachmentReq(VODINSTANCE inVODInstance, long inDBTime, short inTitleIndex, unsigned char* pAttachmentBuffer, int iAttachmentbuffersize)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_TsRtpSource && theClient->m_RtpStreamMode == eRTPSTREAM_TCP)
            theClient->m_TsRtpSource->DoPauseProcess(true);

        if (theClient->m_RTSPClient->attachmentreqMediaSubsession(*(theClient->m_MediaSubSession), inTitleIndex, pAttachmentBuffer, iAttachmentbuffersize) == false)
        {
            long theErrCode;

            if (theClient->m_TsRtpSource && theClient->m_RtpStreamMode == eRTPSTREAM_TCP)
                theClient->m_TsRtpSource->DoPauseProcess(false);

            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        if (theClient->m_TsRtpSource && theClient->m_RtpStreamMode == eRTPSTREAM_TCP)
            theClient->m_TsRtpSource->DoPauseProcess(false);

        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_SendPlay(VODINSTANCE inVODInstance, long inDBTime, float& outStart, float& outEnd, float inStart, short inTitleIndex, short inChapterIndex)
    {
        outStart = outEnd = 0.0;
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        theClient->m_UsageEnvironment->m_bStreamStopFlag = 0;	//gVOD_GlobalStreamStopFlag = 0;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_RTSPClient->setupMediaSubsession(*(theClient->m_MediaSubSession), outStart, outEnd, inStart, inTitleIndex, inChapterIndex) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        theClient->m_CmdState = kVODNet_CmdState_Play;

        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_SendPausePlay(VODINSTANCE inVODInstance, long inDBTime, float inStart, float inEnd, short inTitleIndex, short inChapterIndex, int64_t	inFilepos, int64_t inStartCr)
    {
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) 
            return kVODNet_Error_InvalidVODInstance;

        theClient->m_UsageEnvironment->m_bStreamStopFlag = 0;	//gVOD_GlobalStreamStopFlag = 0;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_RTSPClient->playMediaSubsession(*(theClient->m_MediaSubSession), inStart, inEnd, inTitleIndex, inChapterIndex, inFilepos, inStartCr) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        theClient->m_CmdState = kVODNet_CmdState_PausePlay;

        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_SendPause(VODINSTANCE inVODInstance, long inDBTime)
    {
        VODNet_Error theErr = kVODNet_Error_NoErr;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_RTSPClient->pauseMediaSubsession(*(theClient->m_MediaSubSession)) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        theClient->m_CmdState = kVODNet_CmdState_Pause;

        return kVODNet_Error_NoErr;
    }

    void VODNet_StreamStop(VODINSTANCE inVODInstance)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return;

        theClient->m_UsageEnvironment->m_bStreamStopFlag = 1;	//gVOD_GlobalStreamStopFlag = 1;
    }
    /*
    VODNet_Error
    VODNet_SendIamAlive(VODINSTANCE inVODInstance)
    {
    VODNet_Error theErr = kVODNet_Error_NoErr;
    CClientInstance* theClient;
    theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
    if(theClient == NULL) return kVODNet_Error_InvalidVODInstance;

    if(theClient->m_RTSPClient->getparameterMediaSubsession(*(theClient->m_MediaSubSession)) == false)
    {
    long theErrCode;
    VODNet_GetLastError(inVODInstance,theErrCode);
    if(theErrCode == 11000703)
    return kVODNet_Error_InvalidClientAuthDataFmt;
    if(theErrCode == 11000704)
    return kVODNet_Error_NotAcceptableClient;

    return kVODNet_Error_GenericErr;
    }

    return kVODNet_Error_NoErr;
    }
    */
    VODNet_Error VODNet_SendGetTitleParam(VODINSTANCE inVODInstance,
        long inDBTime,
        short inTitleIndex,
        unsigned short& outVideoWidth, unsigned short& outVideoHeight, float& outVideoFPS, unsigned& outVideoFileType, float& outStart, float& outEnd,
        char **outDVDInfo,
        int* outAttachmentSize)
    {
        VODNet_Error theErr = kVODNet_Error_NoErr;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_SDPLines != NULL)
            delete[] theClient->m_SDPLines;
        theClient->m_SDPLines = theClient->m_RTSPClient->getparameterMediaSubsession(*(theClient->m_MediaSubSession), inTitleIndex);
        if (theClient->m_SDPLines == NULL)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;
            if (theErrCode == 11000901)
                return kVODNet_Error_NoMediaData;

            return kVODNet_Error_GenericErr;
        }

        CMediaSubsession *theSubSession;
        theSubSession = theClient->m_MediaSubSession;
        if (theSubSession == NULL)
            return kVODNet_Error_MediaSubsessionNotValid;

        if (theSubSession->setTitleSDPInfo(theClient->m_SDPLines) == false)
            return kVODNet_Error_ConectFailed;

        outVideoWidth = theSubSession->videoWidth();
        outVideoHeight = theSubSession->videoHeight();
        outVideoFPS = theSubSession->videoFPS();
        outVideoFileType = theSubSession->videoFileType();
        outStart = theSubSession->videoStart();
        outEnd = theSubSession->videoEnd();
        if (outDVDInfo)
            *outDVDInfo = theSubSession->DVDInfo();
        if (outAttachmentSize)
            *outAttachmentSize = theSubSession->GetAttachmentSize();
        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_GetTSPacketData(VODINSTANCE inVODInstance,
        unsigned char* outTo, long inMaxSize, long& outSize,
        unsigned short& outSeqNo, unsigned int &outRtpTimeStemp,
        unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize)
    {
        VODNet_Error theResult = kVODNet_Error_NoErr;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (inMaxSize < 1316)
            return kVODNet_Error_InvalidBufSize;

        theResult = theClient->GetTSPacketData(outTo, inMaxSize, outSize, outSeqNo, outRtpTimeStemp, outRtpExtflag, outRtpExtData, ioRtpExtDataSize);
        return theResult;
    }

    VODNet_Error VODNet_SendTeardown(VODINSTANCE inVODInstance, long inDBTime)
    {
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        theClient->m_UsageEnvironment->m_bStreamStopFlag = 1; //gVOD_GlobalStreamStopFlag = 1;

        if (theClient->m_RTSPClient->teardownMediaSubsession(*(theClient->m_MediaSubSession)) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }
        theClient->m_CmdState = kVODNet_CmdState_TearDown;
        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_SendRecordParam(VODINSTANCE inVODInstance, long& time)
    {
        VODNet_Error theErr = kVODNet_Error_NoErr;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL)
            return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSubSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        bool	rslt;
        rslt = theClient->m_RTSPClient->RecordMediaSubsession(*(theClient->m_MediaSubSession), time);

        if (rslt == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;
            if (theErrCode == 11000901)
                return kVODNet_Error_NoMediaData;

            return kVODNet_Error_GenericErr;
        }

        return kVODNet_Error_NoErr;
    }

    void VODNet_DisposeClient(VODINSTANCE inVODInstance)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return;

        delete theClient;
    }

    double VODNet_GetJitter(VODINSTANCE inVODInstance, unsigned inServerSSRC)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL)
            return 1000000.0;
        return theClient->GetJitter(inServerSSRC);
    }

    VODNet_Error VODNet_ClearDownLoadBuffer(VODINSTANCE inVODInstance)
    {
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_TsRtpSource != NULL)
            theClient->m_TsRtpSource->ResetReorderingBuffer();

        return kVODNet_Error_NoErr;
    }

    char* VODNet_GetLastError(VODINSTANCE inVODInstance, long& outErrCode)
    {
        char* theErrString = NULL;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return NULL;
        theErrString = theClient->m_UsageEnvironment->GetLastError(outErrCode);
        return theErrString;
    }

    VODNet_Error VODNet_GetEvent(VODINSTANCE inVODInstance, VODNet_Event* outVODNetEvent, unsigned long inTimeOutMiliSecs)
    {
        VODNet_Error theResult = kVODNet_Error_GenericErr;
        CClientInstance* theClient;
        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        theResult = theClient->Dequeue(outVODNetEvent, inTimeOutMiliSecs);
        return theResult;
    }

    long g_getNetworkIP = 0;
    char g_MyNetworkIP[256];
    char g_MyNethostname[256];

    long VODNet_GetClientNetworkIP(char *outAddr)
    {
        struct	sockaddr_in client_self;
        char	szBuff[256];
        char	*str_ipaddr;
        short	err = 0;
        long	ret = 0;

        if (g_getNetworkIP)
        {
            if (outAddr)
                strcpy(outAddr, g_MyNetworkIP);
            ret = inet_addr(g_MyNetworkIP);
        }
        else
        {
            ret = initializeWinsockIfNecessary();

            memset(g_MyNetworkIP, 0, sizeof(g_MyNetworkIP));
            memset(g_MyNethostname, 0, sizeof(g_MyNethostname));

            memset(szBuff, 0, sizeof(szBuff));
            err = gethostname(szBuff, sizeof(szBuff));

            strcpy(g_MyNethostname, szBuff);

#if 0
            str_ipaddr = gethostbyname_k(szBuff);
#else
            str_ipaddr = "127.0.0.1";
#endif
            if (str_ipaddr != NULL)
            {
                ret = inet_addr(str_ipaddr);
                if (outAddr)
                    strcpy(outAddr, str_ipaddr);

                if (outAddr)
                    strcpy(outAddr, str_ipaddr);
                strcpy(g_MyNetworkIP, str_ipaddr);

                g_getNetworkIP = 1;
            }

        }
        return ret;
    }

    void VODNet_Initialize()
    {
        char netIP[256];
        VODNet_GetClientNetworkIP(netIP);
        CMediaSubsession::Initialize();
    }

    void VODNet_Finalize()
    {
        CMediaSubsession::Finalize();
    }
    
    VODNet_Error VODNet_DownloadStart(VODINSTANCE inVODInstance, long inDBTime, char* inBuffer, long inBufferSize)
    {
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_TsRtpSource && theClient->m_RtpStreamMode == eRTPSTREAM_TCP)
            theClient->m_TsRtpSource->DoPauseProcess(true);

        if (theClient->m_RTSPClient->downloadstartMediaSubsession(*(theClient->m_MediaSubSession), inBuffer, inBufferSize) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        return kVODNet_Error_NoErr;

    }

    VODNet_Error VODNet_DownloadEnd(VODINSTANCE inVODInstance, long inDBTime)
    {
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_RTSPClient->downloadstopMediaSubsession(*(theClient->m_MediaSubSession)) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        if (theClient->m_TsRtpSource && theClient->m_RtpStreamMode == eRTPSTREAM_TCP)
            theClient->m_TsRtpSource->DoPauseProcess(false);

        return kVODNet_Error_NoErr;
    }

    VODNet_Error VODNet_DownloadData(VODINSTANCE inVODInstance, long inDBTime, char* outBuffer, long& outBufferSize)
    {
        CClientInstance* theClient;

        theClient = dynamic_cast<CClientInstance*>((CClientInstance*)inVODInstance);
        if (theClient == NULL) return kVODNet_Error_InvalidVODInstance;

        if (theClient->m_MediaSession == NULL)
            return kVODNet_Error_MediaSessionObjectNotCreated;

        if (theClient->m_RTSPClient->downloaddataMediaSubsession(*(theClient->m_MediaSubSession), outBuffer, outBufferSize) == false)
        {
            long theErrCode;
            VODNet_GetLastError(inVODInstance, theErrCode);
            if (theErrCode == 11000703)
                return kVODNet_Error_InvalidClientAuthDataFmt;
            if (theErrCode == 11000704)
                return kVODNet_Error_NotAcceptableClient;

            return kVODNet_Error_GenericErr;
        }

        return kVODNet_Error_NoErr;
    }
