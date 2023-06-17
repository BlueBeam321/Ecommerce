
#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#ifdef _WIN32
#include <process.h>
#endif

#include "CTsRtpSource.h"

#include <string.h>
#include <time.h>

#include "CRTSPClient.h"

#ifndef _WIN32
#include "MxSocketConfig.h"
#endif

#ifdef _WIN64
#	define SOCKETRECVBUFFERSIZE (96*1024*1024)
#	define SOCKETSENDBUFFERSIZE 512*1024
#else
#	define SOCKETRECVBUFFERSIZE 8*1024*1024
#	define SOCKETSENDBUFFERSIZE 512*1024
#endif

#define MAX_NET_DIPTH_KCR		500
#define	USE_FIXED_MEMBUFFER	1

extern short gVOD_GlobalStreamStopFlag;

////////// CReorderingPacketBuffer definition //////////
class CReorderingPacketBuffer 
{
public:
  CReorderingPacketBuffer(CBufferedPacketFactory* packetFactory);
  virtual ~CReorderingPacketBuffer();

  CBufferedPacket* getFreePacket(CTsRtpSource* ourSource);
  bool storePacket(CBufferedPacket* bPacket);
  bool getNextCompletedPacket(CUsageEnvironment* inEnv,unsigned char* to, unsigned toSize,
								 unsigned& bytesUsed, unsigned& bytesTruncated,
								 unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
								 struct timeval& presentationTime,
								 bool& hasBeenSyncedUsingRTCP,
								 bool& rtpMarkerBit,
								 unsigned short &outRtpExtflag,unsigned char* outRtpExtData,long &ioRtpExtDataSize);

  void releaseUsedPacket(CBufferedPacket* packet);
  void freePacket(CBufferedPacket* packet);

  void setThresholdTime(unsigned uSeconds) { m_ThresholdTime = uSeconds; }

  void resetPacketQueue();

  CBufferedPacket*	MyGetFreePacket();
	long			GetUsePackCount(); //unused. test m_dipth.
	void				CheckPacket();

private:
  CBufferedPacketFactory*	m_PacketFactory;
  unsigned					m_ThresholdTime; 
  bool						m_HaveSeenFirstPacket;
  unsigned short			 m_NextExpectedSeqNo;
  CBufferedPacket*			m_HeadPacket;
  CBufferedPacket*			m_SavedPacket;


  CBufferedPacket*			m_MallocPacketList;

  struct	timeval			m_LastRecvTime;

	long				m_Dipth;
	struct	timeval			m_NextExpectedTime;	//2014.5.5 by KCR : error at tvrecord.
	struct	timeval			m_LastExpectedTime;
	

#ifdef _WIN32
	HANDLE				m_hMutex;
#else
	pthread_mutex_t		m_hMutex;
#endif  
};

CTsRtpSource*
CTsRtpSource::createNew(CUsageEnvironment* inEnv,
						   CIoSocket* rtpIoSock,
						   unsigned char rtpPayloadFormat,
						   unsigned rtpTimestampFrequency,
						   unsigned long inSSRC,
						   char const* mimeTypeString,
						   unsigned offset, 
                            bool doNormalMBitRule,
                            int                inRtpStreamMode,
							bool			   inAuthStreamMode,
                            CRTSPClient*       inRTSPClientObj)
{
  return new CTsRtpSource(inEnv, rtpIoSock, rtpPayloadFormat, rtpTimestampFrequency,inSSRC, mimeTypeString, offset, doNormalMBitRule,inRtpStreamMode,inAuthStreamMode,inRTSPClientObj);
}

CTsRtpSource::CTsRtpSource(CUsageEnvironment* inEnv, 
							  CIoSocket* rtpIoSock,
							  unsigned char rtpPayloadFormat,
							  unsigned rtpTimestampFrequency,
							  unsigned long inSSRC,
							  char const* mimeTypeString,
							  unsigned offset, 
                                bool doNormalMBitRule,
                                int                inRtpStreamMode,
								bool			   inAuthStreamMode,
                                CRTSPClient*       inRTSPClientObj)
:CRTPSource(inEnv, rtpIoSock, rtpPayloadFormat, rtpTimestampFrequency,inSSRC),
m_MIMEtypeString(strdup(mimeTypeString)), m_Offset(offset)
{
	m_ReadSocketNum = m_RTPInterface->getIoSock()->socketNum();
	m_AuthReadSocket = m_RTPInterface->getIoSock()->GetSocketHandle();

	m_IsCurrentlyAwaitingData = false;
	m_ReorderingBuffer = new CReorderingPacketBuffer(NULL);

	increaseReceiveBufferTo(inEnv,rtpIoSock->socketNum(), SOCKETRECVBUFFERSIZE);

	m_UseMBitForFrameEnd = strncmp(mimeTypeString, "video/", 6) == 0 && doNormalMBitRule;

    m_RtpStreamMode     = inRtpStreamMode;
	m_AuthStreamMode	= inAuthStreamMode;
    m_pRtspClientObj    = inRTSPClientObj;
	m_tcpRtpSockError	= false;
	m_ExitReadThreadFlag = false;
	m_bPauseProcessFlag = false;
	m_iPauseProcessFlag = 0;
	m_bWaitFlag = true;

#ifdef _WIN32
	unsigned int theThreadID;
	m_hReadThread = (HANDLE)_beginthreadex( NULL, 0, (PTHREAD_START)networkReadHandler,this, 0, &theThreadID );
	SetThreadPriority(m_hReadThread, THREAD_PRIORITY_HIGHEST);

#else
	pthread_create( &m_hReadThread , NULL, (PTHREAD_START)networkReadHandler, (void*)this );
	{
		int	i_policy;
		struct sched_param	param;
		
		memset( &param, 0, sizeof(struct sched_param) );
		param.sched_priority = 40;
		i_policy = SCHED_RR;
		pthread_setschedparam(m_hReadThread, i_policy, &param);
	}
#endif	
}

