
#include <stdlib.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include "SocketHelper.h"
#include "CTaskScheduler.h"
#include "ErrorCode.h"
#ifndef _WIN32
#include "MxSocketConfig.h"
#endif

#define	WAIT_TIME	10*1000	//	 5s


    int setupDatagramSocket(CUsageEnvironment* inEnv, uint16_t port)
    {
        if (initializeWinsockIfNecessary() != 0)
        {
            inEnv->setResultMsg(11000326, "Failed to initialize socket!");
            return -1;
        }

        int newSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (newSocket < 0)
        {
            inEnv->setResultMsg(11000327, "Unable to create datagram socket!");
            return newSocket;
        }
        const int reuseFlag = 1;
        if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&reuseFlag, sizeof reuseFlag) < 0)
        {
            inEnv->setResultMsg(11000324, "Udp setsockopt(SO_REUSEADDR) error!");
            closesocket(newSocket);
            return -1;
        }

        // Note: Windoze requires binding, even if the port number is 0
        struct sockaddr_in name;
        name.sin_family = AF_INET;
        name.sin_port = port;
        name.sin_addr.s_addr = INADDR_ANY;//Receiving Address
        if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0)
        {
            inEnv->setResultMsg(11000324, "Udp Socket bind() error!");
            closesocket(newSocket);
            return -1;
        }
#ifdef	_WIN64
        setReceiveBufferTo(inEnv, newSocket, 0x2000000);
#else
        setReceiveBufferTo(inEnv, newSocket, 0x200000);
#endif

        return newSocket;
    }

    int setupStreamSocket(CUsageEnvironment* inEnv, uint16_t /*port*/, bool inMakeNonBlocking)
    {
        if (initializeWinsockIfNecessary() != 0)
        {
            inEnv->setResultMsg(11000326, "Failed to initialize socket!");
            return -1;
        }

        //printf("* setupstreamsock\n");

        int newSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newSocket < 0)
        {
            inEnv->setResultMsg(11000328, "Unable to create stream socket!");
            return newSocket;
        }

        const int reuseFlag = 1;
        if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&reuseFlag, sizeof(reuseFlag)) < 0)
        {
            inEnv->setResultMsg(11000329, "Tcp setsockopt(SO_REUSEADDR) error!");
            closesocket(newSocket);
            return -1;
        }


        setSendBufferTo(inEnv, newSocket, 0x100000);
#ifdef	_WIN64
        setReceiveBufferTo(inEnv, newSocket, 0x2000000);
#else
        setReceiveBufferTo(inEnv, newSocket, 0x200000);
#endif
        //printf("Send: Recv Buffer 0x30000\n");

        // Note: Windoze requires binding, even if the port number is 0
        /*	struct sockaddr_in name;
            name.sin_family = AF_INET;
            name.sin_port = htons(port);
            /*	name.sin_addr.s_addr = INADDR_ANY;//Receiving Address
            if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0)
            {
            inEnv->setResultMsg(11000330,"Tcp Socket bind() error!");
            closesocket(newSocket);
            return -1;
            }
            */
        if (inMakeNonBlocking)
        {
            // Make the socket non-blocking:
            unsigned long arg = 1;
            if (ioctlsocket(newSocket, FIONBIO, &arg) != 0)
            {
                inEnv->setResultMsg(11000331, "Failed to make non-blocking!");
                closesocket(newSocket);
                return -1;
            }
        }

#ifdef _MAC_
        unsigned long i_noSigPipe = 1;
        setsockopt(newSocket,SOL_SOCKET,SO_NOSIGPIPE,(char*)&i_noSigPipe,sizeof(i_noSigPipe));
        printf("Set Sigpipe...\r\n");
