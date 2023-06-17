#ifndef __CGroupsock_VOD_H__
#define __CGroupsock_VOD_H__

#include "MxSocket.h"
#include "SocketHelper.h"
#include "CTaskScheduler.h"
#include "CMedia.h"


    class CPort
    {
    public:
        CPort(uint16_t num);

        uint16_t num() const
        {
            return m_PortNum;
        }

        void	SetNsPortNum(uint16_t inNsPortNum) { m_PortNum = inNsPortNum; }

    private:
        uint16_t m_PortNum;
    };

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CPort& p);

    class CSocket
    {
    public:
        CSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag); // virtual base class
        CSocket(CUsageEnvironment*	inEnv,
            bool inTcp,
            bool bAuthFlag,
            int inTcpSock,
            MxSocket* inAuthTcpSock);


        virtual ~CSocket();

        virtual short handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut) = 0;

        int socketNum() const { return m_SocketNum; }

        MxSocket* GetSocketHandle() const { return m_SocketHandle; }

        CPort port() const { return m_Port; }
        CUsageEnvironment* env() { return m_UsageEnvironment; }
        bool				GetAuthFlag() { return m_bAuthFlag; }
        static int sDebugLevel;

    private:
        bool					m_bAuthFlag;
        int					m_SocketNum;
        MxSocket* m_SocketHandle;

        CPort				   m_Port;
        CUsageEnvironment* m_UsageEnvironment;
        int                   m_bAutoCloseSock;
    };

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CSocket& sock);

    class COutputSocket : public CSocket
    {
    public:
        COutputSocket(CUsageEnvironment* inEnv, bool bAuthFlag);
        virtual ~COutputSocket();

        bool write(uint32_t address, CPort port, unsigned char* buffer, unsigned bufferSize);


    protected:
        COutputSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag);
        COutputSocket(CUsageEnvironment* inEnv,
            bool bAuthFlag,
            bool inTcp,
            int inTcpSock,
            MxSocket* inAuthTcpSock);

        uint16_t sourcePortNum() const { return m_SourcePort.num(); }

    private:
        virtual short handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut);

        CPort m_SourcePort;

    };

    class CDestRecord
    {
    public:
        CDestRecord(struct in_addr const& addr, CPort const& port, short inLineQuality, CDestRecord* next);
        virtual ~CDestRecord();

    public:
        CDestRecord*		m_Next;

        uint32_t	m_Addr;
        CPort				m_Port;

        short				m_LineQuality;
        unsigned			m_Jitter;

    };

    class CIoSocket : public COutputSocket
    {
    public:
        CIoSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag);
        CIoSocket(CUsageEnvironment* inEnv,
            bool bAuthFlag,
            bool bTcp,
            int inTcpSock,
            MxSocket* inAuthTcpSock);

        virtual ~CIoSocket();

        void addDestination(struct in_addr const& destAddr, CPort const& destPort, short inLineQuality);
        void removeDestination(struct in_addr const& addr, CPort const& port);
        void removeAllDestinations();
        void changeDestinationParameters(struct in_addr const& newDestAddr, CPort newDestPort);
        bool output(unsigned char* buffer, unsigned bufferSize);
        void NotifyJitter(struct in_addr const& addr, unsigned inJitter);

        uint32_t getSourceAddr();
        virtual short handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut);
        virtual short handleReadTcp(unsigned char* buffer, unsigned inReadSize);

    private:

        CDestRecord* m_Dests;
    };

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CIoSocket& g);


    typedef void AuxHandlerFunc(void* clientData, unsigned char* packet, unsigned packetSize);

    class CRTPInterface
    {
    public:
        CRTPInterface(CMedium* owner, CIoSocket* ioSock);
        virtual ~CRTPInterface();

        CIoSocket* getIoSock() const { return m_IoSock; }

        bool sendPacket(unsigned char* packet, unsigned packetSize);
        short handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut);
        short handleReadTcp(unsigned char* buffer, unsigned inReadSize);
        void NotifyJitter(struct in_addr const& addr, unsigned inJitter);

        void setAuxilliaryReadHandler(AuxHandlerFunc* handlerFunc, void* handlerClientData)
        {
            m_AuxReadHandlerFunc = handlerFunc;
            m_AuxReadHandlerClientData = handlerClientData;
        }

        CUsageEnvironment* env()	{ return m_Owner->env(); }

    private:
        CMedium*			m_Owner;
        CIoSocket*		 m_IoSock;

        AuxHandlerFunc*	m_AuxReadHandlerFunc;
        void*				m_AuxReadHandlerClientData;
    };

#endif//__CGroupsock_VOD_H__
