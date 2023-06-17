
#ifndef __CMediaSession_VOD_H__
#define __CMediaSession_VOD_H__

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

#include "Sockets.h"
#include "CRTPSource.h"
#include "CRTCPInstance.h"

    class CMediaSubsession;
    class CRTSPClient;

    enum vodRTPStreamMode{
        eRTPSTREAM_AUTO,
        eRTPSTREAM_UDP,
        eRTPSTREAM_TCP
    };

    class CMediaSession : public CMedium
    {
    public:
        CMediaSession(CUsageEnvironment* inEnv, vodRTPStreamMode inRtpStreamMode, bool inAuthStreamMode);
        virtual ~CMediaSession();

        static CMediaSession* createNew(CUsageEnvironment* inEnv, char const* sdpDescription, vodRTPStreamMode inRtpStreamMode, bool inAuthStreamMode);

        static bool lookupByName(CUsageEnvironment* inEnv, char const* sessionName, CMediaSession*& resultSession);

        bool hasSubsessions() const { return m_SubsessionsHead != NULL; }
        float& playEndTime() { return m_MaxPlayEndTime; }
        char* connectionEndpointName()  { return m_ConnectionEndpointName; }
        char const* CNAME() const { return m_CNAME; }

        virtual bool isMediaSession() const;

    private:
        bool initializeWithSDP(char const* sdpDescription);
        bool parseSDPLine_c(char const* sdpLine);
        bool parseSDPAttribute_range(char const* sdpLine);

        static char* lookupPayloadFormat(unsigned char rtpPayloadType, unsigned& rtpTimestampFrequency, unsigned& numChannels);
        static unsigned guessRTPTimestampFrequency(char const* mediumName, char const* codecName);

        friend class CMediaSubsessionIterator;
        char* m_CNAME;

        CMediaSubsession* m_SubsessionsHead;
        CMediaSubsession* m_SubsessionsTail;

        char* m_ConnectionEndpointName;
        float m_MaxPlayEndTime;
    public:
        vodRTPStreamMode m_RtpStreamMode;
        bool			m_AuthStreamMode;
    };


    class CMediaSubsessionIterator
    {
    public:
        CMediaSubsessionIterator(CMediaSession& session);
        virtual ~CMediaSubsessionIterator();

        CMediaSubsession* next();
        void reset();

    private:
        CMediaSession& m_OurSession;
        CMediaSubsession* m_NextPtr;
    };

    class CMediaSubsession
    {
    public:
        CMediaSession&	  parentSession() { return m_Parent; }
        unsigned short		clientPortNum() const { return m_ClientPortNum; }
        unsigned char			rtpPayloadFormat() const { return m_RTPPayloadFormat; }
        char const*			savedSDPLines() const { return m_SavedSDPLines; }
        char const*			mediumName() const { return m_MediumName; }
        char const*			codecName() const { return m_CodecName; }
        char const*			protocolName() const { return m_ProtocolName; }
        char const*			controlPath() const { return m_ControlPath; }

        unsigned				numChannels() const { return m_NumChannels; }
        unsigned short		videoWidth() const { return m_VideoWidth; }
        unsigned short		videoHeight() const { return m_VideoHeight; }
        float					videoFPS() const { return m_VideoFPS; }
        unsigned				videoFileType() const { return m_FileType; }
        unsigned				videoStreamType() const { return m_StreamType; }
        float					videoStart() const { return m_VideoStart; }
        float					videoEnd() const { return m_VideoEnd; }
        char					*DVDInfo(){ return m_DVDInfoStr; };
        int					GetAttachmentSize()	{ return m_AttachmentSize; };
        void					GetThumbServerInfo(unsigned short* outThumbport, char* outThumbAddr, char* outThumbIdStr)
        {
            if (outThumbport) *outThumbport = (unsigned short)m_ThumbserverPort;
            if (outThumbAddr) strcpy(outThumbAddr, m_ThumbserverIp);
            if (outThumbIdStr) strcpy(outThumbIdStr, m_ThumbIdStr);
        };

        CRTPSource*		  rtpSource() { return m_RTPSource; }
        CRTCPInstance*	  rtcpInstance() { return m_RTCPInstance; }
        unsigned			    rtpTimestampFrequency() const { return m_RTPTimestampFrequency; }

        float					playEndTime() const;
        bool					setClientPortNum(unsigned short portNum);
        bool					initiate(unsigned long inSSRC, int useSpecialRTPoffset, CRTSPClient* inRTSPClientObj);
        void					deInitiate();
        bool					setTitleSDPInfo(char* inSDPStr);

        unsigned short		    GetIdleClientPort(unsigned short baseUdpPort);

        char*					connectionEndpointName() { return m_ConnectionEndpointName; }
        void					SeConnectionEndPointName(char* inConnectionEndPointName) { m_ConnectionEndpointName = inConnectionEndPointName; }
        unsigned				connectionEndpointAddress();
        void					setDestinations(unsigned defaultDestAddress);


        vodRTPStreamMode GetRtpStreamMode() { return m_Parent.m_RtpStreamMode; };
        char*				m_sessionId;
        unsigned short	m_serverPortNum;

    private:
        friend class CMediaSession;
        friend class CMediaSubsessionIterator;
        CMediaSubsession(CMediaSession& parent);
        virtual ~CMediaSubsession();

        CUsageEnvironment*  env() { return m_Parent.env(); }
        void					setNext(CMediaSubsession* next) { m_Next = next; }

        bool					parseSDPLine_c(char const* sdpLine);
        bool					parseSDPAttribute_control(char const* sdpLine);
        bool					parseSDPAttribute_range(char const* sdpLine);
        bool					parseSDPAttribute_x_dimensions(char const* sdpLine);
        bool					parseSDPAttribute_x_framerate(char const* sdpLine);
        bool					parseSDPAttribute_fmtp(char const* sdpLine);
        bool					parseSDPAttribute_smtp(char const* sdpLine);
        bool					parseSDPAttribute_DVD_Info(char const* sdpLine);
        bool					parseSDPAttribute_ThumbInfo(char const* sdpLine);
        bool					parseSDPAttribute_AttachmentInfo(char const* sdpLine);

        CMediaSession& m_Parent;
        CMediaSubsession* m_Next;

        char* m_ConnectionEndpointName;
        unsigned short m_ClientPortNum;
        unsigned char m_RTPPayloadFormat;
        char* m_SavedSDPLines;
        char* m_MediumName;
        char* m_CodecName;
        char* m_ProtocolName;
        unsigned m_RTPTimestampFrequency;
        char* m_ControlPath;

        float m_PlayEndTime;
        unsigned short m_VideoWidth, m_VideoHeight;
        float m_VideoFPS;
        unsigned m_NumChannels;
        float m_VideoStart;
        float m_VideoEnd;
        char*		m_DVDInfoStr;
        int			 m_ThumbserverPort;
        char			m_ThumbserverIp[64];
        char			m_ThumbIdStr[64];
        long			m_AttachmentSize;

        unsigned m_FileType;
        unsigned m_StreamType;

        CIoSocket* m_RTPSocket; CIoSocket* m_RTCPSocket;
        CRTPSource* m_RTPSource; CRTCPInstance* m_RTCPInstance;
        //
    public:
        static void		Initialize();
        static void		Finalize();
    protected:

        static vlc_mutex_t s_UdpPortmutex;
        static long					s_InitFlag;
        static long					s_appIndex;
    };

#endif//__CMediaSession_VOD_H__