#endif


        return newSocket;
    }

    static int blockUntilReadable(CUsageEnvironment* inEnv, int socket, struct timeval* timeout)
    {
        int result = -1;

        do
        {
            fd_set rd_set;
            FD_ZERO(&rd_set);
            if (socket < 0) break;
            FD_SET((unsigned)socket, &rd_set);
            const unsigned numFds = socket + 1;

            result = select(numFds, &rd_set, NULL, NULL, timeout);
            if (timeout != NULL && result == 0)
            {
                break; // this is OK - timeout occurred
            }
            else if (result < 0)
            {
                inEnv->setResultMsg(11000332, "select() error!");
                break;
            }

            if (!FD_ISSET(socket, &rd_set))
            {
                inEnv->setResultMsg(11000332, "select() error!");
                break;
            }
        } while (0);

        return result;
    }

    int readSocket(CUsageEnvironment* inEnv, int socket, unsigned char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, struct timeval* /*timeout*/)
    {
        long theErrCode = 0;
        long theAppErrCode = 0;
        int bytesRead = -1;
        unsigned long theCurTicks;
        unsigned long theOldTicks = mdate();

        do
        {
            int addressSize = sizeof fromAddress;
            bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0,
                (struct sockaddr*)&fromAddress,
                &addressSize);
            if (bytesRead < 0)
            {

                theErrCode = WSAGetLastError();
                if (theErrCode == WSAEWOULDBLOCK || theErrCode == WSAEINPROGRESS)
                {
                    theCurTicks = mdate();
                    if ((theCurTicks, theOldTicks) > SOCK_TIMEOUT_VALUE)
                    {
                        bytesRead = -2;
                        inEnv->setResultMsg(11000340, "long wouldblock error at recvfrom!");
#ifndef NDEBUG
                        //fprintf(stderr,"Network time out(%s:%d)\n",__FILE__,__LINE__);
#endif							

                        break;
                    }
                    continue;
                }
                if (theErrCode == WSAECONNRESET)
                {
                    bytesRead = -2;
                }
                inEnv->setResultMsg(11000333, "recvfrom() error!");
                break;
            }
            return bytesRead;
        } while (1);

        return bytesRead;
    }

    char* VodNetGetSockErrorStr(long inErrCode)
    {
        switch (inErrCode)
        {
        case WSAEALREADY:			return "WSAEALREADY:An operation was attempted on a non-blocking socket that already had an operation in progress."; break;
        case WSAENOTSOCK:			return "WSAEDESTADDRREQ:A required address was omitted from an operation on a socket."; break;
        //case WSAECONNABORTED:	return "WSAECONNABORTED:An established connection was aborted by the software in your host machine."; break;
        case WSAECONNRESET:		return "WSAECONNRESET:An existing connection was forcibly closed by the remote host."; break;
        }
        return "";
    }

    int readSocketbyTCP(CUsageEnvironment* inEnv, int s, unsigned char* recvBuf, int recvLen)
    {
        long				len = 0;
        long				ret, err, nfds;
        unsigned long				start_time, cur_time;
        fd_set				readfds;
        struct timeval		timeoutval;

        nfds = 0;
        timeoutval.tv_sec = 0;			// 0s
        timeoutval.tv_usec = 100000;	// microsecond unit( 1ms )

        FD_ZERO(&readfds);
        FD_SET((unsigned long)s, &readfds);

        start_time = mdate();
        do
        {
            FD_ZERO(&readfds);
            FD_SET((unsigned long)s, &readfds);
            ret = select(s + 1, &readfds, NULL, NULL, &timeoutval);
            if (ret == SOCKET_ERROR)
            {
                long ret = WSAGetLastError();
                if ((ret != WSAEINPROGRESS) && (ret != WSAEWOULDBLOCK))
                    return -1;

                cur_time = mdate();
                if ((cur_time, start_time) > WAIT_TIME)
                    return -2;
            }
            else
                if (ret > 0)
                    break;
                else
                {	//ret == 0
                    cur_time = mdate();
                    if ((cur_time, start_time) > WAIT_TIME)
                        return -2;
                }

            msleep(1000);
        } while (1);

        do
        {
            ret = recv(s, (char*)(recvBuf + len), (recvLen - len), 0);
            if (ret == SOCKET_ERROR)		// if socket error
            {
                err = WSAGetLastError();
                if (err == WSAEINPROGRESS || err == WSAENOBUFS || err == WSAEWOULDBLOCK)
                {
                    cur_time = mdate();
                    if ((cur_time, start_time) > WAIT_TIME)		// 10s
                        return -2;
                    msleep(1000);
                    continue;
                }
                else // if sending buffer is not full
                {
                    char msg_Errode[256];
                    sprintf(msg_Errode, "Tcp Sock Error %d(%s)", err, VodNetGetSockErrorStr(err));
                    if (inEnv)
                        inEnv->setResultMsg(10000408, msg_Errode);

                    return -1;
                }
            }
            else if (ret == 0)	// user break
            {
                if (inEnv)
                    inEnv->setResultMsg(10000408, "Tcp Sock error recv=0(Host_UserBreak)!");
                return -1;
            }
            len += ret;
        } while (len < recvLen);		// if all data send

        return recvLen;
    }

    bool writeSocket(CUsageEnvironment* inEnv, int socket, struct in_addr address, uint16_t port, unsigned char* buffer, unsigned bufferSize)
    {
        do
        {
            struct sockaddr_in dest;

            dest.sin_family = AF_INET;
            dest.sin_port = port;
            dest.sin_addr = address;

            int bytesSent = sendto(socket, (char*)buffer, bufferSize, 0, (struct sockaddr*)&dest, sizeof dest);
            if (bytesSent != (int)bufferSize)
            {
                inEnv->setResultMsg(11000334, "sendto() error!");
                break;
            }

            return true;
        } while (0);

        return false;
    }

    static unsigned getBufferSize(CUsageEnvironment* inEnv, int bufOptName, int socket)
    {
        unsigned curSize;

        int sizeSize = sizeof curSize;
        if (getsockopt(socket, SOL_SOCKET, bufOptName,
            (char*)&curSize, &sizeSize) < 0)
        {
            inEnv->setResultMsg(11000335, "getBufferSize() error!");
            return 0;
        }

        return curSize;
    }

    unsigned getSendBufferSize(CUsageEnvironment* inEnv, int socket)
    {
        return getBufferSize(inEnv, SO_SNDBUF, socket);
    }

    unsigned getReceiveBufferSize(CUsageEnvironment* inEnv, int socket)
    {
        return getBufferSize(inEnv, SO_RCVBUF, socket);
    }

    static unsigned setBufferTo(CUsageEnvironment* inEnv, int bufOptName, int socket, unsigned requestedSize)
    {
        int sizeSize = sizeof requestedSize;
        int theResult = setsockopt(socket, SOL_SOCKET, bufOptName, (char*)&requestedSize, sizeSize);
        if (theResult == SOCKET_ERROR)
        {
            inEnv->setResultMsg(11000336, "setBufferTo error!");
            return 0;
        }
        // Get and return the actual, resulting buffer size:
        return getBufferSize(inEnv, bufOptName, socket);
    }

    unsigned setSendBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize)
    {
        return setBufferTo(inEnv, SO_SNDBUF, socket, requestedSize);
    }

    unsigned setReceiveBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize)
    {
        return setBufferTo(inEnv, SO_RCVBUF, socket, requestedSize);
    }

    static unsigned increaseBufferTo(CUsageEnvironment* inEnv, int bufOptName, int socket, unsigned requestedSize)
    {
        // First, get the current buffer size.  If it's already at least
        // as big as what we're requesting, do nothing.
        unsigned curSize = getBufferSize(inEnv, bufOptName, socket);
        if (curSize == 0)
            return 0;
        // Next, try to increase the buffer to the requested size,
        // or to some smaller size, if that's not possible:
        while (requestedSize > curSize)
        {
            int sizeSize = sizeof requestedSize;
            if (setsockopt(socket, SOL_SOCKET, bufOptName,
                (char*)&requestedSize, sizeSize) >= 0)
            {
                // success
                return requestedSize;
            }
            requestedSize = (requestedSize + curSize) / 2;
        }

        return getBufferSize(inEnv, bufOptName, socket);
    }

    unsigned increaseSendBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize)
    {
        return increaseBufferTo(inEnv, SO_SNDBUF, socket, requestedSize);
    }

    unsigned increaseReceiveBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize)
    {
        return increaseBufferTo(inEnv, SO_RCVBUF, socket, requestedSize);
    }

    static bool getSourcePort0(int socket, uint16_t& resultPortNum)
    {
        sockaddr_in test;
        test.sin_port = 0;
        int len = sizeof test;
        if (getsockname(socket, (struct sockaddr*)&test, &len) < 0)
        {
            return false;
        }

        resultPortNum = ntohs(test.sin_port);
        return true;
    }

    bool getSourcePort(CUsageEnvironment* inEnv, int socket, uint16_t& port)
    {
        uint16_t portNum = 0;

        if (!getSourcePort0(socket, portNum) || portNum == 0)
        {
            // Hack - call bind(), then try again:
            struct sockaddr_in name;
            name.sin_family = AF_INET;
            name.sin_port = 0;
            name.sin_addr.s_addr = INADDR_ANY;
            bind(socket, (struct sockaddr*)&name, sizeof name);

            if (!getSourcePort0(socket, portNum) || portNum == 0)
            {
                inEnv->setResultMsg(11000337, "getsockname() error!");
                return false;
            }
        }

        port = htons(portNum);
        return true;
    }

    static bool badAddress(uint32_t addr)
    {
        // Check for some possible erroneous addresses:
        uint32_t hAddr = ntohl(addr);
        return (hAddr == 0x7F000001 /* 127.0.0.1 */
            || hAddr == 0
            || hAddr == (uint32_t)(~0));
    }

    char const* vod_timestampString()
    {
        struct timeval tvNow;
        gettimeofday(&tvNow, NULL);

        static char timeString[9]; // holds hh:mm:ss plus trailing '\0'
        char const* ctimeResult = ctime((time_t*)&tvNow.tv_sec);
        char const* from = &ctimeResult[11];
        int i;
        for (i = 0; i < 8; ++i)
        {
            timeString[i] = from[i];
        }
        timeString[i] = '\0';

        return (char const*)&timeString;
    }

