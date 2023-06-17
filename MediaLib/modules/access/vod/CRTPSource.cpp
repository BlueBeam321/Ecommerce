
#include "CRTPSource.h"


    ////////// CRTPSource //////////
    bool CRTPSource::lookupByName(CUsageEnvironment* inEnv, char const* sourceName, CRTPSource*& resultSource)
    {
        resultSource = NULL;

        CMedium* source;
        if (!CMedium::lookupByName(inEnv, sourceName, source)) return false;

        if (!source->isRTPSource())
        {
            inEnv->setResultMsg(11000110, sourceName, " is not a RTP source");
            return false;
        }

        resultSource = (CRTPSource*)source;
        return true;
    }

    bool
        CRTPSource::hasBeenSynchronizedUsingRTCP()
    {
        return m_CurPacketHasBeenSynchronizedUsingRTCP;
    }

    bool
        CRTPSource::isRTPSource() const
    {
        return true;
    }

    CRTPSource::CRTPSource(CUsageEnvironment* inEnv, CIoSocket* rtpIoSock, unsigned char rtpPayloadFormat, uint32_t rtpTimestampFrequency, unsigned long inSSRC)
        : CMedium(inEnv),
        m_CurPacketHasBeenSynchronizedUsingRTCP(false),
        m_RTPPayloadFormat(rtpPayloadFormat),
        m_TimestampFrequency(rtpTimestampFrequency),
        m_SSRC(inSSRC)
    {
        m_RTPInterface = new CRTPInterface(this, rtpIoSock);
        m_ReceptionStatsDB = new CRTPReceptionStatsDB(*this);
    }

    CRTPSource::~CRTPSource()
    {
        if (m_ReceptionStatsDB != NULL)
            delete m_ReceptionStatsDB;
        m_ReceptionStatsDB = NULL;
        if (m_RTPInterface != NULL)
            delete m_RTPInterface;
        m_RTPInterface = NULL;
    }

    void
        CRTPSource::getAttributes() const
    {

    }

    void CRTPSource::WaitRecvReady()
    {
    }

    ////////// CRTPReceptionStatsDB //////////
    CRTPReceptionStatsDB::CRTPReceptionStatsDB(CRTPSource& rtpSource)
        : m_OurRTPSource(rtpSource),
        m_Table(CHashTable::create(ONE_WORD_HASH_KEYS)), m_TotNumPacketsReceived(0)
    {
#ifdef _WIN32
        m_hMutex = ::CreateMutex(NULL, false, NULL);
#else
        pthread_mutex_init( &m_hMutex, NULL );
#endif	
        reset();
    }

    void
        CRTPReceptionStatsDB::reset()
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        m_NumActiveSourcesSinceLastReset = 0;

        Iterator iter(*this);
        CRTPReceptionStats* stats;
        while ((stats = iter.next()) != NULL)
        {
            stats->reset();
        }

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif  
    }

    CRTPReceptionStatsDB::~CRTPReceptionStatsDB()
    {
        CRTPReceptionStats* stats;
        while ((stats = (CRTPReceptionStats*)m_Table->RemoveNext()) != NULL)
        {
            delete stats;
        }

        delete m_Table;

#ifdef _WIN32
        ::CloseHandle(m_hMutex);
#else
        ::pthread_mutex_destroy(&m_hMutex);
#endif	
    }

    double
        CRTPReceptionStatsDB::GetJitter(uint32_t inServerSSRC)
    {
        double theJitter = 1000000.0;

#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        CRTPReceptionStats* theReceptionStats = lookup(inServerSSRC);
        if (theReceptionStats == NULL)
        {
#ifdef _WIN32
            ::ReleaseMutex(m_hMutex);
#else
            ::pthread_mutex_unlock( &m_hMutex );
#endif	
            return theJitter;
        }
        theJitter = (double)theReceptionStats->jitter();
#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	
        return theJitter;
    }

    void
        CRTPReceptionStatsDB::noteIncomingPacket(uint32_t SSRC, uint16_t seqNum,
        uint32_t rtpTimestamp, unsigned timestampFrequency,
        bool useForJitterCalculation,
    struct timeval& resultPresentationTime,
        bool& resultHasBeenSyncedUsingRTCP,
        unsigned packetSize)
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        ++m_TotNumPacketsReceived;

        CRTPReceptionStats* stats = lookup(SSRC);

        if (stats == NULL)
        {
            stats = new CRTPReceptionStats(m_OurRTPSource, SSRC, seqNum);
            if (stats == NULL)
            {
#ifdef _WIN32
                ::ReleaseMutex(m_hMutex);
#else
                ::pthread_mutex_unlock( &m_hMutex );
#endif	
                return;
            }
            add(SSRC, stats);
        }


        if (stats->numPacketsReceivedSinceLastReset() == 0)
        {
            ++m_NumActiveSourcesSinceLastReset;
        }

        stats->noteIncomingPacket(seqNum, rtpTimestamp, timestampFrequency,
            useForJitterCalculation,
            resultPresentationTime,
            resultHasBeenSyncedUsingRTCP,
            packetSize);

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	
    }

    void
        CRTPReceptionStatsDB::noteIncomingSR(uint32_t SSRC, uint32_t ntpTimestampMSW, uint32_t ntpTimestampLSW, uint32_t rtpTimestamp)
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        CRTPReceptionStats* stats = lookup(SSRC);
        if (stats == NULL)
        {
            stats = new CRTPReceptionStats(m_OurRTPSource, SSRC);
            if (stats == NULL)
            {
#ifdef _WIN32
                ::ReleaseMutex(m_hMutex);
#else
                ::pthread_mutex_unlock( &m_hMutex );
#endif	
                return;
            }

            add(SSRC, stats);
        }

        stats->noteIncomingSR(ntpTimestampMSW, ntpTimestampLSW, rtpTimestamp);
