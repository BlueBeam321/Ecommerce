
#include "CPacketsQueue.h"
#include <stdio.h>
#include <string.h>

#define SEVETSPACKETSQUEUESIZE  18000
#define TS_PACKET_SIZE          188
#define TS_PACKETS_PER_NETWORK_PACKET        7

    CPacketsQueue::CPacketsQueue()
    {
        m_TsCount = 0;
        m_BufMaxSize = (TS_PACKET_SIZE * TS_PACKETS_PER_NETWORK_PACKET + 8) * SEVETSPACKETSQUEUESIZE;
        m_Buffer = new unsigned char[m_BufMaxSize];
        m_PutIndex = m_GetIndex = 0;

        m_LastPutIndex = 0;
        m_LastGetIndex = 0;
        m_PutActionFlag = 0;

        m_FirstRtpTime = 0;
        m_LastRtpTime = 0;

#ifdef _WIN32
        m_hMutex = ::CreateMutex(NULL, false, NULL);
#else
        pthread_mutex_init( &m_hMutex, NULL );
#endif	
    }

    CPacketsQueue::~CPacketsQueue()
    {
#ifdef _WIN32
        ::CloseHandle(m_hMutex);
#else
        ::pthread_mutex_destroy( &m_hMutex );
#endif	

        if (m_Buffer)
            delete[] m_Buffer;
    }

    short
        CPacketsQueue::Enqueue(unsigned char* inPkts, long inPktsSize, unsigned short inSeqNo, unsigned long inRtpTimeStemp)
    {
        short theResult = 0;
        long theTmp1, theTmp2;
        long	packetSize = inPktsSize;

        if (inPktsSize < TS_PACKET_SIZE) return -2;

        packetSize += 8;

#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        if (m_GetIndex < m_PutIndex)
        {
            if (m_PutIndex + packetSize < m_BufMaxSize)
            {
            }
            else
            {
                theTmp1 = (m_PutIndex + packetSize) % m_BufMaxSize;
                if (theTmp1 < m_GetIndex)
                {
                }
                else
                {
                    m_GetIndex += (theTmp1 - m_GetIndex) + (TS_PACKET_SIZE * TS_PACKETS_PER_NETWORK_PACKET + 8);
                    m_GetIndex %= m_BufMaxSize;
                }
            }
        }
        else if (m_GetIndex > m_PutIndex)
        {
            if (m_PutIndex + packetSize < m_GetIndex)
            {
            }
            else
            {
                theTmp1 = m_PutIndex + packetSize;
                theTmp1 %= m_BufMaxSize;
                theTmp1 += (TS_PACKET_SIZE * TS_PACKETS_PER_NETWORK_PACKET + 8);
                m_GetIndex = theTmp1 % m_BufMaxSize;
            }
        }
        else
        {// m_GetIndex == m_PutIndex;
        }


        if (m_BufMaxSize >= (m_PutIndex + packetSize))
        {
            *(long*)&m_Buffer[m_PutIndex] = inSeqNo;
            *(unsigned long*)&m_Buffer[m_PutIndex + 4] = inRtpTimeStemp;

            memcpy(&m_Buffer[m_PutIndex + 8], inPkts, inPktsSize);
            m_PutIndex += packetSize;
            if (m_PutIndex == m_BufMaxSize)
                m_PutIndex = 0;
        }
        else
        {
            theTmp1 = m_BufMaxSize - m_PutIndex;

            *(long*)&m_Buffer[m_PutIndex] = inSeqNo;
            *(unsigned long*)&m_Buffer[m_PutIndex + 4] = inRtpTimeStemp;

            memcpy(&m_Buffer[m_PutIndex + 8], inPkts, theTmp1);

            m_PutIndex = 0;
            theTmp2 = inPktsSize - theTmp1;
            memcpy(&m_Buffer[m_PutIndex], &inPkts[theTmp1], theTmp2);
            m_PutIndex = theTmp2;
        }
        m_TsCount += (inPktsSize / TS_PACKET_SIZE);

        if (inRtpTimeStemp)
        {
            if (m_FirstRtpTime == 0)
                m_FirstRtpTime = inRtpTimeStemp;
            m_LastRtpTime = inRtpTimeStemp;

            if (m_LastRtpTime < m_FirstRtpTime)
            {
                m_FirstRtpTime = inRtpTimeStemp;
                m_LastRtpTime = inRtpTimeStemp;
            }
        }

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	

        return 0;
    }

    short
        CPacketsQueue::Dequeue(unsigned char* outBuf, long outBufSize, long& outDequeueSize, unsigned short& outSeqNo, unsigned long& outRtpTimeStemp)
    {
        short theResult = 0;
        long theTmp1, theTmp2;
        long	packetSize = outBufSize;

        packetSize += 8;

        if (outBufSize < TS_PACKET_SIZE) return -2;

#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	

        if (m_GetIndex < m_PutIndex)
        {
            if (m_GetIndex + packetSize <= m_PutIndex)
            {
                outDequeueSize = outBufSize;

                outSeqNo = (unsigned short)(*(long*)&m_Buffer[m_GetIndex]);
                outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                memcpy(outBuf, &m_Buffer[m_GetIndex + 8], outDequeueSize);

                m_GetIndex += packetSize;
            }
            else
            {
                outDequeueSize = m_PutIndex - m_GetIndex;
                outDequeueSize -= 8;

                outSeqNo = (unsigned short)(*(long*)&m_Buffer[m_GetIndex]);
                outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                memcpy(outBuf, &m_Buffer[m_GetIndex + 8], outDequeueSize);

                m_GetIndex += packetSize;
            }
        }
        else if (m_GetIndex > m_PutIndex)
        {
            if ((m_GetIndex + packetSize) <= m_BufMaxSize)
            {
                outDequeueSize = outBufSize;

                outSeqNo = (unsigned short)*(long*)&m_Buffer[m_GetIndex];
                outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                memcpy(outBuf, &m_Buffer[m_GetIndex + 8], outDequeueSize);

                m_GetIndex += packetSize;
                if (m_GetIndex >= m_BufMaxSize)
                    m_GetIndex = 0;
            }
            else
            {
                theTmp1 = m_BufMaxSize - m_GetIndex;
                theTmp2 = packetSize - theTmp1;
                if (theTmp2 <= m_PutIndex)
                {
                    outDequeueSize = outBufSize;

                    outSeqNo = (unsigned short)*(long*)&m_Buffer[m_GetIndex];
                    outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                    memcpy(outBuf, &m_Buffer[m_GetIndex + 8], theTmp1 - 8);

                    m_GetIndex = 0;
                    memcpy(&outBuf[theTmp1 - 8], &m_Buffer[m_GetIndex], theTmp2);
                    m_GetIndex += theTmp2;
                }
                else
                {
                    theTmp2 = m_PutIndex;
                    if (theTmp2 > 0)
                    {
                        outDequeueSize = theTmp1 + theTmp2 - 8;

                        outSeqNo = (unsigned short)*(long*)&m_Buffer[m_GetIndex];
                        outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                        memcpy(outBuf, &m_Buffer[m_GetIndex + 8], theTmp1 - 8);

                        m_GetIndex = 0;
                        memcpy(&outBuf[theTmp1 - 8], &m_Buffer[m_GetIndex], theTmp2);
                        m_GetIndex = theTmp2;
                    }
                    else
                    {
                        outDequeueSize = theTmp1;

                        outSeqNo = (unsigned short)*(long*)&m_Buffer[m_GetIndex];
                        outRtpTimeStemp = *(unsigned long*)&m_Buffer[m_GetIndex + 4];
                        memcpy(outBuf, &m_Buffer[m_GetIndex + 8], theTmp1);
                        m_GetIndex = 0;
                    }
                }
            }
        }
        else
        {//m_GetIndex == m_PutIndex
            theResult = -16;//theResult = -1;
            outDequeueSize = 0;
            m_TsCount = 0;
        }

        if (theResult == 0)
        {
            m_TsCount -= (outDequeueSize / TS_PACKET_SIZE);
            m_FirstRtpTime = outRtpTimeStemp;
        }

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	

        return theResult;
    }

    void
        CPacketsQueue::ResetQueue()
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        ::pthread_mutex_lock( &m_hMutex );
#endif	
        m_TsCount = 0;
        m_PutIndex = 0;
        m_GetIndex = 0;

        m_LastGetIndex = 0;
        m_LastPutIndex = 0;
        m_PutActionFlag = 0;

        m_FirstRtpTime = 0;
        m_LastRtpTime = 0;

#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        ::pthread_mutex_unlock( &m_hMutex );
#endif	
    }

    long
        CPacketsQueue::CheckState()
    {
        if (m_LastPutIndex == m_PutIndex)
        {
            ++m_PutActionFlag;
            /*		if(m_PutActionFlag == 0)
                        m_PutActionFlag = 1;
                        else if(m_PutActionFlag == 1)
                        m_PutActionFlag = 2;
                        else if(m_PutActionFlag == 2)
                        m_PutActionFlag = 3;*/
        }
        else
        {
            m_LastPutIndex = m_PutIndex;
            m_PutActionFlag = 0;
        }

        return m_PutActionFlag;
    }


    void
        CPacketsQueue::GetReceiveSize(long* outSize, unsigned long* outRTPDeltaTime)
    {
        if (outSize)
            *outSize = m_TsCount * TS_PACKET_SIZE;
        if (outRTPDeltaTime)
        {
            if (m_LastRtpTime < m_FirstRtpTime)
                *outRTPDeltaTime = m_LastRtpTime;// 0xFFFFFFFF + m_LastRtpTime - m_FirstRtpTime;
            else
                *outRTPDeltaTime = m_LastRtpTime - m_FirstRtpTime;
        }
    }