#ifdef _WIN32
#include <sys/timeb.h>

    static int gettimeofday(struct timeval* tp, int* /*tz*/) 
    {
        struct timeb tb;
        ftime(&tb);
        tp->tv_sec = tb.time;
        tp->tv_usec = 1000*tb.millitm;

        return 0;
    }
#endif

    unsigned our_inet_addr(char const* cp)
    {
        return inet_addr(cp);
    }

    char *our_inet_ntoa(struct in_addr in)
    {
        return inet_ntoa(in);
    }

#define WS_VERSION_CHOICE1 0x202/*MAKEWORD(2,2)*/
#define WS_VERSION_CHOICE2 0x101/*MAKEWORD(1,1)*/

    int initializeWinsockIfNecessary(void)
    {
        int errCode = 0;
        /* We need to call an initialization routine before
         * we can do anything with winsock.  (How fucking lame!):
         */
#ifdef _WIN32
        static int _haveInitializedWinsock = 0;

        WSADATA	wsadata;

        if (!_haveInitializedWinsock) 
        {
            char	szBuff[80];
            DWORD	dwSize = sizeof(szBuff);
            short	err = gethostname(szBuff, dwSize);	// 2006.1.16 kms<-RTY
            if(err != 0)
            {
                err = (INT)WSAGetLastError ();
                if( err == WSANOTINITIALISED )//	no socket init
                {
                    if ((WSAStartup(WS_VERSION_CHOICE1, &wsadata) != 0)
                        && ((WSAStartup(WS_VERSION_CHOICE2, &wsadata)) != 0)) 
                    {
                        errCode = WSAGetLastError();
                        return errCode; /* error in initialization */
                    }
                    if ((wsadata.wVersion != WS_VERSION_CHOICE1)
                        && (wsadata.wVersion != WS_VERSION_CHOICE2)) 
                    {
                        WSACleanup();
                        return -1; /* desired Winsock version was not available */
                    }
                }
            }

            _haveInitializedWinsock = 1;
        }
#endif
        return errCode;
    }

    struct hostent* our_gethostbyname(char* name)
    {
        long theErrCode = 0;
        if ((theErrCode = initializeWinsockIfNecessary()) != 0) return NULL;

        return (struct hostent*) gethostbyname(name);
    }

    long our_random()
    {
        return rand();
    }

    void our_srandom(unsigned int x)
    {
        srand(x);
    }