#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	
    }

    /**********************************************************************************/
    /**********************************************************************************/
    void
        CRTPReceptionStatsDB::removeRecord(uint32_t SSRC)
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        CRTPReceptionStats* stats = lookup(SSRC);
        if (stats != NULL)
        {
            long SSRC_long = (long)SSRC;
            m_Table->Remove((char const*)SSRC_long);
            delete stats;
        }

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	
    }

    CRTPReceptionStatsDB::Iterator
        ::Iterator(CRTPReceptionStatsDB& receptionStatsDB)
        : m_Iter(CHashTable::Iterator::create(*(receptionStatsDB.m_Table)))
    {
    }

    CRTPReceptionStatsDB::Iterator::~Iterator()
    {
        delete m_Iter;
    }

    CRTPReceptionStats*
        CRTPReceptionStatsDB::Iterator::next(bool includeInactiveSources)
    {
        char const* key;

        CRTPReceptionStats* stats;
        do
        {
            stats = (CRTPReceptionStats*)(m_Iter->next(key));
        } while (stats != NULL && !includeInactiveSources && stats->numPacketsReceivedSinceLastReset() == 0);

        return stats;
    }

    /**********************************************************************************/
    /**********************************************************************************/
    CRTPReceptionStats*
        CRTPReceptionStatsDB::lookup(uint32_t SSRC) const
    {
        long SSRC_long = (long)SSRC;
        return (CRTPReceptionStats*)(m_Table->Lookup((char const*)SSRC_long));
    }

    CRTPReceptionStats*
        CRTPReceptionStatsDB::LookUp(uint32_t SSRC)
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        CRTPReceptionStats* theStats = lookup(SSRC);

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	

        return theStats;
    }

    /**********************************************************************************/
    /*
    */
    /**********************************************************************************/
    void
        CRTPReceptionStatsDB::add(uint32_t SSRC, CRTPReceptionStats* stats)
    {
        long SSRC_long = (long)SSRC;
        m_Table->Add((char const*)SSRC_long, stats);
    }

    ////////// CRTPReceptionStats //////////
    CRTPReceptionStats::CRTPReceptionStats(CRTPSource& rtpSource, uint32_t SSRC, uint16_t initialSeqNum)
        :m_OurRTPSource(rtpSource)
    {
        initSeqNum(initialSeqNum);
        init(SSRC);
    }

    CRTPReceptionStats::CRTPReceptionStats(CRTPSource& rtpSource, uint32_t SSRC)
        : m_OurRTPSource(rtpSource)
    {
        init(SSRC);
    }

    CRTPReceptionStats::~CRTPReceptionStats()
    {
    }

    /************************************************************************************/
    /************************************************************************************/
    void
        CRTPReceptionStats::init(uint32_t SSRC)
    {
        m_SSRC = SSRC;
        m_TotNumPacketsReceived = 0;//
        m_TotBytesReceived_hi = m_TotBytesReceived_lo = 0;
        m_HaveSeenInitialSequenceNumber = false;
        m_LastTransit = ~0;
        m_PreviousPacketRTPTimestamp = 0;
        m_Jitter = 0.0;
        m_LastReceivedSR_NTPmsw = m_LastReceivedSR_NTPlsw = 0;
        m_LastReceivedSR_time.tv_sec = m_LastReceivedSR_time.tv_usec = 0;
        m_LastPacketReceptionTime.tv_sec = m_LastPacketReceptionTime.tv_usec = 0;
        m_MinInterPacketGapUS = 0x7FFFFFFF;
        m_MaxInterPacketGapUS = 0;
        m_TotalInterPacketGaps.tv_sec = m_TotalInterPacketGaps.tv_usec = 0;
        m_HasBeenSynchronized = false;
        m_SyncTime.tv_sec = m_SyncTime.tv_usec = 0;
        reset();
    }

    void
        CRTPReceptionStats::initSeqNum(uint16_t initialSeqNum)
    {
        m_BaseExtSeqNumReceived = initialSeqNum - 1;
        m_HighestExtSeqNumReceived = initialSeqNum;
        m_HaveSeenInitialSequenceNumber = true;
    }

