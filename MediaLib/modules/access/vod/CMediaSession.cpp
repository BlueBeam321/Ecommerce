#include <stdlib.h>
#include <string.h>



#include "CMediaSession.h"
#include "CTsRtpSource.h"

#include "CRTSPClient.h"

#include <ctype.h>


vlc_mutex_t	CMediaSubsession::s_UdpPortmutex;
long CMediaSubsession::s_InitFlag = false;
long CMediaSubsession::s_appIndex = 0;

////////// CMediaSession //////////
CMediaSession* CMediaSession::createNew(CUsageEnvironment* inEnv, char const* sdpDescription,vodRTPStreamMode inRtpStreamMode,bool inAuthStreamMode)
{
    CMediaSession* newSession = new CMediaSession(inEnv, inRtpStreamMode, inAuthStreamMode);
    if (newSession != NULL)
    {
        if (!newSession->initializeWithSDP(sdpDescription))
        {
            delete newSession;
            return NULL;
        }
    }

    return newSession;
}

extern char	g_MyNethostname[256];

CMediaSession::CMediaSession(CUsageEnvironment* inEnv,vodRTPStreamMode inRtpStreamMode,bool inAuthStreamMode)
  : CMedium(inEnv),
    m_SubsessionsHead(NULL), m_SubsessionsTail(NULL),
    m_ConnectionEndpointName(NULL), m_MaxPlayEndTime(0.0f) 
{
	const unsigned maxCNAMElen = 256;
	char CNAME[maxCNAMElen+1];
	
#if 0
	gethostname((char*)CNAME, maxCNAMElen);
#else
	strcpy((char*)CNAME, g_MyNethostname);
#endif

    CNAME[maxCNAMElen] = '\0';
    m_CNAME = strdup(CNAME);

    m_RtpStreamMode = inRtpStreamMode;
	m_AuthStreamMode = inAuthStreamMode;
}

CMediaSession::~CMediaSession() 
{
    delete m_SubsessionsHead;
    free(m_CNAME);
    delete[] m_ConnectionEndpointName;
}

bool CMediaSession::lookupByName(CUsageEnvironment* inEnv,char const* sessionName, CMediaSession*& resultSession)
{
    resultSession = NULL;

    CMedium* medium;
    if (!CMedium::lookupByName(inEnv, sessionName, medium))
        return false;

    if (!medium->isMediaSession())
    {
        inEnv->setResultMsg(11000103, sessionName, " is not a 'CMediaSession' object");
        return false;
    }

    resultSession = (CMediaSession*)medium;
    return true;
}

bool CMediaSession::isMediaSession() const 
{
    return true;
}

/**********************************************************************************************/
/*
*/
/**********************************************************************************************/
static bool parseSDPLine(char const* inputLine, char const*& nextLine)
{
    // Begin by finding the start of the next line (if any):
    nextLine = NULL;
    for (char const* ptr = inputLine; *ptr != '\0'; ++ptr)
    {
        if (*ptr == '\r' || *ptr == '\n')
        {
            // We found the end of the line
            ++ptr;
            while (*ptr == '\r' || *ptr == '\n') ++ptr;
            nextLine = ptr;
            if (nextLine[0] == '\0') nextLine = NULL; // special case for end
            break;
        }
    }

    // Then, check that this line is a SDP line of the form <char>=<etc>
    // (However, we also accept blank lines in the input.)
    if (inputLine[0] == '\r' || inputLine[0] == '\n')
        return true;

    if (strlen(inputLine) < 2 || inputLine[1] != '=' || inputLine[0] < 'a' || inputLine[0] > 'z')
    {
        //env()->setResultMsg(11000202,"Invalid SDP line: ", inputLine);
        return false;
    }

    return true;
}

static char* parseCLine(char const* sdpLine)
{
    char* resultStr = NULL;
    char* buffer = (char*)malloc(strlen(sdpLine) + 1); // ensures we have enough space
    if (sscanf(sdpLine, "c=IN IP4 %[^a/ ]", buffer) == 1)
    {
        // Later, handle the optional /<ttl> and /<numAddresses> #####
        resultStr = strdup(buffer);
    }
    delete[] buffer;

    return resultStr;
}

