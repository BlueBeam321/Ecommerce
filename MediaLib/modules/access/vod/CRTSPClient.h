#ifndef __CRTSPClient_VOD_H__
#define __CRTSPClient_VOD_H__
#include "MxSocket.h"
#include "CMediaSession.h"
    struct resp_req_t{
        char    resp_req[256 * 1024];
        struct resp_req_t *pNext;
    };

    class CRtcpReqArray{
    public:
        CRtcpReqArray();
        ~CRtcpReqArray();

        void    PushResReq(char* inReqStr);
        bool   PopResReq(char* outReqStr);
    protected:

        void    Lock();
        void    Unlock();

        struct resp_req_t   *m_pRoot;
        struct resp_req_t   *m_pTail;

#ifdef _WIN32
        HANDLE					m_hMutex;
#else
        pthread_mutex_t			m_hMutex;
#endif
    };

    class CRTSPClient : public CMedium
    {
    public:
        CRTSPClient(CUsageEnvironment* inEnv, char* inIpAddrStr, char const* applicationName, long bStbFlag, bool bAuthStreamMode = false);
        virtual ~CRTSPClient();

        static CRTSPClient* createNew(CUsageEnvironment* inEnv, char* inIpAddrStr, char const* applicationName = NULL, long bStbFlag = false, bool bAuthStreamMode = false);
        static bool lookupByName(CUsageEnvironment* inEnv, char const* sourceName, CRTSPClient*& resultClient);

        char* describeURL(char const* url, short inTitleIndex,
            long inRedirect,
            unsigned short& outPort, char* outServerAddr,
            unsigned short* outThumbPort, char* outThumbAddr, char* outThumbIdStr, long inNetSpeed);

        bool setupMediaSubsession(CMediaSubsession& subsession, float& outStart, float& outEnd, float inStart, short inTitleIndex, short inChapterIndex);
        bool playMediaSubsession(CMediaSubsession& subsession, float start, float end = -1.0, short inTitleIndex = -1, short inChapterIndex = -1, int64_t inFilepos = -1, int64_t inStartCr = -1);
        bool pauseMediaSubsession(CMediaSubsession& subsession);
        bool teardownMediaSubsession(CMediaSubsession& subsession);
        bool attachmentreqMediaSubsession(CMediaSubsession& subsession, short inTitleIndex, unsigned char* pAttachmentBuffer, int iAttachmentbuffersize);


        char* getparameterMediaSubsession(CMediaSubsession& subsession, short inTitleIndex);

        bool RecordMediaSubsession(CMediaSubsession& subsession, long& time);

        bool downloadstartMediaSubsession(CMediaSubsession& subsession, char* posInfoBuffer, long inBufferSize);
        bool downloadstopMediaSubsession(CMediaSubsession& subsession);
        bool downloaddataMediaSubsession(CMediaSubsession& subsession, char* outRtpBufferPos, long& ioBufferSize);


        int socketNum() const { return m_InputSocketNum; }
        MxSocket* authSocketHandle() const { return m_AuthInputSocket; }

        unsigned  describeStatus() const { return m_DescribeStatusCode; }

        static bool parseRTSPURL(CUsageEnvironment* inEnv,
            char const* url,
            uint32_t& address,
            char*	addressStr,
            uint16_t& portNum,
            short& lineQuality,
            char** controlServerAddr,
            char** userName,
            short& userKind,
            char* fileName,
            short& fileKind,
            unsigned int& fileID,
            short& listKind,
            char* archDbAddr,
            char** baseUrl);

        long	GetServerTime();

    private:
        virtual bool isRTSPClient() const;
        void reset();
        void resetTCPSockets(bool authStreamMode = 0);

        bool openConnectionFromURL(char const* url);
        bool ConnectToServerEx(uint16_t destPortNum);

        bool sendRequest(char const* requestString, char const* tag, bool authStreamMode);
        bool getResponse(char const* tag, unsigned& bytesRead, unsigned& responseCode, char*& firstLine, char*& nextLineStart, bool checkFor200Response = true, bool checkSeqNo = false);
        bool getResponse_inside(char const* tag, unsigned& bytesRead, unsigned& responseCode, char*& firstLine, char*& nextLineStart, bool checkFor200Response = true, bool checkSeqNo = false);
        unsigned getResponse1(char*& responseBuffer, unsigned responseBufferSize, int skipbyte = 0, int inTimeOutValue = SOCK_TIMEOUT_VALUE);
        long		checkContent(char* inRtspReqStr);
        bool	getContentBody(char* inRtspReqStr, long bytesRead, long content_len, bool authStreamMode = 0);
    public:
        unsigned        RtspResProcessbyRtp(char firstByte);
        unsigned        RtspResProcessbyRtpEx(char* inBuff, int inBuffSize);
        void            RtspTcpSocketError();
        bool            IsRTPChannelID(long inChannelID);
        long            GetNextRtpChannelID();
    private:
        bool parseResponseCode(char const* line, unsigned& responseCode);
        bool parseRangeHeader(char const* buf, float& rangeStart, float& rangeEnd);
        bool parseTransportResponse(char const* line, char*& serverAddressStr, uint16_t& serverPortNum);
        char* createRangeString(float start, float end);
        void constructSubsessionURL(CMediaSubsession const& subsession, char const*& prefix, char const*& separator, char const*& suffix);

        char*				m_UserAgentHeaderStr;
        unsigned long		m_UserAgentHeaderStrSize;
        int				m_InputSocketNum;

        MxSocket* m_AuthInputSocket;

        bool				m_AuthStreamMode;
        unsigned long		m_ServerAddress;
        char				m_ServerAddressStr[256];
        char*				m_BaseURL;
        char*				m_LastSessionId;
        unsigned long 	m_DescribeStatusCode;
        char*				m_ResponseBuffer;
        char*				m_ResponseBuffer1;
        unsigned long		m_ResponseBufferSize;
        bool				m_ServerIsSoBaekSu;
        char*             m_ResponseBufferByOther;
        unsigned long     m_ResponseBufferSizeByOther;


        char*				m_UserSpecificData;/*"<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t"*/
        short				m_LineQuality;
        char*				m_UserName;
        short				m_UserKind;
        char				m_FileName[512];
        short				m_FileKind;
        short				m_ListKind;
        unsigned int		m_FileID;
        char*				m_ControlServerAddr;
        long				m_DestPortNum;

        long				m_bStbFlag;

        long				m_IpAddrNum;

        unsigned            m_CSeq;
        CRtcpReqArray       m_RtcpReqArray;
        vodRTPStreamMode    m_RtpStreamMode;
        long                m_RtpChannelID;
        long                m_bTcpSocketErrorFlag;
    public:
        char              m_IpAddrString[64];

    };

#endif //__CRTSPClient_VOD_H__