#ifndef MILLION
#define MILLION 1000000
#endif

    /*******************************************************************************/
    /*******************************************************************************/
    void
        CRTPReceptionStats::noteIncomingPacket(uint16_t seqNum, uint32_t rtpTimestamp,
        unsigned timestampFrequency,
        bool useForJitterCalculation,
    struct timeval& resultPresentationTime,
        bool& resultHasBeenSyncedUsingRTCP,
        unsigned packetSize)
    {
        if (!m_HaveSeenInitialSequenceNumber) initSeqNum(seqNum);

        ++m_NumPacketsReceivedSinceLastReset;
        ++m_TotNumPacketsReceived;
        uint32_t prevTotBytesReceived_lo = m_TotBytesReceived_lo;
        m_TotBytesReceived_lo += packetSize;
        if (m_TotBytesReceived_lo < prevTotBytesReceived_lo)
        {
            ++m_TotBytesReceived_hi;
        }

        unsigned seqNumCycle = (m_HighestExtSeqNumReceived & 0xFFFF0000);
        unsigned oldSeqNum = (m_HighestExtSeqNumReceived & 0xFFFF);
        unsigned seqNumDifference = (unsigned)((int)seqNum - (int)oldSeqNum);
        if (seqNumDifference >= 0x8000 && vod_seqNumLT((uint16_t)oldSeqNum, seqNum))
        {
            seqNumCycle += 0x10000;
        }

        unsigned newSeqNum = seqNumCycle | seqNum;
        if (newSeqNum > m_HighestExtSeqNumReceived)
        {
            m_HighestExtSeqNumReceived = newSeqNum;
        }

        struct timeval timeNow;
        gettimeofday(&timeNow, NULL);
        if (m_LastPacketReceptionTime.tv_sec != 0 || m_LastPacketReceptionTime.tv_usec != 0)
        {
            unsigned gap
                = (timeNow.tv_sec - m_LastPacketReceptionTime.tv_sec)*MILLION
                + timeNow.tv_usec - m_LastPacketReceptionTime.tv_usec;
            if (gap > m_MaxInterPacketGapUS)
            {
                m_MaxInterPacketGapUS = gap;
            }
            if (gap < m_MinInterPacketGapUS)
            {
                m_MinInterPacketGapUS = gap;
            }
            m_TotalInterPacketGaps.tv_usec += gap;
            if (m_TotalInterPacketGaps.tv_usec >= MILLION)
            {
                ++m_TotalInterPacketGaps.tv_sec;
                m_TotalInterPacketGaps.tv_usec -= MILLION;
            }
        }
        m_LastPacketReceptionTime = timeNow;

        if (useForJitterCalculation && rtpTimestamp != m_PreviousPacketRTPTimestamp)
        {
            unsigned arrival = (timestampFrequency*timeNow.tv_sec);
            arrival += (unsigned)
                ((2.0*timestampFrequency*timeNow.tv_usec + 1000000.0) / 2000000);
            int transit = arrival - rtpTimestamp;
            if (m_LastTransit == (~0)) m_LastTransit = transit; // hack for first time
            int d = transit - m_LastTransit;
            m_LastTransit = transit;
            if (d < 0) d = -d;
            m_Jitter += (1.0 / 16.0) * ((double)d - m_Jitter);
            //    m_Jitter = ((double)d* 0.5 - m_Jitter * 0.5);
        }

        if (m_SyncTime.tv_sec == 0 && m_SyncTime.tv_usec == 0)
        {
            m_SyncTimestamp = rtpTimestamp;
            m_SyncTime = timeNow;
        }

        int timestampDiff = rtpTimestamp - m_SyncTimestamp;

        double timeDiff
            = timestampDiff / (double)(m_OurRTPSource.timestampFrequency());

        // Add this to the 'sync time' to get our result:
        unsigned const million = 1000000;
        unsigned seconds, uSeconds;
        if (timeDiff >= 0.0)
        {
            seconds = m_SyncTime.tv_sec + (unsigned)(timeDiff);
            uSeconds = m_SyncTime.tv_usec
                + (unsigned)((timeDiff - (unsigned)timeDiff)*million);
            if (uSeconds >= million)
            {
                uSeconds -= million;
                ++seconds;
            }
        }
        else
        {
            timeDiff = -timeDiff;
            seconds = m_SyncTime.tv_sec - (unsigned)(timeDiff);
            uSeconds = m_SyncTime.tv_usec
                - (unsigned)((timeDiff - (unsigned)timeDiff)*million);
            if ((int)uSeconds < 0)
            {
                uSeconds += million;
                --seconds;
            }
        }
        resultPresentationTime.tv_sec = seconds;
        resultPresentationTime.tv_usec = uSeconds;
        resultHasBeenSyncedUsingRTCP = m_HasBeenSynchronized;

        m_SyncTimestamp = rtpTimestamp;
        m_SyncTime = resultPresentationTime;

        m_PreviousPacketRTPTimestamp = rtpTimestamp;
    }

    void
        CRTPReceptionStats::noteIncomingSR(uint32_t ntpTimestampMSW, uint32_t ntpTimestampLSW, uint32_t rtpTimestamp)
    {
        m_LastReceivedSR_NTPmsw = ntpTimestampMSW;
        m_LastReceivedSR_NTPlsw = ntpTimestampLSW;

        gettimeofday(&m_LastReceivedSR_time, NULL);

        m_SyncTimestamp = rtpTimestamp;
        m_SyncTime.tv_sec = ntpTimestampMSW - 0x83AA7E80; // 1/1/1900 -> 1/1/1970
        double microseconds = (ntpTimestampLSW*15625.0) / 0x04000000; // 10^6/2^32
        m_SyncTime.tv_usec = (unsigned)(microseconds + 0.5);
        m_HasBeenSynchronized = true;
    }

    /**********************************************************************************/
    /**********************************************************************************/
    double
        CRTPReceptionStats::totNumKBytesReceived() const
    {
        double const hiMultiplier = 0x20000000 / 125.0; // == (2^32)/(10^3)
        return m_TotBytesReceived_hi*hiMultiplier + m_TotBytesReceived_lo / 1000.0;
    }

    double
        CRTPReceptionStats::jitter() const
    {
        return m_Jitter;
    }

    void
        CRTPReceptionStats::reset()
    {
        m_NumPacketsReceivedSinceLastReset = 0;
        m_LastResetExtSeqNumReceived = m_HighestExtSeqNumReceived;
    }

    bool vod_seqNumLT(uint16_t s1, uint16_t s2)
    {
        // a 'less-than' on 16-bit sequence numbers
        if (s1 < s2) return true;
        return false;
    }

    /*
    bool seqNumLT(uint16_t s1, uint16_t s2)
    {
    // a 'less-than' on 16-bit sequence numbers
    int diff = s2-s1;
    if (diff > 0)
    {
    return (diff < 0x8000);
    }
    else if (diff < 0)
    {
    return (diff < -0x8000);
    }
    else
    { // diff == 0
    return false;
    }
    }*/
