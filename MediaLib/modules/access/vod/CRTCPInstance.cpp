
#ifdef _WIN32
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#endif
#include <time.h>

#ifndef _WIN32
#include "MxSocketConfig.h"
#include <netinet/tcp.h>
#endif

#include "SocketHelper.h"
#include "CRTCPInstance.h"
#include "CTaskScheduler.h"
#include "rtcp_from_spec.h"


    ///////////////////////////////// COutPacketBuffer ///////////////////////////////////////
    COutPacketBuffer::COutPacketBuffer(unsigned maxPacketSize)
    {
        m_Limit = maxPacketSize;
        m_Buf = new unsigned char[m_Limit];
        resetPacketStart();
        resetOffset();
    }

    COutPacketBuffer::~COutPacketBuffer()
    {
        delete[] m_Buf;
    }

    void
        COutPacketBuffer::enqueue(unsigned char const* from, unsigned numBytes)
    {
        if (numBytes > totalBytesAvailable())
        {
            numBytes = totalBytesAvailable();
        }

        if (curPtr() != from) memmove(curPtr(), from, numBytes);
        increment(numBytes);
    }

    void
        COutPacketBuffer::enqueueWord(unsigned word)
    {
        unsigned nWord = htonl(word);
        enqueue((unsigned char*)&nWord, 4);
    }

    void
        COutPacketBuffer::insert(unsigned char const* from, unsigned numBytes, unsigned toPosition)
    {
        unsigned realToPosition = m_PacketStart + toPosition;
        if (realToPosition + numBytes > m_Limit)
        {
            if (realToPosition > m_Limit) return; // we can't do this
            numBytes = m_Limit - realToPosition;
        }

        memmove(&m_Buf[realToPosition], from, numBytes);
        if (toPosition + numBytes > m_CurOffset)
        {
            m_CurOffset = toPosition + numBytes;
        }
    }

    void
        COutPacketBuffer::insertWord(unsigned word, unsigned toPosition)
    {
        unsigned nWord = htonl(word);
        insert((unsigned char*)&nWord, 4, toPosition);
    }

    void
        COutPacketBuffer::extract(unsigned char* to, unsigned numBytes, unsigned fromPosition)
    {
        unsigned realFromPosition = m_PacketStart + fromPosition;
        if (realFromPosition + numBytes > m_Limit)
        { // sanity check
            if (realFromPosition > m_Limit) return; // we can't do this
            numBytes = m_Limit - realFromPosition;
        }

        memmove(to, &m_Buf[realFromPosition], numBytes);
    }

    unsigned
        COutPacketBuffer::extractWord(unsigned fromPosition)
    {
        unsigned nWord;
        extract((unsigned char*)&nWord, 4, fromPosition);
        return ntohl(nWord);
    }

    void
        COutPacketBuffer::skipBytes(unsigned numBytes)
    {
        if (numBytes > totalBytesAvailable())
        {
            numBytes = totalBytesAvailable();
        }

        increment(numBytes);
    }

    ////////// CRTCPMemberDatabase //////////
    class CRTCPMemberDatabase
    {
    public:
        CRTCPMemberDatabase(CRTCPInstance& ourRTCPInstance)
            : m_OurRTCPInstance(ourRTCPInstance), m_NumMembers(1),
            m_Table(CHashTable::create(ONE_WORD_HASH_KEYS)) {}

        virtual ~CRTCPMemberDatabase()
        {
            delete m_Table;
        }

        bool isMember(unsigned ssrc) const
        {
            return m_Table->Lookup((char*)(long)ssrc) != NULL;
        }

        bool noteMembership(unsigned ssrc, unsigned curTimeCount)
        {
            bool isNew = !isMember(ssrc);

            if (isNew)
            {
                ++m_NumMembers;
            }

            m_Table->Add((char*)(long)ssrc, (void*)(long)curTimeCount);

            return isNew;
        }

        bool remove(unsigned ssrc)
        {
            bool wasPresent = m_Table->Remove((char*)(long)ssrc);
            if (wasPresent)
            {
                --m_NumMembers;
            }
            return wasPresent;
        }

        unsigned numMembers() const
        {
            return m_NumMembers;
        }

        void reapOldMembers(unsigned threshold);

    private:
        CRTCPInstance&  m_OurRTCPInstance;
        unsigned			m_NumMembers;
        CHashTable*	   m_Table;
    };

    /********************************************************************************/
    /********************************************************************************/
    void
        CRTCPMemberDatabase::reapOldMembers(unsigned threshold)
    {
        bool foundOldMember;
        unsigned oldSSRC = 0;

        do
        {
            foundOldMember = false;

            CHashTable::Iterator* iter = CHashTable::Iterator::create(*m_Table);

            unsigned long timeCount;
            char const* key;
            while ((timeCount = (unsigned long)(iter->next(key))) != 0)
            {
                if (timeCount < (unsigned long)threshold)
                { // this SSRC is old
                    unsigned long ssrc = (unsigned long)key;
                    oldSSRC = (unsigned)ssrc;
                    foundOldMember = true;
                }
            }
            delete iter;

            if (foundOldMember)
            {
                m_OurRTCPInstance.removeSSRC(oldSSRC);
            }
        } while (foundOldMember);
    }


    ////////// CRTCPInstance //////////
    static double dTimeNow()
    {
        struct timeval timeNow;
        gettimeofday(&timeNow, NULL);
        return (double)(timeNow.tv_sec + timeNow.tv_usec / 1000000.0);
    }

    static unsigned const maxPacketSize = 1450;
    static unsigned const preferredPacketSize = 1000;

    CRTCPInstance::CRTCPInstance(CUsageEnvironment* inEnv,
        CIoSocket* rtcpIoSock,
        unsigned totSessionBW,
        unsigned char const* cname,
        CRTPSource const* source)
        : CMedium(inEnv), m_TotSessionBW(totSessionBW),
        m_Source(source),
        m_CNAME(RTCP_SDES_CNAME, cname), m_OutgoingReportCount(1),
        m_AveRTCPSize(0), m_IsInitial(1), m_PrevNumMembers(0),
        m_LastSentSize(0), m_LastReceivedSize(0), m_LastReceivedSSRC(0),
        m_TypeOfEvent(EVENT_UNKNOWN), m_TypeOfPacket(PACKET_UNKNOWN_TYPE),
        m_HaveJustSentPacket(false), m_LastPacketSentSize(0),
        m_ByeHandlerTask(NULL), m_ByeHandlerClientData(NULL)
    {
        m_RTCPInterface = new CRTPInterface(this, rtcpIoSock);

        double timeNow = dTimeNow();
        m_PrevReportTime = m_NextReportTime = timeNow;

        m_KnownMembers = new CRTCPMemberDatabase(*this);
        m_InBuf = new unsigned char[maxPacketSize];
        if (m_KnownMembers == NULL || m_InBuf == NULL)
        {
            env()->setResultMsg(11000602, "m_KnownMembers or m_InBuf Alloc Error!");
            return;
        }

        m_OutBuf = new COutPacketBuffer(maxPacketSize);
        if (m_OutBuf == NULL) return;

        m_RtcpSocket = rtcpIoSock->socketNum();
        m_TaskScheduler = new CTaskScheduler(inEnv);
        m_ExitReportThreadFlag = false;
        /*
        #ifdef _WIN32
        unsigned int theThreadID;
        m_hReportThread = (HANDLE)_beginthreadex( NULL, 0, (PTHREAD_START)incomingReportHandler,this, 0, &theThreadID );
        #else
        pthread_create( &m_hReportThread , NULL, (PTHREAD_START)incomingReportHandler, (void*)this );
        #endif

        m_TypeOfEvent = EVENT_REPORT;
        onExpire(this);
        */
    }

    CRTCPInstance::~CRTCPInstance()
    {
        if (m_hReportThread)
        {
            if (!m_ExitReportThreadFlag)
            {
                m_ExitReportThreadFlag = true;
#ifdef _WIN32
                WaitForSingleObject(m_hReportThread, INFINITE);
#else
                pthread_join(m_hReportThread,NULL);
#endif		
            }
#ifdef _WIN32		
            ::CloseHandle(m_hReportThread);
#endif		
            m_hReportThread = NULL;
        }

        /*
            m_TypeOfEvent = EVENT_BYE; // not used, but...
            sendBYE();
            */
        if (m_KnownMembers != NULL)
            delete m_KnownMembers;
        if (m_OutBuf != NULL)
            delete m_OutBuf;
        if (m_InBuf != NULL)
            delete[] m_InBuf;

        if (m_RTCPInterface != NULL) delete m_RTCPInterface;

        if (m_TaskScheduler)
            delete m_TaskScheduler;
    }

    short
        CRTCPInstance::StartRtcping()
    {
        m_ExitReportThreadFlag = false;
        m_hReportThread = NULL;
#ifdef _WIN32	
        unsigned int theThreadID;
        m_hReportThread = (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)incomingReportHandler, this, 0, &theThreadID);