CTsRtpSource::~CTsRtpSource()
{
	if(m_hReadThread)
	{
		if(!m_ExitReadThreadFlag)
		{
			m_ExitReadThreadFlag = true;
#ifdef _WIN32
			::WaitForSingleObject(m_hReadThread,INFINITE);
#else
			pthread_join(m_hReadThread,NULL);
#endif		
		}
#ifdef _WIN32
		::CloseHandle(m_hReadThread);
#endif		
		m_hReadThread = NULL;
	}

	if(m_ReorderingBuffer)
		delete m_ReorderingBuffer;
	m_ReorderingBuffer = NULL;

	if(m_MIMEtypeString)
		delete[] (char*)m_MIMEtypeString;
}

void 
CTsRtpSource::setPacketReorderingThresholdTime(unsigned uSeconds) 
{
  m_ReorderingBuffer->setThresholdTime(uSeconds);
}

void 
CTsRtpSource::ResetReorderingBuffer()
{
	if(m_ReorderingBuffer)
		m_ReorderingBuffer->resetPacketQueue();
}

bool 
CTsRtpSource::isFramedSource() const 
{
  return true;
}

#define ADVANCE(n) do { bPacket->skip(n); } while (0)

#ifdef _WIN32
void
#else
void*
#endif
CTsRtpSource::networkReadHandler(void* inData)
{
	CTsRtpSource* theSource = (CTsRtpSource*)inData;
	theSource->networkReadHandler1();
#ifndef _WIN32
	return NULL;
#endif	
}

//#define _USE_TIME_OUT_
void CTsRtpSource::WaitRecvReady()
{
	while(m_bWaitFlag && !m_ExitReadThreadFlag)
	{
		msleep(1000);
	}
    msleep(100000);
}

