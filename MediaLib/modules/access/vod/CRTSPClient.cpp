
#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#ifdef _WIN32
#ifndef SO_NOSIGPIPE
#define SO_NOSIGPIPE	0
#endif
#endif

#ifndef SO_NOSIGPIPE
#define SO_NOSIGPIPE    MSG_NOSIGNAL
#endif

#include "MxErrCodes.h"
#include "CRTSPClient.h"
#include "SocketHelper.h"

#include "ErrorCode.h"

#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include "MxSocketConfig.h"
#include <netinet/tcp.h>
#define _strnicmp	strncmp
#endif

    ////////// CRTSPClient //////////
    bool _CRTSPClient_IdleProc(void* inParam)
    {
        return false;
    }

    CRTSPClient*
        CRTSPClient::createNew(CUsageEnvironment* inEnv, char* inIpAddrStr, char const* applicationName, long bStbFlag, bool bAuthStreamMode)
    {
        return new CRTSPClient(inEnv, inIpAddrStr, applicationName, bStbFlag, bAuthStreamMode);
    }

    bool
        CRTSPClient::lookupByName(CUsageEnvironment* inEnv, char const* instanceName, CRTSPClient*& resultClient)
    {
        resultClient = NULL;

        CMedium* medium;
        if (!CMedium::lookupByName(inEnv, instanceName, medium)) return false;

        if (!medium->isRTSPClient())
        {
            inEnv->setResultMsg(11001011, instanceName, " is not a RTSP client");
            return false;
        }

        resultClient = (CRTSPClient*)medium;
        return true;
    }

    CRTSPClient::CRTSPClient(CUsageEnvironment* inEnv, char* inIpAddrStr, char const* /*applicationName*/, long bStbFlag, bool bAuthStreamMode)
        : CMedium(inEnv),
        m_InputSocketNum(-1), m_ServerAddress(0),
        m_BaseURL(NULL), m_LastSessionId(NULL)
    {
        m_ResponseBufferSize = 256 * 1024;
        m_ResponseBuffer = new char[m_ResponseBufferSize + 1];
        m_ResponseBuffer1 = new char[m_ResponseBufferSize + 1];

        m_ResponseBufferSizeByOther = 256 * 1024;//20000;
        m_ResponseBufferByOther = new char[m_ResponseBufferSizeByOther + 1];


        m_LineQuality = 1;
        m_ControlServerAddr = NULL;
        m_UserSpecificData = NULL;
        m_UserName = NULL;

        m_IpAddrNum = inet_addr(inIpAddrStr);
        memset(m_IpAddrString, 0, sizeof(m_IpAddrString));

        m_bStbFlag = bStbFlag;

        memset(m_ServerAddressStr, 0, sizeof(m_ServerAddressStr));
        m_DestPortNum = 9113;
        m_CSeq = 1;
        m_RtpStreamMode = eRTPSTREAM_UDP;
        m_bTcpSocketErrorFlag = 0;
        m_RtpChannelID = 1;
        m_AuthInputSocket = NULL;

        m_AuthStreamMode = bAuthStreamMode;
    }

    CRTSPClient::~CRTSPClient()
    {
        reset();

        if (m_ResponseBuffer != NULL)
            delete[] m_ResponseBuffer;

        if (m_ResponseBuffer1 != NULL)
            delete[] m_ResponseBuffer1;
        if (m_ResponseBufferByOther != NULL)
            delete[] m_ResponseBufferByOther;

        if (m_UserSpecificData != NULL)
            delete[] m_UserSpecificData;
        if (m_UserName != NULL)
            delete[] m_UserName;
        if (m_ControlServerAddr != NULL)
            delete[] m_ControlServerAddr;
    }

    bool
        CRTSPClient::isRTSPClient() const
    {
        return true;
    }

    void
        CRTSPClient::resetTCPSockets(bool inAuthStreamMode)
    {
        if (inAuthStreamMode == false)
        {
            if (m_InputSocketNum >= 0)
            {
                ::closesocket(m_InputSocketNum);
            }
            m_InputSocketNum = -1;
        }
        else if (inAuthStreamMode)
        {
            if (m_AuthInputSocket)
            {
                m_AuthInputSocket->Close();
                delete m_AuthInputSocket;
            }
            m_AuthInputSocket = NULL;
        }
    }

    void
        CRTSPClient::reset()
    {
        resetTCPSockets();
        m_ServerAddress = 0;

        delete[] m_BaseURL; m_BaseURL = NULL;

        delete[] m_LastSessionId; m_LastSessionId = NULL;
    }

    static char* getLine(char* startOfLine)
    {
        // returns the start of the next line, or NULL if none
        for (char* ptr = startOfLine; *ptr != '\0'; ++ptr)
        {
            if (*ptr == '\r' || *ptr == '\n')
            {
                // We found the end of the line
                *ptr++ = '\0';
                if (*ptr == '\n') ++ptr;
                return ptr;
            }
        }

        return NULL;
    }

    static char* getLine1(char* startOfLine)
    {
        // returns the start of the next line, or NULL if none'
        for (char* ptr = startOfLine; *ptr != '\0'; ++ptr)
        {
            if (*ptr == '\r' || *ptr == '\n')
            {
                // We found the end of the line
                //*ptr++ = '\0';
                ptr++;
                if (*ptr == '\n') ++ptr;
                return ptr;
            }
        }
        return NULL;
    }

    char*
        CRTSPClient::describeURL(char const* url, short inTitleIndex,
        long inRedirect,
        unsigned short& outPort, char* outServerAddr,
        unsigned short* outThumbPort, char* outThumbAddr, char* outThumbIdStr, long inNetSpeed)
    {/* For ex:"rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t*/
        char* cmd = NULL;
        char* theLineQualityStr = NULL;
        char* theTitleIndexStr = NULL;
        char* theNetSpeedStr = NULL;

        m_DescribeStatusCode = 0;

        do
        {
            if (!openConnectionFromURL(url)) break;
            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            // DESCRIBE
            short theAuResult;

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);
            theTitleIndexStr = new char[64];
            sprintf(theTitleIndexStr, "Title: %d\r\n", inTitleIndex);
            theNetSpeedStr = new char[128];
            sprintf(theNetSpeedStr, "NetKSpeed: %d\r\n", inNetSpeed);

            char TagStr[32];
            char const* acceptStr = "Accept: application/sdp\r\n";
            char* const cmdFmt =
                "%s %s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "%s"
                "%s"
                "%s"
                "%s"
                "Authenticator: %s\r\n"
                "\r\n";

            memset(TagStr, 0, sizeof(TagStr));
            sprintf(TagStr, "DESCRIBE");
            if (inRedirect)
                strcat(TagStr, "1");
            else
                strcat(TagStr, "0");

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(TagStr)
                + strlen(m_BaseURL)
                + 20 /* max int len */
                + strlen(acceptStr)
                + strlen(theLineQualityStr)
                + strlen(theTitleIndexStr)
                + strlen(theNetSpeedStr)
                + strlen(m_UserSpecificData);
            cmd = new char[cmdSize + 100];
            sprintf(cmd,
                cmdFmt,
                TagStr,
                m_BaseURL,
                ++m_CSeq,
                acceptStr,
                theLineQualityStr,
                theTitleIndexStr,
                theNetSpeedStr,
                m_UserSpecificData);

            if (!sendRequest(cmd, "DESCRIBE", m_AuthStreamMode))
                break;

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse_inside("DESCRIBE", bytesRead, responseCode, firstLine, nextLineStart, false, m_AuthStreamMode))
                break;

            if (responseCode != 200)
            {
                if (responseCode == 100) //
                {
                    int contentLength = -1;
                    char* lineStart;
                    bool IsFind = false;

                    while (1)
                    {
                        lineStart = nextLineStart;
                        if (lineStart == NULL) break;

                        nextLineStart = getLine(lineStart);
                        if (lineStart[0] == '\0') break;

                        if (strstr(lineStart, "Location: "))
                        {
                            IsFind = true;
                            break;
                        }
                    }

                    if (IsFind == false)
                        break;

                    m_DescribeStatusCode = 100;

                    // Parse the URL as "rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t"
                    char const* prefix = "Location: rtsp://";
                    unsigned const prefixLength = 17;
                    if (_strnicmp(lineStart, prefix, prefixLength) != 0)
                    {
                        //inEnv->setResultMsg(11000211,"URL is not of the form \"", prefix, "\"");
                        break;
                    }

                    delete[] cmd;
                    delete[] theLineQualityStr;
                    delete[] theTitleIndexStr;
                    delete[] theNetSpeedStr;

                    if (m_UserName != NULL)
                        delete[] m_UserName;
                    if (m_ControlServerAddr != NULL)
                        delete[] m_ControlServerAddr;

                    m_UserName = NULL;
                    m_ControlServerAddr = NULL;

                    unsigned const parseBufferSize = 100;
                    char parseBuffer[parseBufferSize];
                    char const* from = &lineStart[prefixLength];
                    char* to = &parseBuffer[0];
                    unsigned i;
                    char theServerAddrStr[parseBufferSize];
                    unsigned long	address;
                    unsigned short	portNum;
                    int	thumbPort = 0;
                    char	theThumbAddrStr[64] = { 0, };
                    char	theumbIdStr[128] = { 0, };

                    to = &theServerAddrStr[0];
                    for (i = 0; i < parseBufferSize; ++i)
                    {
                        if (*from == '\0' || *from == ':' || *from == '/')
                        {
                            *to = '\0';
                            break;
                        }
                        *to++ = *from++;
                    }

                    if (i == parseBufferSize)
                    {
                        //inEnv->setResultMsg(11000212,"URL is too long");
                        return NULL;
                    }

                    address = inet_addr(theServerAddrStr);
                    if (outServerAddr)
                        strcpy(outServerAddr, theServerAddrStr);

                    portNum = 9113;//554;
                    char nextChar = *from;
                    if (nextChar == ':')
                    {
                        int portNumInt;
                        if (sscanf(++from, "%d", &portNumInt) != 1)
                        {
                            //inEnv->setResultMsg(11000213,"No port number follows ':'");
                            return NULL;
                        }
                        if (portNumInt < 1 || portNumInt > 65535)
                        {
                            //inEnv->setResultMsg(11000214,"Bad port number!");
                            return NULL;
                        }
                        portNum = (uint16_t)portNumInt;
                        while (*from >= '0' && *from <= '9') ++from;
                    }

                    if (*from == '/') ++from;
                    outPort = portNum;

                    if (nextLineStart != NULL)
                    {
                        //get thumb info
                        char const* prefix = "a=thumbinfo:";
                        unsigned const prefixLength = 12;
                        if (_strnicmp(nextLineStart, prefix, prefixLength) != 0)
                        {
                            //inEnv->setResultMsg(11000211,"URL is not of the form \"", prefix, "\"");
                            break;
                        }

                        from = &nextLineStart[prefixLength];
                        if (strlen(from) > 0)
                        {
                            sscanf(from, "%s %d %s", theThumbAddrStr, &thumbPort, theumbIdStr);

                            //outThumbPort, char* outThumbAddr
                            if (outThumbPort) *outThumbPort = thumbPort;
                            if (outThumbAddr) strcpy(outThumbAddr, theThumbAddrStr);
                            if (outThumbIdStr) strcpy(outThumbIdStr, theumbIdStr);
                        }
                        else
                        {
                            if (outThumbPort) *outThumbPort = 0;
                            if (outThumbAddr) strcpy(outThumbAddr, "");
                            if (outThumbIdStr) strcpy(outThumbIdStr, "");
                        }
                    }
                    else
                    {
                        if (outThumbPort) *outThumbPort = 0;
                        if (outThumbAddr) strcpy(outThumbAddr, "");
                        if (outThumbIdStr) strcpy(outThumbIdStr, "");
                    }

                    resetTCPSockets();

                    return NULL;
                }
                else if (responseCode == 401)
                {
                    env()->setResultMsg(11000704, "No Acceptable Client!", firstLine);
                    m_DescribeStatusCode = 4;
                }
                else if (responseCode == 301)
                {
                    env()->setResultMsg(11000901, "No Media Data!", firstLine);
                    m_DescribeStatusCode = 5;
                }
                else if (responseCode == 300)
                {
                    env()->setResultMsg(11000902, "Over Users!", firstLine);
                    m_DescribeStatusCode = 6;
                }
                else if (responseCode == 500)
                {
                    env()->setResultMsg(11000903, "Internal Server Error!", firstLine);
                    m_DescribeStatusCode = 7;
                }
                else
                    env()->setResultMsg(11000205, "cannot handle DESCRIBE response: ", firstLine);
                break;
            }

            char* serverType = new char[m_ResponseBufferSize];
            int contentLength = -1;
            char* lineStart;

            while (1)
            {
                lineStart = nextLineStart;
                if (lineStart == NULL) break;

                nextLineStart = getLine(lineStart);
                if (lineStart[0] == '\0') break;

                if (sscanf(lineStart, "Content-Length: %d", &contentLength) == 1 || sscanf(lineStart, "Content-length: %d", &contentLength) == 1)
                {
                    if (contentLength < 0)
                    {
                        env()->setResultMsg(11000206, "Bad \"Content-length:\" header: \"", lineStart, "\"");
                        break;
                    }
                }
                /*	  else if (sscanf(lineStart, "Server: %s", serverType) == 1)
                {
                if (strncmp(serverType, "SoBaekSu", 7) == 0) m_ServerIsSoBaekSu = true;
                }*/
            }
            delete[] serverType;

            if (lineStart == NULL)
            {
                env()->setResultMsg(11000207, "no content following header lines: ", m_ResponseBuffer);
                break;
            }

            char* bodyStart = new char[contentLength + 10];
            memcpy(bodyStart, nextLineStart, strlen(nextLineStart) + 1);

            int		readbyte = 0;
            if (contentLength >= 0)
            {
                int numBodyBytes = &firstLine[bytesRead] - nextLineStart;
                if (contentLength > numBodyBytes)
                {
                    int numExtraBytesNeeded = contentLength - numBodyBytes;
                    delete[]	bodyStart;
                    bodyStart = new char[numExtraBytesNeeded + 10];

                    int theResult;
                    struct timeval theTimeOut;
                    theTimeOut.tv_sec = 0;
                    theTimeOut.tv_usec = 20000;
                    fd_set theRd_set;

                    int bytesRead2;
                    unsigned long tick = mdate();

                    readbyte = 0;
                    while (numExtraBytesNeeded > 0)
                    {
                        do
                        {
                            FD_ZERO(&theRd_set);
                            FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                            theResult = select(m_InputSocketNum + 1, &theRd_set, NULL, NULL, &theTimeOut);
                            if (theResult > 0)
                                break;
                            else if (theResult < 0)
                            {
                                theResult = WSAGetLastError();
                                if ((theResult != WSAEWOULDBLOCK) && (theResult != WSAEINPROGRESS))
                                {
                                    theResult = -1;
                                    break;
                                }
                            }
                            msleep(1000);
                            if ((mdate() - tick) > SOCK_TIMEOUT_VALUE)
                            {
                                theResult = -2;// time out error

#ifndef NDEBUG
                                //fprintf(stderr,"Network time out(%s:%d)\n",__FILE__,__LINE__);
#endif							

                                break;
                            }
                        } while (1);

                        if (theResult < 0)
                        {
                            if (theResult == -2)
                                env()->setResultMsg(11000504, "RTSP response SDP Error(TimeOut)!");
                            else
                                env()->setResultMsg(11000504, "RTSP response SDP Error(select)!");

                            break;
                        }
                        char* ptr = &bodyStart[readbyte];
                        bytesRead2 = recv(m_InputSocketNum, (char*)ptr, numExtraBytesNeeded, 0);
                        if (bytesRead2 <= 0)
                        {
                            env()->setResultMsg(11000504, "RTSP response SDP Error(recv)!");
                            break;
                        }

                        ptr[bytesRead2] = '\0';

                        readbyte += bytesRead2;
                        numExtraBytesNeeded -= bytesRead2;
                    }
                    if (numExtraBytesNeeded > 0)
                        break; // one of the reads failed
                }

                if (bodyStart != NULL)
                {
                    int from, to = 0;
                    for (from = 0; from < contentLength; ++from)
                    {
                        if (bodyStart[from] != '\0')
                        {
                            if (to != from) bodyStart[to] = bodyStart[from];
                            ++to;
                        }
                    }

                    bodyStart[to] = '\0';
                }
            }

            if (bodyStart == NULL)
            {
                bodyStart = strdup("\r\n\r\n");
            }

            delete[] cmd;
            delete[] theLineQualityStr;
            delete[] theTitleIndexStr;
            delete[] theNetSpeedStr;

            return bodyStart;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        if (theTitleIndexStr != NULL)delete[] theTitleIndexStr;
        if (theNetSpeedStr != NULL) delete[] theNetSpeedStr;

        if (m_DescribeStatusCode == 0) m_DescribeStatusCode = 2;
        return NULL;
    }

    static bool isAbsoluteURL(char const* url)
    {
        while (*url != '\0' && *url != '/')
        {
            if (*url == ':') return true;
            ++url;
        }

        return false;
    }

    void
        CRTSPClient::constructSubsessionURL(CMediaSubsession const& subsession, char const*& prefix, char const*& separator, char const*& suffix)
    {
        prefix = m_BaseURL;
        if (prefix == NULL) prefix = "";

        suffix = subsession.controlPath();
        if (suffix == NULL) suffix = "";

        if (isAbsoluteURL(suffix))
        {
            prefix = separator = "";
        }
        else
        {
            unsigned prefixLen = strlen(prefix);
            separator = (prefix[prefixLen - 1] == '/' || suffix[0] == '/') ? "" : "/";
        }
    }

    bool CRTSPClient::attachmentreqMediaSubsession(CMediaSubsession& subsession, short inTitleIndex, unsigned char* pAttachmentBuffer, int iAttachmentbuffersize)
    {
        char* cmd = NULL;
        char* theLineQualityStr = NULL;
        char* theTitleIndexStr = NULL;

        do
        {
            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            short theAuResult;
            char*	theAuStr = "";

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);
            theTitleIndexStr = new char[64];
            sprintf(theTitleIndexStr, "Title: %d\r\n", inTitleIndex);
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", inTitleIndex);

            char* const cmdFmt =
                "ATTACHMENTREQ %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "%s" /*Title:*/
                "%s" /*linequaliy:*/
                "%s" /*Authentication*/
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(theLineQualityStr)
                + strlen(theTitleIndexStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                theTitleIndexStr,
                theAuStr);

            delete[] theAuStr;
            delete[] theLineQualityStr;
            delete[] theTitleIndexStr;

            if (!sendRequest(cmd, "ATTACHMENTREQ", false)) break;      //intended not to apply attachment to 414...

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse_inside("ATTACHMENTREQ", bytesRead, responseCode, firstLine, nextLineStart)) break;

            int theResult;
            struct timeval theTimeOut;
            theTimeOut.tv_sec = 0;
            theTimeOut.tv_usec = 20000;
            fd_set theRd_set;

            int bytesRead2;
            unsigned long tick = mdate();
            bytesRead = 0;
            //recv binary data
            while (iAttachmentbuffersize > 0)
            {
                do
                {
                    FD_ZERO(&theRd_set);
                    FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                    theResult = select(m_InputSocketNum + 1, &theRd_set, NULL, NULL, &theTimeOut);
                    if (theResult > 0)
                        break;
                    else if (theResult < 0)
                    {
                        theResult = WSAGetLastError();
                        if ((theResult != WSAEWOULDBLOCK) && (theResult != WSAEINPROGRESS))
                        {
                            theResult = -1;
                            break;
                        }
                    }
                    msleep(1000);
#ifndef _DEBUG
                    if ((mdate(), tick) > SOCK_TIMEOUT_VALUE)
                    {
                        theResult = -2;// time out error
                        break;
                    }
#endif
                } while (1);

                if (theResult < 0)
                {
                    if (theResult == -2)
                        env()->setResultMsg(11000504, "RTSP response Attachment Error(TimeOut)!");
                    else
                        env()->setResultMsg(11000504, "RTSP response Attachment Error(select)!");

                    break;
                }
                char* ptr = (char*)&pAttachmentBuffer[bytesRead];
                bytesRead2 = recv(m_InputSocketNum, (char*)ptr, iAttachmentbuffersize, 0);
                if (bytesRead2 <= 0)
                {
                    env()->setResultMsg(11000504, "RTSP response Attachment Error(recv)!");
                    break;
                }

                bytesRead += bytesRead2;
                iAttachmentbuffersize -= bytesRead2;
            }

            if (cmd != NULL) delete[] cmd;

            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        return false;
    }

    bool
        CRTSPClient::setupMediaSubsession(CMediaSubsession& subsession, float& outStart, float& outEnd, float inStart, short inTitleIndex, short inChapterIndex)
    {
        char* cmd = NULL;
        char* setupStr = NULL;
        char* theLineQualityStr = NULL;
        char* theTitleIndexStr = NULL;
        char* theChapterIndexStr = NULL;
        char* theThumbIndexStr = NULL;

        m_RtpStreamMode = subsession.GetRtpStreamMode();

        do
        {
            char* sessionStr;
            if (m_LastSessionId != NULL)
            {
                sessionStr = new char[20 + strlen(m_LastSessionId)];
                sprintf(sessionStr, "Session: %s\r\n", m_LastSessionId);
            }
            else
            {
                sessionStr = "";
            }

            char* transportStr = NULL;
            char const *prefix, *separator, *suffix;

            constructSubsessionURL(subsession, prefix, separator, suffix);
            char* transportFmt;

            if (strcmp(subsession.protocolName(), "RTP") == 0)
            {
                char const* setupFmt = "SETUP %s%s%s RTSP/1.0\r\n";
                unsigned setupSize = strlen(setupFmt) + strlen(prefix) + strlen(separator) + strlen(suffix);
                setupStr = new char[setupSize + 16];
                sprintf(setupStr, setupFmt, prefix, separator, suffix);

                transportFmt = "Transport: RTP/AVP%s%s=%d-%d\r\n";
            }
            else
                break;

            if (transportStr == NULL)
            {
                // Construct a "Transport:" header.
                char const* transportTypeStr;
                char const* portTypeStr;
                unsigned short rtpNumber, rtcpNumber;

                if (m_RtpStreamMode == eRTPSTREAM_UDP)
                {
                    transportTypeStr = ";unicast";
                    portTypeStr = ";client_port";
                    rtpNumber = subsession.clientPortNum();
                    if (rtpNumber == 0)
                    {
                        env()->setResultMsg(11000209, "Client port number unknown!");
                        break;
                    }
                }
                if (m_RtpStreamMode == eRTPSTREAM_TCP)
                {
                    transportTypeStr = "/TCP;unicast";
                    portTypeStr = ";interleaved";
                    rtpNumber = GetNextRtpChannelID();
                }

                rtcpNumber = rtpNumber + 1;

                unsigned transportSize = strlen(transportFmt)
                    + strlen(transportTypeStr) + strlen(portTypeStr) + 2 * 5 /* max port len */;
                transportStr = new char[transportSize + 16];
                sprintf(transportStr, transportFmt, transportTypeStr, portTypeStr, rtpNumber, rtcpNumber);
            }

            if (m_RtpStreamMode == eRTPSTREAM_UDP)
                env()->setResultMsg(1000, "RtpStreamMode = UDP");
            else if (m_RtpStreamMode == eRTPSTREAM_TCP)
                env()->setResultMsg(1000, "RtpStreamMode = TCP");

            char* rangeStr = createRangeString(inStart, -1.0);

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            short theAuResult;
            char*	theAuStr = "";

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);
            theTitleIndexStr = new char[64];
            sprintf(theTitleIndexStr, "Title: %d\r\n", inTitleIndex);
            theChapterIndexStr = new char[64];
            sprintf(theChapterIndexStr, "Chapter: %d\r\n", inChapterIndex);
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "%s"
                "CSeq: %d\r\n"
                "%s"
                "%s"
                "%s"
                "%s"
                "%s"
                "%s"
                "Pause: 1\r\n" //epg_ref
                "%s"
                "\r\n";

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(setupStr)
                + 20 /* max int len */
                + strlen(transportStr)
                + strlen(sessionStr)
                + strlen(theLineQualityStr)
                + strlen(rangeStr)
                + strlen(theTitleIndexStr)
                + strlen(theChapterIndexStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                setupStr,
                ++m_CSeq,
                transportStr,
                sessionStr,
                theLineQualityStr,
                rangeStr,
                theTitleIndexStr,
                theChapterIndexStr,
                theAuStr);

            if (sessionStr[0] != '\0') delete[] sessionStr;
            delete[] setupStr; delete[] transportStr;
            delete[] theLineQualityStr;
            delete[] rangeStr;
            delete[] theAuStr;
            delete[] theTitleIndexStr;
            delete[] theChapterIndexStr;

            if (!sendRequest(cmd, "SETUP", m_AuthStreamMode)) break;

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("SETUP", bytesRead, responseCode, firstLine, nextLineStart)) break;

            char* lineStart;
            char* sessionId = new char[m_ResponseBufferSize];
            while (1)
            {
                lineStart = nextLineStart;
                if (lineStart == NULL) break;

                nextLineStart = getLine(lineStart);

                if (sscanf(lineStart, "Session: %[^;]", sessionId) == 1)
                {
                    subsession.m_sessionId = strdup(sessionId);
                    delete[] m_LastSessionId; m_LastSessionId = strdup(sessionId);
                    continue;
                }

                if (parseRangeHeader(lineStart, outStart, outEnd))
                    continue;

                char* serverAddressStr;
                uint16_t serverPortNum;
                if (parseTransportResponse(lineStart, serverAddressStr, serverPortNum))
                {
                    delete[] subsession.connectionEndpointName();
                    subsession.SeConnectionEndPointName(serverAddressStr);
                    subsession.m_serverPortNum = serverPortNum;
                    continue;
                }
            }
            delete[] sessionId;

            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000210, "\"Session:\" header is missing in the response!");
                break;
            }
            else
            {
                char		msg_txt[128];
                if (subsession.m_sessionId)
                {
#ifndef __ppc__
                    sprintf(msg_txt, "@@@SessionId %u, serverId=%u", atoi(subsession.m_sessionId), (long)(m_ServerAddress >> 24));
#else
                    sprintf(msg_txt, "@@@SessionId %u, serverId=%u", atoi(subsession.m_sessionId), (long)(m_ServerAddress & 0xFF));
#endif
                    env()->setResultMsg(1000, msg_txt);
                }
            }
            subsession.setDestinations(m_ServerAddress);

            delete[] cmd;
            return true;
        } while (0);

        delete[] cmd;
        return false;
    }

    bool
        CRTSPClient::playMediaSubsession(CMediaSubsession& subsession, float start, float end, short inTitleIndex, short inChapterIndex, int64_t inFilepos, int64_t inStartCr)
    {
        char* cmd = NULL;
        char* theLineQualityStr = NULL;
        char* theTitleIndexStr = NULL;
        char* theChapterIndexStr = NULL;
        char* theThumbIndexStr = NULL;
        char* theRtpChannelIDStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            char* rangeStr = createRangeString(start, end);

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            short theAuResult;
            char*	theAuStr = "";

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);
            theTitleIndexStr = new char[64];
            sprintf(theTitleIndexStr, "Title: %d\r\n", inTitleIndex);
            theChapterIndexStr = new char[64];
            sprintf(theChapterIndexStr, "Chapter: %d\r\n", inChapterIndex);
            theThumbIndexStr = new char[64];
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: 0\r\n", inTitleIndex);

            if (inFilepos == -1 && inStartCr == -1)
            {
                strcpy(theThumbIndexStr, "");
            }
            else
            {
#ifdef _WIN32
                sprintf(theThumbIndexStr, "Thumb: %I64d %I64d\r\n", inFilepos, inStartCr);
#else
                sprintf(theThumbIndexStr, "Thumb: %lld %lld\r\n", inFilepos, inStartCr);
#endif
            }

            theRtpChannelIDStr = new char[64];
            if (m_RtpStreamMode != eRTPSTREAM_TCP)
            {
                strcpy(theRtpChannelIDStr, "");
            }
            else
            {
                if (start == -1 && inChapterIndex == -1 && inFilepos == -1 && inStartCr == -1)
                {
                }
                else
                {
                    sprintf(theRtpChannelIDStr, "RtpChannelID: %d\r\n", GetNextRtpChannelID());
                }
            }

            char* const cmdFmt =
                "PLAY %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s"
                "%s"
                "%s"
                "%s"
                "%s"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theLineQualityStr)
                + strlen(rangeStr)
                + strlen(theTitleIndexStr)
                + strlen(theChapterIndexStr)
                + strlen(theThumbIndexStr)
                + strlen(theRtpChannelIDStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                rangeStr,
                theTitleIndexStr,
                theChapterIndexStr,
                theThumbIndexStr,
                theRtpChannelIDStr,
                theAuStr);

            delete[] theAuStr;
            delete[] rangeStr;
            delete[] theLineQualityStr;
            delete[] theTitleIndexStr;
            delete[] theChapterIndexStr;
            delete[] theThumbIndexStr;
            delete[] theRtpChannelIDStr;

            if (!sendRequest(cmd, "PLAY", m_AuthStreamMode)) break;

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("PLAY", bytesRead, responseCode, firstLine, nextLineStart)) break;

            if (cmd != NULL) delete[] cmd;
            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        return false;
    }

    bool
        CRTSPClient::pauseMediaSubsession(CMediaSubsession& subsession)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "PAUSE %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theLineQualityStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                theAuStr);

            if (!sendRequest(cmd, "PAUSE", m_AuthStreamMode)) break;

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("PAUSE", bytesRead, responseCode, firstLine, nextLineStart, true, true)) break;

            delete[] cmd;
            delete[] theAuStr;
            delete[] theLineQualityStr;
            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        return false;
    }

    char*
        CRTSPClient::getparameterMediaSubsession(CMediaSubsession& subsession, short inTitleIndex)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;
        char* theTitleIndexStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            theTitleIndexStr = new char[64];
            sprintf(theTitleIndexStr, "Title: %d\r\n", inTitleIndex);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: 0\r\n", inTitleIndex);

            char* const cmdFmt =
                "GET_PARAMETER %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s"/*LineQuality*/
                "%s"/*Title*/
                "%s"/*AuStr*/
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theLineQualityStr)
                + strlen(theTitleIndexStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                theTitleIndexStr,
                theAuStr);

            //	  printf("Send_2.59: %s\n",cmd);
            if (!sendRequest(cmd, "GET_PARAMETER", m_AuthStreamMode)) break;

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("GET_PARAMETER", bytesRead, responseCode, firstLine, nextLineStart, false)) break;

            if (responseCode != 200)
            {
                env()->setResultMsg(11000901, "NoMedia Data!", firstLine);
                break;
            }

            int contentLength = -1;
            char* lineStart;

            while (1)
            {
                lineStart = nextLineStart;
                if (lineStart == NULL) break;

                nextLineStart = getLine(lineStart);
                if (lineStart[0] == '\0') break;

                if (sscanf(lineStart, "Content-Length: %d", &contentLength) == 1 || sscanf(lineStart, "Content-length: %d", &contentLength) == 1)
                {
                    if (contentLength < 0)
                    {
                        env()->setResultMsg(11000206, "Bad \"Content-length:\" header: \"", lineStart, "\"");
                        break;
                    }
                }
            }

            if (lineStart == NULL)
            {
                env()->setResultMsg(11000207, "no content following header lines: ", m_ResponseBuffer);
                break;
            }

            if (m_RtpStreamMode == eRTPSTREAM_UDP)
            {
                char* bodyStart = NULL;
                int		readbyte = 0;
                if (contentLength >= 0)
                {
                    int numBodyBytes = &firstLine[bytesRead] - nextLineStart;
                    if (contentLength > numBodyBytes)
                    {
                        int numExtraBytesNeeded = contentLength - numBodyBytes;
                        bodyStart = new char[numExtraBytesNeeded + 10];

                        int theResult;
                        struct timeval theTimeOut;
                        theTimeOut.tv_sec = 0;
                        theTimeOut.tv_usec = 20000;
                        fd_set theRd_set;

                        int bytesRead2;
                        unsigned long tick = mdate();

                        readbyte = 0;
                        while (numExtraBytesNeeded > 0)
                        {
                            do
                            {
                                FD_ZERO(&theRd_set);
                                FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                                theResult = select(m_InputSocketNum + 1, &theRd_set, NULL, NULL, &theTimeOut);
                                if (theResult > 0)
                                    break;
                                else if (theResult < 0)
                                {
                                    theResult = WSAGetLastError();
                                    if ((theResult != WSAEWOULDBLOCK) && (theResult != WSAEINPROGRESS))
                                    {
                                        theResult = -1;
                                        break;
                                    }
                                }
                                msleep(1000);
                                if ((mdate() - tick) > SOCK_TIMEOUT_VALUE)
                                {
                                    theResult = -2;// time out error

#ifndef NDEBUG
                                    //fprintf(stderr,"Network time out(%s:%d)\n",__FILE__,__LINE__);
#endif							

                                    break;
                                }
                            } while (1);

                            if (theResult < 0)
                            {
                                if (theResult == -2)
                                    env()->setResultMsg(11000504, "RTSP response SDP Error(TimeOut)!");
                                else
                                    env()->setResultMsg(11000504, "RTSP response SDP Error(select)!");

                                break;
                            }
                            char* ptr = &bodyStart[readbyte];
                            bytesRead2 = recv(m_InputSocketNum, (char*)ptr, numExtraBytesNeeded, 0);
                            if (bytesRead2 <= 0)
                            {
                                env()->setResultMsg(11000504, "RTSP response SDP Error(recv)!");
                                break;
                            }

                            ptr[bytesRead2] = '\0';

                            readbyte += bytesRead2;
                            numExtraBytesNeeded -= bytesRead2;
                        }
                        if (numExtraBytesNeeded > 0)
                            break; // one of the reads failed
                    }

                    if (bodyStart != NULL)
                    {
                        int from, to = 0;
                        for (from = 0; from < contentLength; ++from)
                        {
                            if (bodyStart[from] != '\0')
                            {
                                if (to != from) bodyStart[to] = bodyStart[from];
                                ++to;
                            }
                        }

                        bodyStart[to] = '\0';
                    }
                }

                if (bodyStart == NULL)
                {
                    bodyStart = strdup("\r\n\r\n");
                }

                if (cmd != NULL) delete[] cmd;
                if (theAuStr != NULL)delete[] theAuStr;
                if (theLineQualityStr != NULL)delete[] theLineQualityStr;
                if (theTitleIndexStr != NULL)delete[] theTitleIndexStr;

                return bodyStart;
            }
            else
            {//if TCP then ready RecvConnectByte?
                char* bodyStart = nextLineStart;

                delete[] cmd;
                delete[] theAuStr;
                delete[] theLineQualityStr;
                delete[] theTitleIndexStr;

                return strdup(bodyStart);
            }
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        if (theTitleIndexStr != NULL)delete[] theTitleIndexStr;

        return NULL;
    }

    bool
        CRTSPClient::teardownMediaSubsession(CMediaSubsession& subsession)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "TEARDOWN %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theLineQualityStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                theAuStr);

            if (!sendRequest(cmd, "TEARDOWN", m_AuthStreamMode))
            {
                delete[](char*)subsession.m_sessionId;
                subsession.m_sessionId = NULL;
                break;
            }

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("TEARDOWN", bytesRead, responseCode, firstLine, nextLineStart))
            {
                delete[](char*)subsession.m_sessionId;
                subsession.m_sessionId = NULL;
                break;
            }

            delete[](char*)subsession.m_sessionId;
            subsession.m_sessionId = NULL;

            delete[] cmd;
            delete[] theAuStr;
            delete[] theLineQualityStr;
            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        return false;
    }

    bool
        CRTSPClient::RecordMediaSubsession(CMediaSubsession& subsession, long& time)
    {
        char* cmd = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            char*	theAuStr;

            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: \r\n");

            char* const cmdFmt =
                "RECORD\t%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s\r\n";

            char RecordStr[64];

            sprintf(RecordStr, "rtsp://%s:%d/VODServer/track1", m_ServerAddressStr, m_DestPortNum);
            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(RecordStr)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theAuStr);

            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                RecordStr,
                ++m_CSeq,
                subsession.m_sessionId,
                theAuStr);

            delete[] theAuStr;

            if (!sendRequest(cmd, "RECORD", m_AuthStreamMode))
            {
                break;
            }

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse("RECORD", bytesRead, responseCode, firstLine, nextLineStart, false)) break;

            if (responseCode != 200)
            {
                env()->setResultMsg(11000901, "NoMedia Data!", firstLine);
                break;
            }

            int contentLength = -1;
            char* lineStart;
            long	starttime, endtime;

            while (1)
            {
                lineStart = nextLineStart;
                if (lineStart == NULL) break;

                nextLineStart = getLine(lineStart);
                if (lineStart[0] == '\0') break;

                if (strstr(lineStart, "Range: npt"))
                {
                    if (sscanf(lineStart, "Range: npt=%ld-%ld", &starttime, &endtime) == 2)
                    {
                        time = endtime;
                        break;
                    }
                }
            }

            delete[] cmd;
            return true;

        } while (0);

        if (cmd != NULL) delete[] cmd;
        return false;
    }

