#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include "Sockets.h"
#include "CMedia.h"
#include "CUsageEnvironment.h"
#ifndef _WIN32
#include "MxSocketConfig.h"
#endif


    ////////// CPort //////////
    CPort::CPort(uint16_t num)
    {
        m_PortNum = htons(num);
    }

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CPort& p)
    {
        return s << ntohs(p.num());
    }

    ////////// CSocket //////////
    int CSocket::sDebugLevel = 1; // default value

    CSocket::CSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag)
        :m_Port(port), m_UsageEnvironment(inEnv)
    {
        m_bAuthFlag = bAuthFlag;

        m_bAutoCloseSock = true;
        if (m_bAuthFlag == false)
            m_SocketNum = setupDatagramSocket(inEnv, port.num());
        else
        {
            m_SocketHandle = new MxSocket();

            int32_t err = m_SocketHandle->CreateUdpSocket(NULL, port.num(), 0x200000, 0x200000);
            if (err)
            {
                inEnv->setResultMsg(11000327, "Unable to create datagram socket!");
            }
        }
    }

    CSocket::CSocket(CUsageEnvironment*	inEnv,
        bool inTcp,
        bool bAuthFlag,
        int inTcpSock,
        MxSocket* inAuthTcpSock)
        :m_Port(0), m_UsageEnvironment(inEnv)
    {
        m_bAuthFlag = bAuthFlag;

        if (m_bAuthFlag == false)
            m_SocketNum = inTcpSock;
        else
            m_SocketHandle = inAuthTcpSock;

        m_bAutoCloseSock = false;
    }

    CSocket::~CSocket()
    {
        if (m_bAutoCloseSock)
        {
            if (m_bAuthFlag == false)
            {
                closesocket(m_SocketNum);
            }
            else
            {
                if (m_bAutoCloseSock)
                {
                    m_SocketHandle->Close();
                    delete m_SocketHandle;
                    m_SocketHandle = NULL;
                }
            }
        }
    }

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CSocket& sock)
    {
        return s << vod_timestampString() << " CSocket(" << sock.socketNum() << ")";
    }

    ///////// COutputSocket //////////
    COutputSocket::COutputSocket(CUsageEnvironment* inEnv, bool bAuthFlag)
        : CSocket(inEnv, 0, bAuthFlag), m_SourcePort(0)
    {
    }

    COutputSocket::COutputSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag)
        : CSocket(inEnv, port, bAuthFlag), m_SourcePort(0)
    {

    }

    COutputSocket::COutputSocket(CUsageEnvironment* inEnv,
        bool bAuthFlag,
        bool inTcp,
        int inTcpSock, 
        MxSocket* inAuthTcpSock)
        : CSocket(inEnv,
        inTcp,
        bAuthFlag,
        inTcpSock,
        inAuthTcpSock), m_SourcePort(0)
    {

    }

    COutputSocket::~COutputSocket()
    {
    }

    bool
        COutputSocket::write(uint32_t address, CPort port, unsigned char* buffer, unsigned bufferSize)
    {
        uint16_t theSourcePortNum;
        struct in_addr destAddr;
        destAddr.s_addr = address;

        if (!writeSocket(env(), socketNum(), destAddr, port.num(), buffer, bufferSize))
            return false;

        if (sourcePortNum() == 0)
        {
            if (!getSourcePort(env(), socketNum(), theSourcePortNum))
            {
                env()->setResultMsg(11000409, "Failed to get source port!");
                return false;
            }
            m_SourcePort.SetNsPortNum(theSourcePortNum);
        }

        return true;
    }

    short
        COutputSocket::handleRead(unsigned char* /*buffer*/, unsigned /*bufferMaxSize*/, unsigned& /*bytesRead*/, struct sockaddr_in& /*fromAddress*/, struct timeval* /*inTimeOut*/)
    {
        return 0;//true;
    }

    ///////// CDestRecord //////////
    CDestRecord::CDestRecord(struct in_addr const& addr, CPort const& port, short inLineQuality, CDestRecord* next)
        : m_Next(next), m_Port(port), m_Addr(addr.s_addr)
    {
        m_LineQuality = inLineQuality;
        m_Jitter = 0;
    }

    CDestRecord::~CDestRecord()
    {
        delete m_Next;
    }

    //////////////// CIoSocket //////////////////////
    CIoSocket::CIoSocket(CUsageEnvironment* inEnv, CPort port, bool bAuthFlag)
        : COutputSocket(inEnv, port, bAuthFlag), m_Dests(NULL)
    {
        //  addDestination(destAddr, port);

        if (sDebugLevel >= 2)
            *env() << *this << ": created\n";
    }

    CIoSocket::CIoSocket(CUsageEnvironment* inEnv,
        bool bAuthFlag,
        bool bTcp,
        int inTcpSock,
        MxSocket* inAuthTcpSock)
        : COutputSocket(inEnv,
        bAuthFlag,
        bTcp,
        inTcpSock,
        inAuthTcpSock), m_Dests(NULL)
    {
        if (sDebugLevel >= 2)
            *env() << *this << ": created tcp\n";
    }

    CIoSocket::~CIoSocket()
    {
        if (sDebugLevel >= 2) *env() << *this << ": deleting\n";
        delete m_Dests;
    }

    void
        CIoSocket::addDestination(struct in_addr const& destAddr, CPort const& destPort, short inLineQuality)
    {
        for (CDestRecord* dests = m_Dests; dests != NULL; dests = dests->m_Next)
        {
            if (destAddr.s_addr == dests->m_Addr && destPort.num() == dests->m_Port.num())
                return;
        }

        m_Dests = new CDestRecord(destAddr, destPort, inLineQuality, m_Dests);
    }

    void
        CIoSocket::removeDestination(struct in_addr const& addr, CPort const& port)
    {
        for (CDestRecord** destsPtr = &m_Dests; *destsPtr != NULL; destsPtr = &((*destsPtr)->m_Next))
        {
            if (addr.s_addr == (*destsPtr)->m_Addr && port.num() == (*destsPtr)->m_Port.num())
            {
                CDestRecord* next = (*destsPtr)->m_Next;
                (*destsPtr)->m_Next = NULL;
                delete (*destsPtr);
                *destsPtr = next;
                return;
            }
        }
    }

    void
        CIoSocket::removeAllDestinations()
    {
        delete m_Dests; m_Dests = NULL;
    }

    void
        CIoSocket::NotifyJitter(struct in_addr const& addr, unsigned inJitter)
    {
        for (CDestRecord** destsPtr = &m_Dests; *destsPtr != NULL; destsPtr = &((*destsPtr)->m_Next))
        {
            if (addr.s_addr == (*destsPtr)->m_Addr)
            {
                (*destsPtr)->m_Jitter = inJitter;
                return;
            }
        }
    }

    uint32_t
        CIoSocket::getSourceAddr()
    {
        uint32_t theSelfAddr = 0;
        return theSelfAddr;
    }

    bool
        CIoSocket::output(unsigned char* buffer, unsigned bufferSize)
    {
        do {
            bool writeSuccess = true;
            for (CDestRecord* dests = m_Dests; dests != NULL; dests = dests->m_Next)
            {
                if (!write(dests->m_Addr, dests->m_Port, buffer, bufferSize))
                {
                    writeSuccess = false;
                    break;
                }
            }
            if (!writeSuccess) break;
            return true;
        } while (0);

        env()->setResultMsg(11000320, "CIoSocket write failed: ");
        return false;
    }

    short
        CIoSocket::handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut)
    {
        if (GetAuthFlag() == false)
        {
            bytesRead = 0;

            int maxBytesToRead = bufferMaxSize;
            int numBytes = readSocket(env(), socketNum(), buffer, maxBytesToRead, fromAddress, inTimeOut);
            if (numBytes < 0)
                return short(numBytes);

            bytesRead = numBytes;
        }
        else
        {
            bytesRead = 0;

            int	err = 0;
            int numBytes = 0;
            int maxBytesToRead = bufferMaxSize;

            MxSocket* pSockHandle = GetSocketHandle();

            err = pSockHandle->ReceiveDatagram((char*)buffer, maxBytesToRead, &numBytes);
            if (err && env())
                env()->setResultMsg(11000333, "recvfrom() error!");

            if(err < 0)
                return err;

            bytesRead = numBytes;
        }

        return 0;
    }

    short CIoSocket::handleReadTcp(unsigned char* buffer, unsigned inReadSize)
    {
        int numBytes = 0;

        if (GetAuthFlag() == false)
            numBytes = readSocketbyTCP(env(), socketNum(), buffer, inReadSize);
        else
        {
            MxSocket* pSockHandle = GetSocketHandle();

            int32_t len = 0;
            int32_t err;
            int32_t theSelectResult = 0;
            int64_t start_time, cur_time;

            start_time = mdate();
            do
            {
                theSelectResult = pSockHandle->SelectRecv(20 * 1000000);

                if (theSelectResult == 0)
                {
                    msleep(1000);

                    cur_time = mdate();
                    if (cur_time - start_time > 10 * 1000000)
                        return -2;

                    continue;
                }
                else if(theSelectResult < 0)
                    return -1;

                break;
            }
            while(1);

            err = pSockHandle->Receive((char*)buffer, inReadSize, &numBytes);

            if (err)
            {
                char msg_Errode[256];
                sprintf(msg_Errode, "Tcp Sock Error %d(SysErr = %d)",err, errno);
                if(env())
                    env()->setResultMsg(11000408,msg_Errode);

                numBytes = err;
            }
        }

        return numBytes;
    }

    void
        CIoSocket::changeDestinationParameters(struct in_addr const& newDestAddr, CPort newDestPort)
    {
        if (newDestAddr.s_addr != 0)
            m_Dests->m_Addr = newDestAddr.s_addr;

        if (newDestPort.num() != 0)
            m_Dests->m_Port = newDestPort;
    }

    CUsageEnvironment& operator<<(CUsageEnvironment& s, const CIoSocket& g)
    {
        CUsageEnvironment& s1 = s << vod_timestampString() << " CIoSocket(" << g.socketNum() << ": " << g.port();
        return s1;
    }


    ////////// CRTPInterface - Implementation //////////
    CRTPInterface::CRTPInterface(CMedium* owner, CIoSocket* ioSock)
        : m_Owner(owner), m_IoSock(ioSock),
        m_AuxReadHandlerFunc(NULL), m_AuxReadHandlerClientData(NULL)
    {
    }

    CRTPInterface::~CRTPInterface()
    {
    }

    void
        CRTPInterface::NotifyJitter(struct in_addr const& addr, unsigned inJitter)
    {
        if (m_IoSock != NULL)
        {
            m_IoSock->NotifyJitter(addr, inJitter);
        }
    }

    bool
        CRTPInterface::sendPacket(unsigned char* packet, unsigned packetSize)
    {
        return m_IoSock->output(packet, packetSize);
    }

    short
        CRTPInterface::handleRead(unsigned char* buffer, unsigned bufferMaxSize, unsigned& bytesRead, struct sockaddr_in& fromAddress, struct timeval* inTimeOut)
    {
        short readSuccess;

        readSuccess = m_IoSock->handleRead(buffer, bufferMaxSize, bytesRead, fromAddress, inTimeOut);
        if (readSuccess == 0 && m_AuxReadHandlerFunc != NULL)
        {
            (*m_AuxReadHandlerFunc)(m_AuxReadHandlerClientData, buffer, bytesRead);
        }
        return readSuccess;
    }

    short CRTPInterface::handleReadTcp(unsigned char* buffer, unsigned inReadSize)
    {
        short readSuccess;

        readSuccess = m_IoSock->handleReadTcp(buffer, inReadSize);
        if (readSuccess == 0 && m_AuxReadHandlerFunc != NULL)
        {
            (*m_AuxReadHandlerFunc)(m_AuxReadHandlerClientData, buffer, inReadSize);
        }
        return readSuccess;
    }