void
CTsRtpSource::networkReadHandler1() 
{
	struct timeval theTimeVal;
	int theMaxNumSocket;
	fd_set theReadSet;

	if (m_RTPInterface->getIoSock()->GetAuthFlag() == false)
	{		
		FD_ZERO(&theReadSet);
		theMaxNumSocket = m_ReadSocketNum + 1;
		theTimeVal.tv_sec = 0;
		theTimeVal.tv_usec = 20000;
	}
    
    unsigned short rtp_tcp_len = 0;
	unsigned char channelID = 0;
    char doler_symbol;
    
	MxBuffer theRcvBuff;
	char*			pRecvBuff = NULL;
	int32_t			theRecvSize = 0;
    
#ifdef _USE_TIME_OUT_
    bool theLineFailed = false;
    unsigned long theOldTicks;
    
    theOldTicks = myGetTickCount();
#endif
    
	m_bWaitFlag = false;

    while(!m_ExitReadThreadFlag)
    {
#ifdef _USE_TIME_OUT_
        if(theLineFailed)
        {
            msleep(1000);
            continue;
        }
#endif
        if (m_RTPInterface->getIoSock()->GetAuthFlag() == false)
		{
			if(m_bPauseProcessFlag == true)
			{
				if(m_iPauseProcessFlag == 0)
					m_iPauseProcessFlag = 1;
                msleep(1000);
				continue;
			}
       
			FD_ZERO(&theReadSet);
			FD_SET((unsigned)m_ReadSocketNum, &theReadSet);
			int theSelectResult = select(theMaxNumSocket, &theReadSet, NULL, NULL, &theTimeVal);
        
			if(theSelectResult == 0)
			{
                msleep(1000);
            
	#ifdef _USE_TIME_OUT_
				if(CalcDeltaTick(myGetTickCount() , theOldTicks) > SOCK_TIMEOUT_VALUE)
				{
					env()->setResultMsg(11000408,"select timeout error. LineFailed!");
					env()->EnqueueEvent(kVODNet_Event_LineFailed);
					theLineFailed = true;
				}
	#endif
				continue;
            
			}
			else if(theSelectResult < 0)
			{
				int theErr = WSAGetLastError();
                msleep(1000);
				continue;
			}

			if(m_tcpRtpSockError)
			{
                msleep(1000);
				continue;
			}


			if(!FD_ISSET(m_ReadSocketNum,&theReadSet))
			{
                msleep(1000);
				continue;
			}
		}
		else if (m_RTPInterface->getIoSock()->GetAuthFlag() == true)
		{
            if(m_bPauseProcessFlag == true)
            {
                if(m_iPauseProcessFlag == 0)
                    m_iPauseProcessFlag = 1;
                msleep(1000);
                continue;
            }
            
			int theSelectResult = 0;
            theSelectResult = m_AuthReadSocket->SelectRecv(20 * 1000000);
			if (theSelectResult == 0)
			{
                msleep(1000);

#ifdef _USE_RECEIVE_TIME_OUT_			
				if((mdate() - theOldTicks) > 10000000)
				{
					env()->setResultMsg(11000408,"select timeout error. LineFailed!");
					env()->EnqueueEvent(kVODNet_Event_LineFailed);
					theLineFailed = true;
				}
#endif			
				continue;

			}
			else if (theSelectResult < 0)
			{
				int theErr = errno;
                msleep(1000);
				continue;
			}

			if (m_tcpRtpSockError)
			{
                msleep(1000);
				continue;
			}
		}
        
        if(m_RtpStreamMode == eRTPSTREAM_TCP)
        {
            //$ read symbol
            //if $ != symbol then RTCP_req_command Process
            //else
            //  read_2 channelID
            //  read_2 rtp_len
            short theResult_tcp;

			if (m_RTPInterface->getIoSock()->GetAuthFlag() == false)
			{
				unsigned int		numBytesRead;
				struct sockaddr_in	fromAddress;

				//$ read symbol
				doler_symbol = 0;
				theResult_tcp = m_RTPInterface->getIoSock()->handleReadTcp((unsigned char*)&doler_symbol,1);
				if(theResult_tcp < 0)
				{
					//error
					//printf("TCP Socket Error\n");
					m_tcpRtpSockError = true;
					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspTcpSocketError();
					continue;
				}

				if(doler_symbol != '$')
				{//if $ != symbol then RTCP_req_command Process
					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspResProcessbyRtp(doler_symbol);
					continue;
				}

				theResult_tcp = m_RTPInterface->getIoSock()->handleReadTcp((unsigned char*)&channelID,1);
				if(theResult_tcp<0)
				{
					m_tcpRtpSockError = true;
					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspTcpSocketError();
					//error
					printf("TSRtpSource   channelID err.\n");
					continue;
				}
				theResult_tcp = m_RTPInterface->getIoSock()->handleReadTcp((unsigned char*)&rtp_tcp_len,2);
				if(theResult_tcp < 0)
				{
					//error
					m_tcpRtpSockError = true;
					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspTcpSocketError();
					printf("TSRtpSource   tcp_len err.\n");
					continue;
				}

				channelID = channelID;//ntohs(channelID);
				rtp_tcp_len = ntohs(rtp_tcp_len);
			}
			else if (m_RTPInterface->getIoSock()->GetAuthFlag() == true)
			{
				theResult_tcp = m_AuthReadSocket->ReceiveBlock(&theRcvBuff);
				if(theResult_tcp < 0)
				{
					//error
					//printf("TCP Socket Error\n");
					m_tcpRtpSockError = true;

					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspTcpSocketError();
					continue;
				}

				pRecvBuff = (char*)theRcvBuff.GetBuffer();
				theRecvSize = theRcvBuff.GetDataSize();

				//$ read symbol
				doler_symbol = pRecvBuff[0];
				if (doler_symbol != '$')
				{//if $ != symbol then RTCP_req_command Process
					if(m_pRtspClientObj)
						m_pRtspClientObj->RtspResProcessbyRtpEx(pRecvBuff, theRecvSize);
					continue;
				}

				channelID = pRecvBuff[1];
                rtp_tcp_len = (unsigned char)pRecvBuff[2];
                rtp_tcp_len <<= 8;
                rtp_tcp_len |= (unsigned char)pRecvBuff[3];
			}
        }else if(m_RtpStreamMode == eRTPSTREAM_UDP)
        {
            if(env()->m_bStreamStopFlag == 1)
            {
                msleep(1000);
                continue;
            }
        }
        
#ifdef _USE_TIME_OUT_
        theOldTicks = myGetTickCount();
#endif
        
        short theResult;
        bool readSuccess = false;
		CBufferedPacket* bPacket = NULL;
		
		char*	pRTPRecvBuff = pRecvBuff;
		int		RecvRTPDataLen = theRecvSize;
		int		noRTPDataFlag = false;
		
		do
		{
			readSuccess = false;
			noRTPDataFlag = false;
			
			bPacket = m_ReorderingBuffer->getFreePacket(this);
			
			if(bPacket == NULL)
				break;
			
			do
			{
				if(m_RtpStreamMode == eRTPSTREAM_TCP)
				{
					if (m_RTPInterface->getIoSock()->GetAuthFlag() == false)
					{
						theResult = bPacket->fillInDataTcp(*m_RTPInterface,rtp_tcp_len);
					}
					else
					{
						channelID = pRTPRecvBuff[1];
                        rtp_tcp_len = (unsigned char)pRTPRecvBuff[2];
                        rtp_tcp_len <<= 8;
                        rtp_tcp_len |= (unsigned char)pRTPRecvBuff[3];
                        
						theResult = 0;
						bPacket->reset();
						bPacket->appendData((unsigned char*)&pRTPRecvBuff[4], rtp_tcp_len);

						pRTPRecvBuff += (4 + rtp_tcp_len);
						RecvRTPDataLen -= (4 + rtp_tcp_len);
					}
					
					if (theResult < 0)
					{
						env()->setResultMsg(11000408,"EnqueueEvent due to LineFailed!");
						m_tcpRtpSockError = true;
						if(m_pRtspClientObj)
							m_pRtspClientObj->RtspTcpSocketError();
						
						noRTPDataFlag = true;
						break;
					}
					
					if(m_pRtspClientObj)
					{
						if(m_pRtspClientObj->IsRTPChannelID(channelID) == false)
						{
							printf("######## Skip Old Packet\n");
							noRTPDataFlag = true;
							break;
						}
					}
					
				}else
				{
					if ((theResult = bPacket->fillInData(*m_RTPInterface)) < 0)
					{
						env()->setResultMsg(11000408,"EnqueueEvent due to LineFailed!");
						noRTPDataFlag = true;
						break;
					}
				}

				if (bPacket->dataSize() < 12)
				{
					noRTPDataFlag = true;
					break;
				}
				unsigned rtpHdr = ntohl(*(unsigned*)(bPacket->data())); ADVANCE(4);
				bool rtpMarkerBit = bool(((rtpHdr&0x00800000) >> 23)==1);
				unsigned rtpTimestamp = ntohl(*(unsigned*)(bPacket->data()));ADVANCE(4);
				unsigned rtpSSRC = ntohl(*(unsigned*)(bPacket->data())); ADVANCE(4);

				if ((rtpHdr&0xC0000000) != 0x80000000)
				{
					noRTPDataFlag = true;
					break;
					
				}

				unsigned cc = (rtpHdr>>24)&0xF;
				if (bPacket->dataSize() < cc)
				{
					noRTPDataFlag = true;
					break;
				}
				ADVANCE(cc*4);

				if (rtpHdr&0x10000000)
				{
					//Extension
					if (bPacket->dataSize() < 4)
					{
						noRTPDataFlag = true;
						break;
					}
					unsigned extHdr = ntohl(*(unsigned*)(bPacket->data())); ADVANCE(4);
					unsigned remExtSize = 4*(extHdr&0xFFFF);
					unsigned remExtFlag = (extHdr >> 16);

					if (bPacket->dataSize() < remExtSize)
					{
						noRTPDataFlag = true;
						break;
					}

					bPacket->SetExtData(remExtFlag,bPacket->data(),remExtSize);
					ADVANCE(remExtSize);
				}
				else
				{
					bPacket->SetExtData(0,NULL,0);
				}

				if (rtpHdr&0x20000000)
				{
					if (bPacket->dataSize() == 0)
					{
						noRTPDataFlag = true;
						break;
					}
					
					unsigned numPaddingBytes
					= (unsigned)(bPacket->data())[bPacket->dataSize()-1];
					if (bPacket->dataSize() < numPaddingBytes)
					{
						noRTPDataFlag = true;
						break;
					}

					bPacket->removePadding(numPaddingBytes);
				}

				if ((unsigned char)((rtpHdr&0x007F0000)>>16) != rtpPayloadFormat())
				{
					noRTPDataFlag = true;
					break;
				}

				m_LastReceivedSSRC = rtpSSRC;

				unsigned short rtpSeqNo = (unsigned short)(rtpHdr&0xFFFF);
				bool usableInJitterCalculation = packetIsUsableInJitterCalculation((bPacket->data()), bPacket->dataSize());

				struct timeval presentationTime; // computed by:
				bool hasBeenSyncedUsingRTCP; // computed by:
				receptionStatsDB().noteIncomingPacket(rtpSSRC,
													  rtpSeqNo,
													  rtpTimestamp,
													  timestampFrequency(),
													  usableInJitterCalculation,
													  presentationTime,
													  hasBeenSyncedUsingRTCP,
													  bPacket->dataSize());

				struct timeval timeNow;
				gettimeofday(&timeNow, NULL);
				bPacket->assignMiscParams(rtpSeqNo, rtpTimestamp, presentationTime,
										  hasBeenSyncedUsingRTCP, rtpMarkerBit,
										  timeNow);
				readSuccess = m_ReorderingBuffer->storePacket(bPacket);
					//			readSuccess = true;
			} while (0);
			
			if (noRTPDataFlag)
				break;
			
			if (m_RTPInterface->getIoSock()->GetAuthFlag() == false)
				break;
			else
			{
				if (RecvRTPDataLen <= 0)
					break;
				
				doler_symbol = pRTPRecvBuff[0];
				if (doler_symbol != '$')
					break;
			}
		}while(1);
		
		if(bPacket == NULL)
			continue;
		
        if (!readSuccess)	
        {
            m_ReorderingBuffer->freePacket(bPacket);
        }
    }
    
    if(m_ReorderingBuffer)
        delete m_ReorderingBuffer;
    m_ReorderingBuffer = NULL;
}

