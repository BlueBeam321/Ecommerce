
#ifndef __CRTPSource_VOD_H__
#define __CRTPSource_VOD_H__

#include "CMedia.h"
#include "CHashTable.h"
#include "Sockets.h"

    class CRTPReceptionStatsDB;

    class CRTPSource : public CMedium
    {
    public:
        CRTPSource(CUsageEnvironment* inEnv, CIoSocket* rtpIoSock, unsigned char rtpPayloadFormat, uint32_t rtpTimestampFrequency, unsigned long inSSRC);
        virtual ~CRTPSource();

        static bool		lookupByName(CUsageEnvironment* inEnv, char const* sourceName, CRTPSource*& resultSource);

        uint16_t		curPacketRTPSeqNum() const { return m_CurPacketRTPSeqNum; }
        uint32_t		curPacketRTPTimestamp() const { return m_CurPacketRTPTimestamp; }
        bool		   curPacketMarkerBit() const { return m_CurPacketMarkerBit; }
        unsigned char  rtpPayloadFormat() const { return m_RTPPayloadFormat; }
        CIoSocket*	  RTPioSock() const { return m_RTPInterface->getIoSock(); }
        uint32_t		SSRC() const { return m_SSRC; }
        unsigned		timestampFrequency() const { return m_TimestampFrequency; }
        CRTPReceptionStatsDB& receptionStatsDB() const { return *m_ReceptionStatsDB; }
        uint32_t		lastReceivedSSRC() const { return m_LastReceivedSSRC; }

        virtual bool	hasBeenSynchronizedUsingRTCP();
        virtual void	setPacketReorderingThresholdTime(unsigned uSeconds) = 0;
        virtual void WaitRecvReady();
    protected:
        CRTPInterface* m_RTPInterface;
        uint16_t		m_CurPacketRTPSeqNum;
        uint32_t		m_CurPacketRTPTimestamp;
        bool		   m_CurPacketMarkerBit;
        bool		   m_CurPacketHasBeenSynchronizedUsingRTCP;
        uint32_t		m_LastReceivedSSRC;

    private:
        virtual bool isRTPSource() const;
        virtual void getAttributes() const;

        unsigned char	m_RTPPayloadFormat;
        unsigned		m_TimestampFrequency;
        uint32_t		 m_SSRC;

        CRTPReceptionStatsDB* m_ReceptionStatsDB;
    };

    class CRTPReceptionStats;

    class CRTPReceptionStatsDB
    {
    public:
        unsigned	totNumPacketsReceived() const { return m_TotNumPacketsReceived; }
        unsigned	numActiveSourcesSinceLastReset() const { return m_NumActiveSourcesSinceLastReset; }

        void		reset();

        class Iterator
        {
        public:
            Iterator(CRTPReceptionStatsDB& receptionStatsDB);
            virtual ~Iterator();

            CRTPReceptionStats* next(bool includeInactiveSources = false);

        private:
            CHashTable::Iterator* m_Iter;
        };

        void noteIncomingPacket(uint32_t SSRC, uint16_t seqNum,
            uint32_t rtpTimestamp,
            unsigned timestampFrequency,
            bool useForJitterCalculation,
        struct timeval& resultPresentationTime,
            bool& resultHasBeenSyncedUsingRTCP,
            unsigned packetSize);

        void noteIncomingSR(uint32_t SSRC, uint32_t ntpTimestampMSW, uint32_t ntpTimestampLSW, uint32_t rtpTimestamp);

        void removeRecord(uint32_t SSRC);
        double GetJitter(uint32_t inServerSSRC);
        CRTPReceptionStats* LookUp(uint32_t SSRC);

    private:
        friend class CRTPSource;
        CRTPReceptionStatsDB(CRTPSource& rtpSource);
        virtual ~CRTPReceptionStatsDB();

        CRTPReceptionStats* lookup(uint32_t SSRC) const;
        void add(uint32_t SSRC, CRTPReceptionStats* stats);

        friend class Iterator;
        CRTPSource&	  m_OurRTPSource;
        CHashTable*	   m_Table;
        unsigned			m_NumActiveSourcesSinceLastReset;
        unsigned			m_TotNumPacketsReceived;

#ifdef _WIN32
        HANDLE			m_hMutex;
#else
        pthread_mutex_t	m_hMutex;
#endif
    };

    class CRTPReceptionStats
    {
    public:
        uint32_t SSRC() const { return m_SSRC; }
        unsigned	numPacketsReceivedSinceLastReset() const { return m_NumPacketsReceivedSinceLastReset; }
        unsigned	totNumPacketsReceived() const { return m_TotNumPacketsReceived; }
        double	totNumKBytesReceived() const;

        unsigned	totNumPacketsExpected() const { return m_HighestExtSeqNumReceived - m_BaseExtSeqNumReceived; }

        unsigned	baseExtSeqNumReceived() const { return m_BaseExtSeqNumReceived; }
        unsigned	lastResetExtSeqNumReceived() const { return m_LastResetExtSeqNumReceived; }
        unsigned	highestExtSeqNumReceived() const { return m_HighestExtSeqNumReceived; }

        double	jitter() const;

        unsigned	lastReceivedSR_NTPmsw() const { return m_LastReceivedSR_NTPmsw; }
        unsigned	lastReceivedSR_NTPlsw() const { return m_LastReceivedSR_NTPlsw; }
        struct timeval const& lastReceivedSR_time() const { return m_LastReceivedSR_time; }

        unsigned	minInterPacketGapUS() const { return m_MinInterPacketGapUS; }
        unsigned	maxInterPacketGapUS() const { return m_MaxInterPacketGapUS; }
        struct timeval const& totalInterPacketGaps() const { return m_TotalInterPacketGaps; }

    private:
        friend class CRTPReceptionStatsDB;
        CRTPReceptionStats(CRTPSource& rtpSource, uint32_t SSRC, uint16_t initialSeqNum);
        CRTPReceptionStats(CRTPSource& rtpSource, uint32_t SSRC);
        virtual ~CRTPReceptionStats();

        void noteIncomingPacket(uint16_t seqNum, uint32_t rtpTimestamp,
            unsigned timestampFrequency,
            bool useForJitterCalculation,
        struct timeval& resultPresentationTime,
            bool& resultHasBeenSyncedUsingRTCP,
            unsigned packetSize /* payload only */);
        void noteIncomingSR(uint32_t ntpTimestampMSW, uint32_t ntpTimestampLSW, uint32_t rtpTimestamp);

        void init(uint32_t SSRC);
        void initSeqNum(uint16_t initialSeqNum);
        void reset();

    private:
        CRTPSource&	  m_OurRTPSource;
        uint32_t			 m_SSRC;
        unsigned			m_NumPacketsReceivedSinceLastReset;
        unsigned			m_TotNumPacketsReceived;
        uint32_t			 m_TotBytesReceived_hi, m_TotBytesReceived_lo;
        bool				m_HaveSeenInitialSequenceNumber;
        unsigned			m_BaseExtSeqNumReceived;
        unsigned			m_LastResetExtSeqNumReceived;
        unsigned			m_HighestExtSeqNumReceived;
        int				 m_LastTransit;
        uint32_t			 m_PreviousPacketRTPTimestamp;
        double			m_Jitter;

        unsigned			m_LastReceivedSR_NTPmsw;
        unsigned			m_LastReceivedSR_NTPlsw;
        struct timeval	 m_LastReceivedSR_time;
        struct timeval	 m_LastPacketReceptionTime;
        unsigned			m_MinInterPacketGapUS, m_MaxInterPacketGapUS;
        struct timeval	 m_TotalInterPacketGaps;

        bool				m_HasBeenSynchronized;
        uint32_t			 m_SyncTimestamp;
        struct timeval	  m_SyncTime;
    };


    bool vod_seqNumLT(uint16_t s1, uint16_t s2);
    // a 'less-than' on 16-bit sequence numbers

#endif//__CRTPSource_VOD_H__