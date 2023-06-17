#ifndef _MX_SOCKET_CONFIG_H_
#define _MX_SOCKET_CONFIG_H_

#ifndef _WIN32

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//#include "arpa_inet_custem.h"
//#include "netdb_custem.h"

#define ioctlsocket(fd, x, y)	fcntl(fd, F_SETFL, O_NONBLOCK)

#define INVALID_SOCKET			-1
#define SOCKET_ERROR			-1

#define WSANOTINITIALISED		-2
#define	WSAENETDOWN				ENETDOWN
#define WSAEACCES				EACCES
#define WSAEINPROGRESS			EINPROGRESS
#define WSAEALREADY				EALREADY
#define WSAEFAULT				EFAULT
#define WSAENETRESET			ENETRESET
#define WSAENOBUFS				ENOBUFS
#define WSAENOTCONN				ENOTCONN
#define WSAEWOULDBLOCK			EWOULDBLOCK
#define WSAEMSGSIZE				EMSGSIZE
#define WSAEHOSTUNREACH			EHOSTUNREACH
#define WSAEINVAL				EINVAL
#define WSAETIMEDOUT			ETIMEDOUT
#define WSAENOTSOCK				ENOTSOCK
#define WSAEAFNOSUPPORT			EAFNOSUPPORT
#define WSAEMFILE				EMFILE
#define WSAEPROTONOSUPPORT		EPROTONOSUPPORT
#define WSAEPROTOTYPE			EPROTOTYPE
#define WSAESOCKTNOSUPPORT		ESOCKTNOSUPPORT
#define WSAECONNRESET			ECONNRESET
#define	WSAEINTR				EINTR
#define	WSAEISCONN				EISCONN

inline int WSAGetLastError()
{
	return errno;
}

#ifndef closesocket
#define closesocket(s)			close(s)
#endif

typedef int						SOCKET;
typedef unsigned long			DWORD;

typedef struct sockaddr			SOCKADDR;
typedef struct sockaddr*		PSOCKADDR;
typedef struct sockaddr*		LPSOCKADDR;

typedef struct sockaddr_in		SOCKADDR_IN;
typedef struct sockaddr_in*		PSOCKADDR_IN;
//typedef struct sockaddr_in FAR *LPSOCKADDR_IN;

typedef struct in_addr			IN_ADDR;
typedef struct in_addr*			PIN_ADDR;
//typedef struct in_addr FAR*	LPIN_ADDR;

typedef /*struct*/ fd_set			FD_SET;
typedef /*struct*/ fd_set*			PFD_SET;
//typedef /*struct*/ fd_set FAR*	LPFD_SET;

typedef struct hostent			HOSTENT;
typedef struct hostent*			PHOSTENT;
//typedef struct hostent FAR*	LPHOSTENT;

typedef struct servent			SERVENT;
typedef struct servent*			PSERVENT;
//typedef struct servent FAR*	LPSERVENT;

typedef struct protoent			PROTOENT;
typedef struct protoent*		PPROTOENT;
//typedef struct protoent FAR*	LPPROTOENT;

typedef struct timeval			TIMEVAL;
typedef struct timeval*			PTIMEVAL;
//typedef struct timeval FAR*	LPTIMEVAL;

#endif

#endif

