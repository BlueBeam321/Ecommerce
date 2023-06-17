
#ifndef __VODNETLIBC_H__
#define __VODNETLIBC_H__
#include <stdint.h>


    typedef void*	VODINSTANCE;
    typedef short	VODNet_Error;

    typedef void(*HookGetVODError)(char* inProgramID, long inErrCode, const char* inErrStr, long inErrStrSize, void* inParams);
    typedef bool(*HookIsStoping)(void* inParam);

    enum
    {
        kVODNet_CmdState_No = -1,
        kVODNet_CmdState_Describe = 0,
        kVODNet_CmdState_Play = 1,
        kVODNet_CmdState_Pause = 2,
        kVODNet_CmdState_PausePlay = 3,
        kVODNet_CmdState_TearDown = 4
    };

    enum
    {
        kVODNet_Error_NoErr = 0,
        kVODNet_Error_GenericErr = -1,
        kVODNet_Error_ConectFailed = -2,
        kVODNet_Error_UnavailableStream = -3,
        kVODNet_Error_MediaSubsessionNotValid = -4,
        kVODNet_Error_InvalidClientAuthDataFmt = -5,
        kVODNet_Error_NotAcceptableClient = -6,
        kVODNet_Error_InvalidBufSize = -7,
        kVODNet_Error_InvalidVODInstance = -8,
        kVODNet_Error_InvalidChannelID = -9,
        kVODNet_Error_InvalidSdpLines = -10,
        kVODNet_Error_MediaSubsessionNotInit = -11,
        kVODNet_Error_MediaSessionObjectNotCreated = -12,
        kVODNet_Error_OverUsers = -13,
        kVODNet_Error_NoMediaData = -14,
        kVODNet_Error_InternalServerError = -15,
        kVODNet_Error_ClientQueueOperationError = -16,
        kVODNet_Error_RedirectError = -17,	// Redirect

        kVODNet_Error_CreateSocket = -100,
        kVODNet_Error_OptionSocket = -101,
        kVODNet_Error_ConnectSocket = -102,
        kVODNet_Error_SendSocket = -103,
        kVODNet_Error_RecvSocket = -104,
        kVODNet_Error_Already_Used = -105,
        kVODNet_Error_Select = -106,
        kVODNet_Error_Select_WAIT_OUT = -107,
        kVODNet_Error_Recv_WAIT_OUT = -108,
        kVODNet_Error_Host_UserBreak = -109,
        kVODNet_Error_Send_WAIT_OUT = -110,
    };


    ////// For Event /////////////////////////
    enum
    {
        kVODNet_Event_LineFailed = 1,
        kVODNet_Event_NoStreamFromServer = 2,
        kVODNet_Event_NoSyncByteStream = 3,
        kVODNet_Event_StartStreaming = 4,
        kVODNet_Event_ServerBye = 5,
        kVODNet_Event_ImustDie = 6
    };

    // kVODNet_Event_ServerBye's Params
    enum
    {
        kVODNet_ByeReason_TearDown = 0,
        kVODNet_ByeReason_LineFailed = 1,
        kVODNet_ByeReason_QueueOperationErr = 8,
        kVODNet_ByeReason_NotRtspCmd = 9,
        kVODNet_ByeReason_SessionIsNotActive = 10,
        kVODNet_ByeReason_ServerDown = 12
    };

    typedef struct tagVODNet_Event
    {
        short	eventKind;
        short	eventTime;
        long	eventParams;
    } VODNet_Event;

    //For Multimedia Client
    void VODNet_Initialize();
    void VODNet_Finalize();


    VODINSTANCE VODNet_CreateClient(unsigned long inSSRC,
        HookGetVODError inHookGetVODErrorFunc,
        HookIsStoping	inStopingFunc,
        void* inHookParams = 0,
        long	  inB_Stb = false,
        bool       inAuthStreamMode = 0);
    VODNet_Error VODNet_SendDescribe(VODINSTANCE inVODInstance,
        const char* inURL/*For ex:"rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t"*/,
        short inTitleIndex, long inRedirect,
        unsigned short& outVideoWidth, unsigned short& outVideoHeight, float& outVideoFPS, unsigned& outVideoFileType, unsigned& outVideoStreamType, char **outDVDInfo,
        unsigned short& outPort, char* outServerAddr,
        unsigned short* outThumbPort = 0, char* outThumbServerAddr = 0, char* outThumbIdStr = 0, int inNetSpeed = 0, int* outAttachmentSize = 0);

    VODNet_Error VODNet_SetRTPStreamMode(VODINSTANCE inVODInstance, long inRtpStreamMode); //add ver 2.60

    long VODNet_GetRTPStreamMode(VODINSTANCE inVODInstance);

    VODNet_Error VODNet_SendPlay(VODINSTANCE inVODInstance, long inDBTime, float& outStart, float& outEnd, float inStart = 0.0, short inTitleIndex = -1, short inChapterIndex = -1);
    VODNet_Error VODNet_SendPausePlay(VODINSTANCE inVODInstance, long inDBTime, float inStart, float inEnd = -1.0, short inTitleIndex = -1, short inChapterIndex = -1,
        int64_t	inFilepos = -1, int64_t inStartCr = -1);

    VODNet_Error VODNet_GetTSPacketData(VODINSTANCE inVODInstance, unsigned char* outTo, long inMaxSize, long& outSize, unsigned short& outSeqNo, unsigned int &outRtpTimeStemp, unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize);

    VODNet_Error VODNet_SendPause(VODINSTANCE inVODInstance, long inDBTime);
    VODNet_Error VODNet_SendTeardown(VODINSTANCE inVODInstance, long inDBTime);
    /*VODNet_Error
    VODNet_SendIamAlive(VODINSTANCE inVODInstance);
    */

    VODNet_Error VODNet_AttachmentReq(VODINSTANCE inVODInstance, long inDBTime, short inTitleIndex, unsigned char* pAttachmentBuffer, int iAttachmentbuffersize);
    VODNet_Error VODNet_SendGetTitleParam(VODINSTANCE inVODInstance, long inDBTime, short inTitleIndex, unsigned short& outVideoWidth, unsigned short& outVideoHeight, float& outVideoFPS, unsigned& outVideoFileType, float& outStart, float& outEnd, char **outDVDInfo, int* outAttachmentSize);
    VODNet_Error VODNet_SendRecordParam(VODINSTANCE inVODInstance, long& time);

    double VODNet_GetJitter(VODINSTANCE inVODInstance, unsigned inServerSSRC);
    VODNet_Error VODNet_ClearDownLoadBuffer(VODINSTANCE inVODInstance);
    char* VODNet_GetLastError(VODINSTANCE inVODInstance, long& outErrCode);
    VODNet_Error VODNet_GetEvent(VODINSTANCE inVODInstance, VODNet_Event* outVODNetEvent, unsigned long inTimeOutMiliSecs = 0);
    void VODNet_DisposeClient(VODINSTANCE inVODInstance);

    long VODNet_GetClientNetworkIP(char *outStr);

    void VODNet_StreamStop(VODINSTANCE inVODInstance);


    VODNet_Error VODNet_DownloadStart(VODINSTANCE inVODInstance, long inDBTime, char* inBuffer, long inBufferSize);
    VODNet_Error VODNet_DownloadEnd(VODINSTANCE inVODInstance, long inDBTime);
    VODNet_Error VODNet_DownloadData(VODINSTANCE inVODInstance, long inDBTime, char* outBuffer, long& outBufferSize);

#endif // __VODNETLIBC_H__
