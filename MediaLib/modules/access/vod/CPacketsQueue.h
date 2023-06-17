
#ifndef __CPacketsQueue_VOD_H__
#define __CPacketsQueue_VOD_H__

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#endif


    class CPacketsQueue
    {
    public:
        CPacketsQueue();
        virtual ~CPacketsQueue();

        short	Enqueue(unsigned char* inPkts, long inPktsSize, unsigned short inSeqNo, unsigned long inRtpTimeStemp);
        short	Dequeue(unsigned char* outBuf, long outBufSize, long& outDequeueSize, unsigned short& outSeqNo, unsigned long& outRtpTimeStemp);
        void	ResetQueue();
        void	GetReceiveSize(long* outSize, unsigned long* outRTPDeltaTime);
        unsigned long	GetTsCount() { return m_TsCount; }
        unsigned long	GetGetIndex() { return m_GetIndex; }
        unsigned long	GetPutIndex() { return m_PutIndex; }

        long	CheckState();

        long	m_TsCount;
        long	m_ChannelID;
        long	m_ChannelPID;

    private:
        unsigned char*	m_Buffer;
        long	m_BufMaxSize;
        long	m_PutIndex;
        long	m_GetIndex;

        long	m_LastPutIndex;
        long	m_LastGetIndex;
        long	m_PutActionFlag;

        unsigned long		m_FirstRtpTime;
        unsigned long		m_LastRtpTime;


#ifdef _WIN32
        HANDLE  			m_hMutex;
#else
        pthread_mutex_t	m_hMutex;
#endif	
    };

#endif//__CPacketsQueue_VOD_H__