/**************************************************************************************/
/*
*/
/**************************************************************************************/
bool CMediaSession::initializeWithSDP(char const* sdpDescription)
{
    if (sdpDescription == NULL)
        return false;

    char const* sdpLine = sdpDescription;
    char const* nextSDPLine;
	
    while (1)
    {
        if (!parseSDPLine(sdpLine, nextSDPLine))
        {
            env()->setResultMsg(11000202, "Invalid SDP line: ", sdpLine);
            return false;
        }

        if (sdpLine[0] == 'm')
            break;
        sdpLine = nextSDPLine;
        if (sdpLine == NULL)
            break;

        if (parseSDPLine_c(sdpLine))
            continue;
        if (parseSDPAttribute_range(sdpLine))
            continue;
    }

    CMediaSubsession::Initialize();
    while (sdpLine != NULL)
    {
        CMediaSubsession* subsession = new CMediaSubsession(*this);
        if (subsession == NULL)
        {
            env()->setResultMsg(11000104, "Unable to create new CMediaSubsession");
            return false;
        }

        char* mediumName = (char*)malloc(strlen(sdpLine) + 1);
        char* protocolName = NULL;
        unsigned payloadFormat;
        if ((sscanf(sdpLine, "m=%s %hu RTP/AVP %u", mediumName, &subsession->m_ClientPortNum, &payloadFormat) == 3 ||
            sscanf(sdpLine, "m=%s %hu/%*u RTP/AVP %u", mediumName, &subsession->m_ClientPortNum, &payloadFormat) == 3) &&
            payloadFormat <= 127)
        {
            protocolName = "RTP";
        }
        else if ((sscanf(sdpLine, "m=%s %hu UDP %u", mediumName, &subsession->m_ClientPortNum, &payloadFormat) == 3 ||
            sscanf(sdpLine, "m=%s %hu udp %u", mediumName, &subsession->m_ClientPortNum, &payloadFormat) == 3 ||
            sscanf(sdpLine, "m=%s %hu RAW/RAW/UDP %u", mediumName, &subsession->m_ClientPortNum, &payloadFormat) == 3) &&
            payloadFormat <= 127)
        {
            protocolName = "UDP";
        }
        else
        {
            char* sdpLineStr;
            if (nextSDPLine == NULL)
            {
                sdpLineStr = (char*)sdpLine;
            }
            else
            {
                sdpLineStr = strdup(sdpLine);
                sdpLineStr[nextSDPLine - sdpLine] = '\0';
            }

            env()->setResultMsg(11000505, "BAD SDP m= ", sdpLineStr);

            if (sdpLineStr != (char*)sdpLine)
                delete[] sdpLineStr;

            delete[] mediumName;
            delete subsession;

            while (1)
            {
                sdpLine = nextSDPLine;
                if (sdpLine == NULL)
                    break;

                if (!parseSDPLine(sdpLine, nextSDPLine))
                    return false;

                if (sdpLine[0] == 'm')
                    break;
            }
            continue;
        }

        if (m_SubsessionsTail == NULL)
            m_SubsessionsHead = m_SubsessionsTail = subsession;
        else
        {
            m_SubsessionsTail->setNext(subsession);
            m_SubsessionsTail = subsession;
        }

        subsession->m_serverPortNum = subsession->m_ClientPortNum;

        char const* mStart = sdpLine;
        subsession->m_SavedSDPLines = strdup(mStart);

        subsession->m_MediumName = strdup(mediumName);
        delete[] mediumName;
        subsession->m_ProtocolName = strdup(protocolName);
        subsession->m_RTPPayloadFormat = payloadFormat;
        subsession->m_AttachmentSize = 0;

        while (1)
        {
            sdpLine = nextSDPLine;
            if (sdpLine == NULL)
                break;
            if (!parseSDPLine(sdpLine, nextSDPLine))
                return false;

            if (sdpLine[0] == 'm')
                break;

            if (subsession->parseSDPLine_c(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_control(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_range(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_fmtp(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_smtp(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_x_dimensions(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_x_framerate(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_DVD_Info(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_ThumbInfo(sdpLine))
                continue;
            if (subsession->parseSDPAttribute_AttachmentInfo(sdpLine))
                continue;
        }

        if (sdpLine != NULL)
            subsession->m_SavedSDPLines[sdpLine - mStart] = '\0';

        if (subsession->m_CodecName == NULL)
        {
            subsession->m_CodecName = lookupPayloadFormat(subsession->m_RTPPayloadFormat, subsession->m_RTPTimestampFrequency, subsession->m_NumChannels);
            if (subsession->m_CodecName == NULL)
            {
                char typeStr[32];
                sprintf(typeStr, "%d", subsession->m_RTPPayloadFormat);
                env()->setResultMsg(11000201, "Unknown codec name for RTP payload type ", typeStr);
                return false;
            }
        }

        if (subsession->m_RTPTimestampFrequency == 0)
            subsession->m_RTPTimestampFrequency = guessRTPTimestampFrequency(subsession->m_MediumName, subsession->m_CodecName);
    }

    return true;
}

bool CMediaSession::parseSDPLine_c(char const* sdpLine) 
{
    char* connectionEndpointName = parseCLine(sdpLine);
    if (connectionEndpointName != NULL)
    {
        delete[] m_ConnectionEndpointName;
        m_ConnectionEndpointName = connectionEndpointName;
        return true;
    }

    return false;
}

static bool parseRangeAttribute(char const* sdpLine, float& startTime,float& endTime) 
{
	float theStartTime,theEndTime;
	bool theResult = sscanf(sdpLine, "a=range: npt = %f - %f", &theStartTime,&theEndTime) == 2;
	startTime = theStartTime;
	endTime = theEndTime;
	return theResult;
}

bool CMediaSession::parseSDPAttribute_range(char const* sdpLine) 
{
    bool parseSuccess = false;
    float playEndTime, playStartTime;
    if (parseRangeAttribute(sdpLine, playStartTime, playEndTime))
    {
        parseSuccess = true;
    }

    return parseSuccess;
}

char* CMediaSession::lookupPayloadFormat(unsigned char rtpPayloadType, unsigned& freq, unsigned& nCh) 
{
    char* temp = NULL;
    switch (rtpPayloadType)
    {
    case 33:
        temp = "MP2T";
        freq = 90000;
        nCh = 1;
        break;
    };

    return strdup(temp);
}

unsigned CMediaSession::guessRTPTimestampFrequency(char const* mediumName, char const* codecName) 
{
    if (strcmp(codecName, "L16") == 0)
        return 44100;

    if (strcmp(codecName, "MPA") == 0 ||
        strcmp(codecName, "MPA-ROBUST") == 0 ||
        strcmp(codecName, "X-MP3-DRAFT-00"))
        return 90000;

    if (strcmp(mediumName, "video") == 0)
        return 90000;

    return 8000;
}

////////// CMediaSubsessionIterator //////////
CMediaSubsessionIterator::CMediaSubsessionIterator(CMediaSession& session)
  : m_OurSession(session) 
{
    reset();
}

CMediaSubsessionIterator::~CMediaSubsessionIterator() 
{
}

CMediaSubsession* CMediaSubsessionIterator::next() 
{
    CMediaSubsession* result = m_NextPtr;

    if (m_NextPtr != NULL)
        m_NextPtr = m_NextPtr->m_Next;

    return result;
}

void CMediaSubsessionIterator::reset() 
{
    m_NextPtr = m_OurSession.m_SubsessionsHead;
}

////////// CMediaSubsession //////////
CMediaSubsession::CMediaSubsession(CMediaSession& parent)
  : m_sessionId(NULL), m_serverPortNum(0),
    m_Parent(parent), m_Next(NULL),
    m_ConnectionEndpointName(NULL),
    m_ClientPortNum(0), m_RTPPayloadFormat(0xFF),
    m_SavedSDPLines(NULL), m_MediumName(NULL), 
	m_CodecName(NULL), m_ProtocolName(NULL),
    m_RTPTimestampFrequency(0), m_ControlPath(NULL),
    m_PlayEndTime(0.0),m_NumChannels(1),m_VideoWidth(0),m_VideoHeight(0),m_VideoFPS(0.0f),m_VideoStart(0.0f),m_VideoEnd(0.0f),
	m_FileType(0),
	m_StreamType(0),
    m_RTPSocket(NULL), m_RTCPSocket(NULL),
    m_RTPSource(NULL), m_RTCPInstance(NULL) 
{
	m_DVDInfoStr = NULL;
	m_ThumbserverPort = 0;
	memset(m_ThumbserverIp,0,sizeof(m_ThumbserverIp));
	memset(m_ThumbIdStr,0,sizeof(m_ThumbIdStr));
	m_AttachmentSize = 0;
}

CMediaSubsession::~CMediaSubsession() 
{
    deInitiate();

    if (m_ConnectionEndpointName)
        delete[] m_ConnectionEndpointName;
    if (m_SavedSDPLines)
        delete[] m_SavedSDPLines;
    if (m_MediumName)
        delete[] m_MediumName;
    if (m_CodecName)
        delete[] m_CodecName;
    if (m_ProtocolName)
        delete[] m_ProtocolName;
    if (m_ControlPath)
        delete[] m_ControlPath;
    if (m_sessionId)
        delete[] m_sessionId;
    if (m_Next)
        delete m_Next;
    if (m_DVDInfoStr)
        delete[] m_DVDInfoStr;
    
    CMediaSubsession::Finalize();
}

void CMediaSubsession::Initialize()
{
    if (s_InitFlag == 0)
	{
		vlc_mutex_init(&s_UdpPortmutex);
		s_InitFlag = 1;
		s_appIndex = 0;
	}
}

void CMediaSubsession::Finalize()
{
	if (s_InitFlag)
	{
        vlc_mutex_destroy(&s_UdpPortmutex);
   		s_appIndex = 0;
        s_InitFlag = 0;
	}
}

float CMediaSubsession::playEndTime() const 
{
  if (m_PlayEndTime > 0)
      return m_PlayEndTime;

  return m_Parent.playEndTime();
}

unsigned short CMediaSubsession::GetIdleClientPort(unsigned short baseUdpPort )
{
	char udpPortName[64] = {0};
	unsigned short maxPort = baseUdpPort + 200;

    if (baseUdpPort & 1)
        baseUdpPort++;
	vlc_mutex_lock(&s_UdpPortmutex);

	baseUdpPort += s_appIndex * 2;
	s_appIndex ++;
	if(s_appIndex > 3)
		s_appIndex = 0;
	
	//while( baseUdpPort < maxPort )
	//{
	//	sprintf(udpPortName , "TV_UDP_%04d",baseUdpPort);
	//	m_hSemaphore = new MxSemaphore(udpPortName);
	//	if(m_hSemaphore->Lock() == false)
	//	{
	//		delete m_hSemaphore;
	//		m_hSemaphore = NULL;
	//		baseUdpPort += 2;
	//		continue;
	//	}
	//	break;
	//}
	
	vlc_mutex_unlock(&s_UdpPortmutex);
	
	return baseUdpPort;
}

/*******************************************************************************************/
/*
*/
/*******************************************************************************************/
bool CMediaSubsession::initiate(unsigned long inSSRC/*RTP*/,int /*useSpecialRTPoffset*/,CRTSPClient* inRTSPClientObj)
{
    if (m_RTPSource != NULL)
        return true; //
  
    do
    {
        if (m_CodecName == NULL)
        {
            env()->setResultMsg(11000203, "Codec is unspecified");
            break;
        }

        bool success = false;
        if (m_Parent.m_RtpStreamMode == eRTPSTREAM_UDP)
        {
            m_ClientPortNum = GetIdleClientPort(m_ClientPortNum);

            while (1)
            {
                if (m_ClientPortNum & 0x1)
                {
                    env()->setResultMsg(11000339, "Client RTP Port is not event!");
                    break;
                }

                m_RTPSocket = new CIoSocket(env(), m_ClientPortNum, m_Parent.m_AuthStreamMode);

                if (m_RTPSocket == NULL)
                {
                    env()->setResultMsg(11000301, "Failed to create RTP socket!");
                    break;
                }
                else if (m_RTPSocket->socketNum() == -1)
                {
                    env()->setResultMsg(11000301, "Failed to create RTP socket!");
                    break;
                }

                success = true;
                break;
            }
        }
        else if (m_Parent.m_RtpStreamMode == eRTPSTREAM_TCP)
        {
            m_RTPSocket = new CIoSocket(env(), m_Parent.m_AuthStreamMode, true, inRTSPClientObj->socketNum(), inRTSPClientObj->authSocketHandle());
            if (m_RTPSocket == NULL)
            {
                env()->setResultMsg(10000301, "Failed to create RTP_TCP socket!");
                break;
            }
            success = true;
        }

        if (!success)
            break;

        unsigned short const rtcpPortNum = m_ClientPortNum | 1;

        if (m_Parent.m_RtpStreamMode == eRTPSTREAM_UDP)
        {
            m_RTCPSocket = new CIoSocket(env(), rtcpPortNum, m_Parent.m_AuthStreamMode);
            char tmpBuf[512];
            if (m_RTCPSocket == NULL)
            {
                sprintf(tmpBuf, "Failed to create RTCP socket (port %d)", rtcpPortNum);
                env()->setResultMsg(11000302, tmpBuf);
                break;
            }
            else if (m_RTCPSocket->socketNum() == -1)
            {
                sprintf(tmpBuf, "Failed to create RTCP socket (port %d)", rtcpPortNum);
                env()->setResultMsg(11000302, tmpBuf);
                break;
            }
        }

        if (strcmp(m_ProtocolName, "RTP") == 0)
        {
            if (strcmp(m_CodecName, "MP2T") == 0)
            { // 
                m_RTPSource = CTsRtpSource::createNew(env(), m_RTPSocket, m_RTPPayloadFormat,
                    m_RTPTimestampFrequency, inSSRC, "video/MP2T", 0, false, m_Parent.m_RtpStreamMode, m_Parent.m_AuthStreamMode, inRTSPClientObj);
            }
            else
            {
                env()->setResultMsg(11000204, "RTP payload format unknown or not supported");
                break;
            }
        }

        if (m_RTPSource == NULL)
        {
            env()->setResultMsg(11000103, "Failed to create read source");
            break;
        }

        if (m_RTPSource != NULL)
        {
            m_RTPSource->WaitRecvReady();
#if 0		
            unsigned totSessionBandwidth = 500; 
            m_RTCPInstance = CRTCPInstance::createNew(env(), 
                m_RTCPSocket,
                totSessionBandwidth,
                (unsigned char const*)m_Parent.
                CNAME(),
                m_RTPSource);
            if (m_RTCPInstance == NULL) 
            {
                env()->setResultMsg(11000104,"Failed to create RTCP instance");
                break;
            }
            m_RTCPInstance->StartRtcping();
#endif		
        }

        return true;
    } while (0);

    if (m_RTPSocket)
        delete m_RTPSocket;
    m_RTPSocket = NULL;
    if (m_RTCPSocket)
        delete m_RTCPSocket;
    m_RTCPSocket = NULL;

    if (m_RTCPInstance != NULL)
        delete m_RTCPInstance;
    m_RTCPInstance = NULL;

    if (m_RTPSource != NULL)
        delete m_RTPSource;
    m_RTPSource = NULL;

    m_ClientPortNum = 0;
    return false;
}

/**********************************************************************************/
/*
*/
/**********************************************************************************/
void CMediaSubsession::deInitiate()
{
    if (m_RTCPInstance != NULL)
    {
        m_RTCPInstance->StopRtcping();
        delete m_RTCPInstance;
    }
    m_RTCPInstance = NULL;

    if (m_RTPSource != NULL)
        delete m_RTPSource;
    m_RTPSource = NULL;

    if (m_RTCPSocket != NULL)
        delete m_RTCPSocket;
    if (m_RTPSocket != NULL)
        delete m_RTPSocket;
    m_RTCPSocket = m_RTPSocket = NULL;
}

/***********************************************************************************/
/***********************************************************************************/
bool CMediaSubsession::setClientPortNum(unsigned short portNum) 
{
    if (m_RTPSource != NULL)
    {
        env()->setResultMsg(11000105, "A read source has already been created");
        return false;
    }

    m_ClientPortNum = portNum;
    return true;
}

/**********************************************************************************/
/*
*/
/**********************************************************************************/
unsigned CMediaSubsession::connectionEndpointAddress() 
{
    do
    {
        char* endpointString = connectionEndpointName();
        if (endpointString == NULL)
            endpointString = parentSession().connectionEndpointName();

        if (endpointString == NULL)
            break;

        uint32_t addr = our_inet_addr((char*)endpointString);

        return addr;
    } while (0);

    return 0;
}

/*********************************************************************************/
/*
*/
/*********************************************************************************/
void CMediaSubsession::setDestinations(unsigned defaultDestAddress)
{
    unsigned destAddress = connectionEndpointAddress();
    if (destAddress == 0) destAddress = defaultDestAddress;

    struct in_addr destAddr;
    destAddr.s_addr = destAddress;

    if (m_RTPSocket != NULL)
    {
        CPort destPort(m_serverPortNum);
        //m_RTPSocket->changeDestinationParameters(destAddr, destPort);
        m_RTPSocket->addDestination(destAddr, destPort, 0);
    }
    if (m_RTCPSocket != NULL)
    {
        CPort destPort(m_serverPortNum + 1);
        //m_RTCPSocket->changeDestinationParameters(destAddr, destPort);
        m_RTCPSocket->addDestination(destAddr, destPort, 0);
    }
}

bool CMediaSubsession::setTitleSDPInfo(char* inSDPStr)
{
    char const* sdpLine = inSDPStr;
    char const* nextSDPLine;

    nextSDPLine = inSDPStr;
    m_AttachmentSize = 0;
    while (1)
    {
        sdpLine = nextSDPLine;// by LGH
        if (sdpLine == NULL) break;
        if (!parseSDPLine(sdpLine, nextSDPLine))
        {
            env()->setResultMsg(11000202, "Invalid SDP line: ", sdpLine);
            return false;
        }

        if (parseSDPAttribute_fmtp(sdpLine))
            continue;
        if (parseSDPAttribute_x_dimensions(sdpLine))
            continue;
        if (parseSDPAttribute_x_framerate(sdpLine))
            continue;
        if (parseSDPAttribute_range(sdpLine))
            continue;
        if (parseSDPAttribute_DVD_Info(sdpLine))
            continue;
        if (parseSDPAttribute_ThumbInfo(sdpLine))
            continue;
        if (parseSDPAttribute_AttachmentInfo(sdpLine))
            continue;
        //sdpLine = nextSDPLine; // by LGH
    }
    return true;
}

bool CMediaSubsession::parseSDPLine_c(char const* sdpLine) 
{
    // Check for "c=IN IP4 <connection-endpoint>"
    // or "c=IN IP4 <connection-endpoint>/<ttl+numAddresses>"
    // (Later, do something with <ttl+numAddresses> also #####)
    char* connectionEndpointName = parseCLine(sdpLine);
    if (connectionEndpointName != NULL)
    {
        delete[] m_ConnectionEndpointName;
        m_ConnectionEndpointName = connectionEndpointName;
        return true;
    }

    return false;
}

bool CMediaSubsession::parseSDPAttribute_control(char const* sdpLine) 
{
    // Check for a "a=control:<control-path>" line:
    bool parseSuccess = false;
    char* controlPath = (char*)malloc(strlen(sdpLine) + 1); // ensures we have enough space
    if (sscanf(sdpLine, "a=control: %s", controlPath) == 1)
    {
        parseSuccess = true;
        delete[] m_ControlPath; m_ControlPath = strdup(controlPath);
    }
    free(controlPath);

    return parseSuccess;
}

bool CMediaSubsession::parseSDPAttribute_range(char const* sdpLine) 
{
    // Check for a "a=range:npt=<startTime>-<endTime>" line:
    // (Later handle other kinds of "a=range" attributes also???#####)
    bool parseSuccess = false;
    float playEndTime, playStartTime;
    if (parseRangeAttribute(sdpLine, playStartTime, playEndTime))
    {
        m_VideoStart = playStartTime;
        m_VideoEnd = playEndTime;
        parseSuccess = true;
        /*
        if (playEndTime > m_PlayEndTime)
        {
        m_PlayEndTime = playEndTime;
        if (playEndTime > m_Parent.playEndTime())
        {
        m_Parent.playEndTime() = playEndTime;
        }
        }*/
    }

    return parseSuccess;
}

bool CMediaSubsession::parseSDPAttribute_x_dimensions(char const* sdpLine) 
{
    // Check for a "a=x-dimensions:<width>,<height>" line:
    bool parseSuccess = false;
    int width, height;
    if (sscanf(sdpLine, "a=x-dimensions:%d,%d", &width, &height) == 2)
    {
        parseSuccess = true;
        m_VideoWidth = (unsigned short)width;
        m_VideoHeight = (unsigned short)height;
    }

    return parseSuccess;
}

bool CMediaSubsession::parseSDPAttribute_x_framerate(char const* sdpLine) 
{
    // Check for a "a=x-framerate:<fps>" line:
    bool parseSuccess = false;
    int rate;
    int rate1;
    if (sscanf(sdpLine, "a=x-fps:%d,%d", &rate, &rate1) == 2)
    {
        parseSuccess = true;
        if (rate1)
            m_VideoFPS = (float)rate / (float)rate1;
    }

    return parseSuccess;
}

bool CMediaSubsession::parseSDPAttribute_DVD_Info(char const* sdpLine) 
{
    // Check for a "a=x-framerate:<fps>" line:
    bool parseSuccess = false;
    long len = 0;

    if (strncmp(sdpLine, "a=dvdinfo:", 10) != 0)
        return parseSuccess;

    len = strlen(sdpLine) + 20;
    if (m_DVDInfoStr != NULL)
        delete[] m_DVDInfoStr;
    m_DVDInfoStr = new char[len];

    if (sscanf(sdpLine, "a=dvdinfo:%s", m_DVDInfoStr) == 1)
        parseSuccess = true;
    else
    {
        if (m_DVDInfoStr != NULL)
            delete[] m_DVDInfoStr;
        m_DVDInfoStr = NULL;
    }

    return parseSuccess;
}

bool CMediaSubsession::parseSDPAttribute_ThumbInfo(char const* sdpLine) 
{
	bool parseSuccess = false;
    int i, len = 0;
	char* temp;

    if (strncmp(sdpLine, "a=thumbinfo:", 12) != 0)
        return parseSuccess;

    len = strlen(sdpLine) + 20;
    temp = (char*)malloc(len);
    strcpy(temp, sdpLine);
    len = strlen(temp);
	
    for (i = 0; i < len; i++)
	{
        if (temp[i] == '\r' || temp[i] == '\n')
		{
			temp[i] = 0;
			break;
		}
	}
	
    if (sscanf(temp, "a=thumbinfo:%s %d %s", m_ThumbserverIp, &m_ThumbserverPort, m_ThumbIdStr) == 3)
		parseSuccess = true;
	
	free(temp);

	return parseSuccess;
}


bool CMediaSubsession::parseSDPAttribute_fmtp(char const* sdpLine)
{
    // Check for a "a=fmtp:" line:
    // TEMP: We check only for a handful of expected parameter names #####
    // Later: (i) check that payload format number matches; #####
    //        (ii) look for other parameters also (generalize?) #####  
    do
    {
        if (strncmp(sdpLine, "a=fmtp:", 7) != 0)
            break;

        sdpLine += 7;
        while (isdigit(*sdpLine))
            ++sdpLine;

        // The remaining "sdpLine" should be a sequence of
        //     <name>=<value>;
        // parameter assignments.  Look at each of these.
        // First, convert the line to lower-case, to ease comparison:
        char* const lineCopy = strdup(sdpLine);
        char* line = lineCopy;
        for (char* c = line; *c != '\0'; ++c)
            *c = tolower(*c);

        while (*line != '\0' && *line != '\r' && *line != '\n')
        {
            unsigned u;
            if (sscanf(line, " codectype = %u", &u) == 1)
            {
                m_FileType = u;
            }

            // Move to the next parameter assignment string:
            while (*line != '\0' && *line != '\r' && *line != '\n' && *line != ';')
                ++line;
            while (*line == ';')
                ++line;

            // Do the same with sdpLine; needed for finding case sensitive values:
            while (*sdpLine != '\0' && *sdpLine != '\r' && *sdpLine != '\n' && *sdpLine != ';')
                ++sdpLine;
            while (*sdpLine == ';')
                ++sdpLine;
        }
        free(lineCopy);

        return true;
    } while (0);

    return false;
}

bool CMediaSubsession::parseSDPAttribute_smtp(char const* sdpLine)
{
	// Check for a "a=fmtp:" line:
	// TEMP: We check only for a handful of expected parameter names #####
	// Later: (i) check that payload format number matches; #####
	//        (ii) look for other parameters also (generalize?) #####  
	do 
	{
		if (strncmp(sdpLine, "a=smtp:", 7) != 0)
            break;
        
        sdpLine += 7;
		while (isdigit(*sdpLine))
            ++sdpLine;
		
		// The remaining "sdpLine" should be a sequence of
		//     <name>=<value>;
		// parameter assignments.  Look at each of these.
		// First, convert the line to lower-case, to ease comparison:
		char* const lineCopy = strdup(sdpLine);
        char* line = lineCopy;
		for (char* c = line; *c != '\0'; ++c)
            *c = tolower(*c);

		while (*line != '\0' && *line != '\r' && *line != '\n') 
		{
			char typestr[8];
			memset(typestr, 0, sizeof(typestr));
			if (sscanf(line, " streamtype = %s", typestr) == 1) 
			{
				if (strcmp(typestr, "ts") == 0)
					m_StreamType = 0;
				else
					m_StreamType = 1;
			} 
			
			// Move to the next parameter assignment string:
			while (*line != '\0' && *line != '\r' && *line != '\n' && *line != ';')
                ++line;
			while (*line == ';')
                ++line;
			
			// Do the same with sdpLine; needed for finding case sensitive values:
			while (*sdpLine != '\0' && *sdpLine != '\r' && *sdpLine != '\n' && *sdpLine != ';')
                ++sdpLine;
			while (*sdpLine == ';')
                ++sdpLine;
		}
		free(lineCopy);

		return true;
	} while (0);
	
	return false;
}

bool CMediaSubsession::parseSDPAttribute_AttachmentInfo(char const* sdpLine)
{
	bool parseSuccess = false;
	long i,len = 0;
	char	*temp;

    if (strncmp(sdpLine, "a=attachmentinfo:", 17) != 0)
        return parseSuccess;

    len = strlen(sdpLine) + 20;
    temp = (char*)malloc(len);
    strcpy(temp, sdpLine);
    len = strlen(temp);

    for (i = 0; i < len; i++)
	{
        if (temp[i] == '\r' || temp[i] == '\n')
		{
			temp[i] = 0;
			break;
		}
	}

    if (sscanf(temp, "a=attachmentinfo:%ld", &m_AttachmentSize) == 1)
		parseSuccess = true;
	free(temp);

	return parseSuccess;
}