bool CTsRtpSource::IsTcpSocketError()
{
	return m_tcpRtpSockError;
}


long
CTsRtpSource::getNextFrame(unsigned char* to, long maxSize,
							unsigned short& outSeqNo, uint32_t& outRtpTimeStemp,
							unsigned short &outRtpExtflag,unsigned char* outRtpExtData,long &ioRtpExtDataSize) 
{
	if (m_IsCurrentlyAwaitingData) 
	{
		env()->setResultMsg(11000404,"getNextFrame(): attempting to read more than once at the same time!");
		return -1;
	}

	m_To = to;
	m_MaxSize = maxSize;	
	m_IsCurrentlyAwaitingData = true;	

	m_FrameSize = doGetNextFrame(outSeqNo, outRtpTimeStemp,outRtpExtflag,outRtpExtData,ioRtpExtDataSize);
	if(m_FrameSize == 0)
	{
		m_IsCurrentlyAwaitingData = false;
		return -2;
	}

	m_FrameSize = afterGettingFrame(m_FrameSize);

	m_IsCurrentlyAwaitingData = false;

	return m_FrameSize;
}

long
CTsRtpSource::doGetNextFrame(unsigned short& outSeqNo, uint32_t& outRtpTimeStemp,
								unsigned short &outRtpExtflag,unsigned char* outRtpExtData,long &ioRtpExtDataSize)
{
	bool theResult;
	unsigned theNumTruncatedBytes;
	struct timeval thePresentationTime;
	unsigned theFrameSize = 0;

	m_FrameSize = 0;	

    theResult
      = m_ReorderingBuffer->getNextCompletedPacket(env(),m_To, m_MaxSize, theFrameSize, theNumTruncatedBytes,
					m_CurPacketRTPSeqNum, m_CurPacketRTPTimestamp,
					thePresentationTime, m_CurPacketHasBeenSynchronizedUsingRTCP,
					m_CurPacketMarkerBit,outRtpExtflag,outRtpExtData,ioRtpExtDataSize);

	m_FrameSize = long(theFrameSize);
	outSeqNo = m_CurPacketRTPSeqNum;
	outRtpTimeStemp = m_CurPacketRTPTimestamp;

	return m_FrameSize;
}