#else
        pthread_create( &m_hReportThread , NULL, (PTHREAD_START)incomingReportHandler, (void*)this );		
#endif

        m_TypeOfEvent = EVENT_REPORT;
        onExpire(this);

        return 0;
    }

    short
        CRTCPInstance::StopRtcping()
    {
        if (m_hReportThread != NULL)
        {
            if (!m_ExitReportThreadFlag)
            {
                m_ExitReportThreadFlag = true;
#ifdef _WIN32
                ::WaitForSingleObject(m_hReportThread, INFINITE);
#else
                ::pthread_join(m_hReportThread,NULL);
#endif			
            }
#ifdef _WIN32		
            ::CloseHandle(m_hReportThread);
#endif		
            m_hReportThread = NULL;
        }

        m_TypeOfEvent = EVENT_BYE; // not used, but...
        sendBYE();

        return 0;
    }

    /**********************************************************************/
    /**********************************************************************/
    CRTCPInstance*
        CRTCPInstance::createNew(CUsageEnvironment* inEnv,
        CIoSocket* rtcpIoSock,
        unsigned totSessionBW,
        unsigned char const* cname,
        CRTPSource const* source)
    {
        return new CRTCPInstance(inEnv, rtcpIoSock, totSessionBW, cname, source);
    }

    /**********************************************************************/
    /*
    */
    /**********************************************************************/
    bool
        CRTCPInstance::lookupByName(CUsageEnvironment* inEnv,
        char const* instanceName,
        CRTCPInstance*& resultInstance)
    {
        resultInstance = NULL; // unless we succeed

        CMedium* medium;
        if (!CMedium::lookupByName(inEnv, instanceName, medium)) return false;

        if (!medium->isRTCPInstance())
        {
            inEnv->setResultMsg(11000108, instanceName, " is not a RTCP instance");
            return false;
        }

        resultInstance = (CRTCPInstance*)medium;
        return true;
    }

    bool
        CRTCPInstance::isRTCPInstance() const
    {
        return true;
    }
    /**********************************************************************/
    /*
    */
    /**********************************************************************/
    unsigned
        CRTCPInstance::numMembers() const
    {
        if (m_KnownMembers == NULL) return 0;

        return m_KnownMembers->numMembers();
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::setByeHandler(TaskFunc* handlerTask, void* clientData, bool handleActiveParticipantsOnly)
    {
        m_ByeHandlerTask = handlerTask;
        m_ByeHandlerClientData = clientData;
        m_ByeHandleActiveParticipantsOnly = handleActiveParticipantsOnly;
    }

    static unsigned const IP_UDP_HDR_SIZE = 28;

#define ADVANCE(n) pkt += (n); packetSize -= (n)

#ifdef _WIN32
    void
#else
    void*
#endif
        CRTCPInstance::incomingReportHandler(void* inData)
    {
        CRTCPInstance* instance = (CRTCPInstance*)inData;
        instance->incomingReportHandler1();
#ifndef _WIN32
        return NULL;
#endif	
    }

    void
        CRTCPInstance::incomingReportHandler1()
    {
        struct timeval theTimeVal;
        int theMaxNumSocket;
        fd_set theReadSet;
        FD_ZERO(&theReadSet);
        theMaxNumSocket = m_RtcpSocket + 1;
        theTimeVal.tv_sec = 0;
        theTimeVal.tv_usec = 20000;

        while (!m_ExitReportThreadFlag)
        {
            if (m_TaskScheduler) m_TaskScheduler->SingleStep(100);

            FD_SET((unsigned)m_RtcpSocket, &theReadSet);
            int theSelectResult = select(theMaxNumSocket, &theReadSet, NULL, NULL, &theTimeVal);

            if (theSelectResult == 0)
                continue;
            else if (theSelectResult < 0)
            {
                int theErr = WSAGetLastError();
                continue;
            }

            if (!FD_ISSET(m_RtcpSocket, &theReadSet))
                continue;

            unsigned char* pkt = m_InBuf;
            unsigned packetSize;
            struct sockaddr_in fromAddress;
            int typeOfPacket = PACKET_UNKNOWN_TYPE;
            int theResult;
            do
            {
                if ((theResult = m_RTCPInterface->handleRead(pkt, maxPacketSize, packetSize, fromAddress, NULL)) < 0)
                {
                    if (theResult == -2)
                    {
                        m_ExitReportThreadFlag = true;
                        //env()->EnqueueEvent(kVODNet_Event_LineFailed);
                    }
                    break;
                }
                if (packetSize == 0)
                    break;

                int totPacketSize = IP_UDP_HDR_SIZE + packetSize;

                if (packetSize < 4 || packetSize > 1450) break;
                unsigned rtcpHdr = ntohl(*(unsigned*)pkt);
                if ((rtcpHdr & 0xE0FE0000) != (0x80000000 | (RTCP_PT_SR << 16)))
                {
                    break;
                }

                unsigned reportSenderSSRC = 0;
                bool packetOK = false;
                while (1)
                {
                    unsigned rc = (rtcpHdr >> 24) & 0x1F;
                    unsigned pt = (rtcpHdr >> 16) & 0xFF;
                    unsigned length = 4 * (rtcpHdr & 0xFFFF);

                    ADVANCE(4);
                    if (length > packetSize) break;

                    if (length < 4) break;
                    length -= 4;

                    reportSenderSSRC = ntohl(*(unsigned*)pkt); ADVANCE(4);

                    bool subPacketOK = false;
                    switch (pt)
                    {
                    case RTCP_PT_SR:
                    {
                        if (length < 20) break;
                        length -= 20;

                        unsigned NTPmsw = ntohl(*(unsigned*)pkt); ADVANCE(4);
                        unsigned NTPlsw = ntohl(*(unsigned*)pkt); ADVANCE(4);
                        unsigned rtpTimestamp = ntohl(*(unsigned*)pkt); ADVANCE(4);
                        if (m_Source != NULL)
                        {
                            CRTPReceptionStatsDB& receptionStats
                                = m_Source->receptionStatsDB();
                            receptionStats.noteIncomingSR(reportSenderSSRC, NTPmsw, NTPlsw, rtpTimestamp);
                        }

                        ADVANCE(8);

                        unsigned reportBlocksSize = rc*(6 * 4);
                        if (length < reportBlocksSize) break;
                        length -= reportBlocksSize;

                        ADVANCE(reportBlocksSize);
                        subPacketOK = true;
                        typeOfPacket = PACKET_RTCP_REPORT;
                        break;
                    }
                    case RTCP_PT_BYE:
                    {
                        unsigned theByeReason;

                        TaskFunc* byeHandler = m_ByeHandlerTask;
                        if (byeHandler != NULL
                            && (!m_ByeHandleActiveParticipantsOnly
                            || (m_Source != NULL
                            && m_Source->receptionStatsDB().LookUp(reportSenderSSRC) != NULL)))
                        {
                            m_ByeHandlerTask = NULL;

                            (*byeHandler)(m_ByeHandlerClientData);
                        }

                        theByeReason = ntohl(*(unsigned*)pkt); ADVANCE(4);
                        length -= 4;
                        //theByeReason = *(unsigned*)pkt; ADVANCE(4);
                        env()->EnqueueEvent(kVODNet_Event_ServerBye, long(theByeReason));

                        subPacketOK = true;
                        typeOfPacket = PACKET_BYE;
                        break;
                    }

                    default:
                        subPacketOK = true;
                        break;
                    }
                    if (!subPacketOK) break;

                    ADVANCE(length);

                    if (packetSize == 0)
                    {
                        packetOK = true;
                        break;
                    }
                    else if (packetSize < 4 || packetSize > 1450)
                    {
                        break;
                    }

                    rtcpHdr = ntohl(*(unsigned*)pkt);
                    if ((rtcpHdr & 0xC0000000) != 0x80000000)
                    {
                        break;
                    }
                }

                if (!packetOK)
                {
                    break;
                }
                else
                {
                }

                onReceive(typeOfPacket, totPacketSize, reportSenderSSRC);
            } while (0);
        }

        if (m_TaskScheduler)
            m_TaskScheduler->unscheduleDelayedTask(nextTask());
    }

    void
        CRTCPInstance::onReceive(int typeOfPacket, int totPacketSize, unsigned ssrc)
    {
        m_TypeOfPacket = typeOfPacket;
        m_LastReceivedSize = totPacketSize;
        m_LastReceivedSSRC = ssrc;

        int members = (int)numMembers();
        int senders = 0;

        OnReceive(this, // p
            this, // e
            &members, // members
            &m_PrevNumMembers, // pmembers
            &senders, // senders
            &m_AveRTCPSize, // avg_rtcp_size
            &m_PrevReportTime, // tp
            dTimeNow(), // tc
            m_NextReportTime);
    }

    void
        CRTCPInstance::sendReport()
    {
        addReport();

        addSDES();

        sendBuiltPacket();

        const unsigned membershipReapPeriod = 5;
        if ((++m_OutgoingReportCount) % membershipReapPeriod == 0)
        {
            unsigned threshold = m_OutgoingReportCount - membershipReapPeriod;
            m_KnownMembers->reapOldMembers(threshold);
        }
    }

    void
        CRTCPInstance::sendBYE()
    {
        addReport();

        addBYE();
        sendBuiltPacket();
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::sendBuiltPacket()
    {
        unsigned reportSize = m_OutBuf->curPacketSize();

        if (m_RTCPInterface->sendPacket(m_OutBuf->packet(), reportSize) == false)
        {
            // 2010318
            //env()->EnqueueEvent(kVODNet_Event_LineFailed);
        }

        m_OutBuf->resetOffset();

        m_LastSentSize = IP_UDP_HDR_SIZE + reportSize;
        m_HaveJustSentPacket = true;
        m_LastPacketSentSize = reportSize;
    }

    /**********************************************************************/
    /*
    *
    /**********************************************************************/
    int
        CRTCPInstance::checkNewSSRC()
    {
        return m_KnownMembers->noteMembership(m_LastReceivedSSRC, m_OutgoingReportCount);
    }

    /**********************************************************************/
    /*
    /*
    /**********************************************************************/
    void
        CRTCPInstance::removeLastReceivedSSRC()
    {
        removeSSRC(m_LastReceivedSSRC);
    }

    void
        CRTCPInstance::removeSSRC(uint32_t ssrc)
    {
        m_KnownMembers->remove(ssrc);

        // Also, remove records of this SSRC from any reception or transmission stats
        if (m_Source != NULL) m_Source->receptionStatsDB().removeRecord(ssrc);
    }

    /**********************************************************************/
    /*
    /*
    /**********************************************************************/
    void
        CRTCPInstance::onExpire(CRTCPInstance* instance)
    {
        instance->onExpire1();
    }

    void
        CRTCPInstance::onExpire1()
    {
        // Note: m_TotSessionBW is kbits per second
        double rtcpBW = 0.05*m_TotSessionBW * 1024 / 8; // -> bytes per second

        OnExpire(this, // event
            numMembers(), // members
            0, // senders
            rtcpBW, // rtcp_bw
            0, // we_sent
            &m_AveRTCPSize, // ave_rtcp_size
            &m_IsInitial, // initial
            dTimeNow(), // tc
            &m_PrevReportTime, // tp
            &m_PrevNumMembers // pmembers
            );
    }

    void
        CRTCPInstance::addReport()
    {
        if (m_Source != NULL)
        {
            addRR();
        }
    }


    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::addRR()
    {
        // ASSERT: m_Source != NULL

        enqueueCommonReportPrefix(RTCP_PT_RR, m_Source->SSRC());
        enqueueCommonReportSuffix();
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::enqueueCommonReportPrefix(unsigned char packetType, unsigned SSRC, unsigned numExtraWords)
    {
        unsigned numReportingSources;
        if (m_Source == NULL)
        {
            numReportingSources = 0;
        }
        else
        {//
            CRTPReceptionStatsDB& allReceptionStats
                = m_Source->receptionStatsDB();
            numReportingSources = allReceptionStats.numActiveSourcesSinceLastReset();

            if (numReportingSources >= 32) { numReportingSources = 32; }
        }

        unsigned rtcpHdr = 0x80000000; // version 2, no padding
        rtcpHdr |= (numReportingSources << 24);
        rtcpHdr |= (packetType << 16);
        rtcpHdr |= (1 + numExtraWords + 6 * numReportingSources);

        m_OutBuf->enqueueWord(rtcpHdr);

        m_OutBuf->enqueueWord(SSRC);
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::enqueueCommonReportSuffix()
    {
        // Output the report blocks for each source:
        if (m_Source != NULL)
        {
            CRTPReceptionStatsDB& allReceptionStats
                = m_Source->receptionStatsDB();

            CRTPReceptionStatsDB::Iterator iterator(allReceptionStats);
            while (1)
            {
                CRTPReceptionStats* receptionStats = iterator.next();
                if (receptionStats == NULL) break;
                enqueueReportBlock(receptionStats);
            }

            allReceptionStats.reset();
        }
    }

    void
        CRTCPInstance::enqueueReportBlock(CRTPReceptionStats* stats)
    {
        m_OutBuf->enqueueWord(stats->SSRC());

        unsigned highestExtSeqNumReceived = stats->highestExtSeqNumReceived();

        unsigned totNumExpected
            = highestExtSeqNumReceived - stats->baseExtSeqNumReceived();
        int totNumLost = totNumExpected - stats->totNumPacketsReceived();
        // 'Clamp' this loss number to a 24-bit signed value:
        if (totNumLost > 0x007FFFFF)
        {
            totNumLost = 0x007FFFFF;
        }
        else if (totNumLost < 0)
        {
            if (totNumLost < -0x00800000) totNumLost = 0x00800000; // unlikely, but...
            totNumLost &= 0x00FFFFFF;
        }

        unsigned numExpectedSinceLastReset
            = highestExtSeqNumReceived - stats->lastResetExtSeqNumReceived();
        int numLostSinceLastReset
            = numExpectedSinceLastReset - stats->numPacketsReceivedSinceLastReset();
        unsigned char lossFraction;
        if (numExpectedSinceLastReset == 0 || numLostSinceLastReset < 0)
        {
            lossFraction = 0;
        }
        else
        {
            lossFraction = (unsigned char)
                ((numLostSinceLastReset << 8) / numExpectedSinceLastReset);
        }

        m_OutBuf->enqueueWord((lossFraction << 24) | totNumLost);
        m_OutBuf->enqueueWord(highestExtSeqNumReceived);

        m_OutBuf->enqueueWord(unsigned(stats->jitter()));

        unsigned NTPmsw = stats->lastReceivedSR_NTPmsw();
        unsigned NTPlsw = stats->lastReceivedSR_NTPlsw();
        unsigned LSR = ((NTPmsw & 0xFFFF) << 16) | (NTPlsw >> 16); // middle 32 bits
        m_OutBuf->enqueueWord(LSR);

        // Figure out how long has elapsed since the last SR rcvd from this src:
        struct timeval const& LSRtime = stats->lastReceivedSR_time(); // "last SR"
        struct timeval timeNow, timeSinceLSR;
        gettimeofday(&timeNow, NULL);
        if (timeNow.tv_usec < LSRtime.tv_usec)
        {
            timeNow.tv_usec += 1000000;
            timeNow.tv_sec -= 1;
        }
        timeSinceLSR.tv_sec = timeNow.tv_sec - LSRtime.tv_sec;
        timeSinceLSR.tv_usec = timeNow.tv_usec - LSRtime.tv_usec;
        // The enqueued time is in units of 1/65536 seconds.
        // (Note that 65536/1000000 == 1024/15625) 
        unsigned DLSR;
        if (LSR == 0)
        {
            DLSR = 0;
        }
        else
        {
            DLSR = (timeSinceLSR.tv_sec << 16)
                | ((((timeSinceLSR.tv_usec << 11) + 15625) / 31250) & 0xFFFF);
        }
        m_OutBuf->enqueueWord(DLSR);
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::addSDES()
    {
        // Begin by figuring out the size of the entire SDES report:
        unsigned numBytes = 4;
        // counts the SSRC, but not the header; it'll get subtracted out
        numBytes += m_CNAME.totalSize(); // includes id and length
        numBytes += 1; // the special END item

        unsigned num4ByteWords = (numBytes + 3) / 4;

        unsigned rtcpHdr = 0x81000000; // version 2, no padding, 1 SSRC chunk
        rtcpHdr |= (RTCP_PT_SDES << 16);
        rtcpHdr |= num4ByteWords;
        m_OutBuf->enqueueWord(rtcpHdr);

        if (m_Source != NULL)
        {
            m_OutBuf->enqueueWord(m_Source->SSRC());
        }

        // Add the CNAME:
        m_OutBuf->enqueue(m_CNAME.data(), m_CNAME.totalSize());

        // Add the 'END' item (i.e., a zero byte), plus any more needed to pad:
        unsigned numPaddingBytesNeeded = 4 - (m_OutBuf->curPacketSize() % 4);
        unsigned char const zero = '\0';
        while (numPaddingBytesNeeded-- > 0) m_OutBuf->enqueue(&zero, 1);
    }

    /**********************************************************************/
    /**********************************************************************/
    void
        CRTCPInstance::addBYE()
    {
        unsigned rtcpHdr = 0x81000000; // version 2, no padding, 1 SSRC
        rtcpHdr |= (RTCP_PT_BYE << 16);
        rtcpHdr |= 1; // 2 32-bit words total (i.e., with 1 SSRC)
        m_OutBuf->enqueueWord(rtcpHdr);

        if (m_Source != NULL)
        {
            m_OutBuf->enqueueWord(m_Source->SSRC());
        }
    }

    void
        CRTCPInstance::schedule(double nextTime)
    {
        m_NextReportTime = nextTime;

        double secondsToDelay = nextTime - dTimeNow();

        int usToGo = (int)(secondsToDelay * 1000000);
        nextTask() = m_TaskScheduler->scheduleDelayedTask(usToGo, (TaskFunc*)CRTCPInstance::onExpire, this);
    }

    void
        CRTCPInstance::reschedule(double nextTime)
    {
        m_TaskScheduler->unscheduleDelayedTask(nextTask());
        schedule(nextTime);
    }

    ////////// CSDESItem //////////
    CSDESItem::CSDESItem(unsigned char tag, unsigned char const* value)
    {
        unsigned length = strlen((char const*)value);
        if (length > 511) length = 511;

        m_Data[0] = tag;
        m_Data[1] = (unsigned char)length;
        memmove(&m_Data[2], value, length);

        while ((length) % 4 > 0) m_Data[2 + length++] = '\0';
    }

    unsigned CSDESItem::totalSize() const
    {
        return 2 + (unsigned)m_Data[1];
    }


    void Schedule(double nextTime, event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return;

        instance->schedule(nextTime);
    }

    void Reschedule(double nextTime, event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return;

        instance->reschedule(nextTime);
    }

    void SendRTCPReport(event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return;

        instance->sendReport();
    }

    void SendBYEPacket(event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return;

        instance->sendBYE();
    }

    int TypeOfEvent(event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return EVENT_UNKNOWN;

        return instance->typeOfEvent();
    }

    int SentPacketSize(event e)
    {
        CRTCPInstance* instance = (CRTCPInstance*)e;
        if (instance == NULL) return 0;

        return instance->sentPacketSize();
    }

    int PacketType(packet p)
    {
        CRTCPInstance* instance = (CRTCPInstance*)p;
        if (instance == NULL) return PACKET_UNKNOWN_TYPE;

        return instance->packetType();
    }

    int ReceivedPacketSize(packet p)
    {
        CRTCPInstance* instance = (CRTCPInstance*)p;
        if (instance == NULL) return 0;

        return instance->receivedPacketSize();
    }

    int NewMember(packet p)
    {
        CRTCPInstance* instance = (CRTCPInstance*)p;
        if (instance == NULL) return 0;

        return instance->checkNewSSRC();
    }

    int NewSender(packet /*p*/)
    {
        return 0;
    }

    void AddMember(packet /*p*/)
    {
    }

    void AddSender(packet /*p*/)
    {
    }

    void RemoveMember(packet p)
    {
        CRTCPInstance* instance = (CRTCPInstance*)p;
        if (instance == NULL) return;

        instance->removeLastReceivedSSRC();
    }

    void RemoveSender(packet /*p*/)
    {
    }

    double drand30()
    {
        unsigned tmp = our_random() & 0x3FFFFFFF; // a random 30-bit integer
        return tmp / (double)(1024 * 1024 * 1024);
    }
