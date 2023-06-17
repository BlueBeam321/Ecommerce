
#ifndef __CTsRtpSource_VOD_H__
#define __CTsRtpSource_VOD_H__

#include "CRTPSource.h"


#ifdef _WIN32
    typedef unsigned(__stdcall *PTHREAD_START)(void*);
#else
#include <pthread.h>
    typedef void* (*PTHREAD_START)(void*);
#endif

    class CRTSPClient;
    class CTsRtpSource : public CRTPSource
    {
    public:
        CTsRtpSource(CUsageEnvironment* inEnv,
            CIoSocket* rtpIoSock,
            unsigned char rtpPayloadFormat,
            unsigned rtpTimestampFrequency,
            unsigned long inSSRC,
            char const* mimeTypeString,
            unsigned offset,
            bool               doNormalMBitRule,
            int                inRtpStreamMode,
            bool				inAuthStreamMode,
            CRTSPClient*       inRTSPClientObj);
        virtual ~CTsRtpSource();

        static CTsRtpSource* createNew(CUsageEnvironment* inEnv, CIoSocket* rtpIoSock,
            unsigned char rtpPayloadFormat,
            unsigned rtpTimestampFrequency,
            unsigned long inSSRC,
            char const* mimeTypeString,
            unsigned            offset,
            bool                doNormalMBitRule,
            int                 inRtpStreamMode,
            bool				inAuthStreamMode,
            CRTSPClient*         inRTSPClientObj);

        long getNextFrame(unsigned char* to, long maxSize,
            unsigned short& outSeqNo, uint32_t& outRtpTimeStemp,
            unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize);
        void stopGettingFrames();
        bool isCurrentlyAwaitingData() const { return m_IsCurrentlyAwaitingData; }

        virtual void	setPacketReorderingThresholdTime(unsigned uSeconds);

        void ResetReorderingBuffer();
        virtual void WaitRecvReady();

#ifdef _WIN32
        static void networkReadHandler(void* inData);
#else
        static void* networkReadHandler(void* inData);
#endif	
        void	networkReadHandler1();

        bool		IsTcpSocketError();

        void		DoPauseProcess(bool bPauseProcessFlag);
    private:
        virtual char const* MIMEtype() const;
        virtual bool isFramedSource() const;
        bool packetIsUsableInJitterCalculation(unsigned char* /*packet*/, unsigned /*packetSize*/);
        long	doGetNextFrame(unsigned short& outSeqNo, uint32_t& outRtpTimeStemp, unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize);
        long	afterGettingFrame(long inFrameSize);
        void	doStopGettingFrames();


        unsigned char*	m_To;
        long				m_MaxSize;
        long				m_FrameSize;
        unsigned char*	m_SavedTo;
        long				m_SavedMaxSize;

        int				 m_RtpStreamMode;
        bool			m_AuthStreamMode;
        CRTSPClient*    m_pRtspClientObj;


        class CReorderingPacketBuffer* m_ReorderingBuffer;

        char const*		m_MIMEtypeString;
        long				m_Offset;
        bool			m_UseMBitForFrameEnd;

        bool			m_IsCurrentlyAwaitingData;

#ifdef _WIN32
        HANDLE		m_hReadThread;
#else
        pthread_t		m_hReadThread;
#endif	

        int				m_ReadSocketNum;
        MxSocket*	m_AuthReadSocket;

    public:
        bool			m_ExitReadThreadFlag;
        bool			m_tcpRtpSockError;
        bool			m_bPauseProcessFlag;
        long				m_iPauseProcessFlag;

        bool			m_bWaitFlag;

        int				reserved[32];
    };


    class CBufferedPacket
    {
    public:
        CBufferedPacket();
        virtual ~CBufferedPacket();

        bool hasUsableData() const { return m_Tail > m_Head; }
        unsigned useCount() const { return m_UseCount; }

        short fillInData(CRTPInterface& rtpInterface);
        short fillInDataTcp(CRTPInterface& rtpInterface, int inRecvLen);

        void assignMiscParams(unsigned short rtpSeqNo, unsigned rtpTimestamp,
        struct timeval presentationTime,
            bool hasBeenSyncedUsingRTCP,
            bool rtpMarkerBit, struct timeval timeReceived);
        void skip(unsigned numBytes);
        void removePadding(unsigned numBytes);
        void appendData(unsigned char* newData, unsigned numBytes);
        void use(unsigned char* to, unsigned toSize,
            unsigned& bytesUsed, unsigned& bytesTruncated,
            unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
        struct timeval& presentationTime,
            bool& hasBeenSyncedUsingRTCP, bool& rtpMarkerBit,
            unsigned short &outRtpExtflag, unsigned char* outRtpExtData, long &ioRtpExtDataSize);

        CBufferedPacket*& nextPacket() { return m_NextPacket; }

        unsigned short rtpSeqNo() const { return m_RTPSeqNo; }
        struct timeval const& timeReceived() const { return m_TimeReceived; }

        unsigned char* data() const { return &m_Buf[m_Head]; }
        unsigned dataSize() const { return m_Tail - m_Head; }
        bool rtpMarkerBit() const { return m_RTPMarkerBit; }
        void	SetExtData(unsigned short inExtFlag, unsigned char* inExtData, long inExtSize);

        CBufferedPacket*	m_NextPacket;

        virtual void		reset();

        int				isfree();
        void				Malloc();
        void				Free();
    public:

        unsigned			m_PacketSize;
        unsigned char*	m_Buf;
        unsigned			m_Head;
        unsigned			m_Tail;

        unsigned			m_ExtFlag;
        unsigned char		m_ExtData[64];
        long				m_ExtSize;
    public:

        unsigned			m_UseCount;
        unsigned short	m_RTPSeqNo;
        unsigned			m_RTPTimestamp;
        struct timeval	m_PresentationTime;
        bool				m_HasBeenSyncedUsingRTCP;
        bool				m_RTPMarkerBit;
        struct timeval	m_TimeReceived;

        bool				m_UseFlag;	//add 2010.11.13

    };

    class CBufferedPacketFactory
    {
    public:
        CBufferedPacketFactory();
        virtual ~CBufferedPacketFactory();

        virtual CBufferedPacket* createNewPacket(CTsRtpSource* ourSource);
    };

#endif // __CTsRtpSource_VOD_H__