#define TS_PACKET_SIZE      188
#define TS_SYNC_BYTE        0x47
long
CTsRtpSource::afterGettingFrame(long /*inFrameSize*/)
{
    unsigned const numTSPackets = m_FrameSize / TS_PACKET_SIZE;
    m_FrameSize = numTSPackets*TS_PACKET_SIZE;
  if (m_FrameSize == 0) 
  {
    //env()->setResultMsg(10000804,"Input Source Closed!");
    return - 2;
  }

  long syncBytePosition;
  for (syncBytePosition = 0; syncBytePosition < m_FrameSize; ++syncBytePosition) 
  {
      if (m_To[syncBytePosition] == TS_SYNC_BYTE)
	{
		if(syncBytePosition + 188 < m_FrameSize)
		{
            if (m_To[syncBytePosition + 188] == TS_SYNC_BYTE)
				break;	
		}
		else
			break;
	}
  }

  if (syncBytePosition == m_FrameSize) 
  {
    env()->setResultMsg(11000801,"No Transport Stream sync byte in data!");
	m_FrameSize = 0;
    return -3;
  } 
  else if (syncBytePosition > 0)
  {
     memmove(m_To, &m_To[syncBytePosition], m_FrameSize - syncBytePosition);
    m_FrameSize -= syncBytePosition;
	env()->setResultMsg(11000802,"Transport Stream sync byte not at first!");
  }

  return m_FrameSize;
}

char const* 
CTsRtpSource::MIMEtype() const 
{
  return m_MIMEtypeString;
}

bool
CTsRtpSource::packetIsUsableInJitterCalculation(unsigned char* /*packet*/, unsigned /*packetSize*/) 
{
  return true;
}

void
CTsRtpSource::stopGettingFrames() 
{
  m_IsCurrentlyAwaitingData = false;

  doStopGettingFrames();
}

void 
CTsRtpSource::doStopGettingFrames() 
{
 	if(m_hReadThread)
	{
		if(!m_ExitReadThreadFlag)
		{
			m_ExitReadThreadFlag = true;
#ifdef _WIN32
			::WaitForSingleObject(m_hReadThread,INFINITE);
#else
			pthread_join(m_hReadThread,NULL);
#endif		
		}
#ifdef _WIN32		
		::CloseHandle(m_hReadThread);
#endif		
		m_hReadThread = NULL;
	}
}

void CTsRtpSource::DoPauseProcess(bool bPauseProcessFlag)
{
	long i;

	if(bPauseProcessFlag != m_bPauseProcessFlag)
	{
		if(bPauseProcessFlag == true)
		{
			m_iPauseProcessFlag = 0;
			m_bPauseProcessFlag = bPauseProcessFlag;
			for(i=0; i<1000 && m_iPauseProcessFlag == 0; i++)
			{
                msleep(1000);
			}
		}else
		{
			m_bPauseProcessFlag = bPauseProcessFlag;
		}
	}
}


////////// CBufferedPacket and CBufferedPacketFactory implementation /////
#define MAX_PACKET_SIZE (1500)		//(1328)

CBufferedPacket::CBufferedPacket()
  : m_PacketSize(MAX_PACKET_SIZE),
    m_Buf(new unsigned char[MAX_PACKET_SIZE]),
    m_NextPacket(NULL) 
{
	m_UseFlag = false;
}

CBufferedPacket::~CBufferedPacket() 
{
//	if(m_NextPacket)
//		delete m_NextPacket;
	m_NextPacket = NULL;
	delete[] m_Buf;
	m_Buf = NULL;
}


void 
CBufferedPacket::reset() 
{
  m_Head = m_Tail = 0;
  m_UseCount = 0;
  m_NextPacket = NULL;
  m_ExtFlag = 0;
  m_ExtSize = 0;
}

void
CBufferedPacket::Malloc()
{
	reset();
	m_UseFlag = 1;
}

void
CBufferedPacket::Free()
{
	m_UseFlag = 0;
}

int
CBufferedPacket::isfree()
{
	return m_UseFlag == 0 ? 1 : 0;
}

/**************************************************************************/
/**************************************************************************/
short
CBufferedPacket::fillInData(CRTPInterface& rtpInterface)
{
  reset();
  short theResult;
  unsigned numBytesRead;
  struct sockaddr_in fromAddress;
	
  theResult = rtpInterface.handleRead(&m_Buf[m_Tail], m_PacketSize-m_Tail, numBytesRead, fromAddress,NULL);
	
  if (theResult < 0) 
  {
    return theResult;
  }
  if(numBytesRead == 0) return -1;

  m_Tail += numBytesRead;
  return 0;
}

short
CBufferedPacket::fillInDataTcp(CRTPInterface& rtpInterface,int inReadSize)
{
    short				theResult;
    unsigned			numBytesRead;
    
    reset();
    
    if(m_PacketSize < inReadSize)
    {
        //ResizeBuffer
        if(m_Buf)
            delete[] m_Buf;
        
        m_PacketSize = inReadSize + 1024;
        m_Buf = new unsigned char[m_PacketSize];
    }
    
    reset();
    theResult = rtpInterface.handleReadTcp(&m_Buf[m_Tail],
                                           inReadSize);
    if(theResult < 0)
        return theResult;
    
    m_Tail += inReadSize;
    
    return 0;
}

/**************************************************************************/
/**************************************************************************/
void 
CBufferedPacket::assignMiscParams( unsigned short rtpSeqNo, unsigned rtpTimestamp,
								   struct timeval presentationTime,
								   bool hasBeenSyncedUsingRTCP, bool rtpMarkerBit,
								   struct timeval timeReceived) 
{
  m_RTPSeqNo = rtpSeqNo;
  m_RTPTimestamp = rtpTimestamp;
  m_PresentationTime = presentationTime;
  m_HasBeenSyncedUsingRTCP = hasBeenSyncedUsingRTCP;
  m_RTPMarkerBit = rtpMarkerBit;
  m_TimeReceived = timeReceived;
}

/**************************************************************************/
/**************************************************************************/
void 
CBufferedPacket::skip(unsigned numBytes) 
{
  m_Head += numBytes;
  if (m_Head > m_Tail) m_Head = m_Tail;
}

/**************************************************************************/
/**************************************************************************/
void 
CBufferedPacket::removePadding(unsigned numBytes) 
{
  if (numBytes > m_Tail-m_Head) numBytes = m_Tail-m_Head;
  m_Tail -= numBytes;
}