#if 0 //ndef _USE_TDBMODE_CONNECT_
    bool CRTSPClient::ConnectToServerEx(portNumBits destPortNum)
    {
        long theAppErrCode = 0;
        long theErrCode = 0;
        bool bConnectFlag = false;

        struct sockaddr_in remoteName;
        remoteName.sin_family = AF_INET;
        remoteName.sin_port = htons(destPortNum);
        remoteName.sin_addr.s_addr = m_ServerAddress;

        do{
            if (connect(m_InputSocketNum, (struct sockaddr*)&remoteName, sizeof remoteName) != 0)
            {
                theErrCode = WSAGetLastError();
                if ((theErrCode != WSAEWOULDBLOCK) && (theErrCode != WSAEINPROGRESS))
                {
                    AnalysisSocketErrorCode(theErrCode, theAppErrCode);
                    env()->setResultMsg(theAppErrCode, "connect() failed!");
                    break;
                }
                int theResult;
                struct timeval		timeout;
                fd_set	theWritefds;
                fd_set	theReadfds;
                FD_ZERO(&theWritefds);
                FD_ZERO(&theReadfds);
                FD_SET((unsigned)m_InputSocketNum, &theWritefds);
                FD_SET((unsigned)m_InputSocketNum, &theReadfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                theErrCode = 0;
#ifdef	_WIN32
                theResult = select(m_InputSocketNum + 1, NULL, &theWritefds, &theReadfds, &timeout);
                if (theResult <= 0)
                {
                    theErrCode = WSAGetLastError();
                    AnalysisSocketErrorCode(theErrCode, theAppErrCode);
                    break;
                }
                if (FD_ISSET(m_InputSocketNum, &theReadfds))
                {
                    theErrCode = WSAGetLastError();
                    AnalysisSocketErrorCode(theErrCode, theAppErrCode);
                    break;
                }
#else
                do
                {
                    FD_ZERO(&theWritefds);
                    FD_ZERO(&theReadfds);
                    FD_SET((unsigned)m_InputSocketNum, &theWritefds);
                    FD_SET((unsigned)m_InputSocketNum, &theReadfds);

                    theResult = select(m_InputSocketNum + 1, &theReadfds, &theWritefds, 0, &timeout);
                    if (theResult <= 0)
                        theErrCode = WSAGetLastError();
                } while (theErrCode == WSAEINTR);

                if (theErrCode == 0)
                {
                    int len;
                    short err;
                    struct sockaddr	my_sock_addr;
                    len = 0;
                    err = getpeername(m_InputSocketNum, &my_sock_addr, &len);
                    if (err<0)
                    {
                        theErrCode = WSAGetLastError();
                    }
                    if (theErrCode == 0)
                    {
                        
                    }
                    else if (theErrCode == WSAENOTCONN || theErrCode == WSAEINVAL)
                    {
                        break;
                    }
                }
                else
                    break;
#endif

            }
            bConnectFlag = true;
        } while (0);

        if (m_InputSocketNum != INVALID_SOCKET && bConnectFlag == true)
        {
            struct sockaddr_in self_addr;
            long i_err;
            char strClientIp[64];

            int addr_size = sizeof(self_addr);
            i_err = ::getsockname(m_InputSocketNum, (struct sockaddr*)&self_addr, &addr_size);
            if (i_err == 0)
            {
                ::strcpy(strClientIp, ::inet_ntoa(self_addr.sin_addr));
                m_IpAddrNum = inet_addr(strClientIp);
                strcpy(m_IpAddrString, strClientIp);
            }
        }
        return bConnectFlag;
    }
#else
    bool CRTSPClient::ConnectToServerEx(uint16_t inPort)
    {
        char msg_log[256];
        if (m_AuthStreamMode == false)
        {
            struct sockaddr_in sock_addr;
            int			err;
            struct in_addr		in_addr;
            struct sockaddr_in	addr;

            if (m_InputSocketNum == INVALID_SOCKET)
            {
                err = (int)WSAGetLastError();
                return INVALID_SOCKET;
            }

            memset(&addr, 0, sizeof(struct sockaddr_in));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(inPort);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);

            in_addr.s_addr = m_ServerAddress;
            memset(&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_port = htons(inPort);
            sock_addr.sin_addr = in_addr;

            fd_set writefds, exceptfds;
            struct timeval timeout;
            int nfound;

            unsigned long tick = mdate();

            err = connect(m_InputSocketNum, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
            if (err < 0)
                err = (int)WSAGetLastError();

            while (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS || err == WSAEALREADY) {
                FD_ZERO(&writefds);
                FD_ZERO(&exceptfds);
                FD_SET(m_InputSocketNum, &writefds);	// register listen socket
                FD_SET(m_InputSocketNum, &exceptfds);	// register listen socket
                //printf("connect err = %d\r\n",err);
                timeout.tv_sec = 1;				// seconds
                timeout.tv_usec = 0;			// and microseconds
                nfound = ::select(m_InputSocketNum + 1, (fd_set *)0, &writefds, (fd_set *)&exceptfds, &timeout);
                if (nfound < 0) {  // time out??
                    err = (int)WSAGetLastError();
                }

                if (nfound > 0 && FD_ISSET(m_InputSocketNum, &writefds)) {
#ifdef _MAC_
                    err = ::connect(m_InputSocketNum, (struct sockaddr *)&(sock_addr), sizeof(sock_addr));
                    if (err < 0)
                        err = (int)WSAGetLastError();
                    if (err == WSAEISCONN)
                        err = 0;
                    if (err == 0)
                        break;
#else
                    err = 0;
                    break;
#endif
                }

                if (nfound > 0 && FD_ISSET(m_InputSocketNum, &exceptfds)) {
                    err = ::connect(m_InputSocketNum, (struct sockaddr *)&(sock_addr), sizeof(sock_addr));
                    if (err < 0)
                        err = (int)WSAGetLastError();
                    if (err == WSAEISCONN)
                        err = 0;
                    if (err == 0)
                        break;
                }

                if ((mdate() - tick) > 5000000)
                    break;

                msleep(1000);
            }
            if (err != 0)
            {
                sprintf(msg_log, "RtsptcpConnect Fail(%08X,%d) err= %d(%s)", m_ServerAddress, inPort, err, VodNetGetSockErrorStr(err));
                env()->setResultMsg(10000308, msg_log);
                return false;
            }


            if (m_InputSocketNum != INVALID_SOCKET)
            {
                struct sockaddr_in self_addr;
                long i_err;
                char strClientIp[64];

                int addr_size = sizeof(self_addr);
                i_err = ::getsockname(m_InputSocketNum, (struct sockaddr*)&self_addr, &addr_size);
                if (i_err == 0)
                {
                    ::strcpy(strClientIp, ::inet_ntoa(self_addr.sin_addr));
                    m_IpAddrNum = inet_addr(strClientIp);
                    strcpy(m_IpAddrString, strClientIp);
                }
            }
        }
        else if (m_AuthStreamMode == true)
        {
            char		parseInfo[256];
            strcpy(parseInfo, m_ServerAddressStr);

            char		proxyAddr[64];
            uint16_t	proxyPort;
            char		serverAlias[100];
            int			serverIndex;

            char*	token;
            token = strtok(parseInfo, "_");
            strcpy(proxyAddr, token);

            token = strtok(NULL, "_");
            proxyPort = atoi(token);

            token = strtok(NULL, "_");
            strcpy(serverAlias, token);

            token = strtok(NULL, "_");
            serverIndex = atoi(token);

            int32_t		theErrorCode = 0;
            long		theAppErrCode = 0;

            // no tcp socket
            m_AuthInputSocket = new MxSocket;

            theErrorCode = m_AuthInputSocket->CreateAndConnectByTcp(serverAlias, serverIndex, 10 * 1000000, 0x200000, 0x200000, mxSocketFlag_DefaultTCP);

            if (theErrorCode == 0)
            {
            }
            else
            {
                theErrorCode = errno;
                AnalysisSocketErrorCode(theErrorCode, theAppErrCode);

                AnalysisSocketErrorCode(theErrorCode, theAppErrCode);
                env()->setResultMsg(theAppErrCode, "connect() failed!");

                sprintf(msg_log, "======== LogPlay: Connect err = %d", theErrorCode);
                return false;
            }
        }

        sprintf(msg_log, "RtsptcpConnect success(%08X,%d), clientip=%s", m_ServerAddress, inPort, m_IpAddrString);
        env()->setResultMsg(100000, msg_log);
        return true;
    }

#endif

    bool
        CRTSPClient::openConnectionFromURL(char const* url)
    {
        bool bConnectFlag = false;

        do
        {
            if (url == NULL) break;

            if (m_BaseURL != NULL)
                delete[] m_BaseURL;
            m_BaseURL = NULL;

            if (m_UserName != NULL)
                delete[] m_UserName;
            m_UserName = NULL;

            uint32_t destAddress;
            uint16_t urlPortNum;
            char			strArchDB_addr[64];

            printf("VURL = %s\r\n", url);
            if (!parseRTSPURL(env(), url, destAddress, m_ServerAddressStr, urlPortNum,
                m_LineQuality,
                &m_ControlServerAddr,
                &m_UserName, m_UserKind,
                m_FileName, m_FileKind, m_FileID,
                m_ListKind, strArchDB_addr, &m_BaseURL))
            {
                env()->setResultMsg(11000211, "URL's form is not valid!");
                break;
            }

            m_DestPortNum = urlPortNum;

            uint16_t destPortNum = urlPortNum;
            if (m_AuthStreamMode == false)
            {
                if (m_InputSocketNum < 0)
                {
                    m_InputSocketNum = setupStreamSocket(env(), destPortNum, true /* =>none blocking */);
                    if (m_InputSocketNum < 0)
                        break;

                    m_ServerAddress = destAddress;
                    bConnectFlag = ConnectToServerEx(destPortNum);
                    if (bConnectFlag == false)
                        break;
                }
            }
            else if (m_AuthStreamMode == true)
            {
                bConnectFlag = ConnectToServerEx(destPortNum);
                if (bConnectFlag == false)
                    break;
            }

            if (m_UserSpecificData != NULL)
                delete[] m_UserSpecificData;
            m_UserSpecificData = new char[strlen(url) + 16];
            if (m_ListKind != 8)
                sprintf(m_UserSpecificData, "%s\t%d\t%s\t%d\t%lu\t%d\t%s\t", m_UserName, m_UserKind, m_FileName, m_FileKind, m_FileID, m_ListKind, m_ControlServerAddr);
            else
                sprintf(m_UserSpecificData, "%s\t%d\t%s\t%d\t%lu\t%d\t%s\t%s\t", m_UserName, m_UserKind, m_FileName, m_FileKind, m_FileID, m_ListKind, m_ControlServerAddr, strArchDB_addr);

            return true;
        } while (0);

        m_DescribeStatusCode = 1;
        resetTCPSockets();
        return false;
    }

    bool
        CRTSPClient::parseRTSPURL(CUsageEnvironment* inEnv,
        char const* url,
        uint32_t& address,
        char*	addressStr,
        uint16_t& portNum,
        short& lineQuality,
        char** controlServerAddr,
        char** userName,
        short& userKind,
        char* fileName,
        short& fileKind,
        unsigned int& fileID,
        short& listKind,
        char* archDbAddr,
        char** baseUrl)
    {
        do
        {
            // Parse the URL as "rtsp://<address>:<port>/<linequality>\t<username>\t<userkind>\t<filename>\t<filekind>\t<fileID>\t<listkind>\t<control server address>\t"
            char const* prefix = "rtsp://";
            unsigned const prefixLength = 7;
            if (_strnicmp(url, prefix, prefixLength) != 0)
            {
                inEnv->setResultMsg(11000211, "URL is not of the form \"", prefix, "\"");
                break;
            }

            unsigned const parseBufferSize = 100;
            char parseBuffer[parseBufferSize];
            char const* from = &url[prefixLength];
            char* to = &parseBuffer[0];
            unsigned i;
            char theServerAddrStr[parseBufferSize];

            to = &theServerAddrStr[0];
            for (i = 0; i < parseBufferSize; ++i)
            {
                if (*from == '\0' || *from == ':' || *from == '/')
                {
                    *to = '\0';
                    break;
                }
                *to++ = *from++;
            }

            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000212, "URL is too long");
                break;
            }

            address = inet_addr(theServerAddrStr);

            if (addressStr)
                strcpy(addressStr, theServerAddrStr);

            portNum = 9113;//554;
            char nextChar = *from;
            if (nextChar == ':')
            {
                int portNumInt;
                if (sscanf(++from, "%d", &portNumInt) != 1)
                {
                    inEnv->setResultMsg(11000213, "No port number follows ':'");
                    break;
                }
                if (portNumInt < 1 || portNumInt > 65535)
                {
                    inEnv->setResultMsg(11000214, "Bad port number!");
                    break;
                }
                portNum = (uint16_t)portNumInt;
                while (*from >= '0' && *from <= '9') ++from;
            }

            if (*from == '/') ++from;

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            lineQuality = atoi(parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            *userName = strdup(parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            userKind = atoi(parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            strcpy(fileName, parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            fileKind = atoi(parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            sscanf(parseBuffer, "%lu", &fileID);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            listKind = atoi(parseBuffer);

            to = &parseBuffer[0];
            for (i = 0; i<parseBufferSize; ++i)
            {
                if (*from == '\t') break;
                *to++ = *from;
                ++from;
            }
            ++from;
            *to = '\0';
            if (i == parseBufferSize)
            {
                inEnv->setResultMsg(11000211, "URL is invalid");
                break;
            }
            *controlServerAddr = strdup(parseBuffer);

            if (listKind == 8) //archview,s video
            {
                to = &parseBuffer[0];
                for (i = 0; i<parseBufferSize; ++i)
                {
                    if (*from == '\t' || *from == '\0') break;
                    *to++ = *from;
                    ++from;
                }
                ++from;
                *to = '\0';
                strcpy(archDbAddr, parseBuffer);
            }
            else
            {
                strcpy(archDbAddr, "");
            }


            i = strlen("rtsp://") + strlen(theServerAddrStr) + 1 + 7 + 30;
            *baseUrl = new char[i + 16];
            sprintf(*baseUrl, "rtsp://%s:%d/VODServer", theServerAddrStr, portNum);

            return true;
        } while (0);

        return false;
    }

    bool CRTSPClient::sendRequest(char const* requestString, char const* tag, bool authStreamMode)
    {
        msg_Dbg((vlc_object_t*)NULL, "%s", requestString);

        int result;
        if (authStreamMode == false)
        {
            if (m_InputSocketNum < 0)
                return false;

            result = send(m_InputSocketNum, requestString, strlen(requestString), SO_NOSIGPIPE);
            if (result == SOCKET_ERROR)
                env()->setResultMsg(11000220, "Send Request RTSP failed!");
            else {
                char msg_txt[128];
                long len;
                len = strlen(requestString);
                if (len > 125)
                {
                    len = 125;
                }
                memcpy(msg_txt, requestString, len);
                msg_txt[len] = 0;

                env()->setResultMsg(1000, msg_txt);
            }

            return bool(result != SOCKET_ERROR);
        }
        else if (authStreamMode == true)
        {
            if (m_AuthInputSocket == NULL)
                return false;
            result = m_AuthInputSocket->SendBlock((char*)requestString, strlen(requestString), NULL);
            if (result != 0)
                env()->setResultMsg(11000220, "Send Request RTSP failed!");

            return bool(result == 0);
        }
    }

    bool
        CRTSPClient::getResponse(char const*		tag,
        unsigned&			bytesRead,
        unsigned&			responseCode,
        char*&             firstLine,
        char*&             nextLineStart,
        bool				checkFor200Response,
        bool               checkSeqNo)
    {
        bool	rslt = false;
        if (m_RtpStreamMode == eRTPSTREAM_UDP)
            return getResponse_inside(tag, bytesRead, responseCode, firstLine, nextLineStart, checkFor200Response, checkSeqNo);
        else
        {
            bool bReq = false;
            int i_loop = 0;
            char* readBuf = m_ResponseBuffer;
            //wait command_req by RTP_Source_Thread...
            msg_Dbg((vlc_object_t*)NULL, "Wait %s", tag);

            while ((bReq = m_RtcpReqArray.PopResReq(m_ResponseBuffer)) == false && m_bTcpSocketErrorFlag == 0)
            {
                msleep(10000);
                i_loop++;
                if (i_loop > 2000)
                {//20 0000ms = 20s
                    break; //timeout
                }
            }

            do{
                if (bReq == false)
                {
                    env()->setResultMsg(11000321, "Failed to read response!");
                    break;
                }
                else
                {
                    char msg_txt[128];
                    long len;
                    len = strlen(m_ResponseBuffer);
                    if (len > 125)
                        len = 125;
                    memcpy(msg_txt, m_ResponseBuffer, len);
                    msg_txt[len] = 0;

                    env()->setResultMsg(1000, msg_txt);

                    bytesRead = strlen(m_ResponseBuffer);

                    if (checkSeqNo)
                    {
                        long SeqCount = 0;
                        unsigned long theSeqNo;
                        char* lineStart;
                        char* first;
                        char* next;

                        memcpy(m_ResponseBuffer1, m_ResponseBuffer, m_ResponseBufferSize);

                        first = m_ResponseBuffer1;
                        next = getLine(first);

                        while (1)
                        {
                            lineStart = next;
                            if (lineStart == NULL)
                                break;

                            next = getLine(lineStart);
                            if (lineStart[0] == '\0') break;

                            if (sscanf(lineStart, "CSeq: %ld", &theSeqNo) == 1)
                            {
                                if (theSeqNo > m_CSeq)
                                {
                                    env()->setResultMsg(11000321, "Failed to read response(CurSeqNo < RecvSeqNo)!");
                                    bytesRead = 0;
                                    break;
                                }
                                else if (theSeqNo < m_CSeq)
                                {
                                    env()->setResultMsg(11000321, "Failed to read response(CurSeqNo != RecvSeqNo)!");
                                    SeqCount++;

                                    readBuf = m_ResponseBuffer;
                                    memset(m_ResponseBuffer, 0, sizeof(m_ResponseBufferSize));
                                    getResponse(tag, bytesRead, responseCode, firstLine, nextLineStart, checkFor200Response, checkSeqNo);
                                    if (bytesRead == 0)
                                    {
                                        env()->setResultMsg(11000321, "Failed to read response!");
                                        break;
                                    }

                                    memcpy(m_ResponseBuffer1, m_ResponseBuffer, m_ResponseBufferSize);

                                    first = m_ResponseBuffer1;
                                    next = getLine(first);
                                    continue;
                                }
                                else
                                {
                                    rslt = true;
                                    break;
                                }
                            }

                            if (SeqCount > 5)
                                break;
                        }
                    }
                    else
                        rslt = true;

                    if (rslt == false)
                    {
                        env()->setResultMsg(11000321, "Failed to read response!");
                        break;
                    }

                    firstLine = readBuf;
                    nextLineStart = getLine(firstLine);
                    if (!parseResponseCode(firstLine, responseCode)) break;

                    if (responseCode != 200 && checkFor200Response)
                    {
                        if (responseCode == 401)
                            env()->setResultMsg(11000704, "No Acceptable Client!");
                        else
                            env()->setResultMsg(11000503, tag, ": cannot handle response: ", firstLine);
                        break;
                    }

                    msg_Dbg((vlc_object_t*)NULL, "Recv %s", readBuf);
                    return true;
                }
            } while (0);
        }

        msg_Dbg((vlc_object_t*)NULL, "Error %s", tag);

        return false;
    }

    bool
        CRTSPClient::getResponse_inside(char const* tag, unsigned& bytesRead, unsigned& responseCode, char*& firstLine, char*& nextLineStart, bool checkFor200Response, bool checkSeqNo)
    {
        int		TimeOutValue = SOCK_TIMEOUT_VALUE;
        if (strcmp(tag, "DESCRIBE") == 0)
            TimeOutValue = 60000000;
        do
        {
            bool	rslt = false;
            char* readBuf = m_ResponseBuffer;
            memset(m_ResponseBuffer, 0, sizeof(m_ResponseBufferSize));

            bytesRead = getResponse1(readBuf, m_ResponseBufferSize, 0, TimeOutValue);
            if (bytesRead == 0)
            {
                env()->setResultMsg(11000321, "Failed to read response!");
                break;
            }
            else
            {
                char msg_txt[128];
                long len;
                len = strlen(m_ResponseBuffer);
                if (len > 125)
                    len = 125;
                memcpy(msg_txt, m_ResponseBuffer, len);
                msg_txt[len] = 0;

                env()->setResultMsg(1000, msg_txt);
            }

#if 1
            if (checkSeqNo)
            {
                long	SeqCount = 0;
                unsigned long	theSeqNo;
                char*	lineStart;
                char*	first;
                char*	next;

                memcpy(m_ResponseBuffer1, m_ResponseBuffer, m_ResponseBufferSize);

                first = m_ResponseBuffer1;
                next = getLine(first);

                while (1)
                {
                    lineStart = next;
                    if (lineStart == NULL)
                        break;

                    next = getLine(lineStart);
                    if (lineStart[0] == '\0') break;

                    if (sscanf(lineStart, "CSeq: %ld", &theSeqNo) == 1)
                    {
                        if (theSeqNo > m_CSeq)
                        {
                            env()->setResultMsg(11000321, "Failed to read response(CurSeqNo < RecvSeqNo)!");
                            bytesRead = 0;
                            break;
                        }
                        else if (theSeqNo < m_CSeq)
                        {
                            env()->setResultMsg(11000321, "Failed to read response(CurSeqNo != RecvSeqNo)!");
                            SeqCount++;

                            readBuf = m_ResponseBuffer;
                            memset(m_ResponseBuffer, 0, sizeof(m_ResponseBufferSize));
                            bytesRead = getResponse1(readBuf, m_ResponseBufferSize);
                            if (bytesRead == 0)
                            {
                                env()->setResultMsg(11000321, "Failed to read response!");
                                break;
                            }

                            memcpy(m_ResponseBuffer1, m_ResponseBuffer, m_ResponseBufferSize);

                            first = m_ResponseBuffer1;
                            next = getLine(first);
                            continue;
                        }
                        else
                        {
                            rslt = true;
                            break;
                        }
                    }

                    if (SeqCount > 5)
                        break;
                }
            }
            else
                rslt = true;

            if (rslt == false)
            {
                env()->setResultMsg(11000321, "Failed to read response!");
                break;
            }
#endif

            firstLine = readBuf;
            nextLineStart = getLine(firstLine);

            if (strcmp(tag, "DESCRIBE") == 0)
            {
                if (strstr(firstLine, "REDIRECT"))
                {
                    responseCode = 100;
                    return true;
                }
            }

            if (!parseResponseCode(firstLine, responseCode)) break;

            if (responseCode != 200 && checkFor200Response)
            {
                if (responseCode == 401)
                    env()->setResultMsg(11000704, "No Acceptable Client!");
                else
                    env()->setResultMsg(11000503, tag, ": cannot handle response: ", firstLine);
                break;
            }

            return true;
        } while (0);

        return false;
    }

    unsigned CRTSPClient::RtspResProcessbyRtpEx(char* inBuff, int inBuffSize)
    {
        unsigned     bytesRead;
        long		content_len;

        memcpy(m_ResponseBufferByOther, inBuff, inBuffSize);
        m_ResponseBufferByOther[inBuffSize] = 0;

        char*   readBuf = m_ResponseBufferByOther;
        bytesRead = inBuffSize;

        content_len = checkContent(m_ResponseBufferByOther);
        if (content_len>0)
        {
            getContentBody(m_ResponseBufferByOther, bytesRead, content_len);
        }
        m_RtcpReqArray.PushResReq(m_ResponseBufferByOther);

        return true;
    }

    unsigned CRTSPClient::RtspResProcessbyRtp(char firstByte)
    {
        unsigned     bytesRead;
        long		content_len;
        do
        {
            char*   readBuf = m_ResponseBufferByOther;
            readBuf[0] = firstByte;
            bytesRead = getResponse1(readBuf, m_ResponseBufferSizeByOther, 1);
            if (bytesRead == 0)
            {
                env()->setResultMsg(10000321, "Failed to read response!");
                break;
            }

            content_len = checkContent(m_ResponseBufferByOther);
            if (content_len>0)
            {
                getContentBody(m_ResponseBufferByOther, bytesRead, content_len);
            }
            m_RtcpReqArray.PushResReq(m_ResponseBufferByOther);

            return true;
        } while (0);

        return false;
    }

    long CRTSPClient::checkContent(char* inRtspReqStr)
    {
        char*	nextLineStart;
        char*	lineStart;
        int contentLength = -1;
        nextLineStart = inRtspReqStr;
        while (1)
        {
            lineStart = nextLineStart;
            if (lineStart == NULL) break;

            nextLineStart = getLine1(lineStart);
            if (lineStart[0] == '\0') break;

            if (sscanf(lineStart, "Content-Length: %d", &contentLength) == 1 || sscanf(lineStart, "Content-length: %d", &contentLength) == 1)
            {
                //Use Contect Length yeard.
            }
        }
        if (contentLength == -1)
            return 0;
        return contentLength;
    }

    bool CRTSPClient::downloadstartMediaSubsession(CMediaSubsession& subsession, char* posInfoBuffer, long inBufferSize)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;
        char*	thePosInfoStr = NULL;
        thePosInfoStr = new char[(inBufferSize + 1) * 2 + 16];
        long		i, cnt;

        for (i = 0; i<inBufferSize; i++)
        {
            sprintf(&thePosInfoStr[i * 2], "%02X", (unsigned char)posInfoBuffer[i]);
        }

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "HIGH_DOWNLOAD_START %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "PosInfo: %s\r\n"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(thePosInfoStr)
                + strlen(theLineQualityStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                thePosInfoStr,
                theLineQualityStr,
                theAuStr);

            if (!sendRequest(cmd, "HIGH_DOWNLOAD_START", false)) break;   //intended not to apply attachment to 414...

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse_inside("HIGH_DOWNLOAD_START", bytesRead, responseCode, firstLine, nextLineStart, true, true)) break;

            delete[] cmd;
            delete[] theAuStr;
            delete[] theLineQualityStr;
            delete[] thePosInfoStr;
            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        if (thePosInfoStr) delete[]thePosInfoStr;
        return false;
    }

    bool CRTSPClient::downloadstopMediaSubsession(CMediaSubsession& subsession)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "HIGH_DOWNLOAD_STOP %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + strlen(theLineQualityStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                theLineQualityStr,
                theAuStr);

            if (!sendRequest(cmd, "HIGH_DOWNLOAD_STOP", false)) break;               //intended not to apply attachment to 414...

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse_inside("HIGH_DOWNLOAD_STOP", bytesRead, responseCode, firstLine, nextLineStart, true, true)) break;

            delete[] cmd;
            delete[] theAuStr;
            delete[] theLineQualityStr;
            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        return false;
    }
    bool
        CRTSPClient::downloaddataMediaSubsession(CMediaSubsession& subsession, char* outRtpBufferPos, long& ioBufferSize)
    {
        char* cmd = NULL;
        char*	theAuStr = NULL;
        char* theLineQualityStr = NULL;

        do
        {
            if (subsession.m_sessionId == NULL)
            {
                env()->setResultMsg(11000403, "No RTSP session is currently in progress\n");
                break;
            }

            if (m_UserSpecificData == NULL)
            {
                env()->setResultMsg(11000701, "No Authentication Data!");
                break;
            }

            theLineQualityStr = new char[64];
            sprintf(theLineQualityStr, "LineQuality: %d\r\n", m_LineQuality);

            short theAuResult;
            theAuStr = new char[1024];
            sprintf(theAuStr, "Authenticator: %s\r\n", m_UserSpecificData);

            char* const cmdFmt =
                "HIGH_DOWNLOAD_DATA %s%s%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %s\r\n"
                "Size: %d\r\n"
                "%s"
                "%s"
                "\r\n";

            char const *prefix, *separator, *suffix;
            constructSubsessionURL(subsession, prefix, separator, suffix);

            unsigned cmdSize = strlen(cmdFmt)
                + strlen(prefix) + strlen(separator) + strlen(suffix)
                + 20 /* max int len */
                + strlen(subsession.m_sessionId)
                + 20
                + strlen(theLineQualityStr)
                + strlen(theAuStr);
            cmd = new char[cmdSize + 16];
            sprintf(cmd, cmdFmt,
                prefix, separator, suffix,
                ++m_CSeq,
                subsession.m_sessionId,
                ioBufferSize,
                theLineQualityStr,
                theAuStr);

            if (!sendRequest(cmd, "HIGH_DOWNLOAD_DATA", false)) break;            //intended not to apply attachment to 414...

            unsigned bytesRead; unsigned responseCode;
            char* firstLine; char* nextLineStart;
            if (!getResponse_inside("HIGH_DOWNLOAD_DATA", bytesRead, responseCode, firstLine, nextLineStart, true, true)) break;

            char* lineStart;
            int		rtpbuffsize = 0;
            while (1)
            {
                lineStart = nextLineStart;
                if (lineStart == NULL) break;

                nextLineStart = getLine(lineStart);

                if (sscanf(lineStart, "Size: %d", &rtpbuffsize) == 1)
                {
                    break;
                }
            }

            delete[] cmd;
            delete[] theAuStr;
            delete[] theLineQualityStr;

            //Recv rtpbuffersize; outRtpBufferPos,  ioBufferSize
            int numBodyBytes = rtpbuffsize;
            bytesRead = 0;

            if (rtpbuffsize > 0)
            {
                int theResult;
                struct timeval theTimeOut;
                theTimeOut.tv_sec = 0;
                theTimeOut.tv_usec = 20000;
                fd_set theRd_set;

                int bytesRead2;
                unsigned long tick = mdate();

                while (numBodyBytes > 0)
                {
                    do
                    {
                        FD_ZERO(&theRd_set);
                        FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                        theResult = select(m_InputSocketNum + 1, &theRd_set, NULL, NULL, &theTimeOut);
                        if (theResult > 0)
                            break;
                        else if (theResult < 0)
                        {
                            theResult = WSAGetLastError();
                            if ((theResult != WSAEWOULDBLOCK) && (theResult != WSAEINPROGRESS))
                            {
                                theResult = -1;
                                break;
                            }
                        }
                        msleep(1000);
                        if ((mdate() - tick) > SOCK_TIMEOUT_VALUE)
                        {
                            theResult = -2;// time out error
                            break;
                        }
                    } while (1);

                    if (theResult < 0)
                    {
                        if (theResult == -2)
                            env()->setResultMsg(11000504, "RTSP response downloaddata(TimeOut)!");
                        else
                            env()->setResultMsg(11000504, "RTSP response downloaddata Error(select)!");

                        break;
                    }
                    char* ptr = &outRtpBufferPos[bytesRead];
                    bytesRead2 = recv(m_InputSocketNum, (char*)ptr, numBodyBytes, 0);
                    if (bytesRead2 <= 0)
                    {
                        env()->setResultMsg(11000504, "RTSP response SDP Error(recv)!");
                        break;
                    }

                    bytesRead += bytesRead2;
                    numBodyBytes -= bytesRead2;
                }
            }

            ioBufferSize = bytesRead;

            return true;
        } while (0);

        if (cmd != NULL) delete[] cmd;
        if (theAuStr != NULL)delete[] theAuStr;
        if (theLineQualityStr != NULL)delete[] theLineQualityStr;
        return false;
    }

    bool CRTSPClient::getContentBody(char* inRtspReqStr, long bytesRead, long content_len, bool inAuthStreamMode)
    {
        char* bodyStart = inRtspReqStr + bytesRead;
        if (content_len >= 0)
        {
            int numExtraBytesNeeded = content_len;
            int theResult;
            struct timeval theTimeOut;
            theTimeOut.tv_sec = 0;
            theTimeOut.tv_usec = 20000;
            fd_set theRd_set;

            int bytesRead2, bytesRead = 0;
            unsigned long tick = mdate();

            while (numExtraBytesNeeded > 0)
            {
                do
                {
                    if (inAuthStreamMode == false)
                    {
                        FD_ZERO(&theRd_set);
                        FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                        theResult = select(m_InputSocketNum + 1, &theRd_set, NULL, NULL, &theTimeOut);
                        if (theResult > 0)
                            break;
                        else if (theResult < 0)
                        {
                            theResult = WSAGetLastError();
                            if ((theResult != WSAEWOULDBLOCK) && (theResult != WSAEINPROGRESS))
                            {
                                theResult = -1;
                                break;
                            }
                            else
                            {
                                if ((mdate() - tick) > SOCK_TIMEOUT_VALUE)
                                {
                                    theResult = -2;// time out error
                                    break;
                                }
                            }
                        }
                        msleep(1000);
                    }
                    else if (inAuthStreamMode == true)
                    {
                        theResult = m_AuthInputSocket->SelectRecv(1000000 * 20);
                        if (theResult > 0)
                            break;
                        else if (theResult < 0)
                        {
                            int32_t theSysErr = errno;
                            if ((theSysErr != WSAEWOULDBLOCK) && (theSysErr != WSAEINPROGRESS))
                            {
                                theResult = -1;
                                break;
                            }
                            else
                            {
                                if (mdate() - tick > 10 * 1000000)
                                {
                                    theResult = -2;// time out error
                                    break;
                                }
                            }
                        }
                        msleep(1000);
                    }

                } while (1);

                if (theResult < 0)
                {
                    if (theResult == -2)
                        env()->setResultMsg(11000504, "RTSP response SDP Error(TimeOut)!");
                    else
                        env()->setResultMsg(11000504, "RTSP response SDP Error(select)!");

                    break;
                }
                char* ptr = &bodyStart[bytesRead];
                bytesRead2 = recv(m_InputSocketNum, (char*)ptr, numExtraBytesNeeded, 0);
                if (bytesRead2 <= 0)
                {
                    env()->setResultMsg(11000504, "RTSP response SDP Error(recv)!");
                    break;
                }

                ptr[bytesRead2] = '\0';

                bytesRead += bytesRead2;
                numExtraBytesNeeded -= bytesRead2;
            }

            int from, to = 0;
            for (from = 0; from < content_len; ++from)
            {
                if (bodyStart[from] != '\0')
                {
                    if (to != from) bodyStart[to] = bodyStart[from];
                    ++to;
                }
            }

            bodyStart[to] = '\0';
        }
        return true;
    }

    void CRTSPClient::RtspTcpSocketError()
    {
        m_bTcpSocketErrorFlag = 1;
    }

    bool CRTSPClient::IsRTPChannelID(long inChannelID)
    {
        if (m_RtpChannelID == inChannelID)
            return true;

        return false;
    }

    unsigned CRTSPClient::getResponse1(char*& responseBuffer, unsigned responseBufferSize, int inSkipByte, int inTimeOutValue)
    {
        if (responseBufferSize == 0)
            return 0; // just in case...
        responseBuffer[inSkipByte] = '\0'; // ditto

        char* p = responseBuffer;
        bool haveSeenNonCRLF = false;
        int bytesRead = inSkipByte;
        int bytesReadNow;

        int theResult;
        struct timeval theTimeOut;
        theTimeOut.tv_sec = 1;
        theTimeOut.tv_usec = 20000;
        fd_set theRd_set;
        //fd_set theWd_set;
        unsigned long	tick;
        unsigned long theTick;

        theTick = mdate();
        while (bytesRead < (int)responseBufferSize)
        {
#if 0
            if ((mdate(), theTick) > inTimeOutValue)
            {
                env()->setResultMsg(11000504, "RTSP response Error(TimeOut)!");
                break;
            }
#endif
            if (m_AuthStreamMode == false)
            {
                tick = mdate();
                do
                {
                    FD_ZERO(&theRd_set);
                    //FD_ZERO(&theWd_set);
                    FD_SET((unsigned)m_InputSocketNum, &theRd_set);
                    //FD_SET((unsigned)m_InputSocketNum, &theWd_set);
                    theResult = select(m_InputSocketNum + 1, &theRd_set, 0/*&theWd_set*/, 0, &theTimeOut);
                    if (theResult > 0)
                        break;
                    else if (theResult < 0)
                    {
                        theResult = WSAGetLastError();
                        if (theResult == WSAEWOULDBLOCK || theResult == WSAEINPROGRESS)
                        {
                            if ((mdate() - tick) < inTimeOutValue)
                            {
                                msleep(1000);
                                continue;
                            }
                            else
                            {
                                theResult = -2;// time out error.
                                break;
                            }
                        }
                        theResult = -1;
                        break;
                    }
                    if ((mdate() - tick) < inTimeOutValue)
                        msleep(1000);
                    else
                    {
                        theResult = -2;// time out error.
                        break;
                    }
                } while (1);

                if (theResult < 0)
                {
                    if (theResult == -2)
                        env()->setResultMsg(11000504, "RTSP response Error(TimeOut)!");
                    else
                        env()->setResultMsg(11000504, "RTSP response Error!");
                    break;
                }

                bytesReadNow = recv(m_InputSocketNum, (char*)(responseBuffer + bytesRead), 1, 0);
                if (bytesReadNow < 0)
                {
                    theResult = WSAGetLastError();
                    if (theResult == WSAEWOULDBLOCK || theResult == WSAEINPROGRESS)
                    {
                        msleep(1000);
                        continue;
                    }

                    env()->setResultMsg(11000504, "RTSP response was truncated!");
                    break;
                }

                if (bytesReadNow == 0)
                {
                    env()->setResultMsg(11000504, "RTSP response was truncated!");
                    break;
                }

                bytesRead += bytesReadNow;

            }
            else if (m_AuthStreamMode == true)
            {
                MxBuffer	theRcvBuffer;
                theResult = m_AuthInputSocket->ReceiveBlock(&theRcvBuffer, inTimeOutValue * 1000);

                if (mdate() - theTick > inTimeOutValue * 1000)
                {
                    env()->setResultMsg(11000504, "RTSP response Error(getResponse1 TimeOut)!");
                    break;
                }

                if (theResult == mxErr_SockIdle)
                {
                    msleep(5000);
                    continue;
                }

                if (theResult)
                {
                    env()->setResultMsg(11000504, "RTSP response Error(getResponse1)!");
                    break;
                }

                bytesReadNow = theRcvBuffer.GetDataSize();
                if (bytesReadNow == 0)
                {
                    env()->setResultMsg(11000504, "RTSP response was truncated!");
                    break;
                }

                memcpy(&responseBuffer[bytesRead], theRcvBuffer.GetBuffer(), bytesReadNow);
                bytesRead += bytesReadNow;
            }

            char* lastToCheck = responseBuffer + bytesRead - 4;
            if (lastToCheck < responseBuffer) continue;
            for (; p <= lastToCheck; ++p)
            {
                if (haveSeenNonCRLF)
                {
                    if (*p == '\r' && *(p + 1) == '\n' &&
                        *(p + 2) == '\r' && *(p + 3) == '\n')
                    {
                        responseBuffer[bytesRead] = '\0';

                        while (*responseBuffer == '\r' || *responseBuffer == '\n')
                        {
                            ++responseBuffer;
                            --bytesRead;
                        }

                        return bytesRead;
                    }
                }
                else
                {
                    if (*p != '\r' && *p != '\n')
                    {
                        haveSeenNonCRLF = true;
                    }
                }
            }
        }

        msg_Dbg((vlc_object_t*)NULL, "responseBuffer=%s", responseBuffer);

        long theErrCode = 0;
        char* theErrMsg = env()->GetLastError(theErrCode);
        if (theErrCode != 11000504)
            env()->setResultMsg(11000215, "We received a response not ending with <CR><LF><CR><LF>");
        return 0;
    }

    bool
        CRTSPClient::parseResponseCode(char const* line, unsigned& responseCode)
    {
        if (sscanf(line, "%*s%u", &responseCode) != 1)
        {
            env()->setResultMsg(11000216, "no response code in line: \"", line, "\"");
            return false;
        }

        return true;
    }

    bool
        CRTSPClient::parseTransportResponse(char const* line, char*& serverAddressStr, uint16_t& serverPortNum)
    {
        // Initialize the return parameters to 'not found' values:
        serverAddressStr = NULL;
        serverPortNum = 0;

        char* foundServerAddressStr = NULL;
        bool foundServerPortNum = false;

        // First, check for "Transport:"
        if (_strnicmp(line, "Transport: ", 11) != 0) return false;
        line += 11;

        // Then, run through each of the fields, looking for ones we handle:
        char const* fields = line;
        char* field = (char*)malloc((strlen(fields) + 1));
        while (sscanf(fields, "%[^;]", field) == 1)
        {
            if (sscanf(field, "server_port=%hu", &serverPortNum) == 1)
            {
                foundServerPortNum = true;
            }
            else if (_strnicmp(field, "source=", 7) == 0)
            {
                delete[] foundServerAddressStr;
                foundServerAddressStr = strdup(field + 7);
            }

            fields += strlen(field);
            while (fields[0] == ';') ++fields; // skip over all leading ';' chars
            if (fields[0] == '\0') break;
        }
        delete[] field;

        if (foundServerPortNum)
        {
            serverAddressStr = foundServerAddressStr;
            return true;
        }

        delete[] foundServerAddressStr;
        return false;
    }

    bool
        CRTSPClient::parseRangeHeader(char const* buf, float& rangeStart, float& rangeEnd)
    {
        // Initialize the result parameters to default values:

        // First, find "Range:"
        while (1)
        {
            if (*buf == '\0') return false; // not found
            if (_strnicmp(buf, "Range: ", 7) == 0) break;
            ++buf;
        }

        rangeStart = rangeEnd = 0.0;

        // Then, run through each of the fields, looking for ones we handle:
        char const* fields = buf + 7;
        while (*fields == ' ') ++fields;
        float start, end;
        if (sscanf(fields, "npt = %f - %f", &start, &end) == 2)
        {
            rangeStart = start;
            rangeEnd = end;
        }
        else if (sscanf(fields, "npt = %f -", &start) == 1)
        {
            rangeStart = start;
        }
        else
        {
            return false; // The header is malformed
        }

        return true;
    }

    char*
        CRTSPClient::createRangeString(float start, float end)
    {
        char buf[100];

        /*  if (start < 0)
        {
        // We're resuming from a PAUSE; there's no "Range:" header at all
        buf[0] = '\0';
        }
        else */if (end < 0)
        {
            // There's no end time:
            sprintf(buf, "Range: npt=%.3f-\r\n", start);
        }
        else
        {
            // There's both a start and an end time; include them both in the "Range:" hdr
            sprintf(buf, "Range: npt=%.3f-%.3f\r\n", start, end);
        }

        return strdup(buf);
    }

    long
        CRTSPClient::GetServerTime()
    {
        //	return m_DBTime;
#ifndef _MAC_OS_
        time_t	dbTime;
        time(&dbTime);
        return (long)dbTime;
#else
#ifndef	_x86_64
        UInt32	org;
        UInt32	sec;
        DateTimeRec		d;

        GetDateTime(&sec);

        memset(&d, 0, sizeof(DateTimeRec));
        d.year = 1970;
        d.month = 1;
        d.day = 1;
        d.hour = 9;

        DateToSeconds(&d, &org);
        sec -= org;

        return (long)sec;
#else
        CFAbsoluteTime	org;
        CFAbsoluteTime	sec;

        org = -kCFAbsoluteTimeIntervalSince1970;
        sec = CFAbsoluteTimeGetCurrent();

        return (long)sec - (long)org;
#endif
#endif
    }

    long CRTSPClient::GetNextRtpChannelID()
    {
        m_RtpChannelID += 2;
        if (m_RtpChannelID > 100)
            m_RtpChannelID = 1;
        return m_RtpChannelID;
    }


    ///----------
    CRtcpReqArray::CRtcpReqArray()
    {
        m_pRoot = NULL;
        m_pTail = NULL;

#ifdef _WIN32
        m_hMutex = ::CreateMutex(NULL, false, NULL);
#else
        pthread_mutex_init(&m_hMutex, NULL);
#endif

    }

    CRtcpReqArray::~CRtcpReqArray()
    {
        int i;
        for (i = 0; i<100 && PopResReq(NULL) != false; i++)
        {
            printf("Rtsp req pop ...\n");
        }


#ifdef _WIN32
        ::CloseHandle(m_hMutex);
#else
        pthread_mutex_destroy(&m_hMutex);
#endif
    }

    void    CRtcpReqArray::Lock()
    {
#ifdef _WIN32
        ::WaitForSingleObject(m_hMutex, INFINITE);
#else
        pthread_mutex_lock(&m_hMutex);
#endif
    }

    void    CRtcpReqArray::Unlock()
    {
#ifdef _WIN32
        ::ReleaseMutex(m_hMutex);
#else
        pthread_mutex_unlock(&m_hMutex);
#endif
    }


    void    CRtcpReqArray::PushResReq(char* inReqStr)
    {
        if (inReqStr == NULL)
            return;

        Lock();
        struct resp_req_t   *pNewCell;
        pNewCell = new struct resp_req_t;
        strcpy(pNewCell->resp_req, inReqStr);
        pNewCell->pNext = NULL;

        if (m_pTail)
        {
            m_pTail->pNext = pNewCell;
        }
        else
        {
            m_pRoot = m_pTail = pNewCell;
        }

        Unlock();
    }

    bool   CRtcpReqArray::PopResReq(char* outStr)
    {
        bool   bResult = false;
        struct resp_req_t   *pDelCell;
        Lock();

        if (m_pRoot != NULL)
        {
            pDelCell = m_pRoot;
            m_pRoot = pDelCell->pNext;
            if (m_pRoot == NULL)
                m_pTail = NULL;
            if (outStr)
                strcpy(outStr, pDelCell->resp_req);
            delete pDelCell;

            bResult = true;
        }

        Unlock();
        return bResult;
    }

