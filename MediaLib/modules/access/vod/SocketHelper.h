
#ifndef _SocketHelper_VOD_H__
#define _SocketHelper_VOD_H__

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else

	#include <sys/socket.h>
	#include <netinet/in.h>
#ifdef _MAC_OS_
	#include "netdb_custem.h"
#endif
	#include <net/if.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
#ifdef _MAC_OS_
	#include <sys/filio.h>
	#include "arpa_inet_custem.h"
#endif
	
#endif

#include <string.h>
#include <stdio.h>
#ifdef _WIN32
	#include <time.h>
#else
	#include <sys/time.h>
#endif

#include "CUsageEnvironment.h"

#define SOCK_TIMEOUT_VALUE		20000000

    int setupDatagramSocket(CUsageEnvironment* inEnv, uint16_t port);
    int setupStreamSocket(CUsageEnvironment* inEnv, uint16_t port, bool makeNonBlocking = true);

    int readSocket(CUsageEnvironment* inEnv, int socket, unsigned char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, struct timeval* timeout = NULL);
    int readSocketbyTCP(CUsageEnvironment* inEnv, int s, unsigned char* recvBuf, int recvLen);
    bool writeSocket(CUsageEnvironment* inEnv, int socket, struct in_addr address, uint16_t port, unsigned char* buffer, unsigned bufferSize);

    unsigned getSendBufferSize(CUsageEnvironment* inEnv, int socket);
    unsigned getReceiveBufferSize(CUsageEnvironment* inEnv, int socket);
    unsigned setSendBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize);
    unsigned setReceiveBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize);
    unsigned increaseSendBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize);
    unsigned increaseReceiveBufferTo(CUsageEnvironment* inEnv, int socket, unsigned requestedSize);

    bool getSourcePort(CUsageEnvironment* inEnv, int socket, uint16_t& port);
    char const* vod_timestampString();

    unsigned our_inet_addr(char const* cp);
    char *our_inet_ntoa(struct in_addr in);
    int initializeWinsockIfNecessary(void);
    struct hostent* our_gethostbyname(char* name);
    long our_random();
    void our_srandom(unsigned int x);
    char* VodNetGetSockErrorStr(long inErrCode);


#endif//_SocketHelper_VOD_H__