/***********************************************************************/
/***********************************************************************/
void
CBufferedPacket::appendData(unsigned char* newData, unsigned numBytes) 
{
  if (numBytes > m_PacketSize-m_Tail) numBytes = m_PacketSize - m_Tail;
  memmove(&m_Buf[m_Tail], newData, numBytes); 
  m_Tail += numBytes;
}

void CBufferedPacket::SetExtData(unsigned short inExtFlag, unsigned char* inExtData, long inExtSize)
{
	m_ExtFlag = inExtFlag;
	m_ExtSize = inExtSize;
	if(inExtFlag != 0 && inExtData != NULL && inExtSize > 0)
	{
		if(inExtSize > sizeof(m_ExtData))
		{
			m_ExtSize = sizeof(m_ExtData);
		}
		memcpy(m_ExtData,inExtData,m_ExtSize);
	}
}

/*********************************************************************************/
/*********************************************************************************/
void 
CBufferedPacket::use(unsigned char* to, unsigned toSize,
					 unsigned& bytesUsed, unsigned& bytesTruncated,
					 unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
					 struct timeval& presentationTime,
					 bool& hasBeenSyncedUsingRTCP,
					 bool& rtpMarkerBit,
					 unsigned short &outRtpExtflag,unsigned char* outRtpExtData,long &ioRtpExtDataSize)
{
  unsigned char* origFramePtr = &m_Buf[m_Head];
  unsigned char* newFramePtr = origFramePtr;
  unsigned frameDurationInMicroseconds;
  
  frameDurationInMicroseconds = 0;
  if (m_Tail - m_Head > toSize) 
  {
    bytesTruncated = m_Tail - m_Head - toSize;
    bytesUsed = toSize;
  } 
  else 
  {
    bytesTruncated = 0;
    bytesUsed = m_Tail - m_Head;
  }

  memmove(to, newFramePtr, bytesUsed);

  rtpSeqNo = m_RTPSeqNo;
  rtpTimestamp = m_RTPTimestamp;
  presentationTime = m_PresentationTime;
  hasBeenSyncedUsingRTCP = m_HasBeenSyncedUsingRTCP;
  rtpMarkerBit = m_RTPMarkerBit;

  m_PresentationTime.tv_usec += frameDurationInMicroseconds;
  if (m_PresentationTime.tv_usec >= 1000000) 
  {
    m_PresentationTime.tv_sec += m_PresentationTime.tv_usec/1000000;
    m_PresentationTime.tv_usec = m_PresentationTime.tv_usec%1000000;
  }

  outRtpExtflag = m_ExtFlag;
  
  if(m_ExtFlag && m_ExtSize>0)
  {
	  if(m_ExtSize < ioRtpExtDataSize)
		  ioRtpExtDataSize = m_ExtSize;	  
	  memcpy(outRtpExtData,m_ExtData,ioRtpExtDataSize);
  }
}

CBufferedPacketFactory::CBufferedPacketFactory() 
{
}

CBufferedPacketFactory::~CBufferedPacketFactory() 
{
}

CBufferedPacket* 
CBufferedPacketFactory::createNewPacket(CTsRtpSource* /*ourSource*/) 
{
  return new CBufferedPacket;
}

////////// CReorderingPacketBuffer definition //////////
CReorderingPacketBuffer::CReorderingPacketBuffer(CBufferedPacketFactory* packetFactory)
  : m_ThresholdTime(100000) ,
    m_HaveSeenFirstPacket(false), m_HeadPacket(NULL), m_SavedPacket(NULL) 
{
#ifdef _WIN32
	m_hMutex = ::CreateMutex(NULL, false, NULL);
#else
	pthread_mutex_init(&m_hMutex,NULL);
#endif	
	m_PacketFactory = (packetFactory == NULL) ? (new CBufferedPacketFactory) : packetFactory;

	m_LastRecvTime.tv_sec = 0;
	m_LastRecvTime.tv_usec = 0;
	m_Dipth = 0;

	m_MallocPacketList = new CBufferedPacket[MAX_NET_DIPTH_KCR];
}

CReorderingPacketBuffer::~CReorderingPacketBuffer() 
{
	if (m_HeadPacket == NULL)
	{
		delete m_SavedPacket;
		m_SavedPacket = NULL;
	} 
	else 
	{
		CBufferedPacket* theCur = NULL;
		CBufferedPacket* theNext = NULL;
		theCur = m_HeadPacket;
		while(theCur!=NULL)
		{
			theNext = theCur->m_NextPacket;
			if(theCur == m_SavedPacket)
			{
				delete m_SavedPacket;
				m_SavedPacket = NULL;
			}else
				theCur->Free();//delete theCur;
			
			theCur = theNext;
		}
		m_HeadPacket = NULL;

		if(m_SavedPacket!=NULL)
			delete m_SavedPacket;
	}

	if(m_MallocPacketList)
		delete []m_MallocPacketList;
	m_MallocPacketList = NULL;

	delete m_PacketFactory;
	m_PacketFactory = NULL;

#ifdef _WIN32
	::CloseHandle(m_hMutex);
#else
	pthread_mutex_destroy(&m_hMutex);
#endif	
	
	m_Dipth = 0;
}

void 
CReorderingPacketBuffer::resetPacketQueue()
{
	int i;
#ifdef _WIN32
	::WaitForSingleObject(m_hMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hMutex);
#endif	
	CBufferedPacket* theCur = NULL;
	CBufferedPacket* theNext = NULL;
	theCur = m_HeadPacket;
	while(theCur!=NULL)
	{
		theNext = theCur->m_NextPacket;
		if(theCur == m_SavedPacket)
		{
			delete theCur;
			m_SavedPacket = NULL;
		}else
		{
			theCur->Free();//delete theCur;
		}
		theCur = theNext;
	}

	for(i=0; i<MAX_NET_DIPTH_KCR; i++)
	{
		if(m_MallocPacketList[i].isfree() == 0)
		{
			m_MallocPacketList[i].Free();
		}
	}

	m_HeadPacket = NULL;
	
	m_Dipth = 0;

	m_HaveSeenFirstPacket = false;

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
}

