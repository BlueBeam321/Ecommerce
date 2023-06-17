
#ifndef __CRTCPInstance_VOD_H__
#define __CRTCPInstance_VOD_H__

#include "CRTPSource.h"


#ifdef _WIN32
    typedef unsigned(__stdcall *PTHREAD_START)(void*);
#else
#include <pthread.h>
    typedef void* (*PTHREAD_START)(void*);
#endif


    class COutPacketBuffer
    {
    public:
        COutPacketBuffer(unsigned maxPacketSize);
        ~COutPacketBuffer();

        unsigned char*		curPtr() const { return &m_Buf[m_PacketStart + m_CurOffset]; }
        unsigned				totalBytesAvailable() const { return m_Limit - (m_PacketStart + m_CurOffset); }
        unsigned				totalBufferSize() const { return m_Limit; }
        unsigned char*		packet() const { return &m_Buf[m_PacketStart]; }
        unsigned				curPacketSize() const { return m_CurOffset; }

        void					increment(unsigned numBytes) { m_CurOffset += numBytes; }

        void					enqueue(unsigned char const* from, unsigned numBytes);
        void					enqueueWord(unsigned word);
        void					insert(unsigned char const* from, unsigned numBytes, unsigned toPosition);
        void					insertWord(unsigned word, unsigned toPosition);
        void					extract(unsigned char* to, unsigned numBytes, unsigned fromPosition);
        unsigned				extractWord(unsigned fromPosition);

        void					skipBytes(unsigned numBytes);

        void					resetPacketStart() { m_PacketStart = 0; }
        void					resetOffset() { m_CurOffset = 0; }

    private:
        unsigned				m_PacketStart, m_CurOffset, m_Limit;
        unsigned char*		m_Buf;
    };

    class CSDESItem
    {
    public:
        CSDESItem(unsigned char tag, unsigned char const* value);

        unsigned char const* data() const { return m_Data; }
        unsigned totalSize() const;

    private:
        unsigned char m_Data[2 + 0xFF];
    };

    class CRTCPMemberDatabase;

    class CRTCPInstance : public CMedium
    {
    public:
        CRTCPInstance(CUsageEnvironment* inEnv,
            CIoSocket* rtcpIoSock,
            unsigned totSessionBW,
            unsigned char const* cname,
            CRTPSource const* source);
        virtual ~CRTCPInstance();

        static CRTCPInstance* createNew(CUsageEnvironment* inEnv,
            CIoSocket* rtcpIoSock,
            unsigned totSessionBW, /* in kbps */
            unsigned char const* cname,
            CRTPSource const* source);

        static bool lookupByName(CUsageEnvironment* inEnv, char const* instanceName, CRTCPInstance*& resultInstance);

        short	StartRtcping();
        short	StopRtcping();

        unsigned numMembers() const;

        void setByeHandler(TaskFunc* handlerTask, void* clientData, bool handleActiveParticipantsOnly = true);

        CIoSocket* getIoSock() const { return m_RTCPInterface->getIoSock(); }

        void setAuxilliaryReadHandler(AuxHandlerFunc* handlerFunc, void* handlerClientData)
        {
            m_RTCPInterface->setAuxilliaryReadHandler(handlerFunc, handlerClientData);
        }

        void schedule(double nextTime);
        void reschedule(double nextTime);
        void sendReport();
        void sendBYE();
        int typeOfEvent() { return m_TypeOfEvent; }
        int sentPacketSize() { return m_LastSentSize; }
        int packetType() { return m_TypeOfPacket; }
        int receivedPacketSize() { return m_LastReceivedSize; }
        int checkNewSSRC();
        void removeLastReceivedSSRC();
        void removeSSRC(uint32_t ssrc);

    private:
        virtual bool isRTCPInstance() const;

        void addReport();
        void addRR();
        void enqueueCommonReportPrefix(unsigned char packetType, uint32_t SSRC, unsigned numExtraWords = 0);
        void enqueueCommonReportSuffix();
        void enqueueReportBlock(CRTPReceptionStats* receptionStats);
        void addSDES();
        void addBYE();

        void sendBuiltPacket();

        static void	onExpire(CRTCPInstance* instance);
        void		   onExpire1();

#ifdef _WIN32	
        static void incomingReportHandler(void* inData);
#else
        static void* incomingReportHandler(void* inData);
#endif	


        void		 incomingReportHandler1();

        void onReceive(int typeOfPacket, int totPacketSize, uint32_t ssrc);

        unsigned char*	  m_InBuf;
        COutPacketBuffer*	 m_OutBuf;
        CRTPInterface*	 m_RTCPInterface;
        unsigned			  m_TotSessionBW;
        CRTPSource const*	m_Source;

        CSDESItem					m_CNAME;
        CRTCPMemberDatabase*	  m_KnownMembers;
        unsigned					  m_OutgoingReportCount;// used for SSRC member aging

        double	m_AveRTCPSize;
        int		m_IsInitial;
        double	m_PrevReportTime;
        double	m_NextReportTime;
        int		m_PrevNumMembers;

        int m_LastSentSize;
        int m_LastReceivedSize;
        uint32_t m_LastReceivedSSRC;
        int m_TypeOfEvent;
        int m_TypeOfPacket;
        bool m_HaveJustSentPacket;
        unsigned m_LastPacketSentSize;

        TaskFunc* m_ByeHandlerTask;
        void* m_ByeHandlerClientData;
        bool m_ByeHandleActiveParticipantsOnly;

        CTaskScheduler*	m_TaskScheduler;

        bool	m_ExitReportThreadFlag;

#ifdef _WIN32
        HANDLE	m_hReportThread;
#else
        pthread_t	m_hReportThread;
#endif	

        int	m_RtcpSocket;
        TaskToken& nextTask() { return m_NextTask; }
        TaskToken				 m_NextTask;
    };

    const unsigned char RTCP_PT_SR = 200;
    const unsigned char RTCP_PT_RR = 201;
    const unsigned char RTCP_PT_SDES = 202;
    const unsigned char RTCP_PT_BYE = 203;
    const unsigned char RTCP_PT_APP = 204;

    const unsigned char RTCP_SDES_END = 0;
    const unsigned char RTCP_SDES_CNAME = 1;
    const unsigned char RTCP_SDES_NAME = 2;
    const unsigned char RTCP_SDES_EMAIL = 3;
    const unsigned char RTCP_SDES_PHONE = 4;
    const unsigned char RTCP_SDES_LOC = 5;
    const unsigned char RTCP_SDES_TOOL = 6;
    const unsigned char RTCP_SDES_NOTE = 7;
    const unsigned char RTCP_SDES_PRIV = 8;

#endif // __CRTCPInstance_VOD_H__