CBufferedPacket*
CReorderingPacketBuffer::MyGetFreePacket()
{
	int i;
	for(i=0; i<MAX_NET_DIPTH_KCR; i++)
	{
		if(m_MallocPacketList[i].isfree() != 0)
		{
			m_MallocPacketList[i].Malloc();
			m_Dipth ++;
			return &m_MallocPacketList[i];
		}
	}
	return NULL;
}

long CReorderingPacketBuffer::GetUsePackCount()
{
	int i,cnt = 0;
	for(i=0; i<MAX_NET_DIPTH_KCR; i++)
	{
		if(m_MallocPacketList[i].isfree() == 0)
		{
			cnt ++;
		}
	}
	return cnt;
}

CBufferedPacket*
CReorderingPacketBuffer::getFreePacket(CTsRtpSource* ourSource)
{
	CBufferedPacket* theFreePacket;

	do{
#ifdef _WIN32
		::WaitForSingleObject(m_hMutex, INFINITE);
#else
		pthread_mutex_lock(&m_hMutex);
#endif	

		if (m_SavedPacket == NULL)
		{
			m_SavedPacket = m_PacketFactory->createNewPacket(ourSource);
		}

#ifdef USE_FIXED_MEMBUFFER
		theFreePacket = m_HeadPacket == NULL ? m_SavedPacket : MyGetFreePacket()/*m_PacketFactory->createNewPacket(ourSource)*/;
#else
		theFreePacket = m_HeadPacket == NULL ? m_SavedPacket : /*MyGetFreePacket()*/m_PacketFactory->createNewPacket(ourSource);
#endif

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif
		if(theFreePacket)
			break;
        msleep(1000);
	}while(ourSource && ourSource->m_ExitReadThreadFlag == false);

  return theFreePacket;
}

bool
CReorderingPacketBuffer::storePacket(CBufferedPacket* bPacket)
{
#ifdef _WIN32
	::WaitForSingleObject(m_hMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hMutex);
#endif	

	unsigned short rtpSeqNo = bPacket->rtpSeqNo();

	if (!m_HaveSeenFirstPacket) 
	{
//		m_NextExpectedSeqNo = 300;
		m_NextExpectedSeqNo = rtpSeqNo;
		m_HaveSeenFirstPacket = true;
		
		
		m_NextExpectedTime = bPacket->m_TimeReceived;
		m_LastExpectedTime = bPacket->m_TimeReceived;
		
	}

	unsigned short const seqNoThreshold = 100;
	if (vod_seqNumLT(rtpSeqNo, m_NextExpectedSeqNo) && vod_seqNumLT(m_NextExpectedSeqNo, rtpSeqNo+seqNoThreshold))
	{
#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
		return false;
	}

	CBufferedPacket* beforePtr = NULL;
	CBufferedPacket* afterPtr = m_HeadPacket;

	while (afterPtr != NULL) 
	{
		if (vod_seqNumLT(rtpSeqNo, afterPtr->rtpSeqNo())) break; // it comes here
		if (rtpSeqNo == afterPtr->rtpSeqNo()) 
		{
#ifdef _WIN32
			::ReleaseMutex(m_hMutex);
#else
			pthread_mutex_unlock(&m_hMutex);
#endif		
			return false;
		}

		beforePtr = afterPtr;
		afterPtr = afterPtr->nextPacket();
	}

	bPacket->nextPacket() = afterPtr;
	if (beforePtr == NULL) 
	{
		m_HeadPacket = bPacket;
	} 
	else 
	{
	  beforePtr->nextPacket() = bPacket;
	}

#ifdef _WIN32
	::ReleaseMutex(m_hMutex);
#else
	pthread_mutex_unlock(&m_hMutex);
#endif		
	return true;

}

/***********************************************************************************/
/***********************************************************************************/
void
CReorderingPacketBuffer::releaseUsedPacket(CBufferedPacket* packet) 
{
	// ASSERT: packet == m_HeadPacket
	// ASSERT: m_NextExpectedSeqNo == packet->rtpSeqNo()
	++m_NextExpectedSeqNo;
	m_NextExpectedTime = packet->m_TimeReceived;

	if(m_HeadPacket== packet)
		m_HeadPacket = m_HeadPacket->nextPacket();

	packet->m_NextPacket = NULL;

	if (packet != m_SavedPacket) 
	{
#ifdef USE_FIXED_MEMBUFFER
		packet->Free(); //delete packet;//add kcr 2010.11.13
		
		m_Dipth --; 
#else
		delete packet;
#endif
	}
	
	if(m_NextExpectedTime.tv_sec > m_LastExpectedTime.tv_sec + 2)
	{//2sec -> 
		CheckPacket();
	}
	
}

void
CReorderingPacketBuffer::CheckPacket()
{
	CBufferedPacket* deletePtr = NULL;
	CBufferedPacket* beforePtr = NULL;
	CBufferedPacket* afterPtr = m_HeadPacket;
	long			i_loopcnt = 0;			
	
	m_LastExpectedTime = m_NextExpectedTime;
	
	while (afterPtr != NULL) 
	{
		i_loopcnt ++;
		if(i_loopcnt > 300)
			break;
		
		if(afterPtr->m_TimeReceived.tv_sec + 2 < m_NextExpectedTime.tv_sec)
		{//IF 2 sec old data THEN this packet free.
			deletePtr = afterPtr;
			
			if(beforePtr != NULL)
			{
				beforePtr->m_NextPacket = afterPtr->m_NextPacket;
				afterPtr = beforePtr;
			}
			else
			{
				m_HeadPacket = afterPtr->m_NextPacket;
				afterPtr = m_HeadPacket;
			}
			
			if(deletePtr)
			{
				deletePtr->m_NextPacket = NULL;
				
				if (deletePtr != m_SavedPacket) 
				{
					deletePtr->Free();
					m_Dipth --;
				}
			}
		}
		
		beforePtr = afterPtr;
		if(afterPtr)
			afterPtr = afterPtr->m_NextPacket;
	}
}

/*************************************************************************************/
/*************************************************************************************/
bool
CReorderingPacketBuffer::getNextCompletedPacket(CUsageEnvironment* inEnv,unsigned char* to, unsigned toSize,
												 unsigned& bytesUsed, unsigned& bytesTruncated,
												 unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
												 struct timeval& presentationTime,
												 bool& hasBeenSyncedUsingRTCP,
												 bool& rtpMarkerBit,
												 unsigned short &outRtpExtflag,unsigned char* outRtpExtData,long &ioRtpExtDataSize)					 
{
	bool packetLossPreceded;

	bytesUsed = 0;
#ifdef _WIN32
	DWORD dwWaitResult = ::WaitForSingleObject(m_hMutex, 10);
    if (dwWaitResult != WAIT_OBJECT_0)
    {
        ::ReleaseMutex(m_hMutex);
        return false;
    }
#else
	pthread_mutex_lock(&m_hMutex);
#endif

	if (m_HeadPacket == NULL) 
	{
#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
		return false;
	}

	// ASSERT: m_HeadPacket->rtpSeqNo() >= m_NextExpectedSeqNo
	if (m_HeadPacket->rtpSeqNo() == m_NextExpectedSeqNo) 
	{
		packetLossPreceded = false;

		m_HeadPacket->use(to,toSize,bytesUsed,bytesTruncated,rtpSeqNo,rtpTimestamp,presentationTime,hasBeenSyncedUsingRTCP,rtpMarkerBit,outRtpExtflag,outRtpExtData,ioRtpExtDataSize);
		releaseUsedPacket(m_HeadPacket);

		gettimeofday(&m_LastRecvTime, NULL);

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
		return true;
	}
	else
	{
		CBufferedPacket* beforePtr = NULL;
		CBufferedPacket* afterPtr = m_HeadPacket;

		while (afterPtr != NULL) 
		{
			if (m_NextExpectedSeqNo == afterPtr->rtpSeqNo()) 
			{
				packetLossPreceded = false;	

				beforePtr->m_NextPacket = afterPtr->m_NextPacket;
				afterPtr->m_NextPacket = NULL;

				afterPtr->use(to,toSize,bytesUsed,bytesTruncated,rtpSeqNo,rtpTimestamp,presentationTime,hasBeenSyncedUsingRTCP,rtpMarkerBit,outRtpExtflag,outRtpExtData,ioRtpExtDataSize);
				releaseUsedPacket(afterPtr);

				gettimeofday(&m_LastRecvTime, NULL);

#ifdef _WIN32
				::ReleaseMutex(m_hMutex);
#else
				pthread_mutex_unlock(&m_hMutex);
#endif		
				return true;
			}

			beforePtr = afterPtr;
			afterPtr = afterPtr->m_NextPacket;
		}
		
	}

	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);

	if(m_LastRecvTime.tv_sec == 0 && m_LastRecvTime.tv_usec == 0)
		gettimeofday(&m_LastRecvTime, NULL);

	unsigned uSecondsSinceReceived
	= (timeNow.tv_sec - m_LastRecvTime.tv_sec)*1000000
	+ (timeNow.tv_usec - m_LastRecvTime.tv_usec);

/*
	unsigned uSecondsSinceReceived
	= (timeNow.tv_sec - m_HeadPacket->timeReceived().tv_sec)*1000000
	+ (timeNow.tv_usec - m_HeadPacket->timeReceived().tv_usec);
*/
	
	
	if (uSecondsSinceReceived > m_ThresholdTime || m_Dipth > 80) 
	{
		char theTmpMsg[300];
		unsigned short		preSeqNo = m_NextExpectedSeqNo;
		
		m_NextExpectedSeqNo = m_HeadPacket->rtpSeqNo();
		m_NextExpectedTime = m_HeadPacket->m_TimeReceived;

		packetLossPreceded = true;
		
		sprintf(theTmpMsg,"Packet Loss Occured!(preSeqNo = %d, NewSeqNo = %d, delta = %d, dipth = %d)",
			preSeqNo, m_NextExpectedSeqNo, m_NextExpectedSeqNo - preSeqNo,m_Dipth);
		inEnv->setResultMsg(11000805, theTmpMsg);

		m_HeadPacket->use(to,toSize,bytesUsed,bytesTruncated,rtpSeqNo,rtpTimestamp,presentationTime,hasBeenSyncedUsingRTCP,rtpMarkerBit,outRtpExtflag,outRtpExtData,ioRtpExtDataSize);
		releaseUsedPacket(m_HeadPacket);

		gettimeofday(&m_LastRecvTime, NULL);

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
		return true;
	}

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
	return false;
}

void 
CReorderingPacketBuffer::freePacket(CBufferedPacket* packet)
{ 
#ifdef _WIN32
	DWORD dwWaitResult = ::WaitForSingleObject(m_hMutex, INFINITE);
#else
	pthread_mutex_lock(&m_hMutex);
#endif	

	if (packet != m_SavedPacket) 
	{
		packet->Free();	//delete packet;
	}
	else if(m_SavedPacket!=NULL)
		m_SavedPacket->reset();
		

#ifdef _WIN32
		::ReleaseMutex(m_hMutex);
#else
		pthread_mutex_unlock(&m_hMutex);
#endif		
}

