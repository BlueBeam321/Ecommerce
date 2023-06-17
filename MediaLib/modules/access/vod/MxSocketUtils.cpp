#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include "MxErrCodes.h"
#include "MxSocketUtils.h"

#ifdef _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <MSTcpip.h>
#endif
#ifdef __linux__
#include "MxSocketConfig.h"
#include <netinet/tcp.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#endif

#define MX_TIME_ONE_SEC     1000

#define	mxSocket_TCP			1
#define	mxSocket_UDP			2
#define	mxSocket_MULTI_Send		3
#define	mxSocket_MULTI_Recv		4

namespace MxSocketUtils
{

int32_t
InitializeSocket(
	int32_t*	outSysErr)
{
	int32_t i_ret = 0;
	int32_t i_sys_err = 0;
	
#ifdef _WIN32
	static int32_t g_MX_var_socket_initialized = 0;
	
	if (g_MX_var_socket_initialized == 0)
	{
		int32_t i_err;
		char name[128];
		int name_size = sizeof(name);
		
		i_err = ::gethostname(name, name_size);
		if(i_err != 0)
		{
			i_err = ::WSAGetLastError();
			if(i_err == WSANOTINITIALISED)
			{
				WSADATA wsaData;
				WORD wVersionRequested = MAKEWORD(2,2);
				
				i_err = ::WSAStartup(wVersionRequested, &wsaData);
			}
		}
		
		if(i_err)
			g_MX_var_socket_initialized = -1;
		else
			g_MX_var_socket_initialized = 1;
	}
	
	if(g_MX_var_socket_initialized == -1)
	{
		i_ret = mxErr_SockInit;
		i_sys_err = -1;
	}
#endif
	
	if(outSysErr)
		*outSysErr = i_sys_err;
	
	return i_ret;
}

void
GetLocalIPAddress(
	char*		outLocalAddr,
	MxSockRef	inSocket)
{
	int32_t i_err, i_sys_err;

	InitializeSocket(&i_sys_err);

	if(inSocket == mxSocket_Invalid)
	{
#if 1
		::strcpy(outLocalAddr, "");
#else
#if defined(_MX_MAC_)
		::strcpy(outLocalAddr, "");
#elif !defined(_MX_LINUX_)
		int32_t szSize;
		char szBuff[128];
		PHOSTENT phe;
		DecVar(SOCKADDR_IN, self_addr);
		
		szSize = sizeof(szBuff);
		i_err = ::gethostname(szBuff, szSize); // get local computer's name. ex: "01-Dell-khn"
		phe = ::gethostbyname(szBuff);
		
		if(phe != NULL)
		{
			::memcpy((void*)&self_addr.sin_addr, phe->h_addr, phe->h_length);
			char* p_addr = ::inet_ntoa(self_addr.sin_addr);
			::strcpy(outLocalAddr, p_addr);
		}
#else
		int fd;
		struct ifreq ifr;
		unsigned char data[4];
		memset(data, 0x0, 4);

		fd = socket(PF_INET, SOCK_DGRAM, 0);
		for (int i = 0; i < 10; i++)
		{
			sprintf(ifr.ifr_name, "eth%d", i);
			if(ioctl(fd, SIOCGIFADDR, &ifr) != -1)
			{
				memcpy(data, &ifr.ifr_addr.sa_data[2], 4);
				sprintf(outLocalAddr, "%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
				break;
			}
		}

		close(fd);
#endif
#endif
	}
	else
	{
		SOCKADDR_IN self_addr;
		socklen_t addr_size = sizeof(self_addr);

		i_err = ::getsockname(inSocket, (LPSOCKADDR)&self_addr, &addr_size);
		if (i_err == 0)
			::strcpy(outLocalAddr, ::inet_ntoa(self_addr.sin_addr));
		else
			::strcpy(outLocalAddr, "");
	}
}

int32_t
GetSockSinPort(
	MxSockRef	inSocket)
{
	int32_t i_ret = 0;

	if(inSocket != mxSocket_Invalid)
	{
		SOCKADDR_IN theAddr;
		socklen_t addr_size = sizeof(theAddr);

		::getsockname(inSocket, (LPSOCKADDR)&theAddr, &addr_size);
		i_ret = (int32_t)ntohs(theAddr.sin_port);
	}

	return i_ret;
}

uint32_t
ConvertIPAddrToLong(
	char*		inIPAddr)
{
	return (uint32_t)(::inet_addr(inIPAddr));
}

void
ConvertLongToIPAddr(
	uint32_t	inNumAddr,
	char*		outIPAddr)
{
	IN_ADDR addr;
#ifdef _WIN32	
	addr.S_un.S_addr = inNumAddr;
#else
	addr.s_addr = inNumAddr;
#endif
	::strcpy(outIPAddr, ::inet_ntoa(addr));
}

int32_t
SetKeepAliveTime(
	MxSockRef	inSocket,
	bool		bSetKeepAlive,
	mtime_t		inKeepAliveTime,
	mtime_t		inKeepAliveInterval,
	int32_t*	outSysErr)
{
	if(inSocket == mxSocket_Invalid)
		return mxErr_SockGeneric;

#ifdef _WIN32	
	DWORD dwBytes;
	int32_t i_err, i_ret, i_sys_err;
	tcp_keepalive keepalive;
	
	i_ret = mxErr_None;
	i_sys_err = 0;

	keepalive.onoff = (bSetKeepAlive) ? 1 : 0;
	keepalive.keepalivetime = (u_long)(inKeepAliveTime / 1000);
	keepalive.keepaliveinterval = (u_long)(inKeepAliveInterval / 1000);
	
	i_err = ::WSAIoctl(inSocket, SIO_KEEPALIVE_VALS, (LPVOID)&keepalive, sizeof(tcp_keepalive), NULL, 0, &dwBytes, NULL, NULL);

	if (i_err == SOCKET_ERROR)
	{
		i_sys_err = ::WSAGetLastError();
		i_ret = mxErr_SockKeepAliveTime;
	}

	if(outSysErr)
		*outSysErr = i_sys_err;
	
	return i_ret;
#else
	return mxErr_None;
#endif
}

int32_t
_GetBufferSize(
	MxSockRef	inSocket,
	int32_t		inBufType,
	int32_t*	outBufSize) 
{
	SOCKET i_socket = inSocket;

	int cur_size = 0, i_sys_err = 0;
	socklen_t sizeoftype = sizeof(cur_size);

	if(::getsockopt(i_socket, SOL_SOCKET, inBufType, (char*)&cur_size, &sizeoftype) < 0)
	{
		i_sys_err = ::WSAGetLastError();
		cur_size = 0;
	}

	if(outBufSize)
		*outBufSize = cur_size;

	return i_sys_err;
}

int32_t
_SetBufferSize(
	MxSockRef	inSocket,
	int32_t		inBufType,
	int32_t		inReqSize,
	int32_t*	outSysErr)
{
	int32_t i_err = 0, i_sys_err = 0;
	int32_t cur_size = 0;
	int req_size = inReqSize;
	
	i_sys_err = _GetBufferSize(inSocket, inBufType, &cur_size);
#if 0 // 1.15
	if(cur_size == 0)
	{
		if(inBufType == SO_SNDBUF)
			i_err = mxErr_SockSendBuf;
		else
			i_err = mxErr_SockRecvBuf;
	}
	else
#endif
	{
		SOCKET i_socket = inSocket;
		
		while(req_size > cur_size)
		{
			if(::setsockopt(i_socket, SOL_SOCKET, inBufType, (char*)&req_size, sizeof(req_size)) == 0)
			{
				i_err = 0;
				i_sys_err = 0;
				break;
			}
			
			i_sys_err = ::WSAGetLastError();
			if(inBufType == SO_SNDBUF)
				i_err = mxErr_SockSendBuf;
			else
				i_err = mxErr_SockRecvBuf;
            
			if(cur_size <= 0)
				break;

			req_size = (req_size + cur_size) / 2;
		}

		if(outSysErr)
			*outSysErr = i_sys_err;
	}

	return i_err;
}

int32_t
_SetSocketOptionWithLog(
	MxSockRef	inSocket,
	int32_t		inSocketFlags,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t*	outSysErr,
	bool		bIgnoreSetBufSizeError)
{
	int32_t i_ret, i_err, i_sys_err;
	uint32_t i_32;
	int i_flag;
	SOCKET i_socket;

	i_ret = mxErr_None;
	i_sys_err = 0;
	i_socket = inSocket;

	if(i_socket == mxSocket_Invalid)
		i_ret = mxErr_SockGeneric;

	// RecvBufSize
	if(i_ret == mxErr_None && inRecvBufSize > 0)
	{
		i_ret = _SetBufferSize(i_socket, SO_RCVBUF, inRecvBufSize, &i_sys_err);

		if(bIgnoreSetBufSizeError)
		{
			i_ret = mxErr_None;
			i_sys_err = 0;
		}
	}

	// SendBufSize
	if(i_ret == mxErr_None && inSendBufSize > 0)
	{
		i_ret = _SetBufferSize(i_socket, SO_SNDBUF, inSendBufSize, &i_sys_err);

		if(bIgnoreSetBufSizeError)
		{
			i_ret = mxErr_None;
			i_sys_err = 0;
		}
	}

	// ReuseAddr
	if(i_ret == mxErr_None && (inSocketFlags & mxSocketFlag_SockReuseAddr))
	{
		i_flag = 1;
		i_err = ::setsockopt(i_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&i_flag, sizeof(i_flag));
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockReuseAddr;
		}
	}
	// Non-blocking
	if(i_ret == mxErr_None && (inSocketFlags & mxSocketFlag_SockNonBlock))
	{
		i_32 = 1;
		i_err = ::ioctlsocket(i_socket, FIONBIO, (unsigned long*)&i_32);
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockNonBlock;
		}
	}
	// KeepAlive
	if(i_ret == mxErr_None && (inSocketFlags & mxSocketFlag_SockKeepAlive))
	{
		i_flag = 1;
		i_err = ::setsockopt(i_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&i_flag, sizeof(i_flag));
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockKeepAlive;
		}
		else
		{
			i_err = SetKeepAliveTime(i_socket, true, 3000000, 1000000, &i_sys_err);
			if(i_err)
				i_ret = mxErr_SockKeepAliveTime;
		}
	}
	// No-delay
	if(i_ret == mxErr_None && (inSocketFlags & mxSocketFlag_SockNoDelay))
	{
		i_flag = 1;
		i_err = ::setsockopt(i_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&i_flag, sizeof(i_flag));
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockNoDelay;
		}
	}
	
	if(outSysErr)
		*outSysErr = i_sys_err;

	return i_ret;
}

int32_t
SetSocketOption(
	MxSockRef	inSocket,
	int32_t		inSocketFlags,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t*	outSysErr)
{
	return _SetSocketOptionWithLog(inSocket, inSocketFlags, inSendBufSize, inRecvBufSize, outSysErr, false);
}

bool
IsMulticastAddress(
	char*		inMulticastAddr)
{
	uint32_t i_multi_addr = ntohl(::inet_addr(inMulticastAddr));
	return ((i_multi_addr>>8) & 0x00f00000) == 0x00e00000;	//	multicast address class == "class D" == 0xExxxxxxx == 1,1,1,0,<28 bits>
}

int32_t
_CreateSocket(
	int32_t		inSocketType,
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inMultiAddress,
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inTTL,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err, i_err;
	SOCKET i_socket;
	
	i_sys_err = 0;
	i_socket = INVALID_SOCKET;

	i_ret = InitializeSocket(&i_sys_err);

	if(i_ret == mxErr_None && inMultiAddress && !IsMulticastAddress(inMultiAddress))
		i_ret = mxErr_SockInvalidAddr;

	// Create Socket
	if(i_ret == mxErr_None)
	{
		switch(inSocketType)
		{
		case mxSocket_UDP:
		case mxSocket_MULTI_Send:
		case mxSocket_MULTI_Recv:
			{
				i_socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
				break;
			}
		case mxSocket_TCP:
		default:
			{
				i_socket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
				break;
			}
		}

		if(i_socket == INVALID_SOCKET)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockCreate;
		}
	}

	// Set Options
	if(i_ret == mxErr_None)
		i_ret = _SetSocketOptionWithLog(i_socket, inSocketFlags, inSendBufSize, inRecvBufSize, &i_sys_err, true);

	// Bind
	if(i_ret == mxErr_None && (inBindAddress || inBindPort))
	{
		SOCKADDR_IN bind_addr;
		
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = (inBindAddress) ? ::inet_addr(inBindAddress) : INADDR_ANY;
		bind_addr.sin_port = htons(inBindPort);
		
		i_err = ::bind(i_socket, (SOCKADDR*)&bind_addr, sizeof(bind_addr));
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockBind;
		}

		if(inSocketType == mxSocket_MULTI_Send)
		{
			// Multicast interface
			if(i_ret == mxErr_None)
			{
				i_err = ::setsockopt(i_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&bind_addr.sin_addr, sizeof(bind_addr.sin_addr));
				if(i_err)
				{
					i_sys_err = ::WSAGetLastError();
					i_ret = mxErr_SockMultiIF;
				}
			}
			// TTL
			if(i_ret == mxErr_None && inTTL)
			{
				int nOptVal = inTTL;
				i_err = ::setsockopt(i_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&nOptVal, sizeof(nOptVal));
				if(i_err)
				{
					i_sys_err = ::WSAGetLastError();
					i_ret = mxErr_SockMultiTTL;
				}
			}
		}
	}

	if(i_ret == mxErr_None && inMultiAddress && inSocketType == mxSocket_MULTI_Recv)
	{
		struct ip_mreq multi_req;
		
		multi_req.imr_multiaddr.s_addr = ::inet_addr(inMultiAddress);
		multi_req.imr_interface.s_addr = (inBindAddress) ? ::inet_addr(inBindAddress) : INADDR_ANY;
		
		i_err = ::setsockopt(i_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&multi_req, sizeof(multi_req));
		if(i_err)
		{
			i_sys_err = ::WSAGetLastError();
			i_ret = mxErr_SockMultiMembership;
		}
	}

	if(i_ret != mxErr_None && i_socket != INVALID_SOCKET)
	{
		::closesocket(i_socket);
		i_socket = INVALID_SOCKET;
	}

	if(i_socket == INVALID_SOCKET)
		i_socket = mxSocket_Invalid;

	*outSocket = i_socket;
	if(outSysErr)
		*outSysErr = i_sys_err;

	return i_ret;
}

int32_t
CreateTcpSocket(
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	return _CreateSocket(mxSocket_TCP, outSocket, outSysErr, NULL, inBindAddress, inBindPort, inSendBufSize, inRecvBufSize, 0, inSocketFlags);
}

int32_t
CreateUdpSocket(
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	return _CreateSocket(mxSocket_UDP, outSocket, outSysErr, NULL, inBindAddress, inBindPort, inSendBufSize, inRecvBufSize, 0, inSocketFlags);
}

int32_t
CreateMultiSendSocket(
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inBindAddress,
	int32_t		inTTL,
	int32_t		inSendBufSize,
	int32_t		inSocketFlags)
{
	return _CreateSocket(mxSocket_MULTI_Send, outSocket, outSysErr, NULL, inBindAddress, 0, inSendBufSize, 0, inTTL, inSocketFlags);
}

int32_t
CreateMultiRecvSocket(
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inMultiAddress,
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	return _CreateSocket(mxSocket_MULTI_Recv, outSocket, outSysErr, inMultiAddress, inBindAddress, inBindPort, 0, inRecvBufSize, 0, inSocketFlags);
}

void
CloseSocket(
	MxSockRef	inSocket)
{
	if(inSocket != mxSocket_Invalid)
	{
		::closesocket(inSocket);
	}
}

void
CloseMultiRecvSocket(
	MxSockRef	inSocket,
	char*		inMultiAddress)
{
	if(inSocket != mxSocket_Invalid)
	{
		if(inMultiAddress && inMultiAddress[0])
		{
			int32_t i_err;
			struct ip_mreq mreq;
			
			mreq.imr_multiaddr.s_addr = ::inet_addr(inMultiAddress);
			mreq.imr_interface.s_addr = INADDR_ANY; 
			
			i_err = ::setsockopt(inSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
			//if(i_err)
			//	i_err = GetLastError();
		}
		
		::closesocket(inSocket);
	}
}

int32_t
Connect(
	MxSockRef	inSocket,
	char*		inServerAddress,
	uint32_t	inServerPort,
	int32_t*	outSysErr,
	mtime_t		inTimeout)
{
	int32_t i_ret, i_sys_err, i_err;
	SOCKET i_socket;
	SOCKADDR_IN server_addr;
	
	i_sys_err = 0;
	i_ret = 0;
	i_socket = inSocket;
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = ::inet_addr(inServerAddress);
	server_addr.sin_port = htons(inServerPort);
	
	i_err = ::connect(i_socket, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if(i_err == SOCKET_ERROR)
	{
		i_sys_err = ::WSAGetLastError();
		if(i_sys_err != WSAEWOULDBLOCK && i_sys_err != WSAEINPROGRESS && i_sys_err != WSAEALREADY)
		{
			i_ret = mxErr_SockConnect;
		}
		else
		{
			bool b_stopped;
			int nfound;
			FD_SET writefds, exceptfds;
			TIMEVAL timeout, one_timeout;
			mtime_t start_time, cur_time, i_timeout;
			bool b_connected;
			
			i_timeout = inTimeout;
			if(i_timeout < 0)
				i_timeout = 0;
			
			i_err = 0;
			b_stopped = false;
			b_connected = false;
			start_time = mdate();
			
			one_timeout.tv_sec = 0;
			one_timeout.tv_usec = 1000;
			
			while((i_sys_err == WSAEWOULDBLOCK || i_sys_err == WSAEINPROGRESS || i_sys_err == WSAEALREADY))
			{
				timeout = one_timeout;
				
				FD_ZERO(&writefds); FD_SET(i_socket, &writefds);
				FD_ZERO(&exceptfds); FD_SET(i_socket, &exceptfds);
				
				nfound = ::select(i_socket + 1, (FD_SET*)0, (FD_SET*)&writefds, (FD_SET*)&exceptfds, &timeout);
				if(nfound < 0)
				{
					i_sys_err = ::WSAGetLastError();
					i_err = i_sys_err;
				}
				
				if(nfound > 0 && FD_ISSET(i_socket, &writefds))
				{
#ifdef _WIN32
					b_connected = true;
					break;
#endif
#if defined( _MX_MAC_ ) || defined(_MX_LINUX_)
					i_err = ::connect(i_socket, (SOCKADDR*)&server_addr, sizeof(server_addr));
					if(i_err < 0)
						i_err = ::WSAGetLastError();
					if(i_err == WSAEISCONN)
						i_err = 0;
					if(i_err == 0)
					{
						b_connected = true;
						break;
					}
					i_sys_err = i_err;
#endif
				}
				
				if(nfound > 0 && FD_ISSET(i_socket, &exceptfds))
				{
					i_err = ::connect(i_socket, (SOCKADDR*)&server_addr, sizeof(server_addr));
					if(i_err < 0)
						i_err = ::WSAGetLastError();
					if(i_err == WSAEISCONN)
						i_err = 0;
					if(i_err == 0)
					{
						b_connected = true;
						break;
					}
					i_sys_err = i_err;
				}
				
				cur_time = mdate();
				if((cur_time - start_time) > (uint64_t)i_timeout)
					break;
				
				msleep(1000);
			}
			
			if(!b_connected)
			{
				if(i_err)
					i_ret = mxErr_SockGeneric;
				else if(b_stopped)
					i_ret = mxErr_SockStopByUser;
				else
					i_ret = mxErr_SockTimeout;
			}
			else
			{
				i_sys_err = 0;
				i_ret = mxErr_None;
			}
		}
	}

	if(outSysErr)
		*outSysErr = i_sys_err;
	
	return i_ret;
}

int32_t
CreateAndConnectByTcp(
	MxSockRef*	outSocket,
	int32_t*	outSysErr,
	char*		inServerAddress,
	uint32_t	inServerPort,
	mtime_t		inTimeout,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err;
	MxSockRef i_socket;

	i_ret = CreateTcpSocket(&i_socket, &i_sys_err, NULL, 0, inSendBufSize, inRecvBufSize, inSocketFlags);
	if(i_ret == mxErr_None)
		i_ret = Connect(i_socket, inServerAddress, inServerPort, &i_sys_err, inTimeout);

	if(i_ret != mxErr_None && i_socket != mxSocket_Invalid)
	{
		CloseSocket(i_socket);
		i_socket = mxSocket_Invalid;
	}

	*outSocket = i_socket;
	if(outSysErr)
		*outSysErr = i_sys_err;

	return i_ret;
}

int32_t
Listen(
	MxSockRef	inSocket,
	int32_t		inMaxConn,
	int32_t*	outSysErr)
{
	int32_t i_ret, i_err, i_sys_err;

	i_ret = mxErr_None;
	i_sys_err = 0;

	i_err = ::listen(inSocket, inMaxConn);

	if(i_err == SOCKET_ERROR)
	{
		i_sys_err = ::WSAGetLastError();
		i_ret = mxErr_SockListen;
	}

	if(outSysErr)
		*outSysErr = i_sys_err;

	return i_ret;
}

MxSockRef
_AcceptWithLog(
	MxSockRef	inSocket,
	mtime_t		inTimeout,
	char*		outClientAddr,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	int nfound;
	SOCKET i_sock_client;
	FD_SET readfds;
	TIMEVAL timeout;
	SOCKADDR_IN client_addr;
	socklen_t client_addr_len;
	
	FD_ZERO(&readfds);
	FD_SET(inSocket, &readfds);
	
    timeout.tv_sec = (int32_t)(inTimeout / MX_TIME_ONE_SEC);
    timeout.tv_usec = (int32_t)(inTimeout % MX_TIME_ONE_SEC);

	nfound = ::select(inSocket + 1, &readfds, (FD_SET*)0, (FD_SET*)0, &timeout);
	if(nfound <= 0)
		return mxSocket_Invalid;

	client_addr_len = sizeof(client_addr);
	i_sock_client = ::accept(inSocket, (LPSOCKADDR)&client_addr, &client_addr_len);

	if(i_sock_client != INVALID_SOCKET)
	{
		if(_SetSocketOptionWithLog(i_sock_client, inSocketFlags, inSendBufSize, inRecvBufSize, NULL, true) != 0)
		{
			CloseSocket(i_sock_client);
			i_sock_client = INVALID_SOCKET;
		}
		else if(outClientAddr)
		{
			char* host_name = ::inet_ntoa(client_addr.sin_addr);
			::strcpy(outClientAddr, host_name);
		}
	}
	
	if(i_sock_client == INVALID_SOCKET)
		i_sock_client = mxSocket_Invalid;
	
	return i_sock_client;
}

MxSockRef
Accept(
	MxSockRef	inSocket,
	mtime_t		inTimeout,
	char*		outClientAddr,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	return _AcceptWithLog(inSocket, inTimeout, outClientAddr, inSendBufSize, inRecvBufSize, inSocketFlags);
}

// --------------------------------------------------------

int32_t
_SendData(
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	char*		inDestAddr,
	uint32_t	inDestPort,
	int32_t*	outSysErr,
	mtime_t		inTimeout,
	int32_t		inSendUnit,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err, i_send, i_send_unit;
	SOCKET i_socket;
	mtime_t i_timeout;

	SOCKADDR_IN* p_dest;
	SOCKADDR_IN dest_addr;

	char* p_buffer = inBuffer;
	int32_t i_size = inSize;
	
	i_ret = mxErr_None;
	i_sys_err = 0;
	i_timeout = inTimeout;
	i_socket = inSocket;
	i_send_unit = inSendUnit;

	p_dest = NULL;
	if(inDestAddr || inDestPort)
	{
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_addr.s_addr = (inDestAddr) ? ::inet_addr(inDestAddr) : INADDR_ANY;
		dest_addr.sin_port = htons(inDestPort);

		p_dest = &dest_addr;
	}
	
	if (i_timeout <= 0)
		i_timeout = MX_TIME_ONE_SEC * 30;
	
	// UDP
	if(p_dest)
		i_send_unit = 0;

	while(i_size && i_ret == mxErr_None)
	{
		if (i_send_unit > 0)
			i_send = i_size > i_send_unit ? i_send_unit : i_size;
		else
			i_send = i_size;
		
		// send data
		{
			int sendlen;
			mtime_t cur_time, start_time;
			char* p_send_buf = p_buffer;
			int32_t i_send_buf = i_send;
			
			start_time = mdate();
			
			while(i_send_buf)
			{

				int32_t	theSendFlag = 0;
//#ifndef _WIN32
//				theSendFlag = MSG_NOSIGNAL;
//#endif
				
				if(p_dest)
					sendlen = ::sendto(i_socket, (const char*)p_send_buf, i_send_buf, 0, (SOCKADDR*)p_dest, sizeof(SOCKADDR_IN));
				else
					sendlen = ::send(i_socket, (const char*)p_send_buf, i_send_buf, theSendFlag);

				if(sendlen == SOCKET_ERROR)
				{
					i_sys_err = ::WSAGetLastError();
					
					if( (p_dest && i_sys_err == WSAEWOULDBLOCK) ||
						(p_dest == NULL && (i_sys_err == WSAEINPROGRESS || i_sys_err == WSAENOBUFS || i_sys_err == WSAEWOULDBLOCK)) )
					{
						cur_time = mdate();
						if((cur_time - start_time) > (uint64_t)i_timeout)
						{
							i_ret = mxErr_SockTimeout;
							break;
						}
						else
						{
							i_sys_err = 0;
							msleep(1000);
							continue;
						}
					}
					else
					{
						i_ret = mxErr_SockGeneric;
						break;
					}
				}
				
				i_send_buf -= sendlen;
				p_send_buf += sendlen;
			}
		}
		
		if(i_send_unit > 0)
		{
			mtime_t sleep_time = (mtime_t)(i_send * 1000) / i_send_unit;
			if (sleep_time > 0)
				msleep(sleep_time);
		}
		
		i_size -= i_send;
		p_buffer += i_send;
	}
	
	if(outSysErr)
		*outSysErr = i_sys_err;
	
	return i_ret;
}

int32_t
Send(
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSysErr,
	mtime_t		inTimeout,
	int32_t		inSendUnit,
	int32_t		inFlags)
{
	return _SendData(inSocket, inBuffer, inSize, NULL, 0, outSysErr, inTimeout, inSendUnit, inFlags);
}

int32_t
SendDatagram(
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	char*		inDestAddr,
	uint32_t	inDestPort,
	int32_t*	outSysErr,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	return _SendData(inSocket, inBuffer, inSize, inDestAddr, inDestPort, outSysErr, inTimeout, 0, inFlags);
}

// --------------------------------------------------------

int32_t
OnceSelectRecv(
	MxSockRef	inSocket,
	int32_t*	outSysErr,
	mtime_t		inTimeout)
{
	int32_t i_sys_err = mxErr_None;

	int nfound;
	FD_SET readfds;
	TIMEVAL timeout;
	SOCKET i_socket = inSocket;
	
	FD_ZERO(&readfds);
	FD_SET(i_socket, &readfds);
	
    timeout.tv_sec = (int32_t)(inTimeout / MX_TIME_ONE_SEC);
    timeout.tv_usec = (int32_t)(inTimeout % MX_TIME_ONE_SEC);
	
	nfound = ::select(i_socket + 1, &readfds, (FD_SET*)0, (FD_SET*)0, &timeout);

	if(nfound < 0 && outSysErr)
		i_sys_err = ::WSAGetLastError();
	
	if(outSysErr)
		*outSysErr = i_sys_err;

	return nfound;
}

int32_t
SelectRecv(
	MxSockRef	inSocket,
	int32_t*	outSysErr,
	mtime_t		inTimeout)
{
	int32_t i_found = 0;
	mtime_t prev_time = mdate();
	bool b_stopped = false;

	while(1)
	{
		i_found = OnceSelectRecv(inSocket, outSysErr, 1000);

		if(i_found)
			break;
		else if((mdate() - prev_time) > (uint32_t)inTimeout)
			break;

		msleep(1000);
	}

	return i_found;
}

int32_t
_ReceiveData(
	bool		bTCP,
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	int32_t*	outSysErr,
	char*		outSrcAddr,
	uint32_t*	outSrcPort,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err;
	int nfound;
	int32_t recvlen, packlen;
	FD_SET readfds;
	TIMEVAL timeout;
	SOCKADDR_IN src_addr;
	bool b_addr_valid;

	bool b_stopped;
	mtime_t start_time, cur_time;
	mtime_t i_timeout = inTimeout;
	SOCKET i_socket = inSocket;

	if(i_timeout <= 0)
		i_timeout = MX_TIME_ONE_SEC * 30;

	nfound = 0;
	i_ret = 0;
	i_sys_err = 0;
	b_stopped = false;
	packlen = 0;
	b_addr_valid = false;

	start_time = mdate();

	char* p_buffer = inBuffer;
	int32_t i_size = inSize;
	socklen_t src_addr_size;
	bool firstFlag = true;

	while(1)
	{
		// sleep
		if (!firstFlag)
		{
			cur_time = mdate();
			if((cur_time - start_time) > (uint64_t)i_timeout)
			{
				i_ret = mxErr_SockTimeout;
				break;
			}
			else
                /*MxMilliSleep(1);*/msleep(1000);
		}

		// select
		if (nfound == 0)
		{
			FD_ZERO(&readfds);
			FD_SET(i_socket, &readfds);

			// 1ms
			timeout.tv_sec  = 0;
			timeout.tv_usec = 1000;

			nfound = ::select(i_socket + 1, &readfds, (FD_SET*)0, (FD_SET*)0, &timeout);
			if (nfound < 0)
			{
				i_sys_err = ::WSAGetLastError();
				if(bTCP)
				{
#if defined( _MX_MAC_ ) || defined(_MX_LINUX_)
					if(i_sys_err == EINTR) // in case which the function call cancelled by User
					{
						i_sys_err = 0;
						nfound = 0;
					}
#endif
				}
				else
				{
					i_sys_err = 0;
					nfound = 0;
				}
			}

			if(firstFlag && nfound == 0 && (inFlags, mxSocketFlag_RecvTry))
			{
				i_ret = mxErr_SockIdle;
				break;
			}

			if (nfound < 0)
			{
				i_ret = mxErr_SockGeneric;
				break;
			}
		}
		firstFlag = false;
			
		// recv
		if (nfound > 0)
		{
			src_addr_size = sizeof(src_addr);

			if(bTCP)
				recvlen = ::recv(i_socket, (char*)p_buffer, i_size, 0);
			else
				recvlen = ::recvfrom(i_socket, (char*)p_buffer, i_size, 0, (SOCKADDR*)&src_addr, &src_addr_size);

			if(recvlen == 0)
			{
				i_ret = mxErr_SockRecvGracefulClose;
				break;
			}
			else if(recvlen == SOCKET_ERROR)
			{
				i_sys_err = ::WSAGetLastError();

				if (bTCP)
				{
					if (i_sys_err == WSAEWOULDBLOCK || i_sys_err == WSAEINPROGRESS || i_sys_err == WSAENOBUFS)
						continue;
				}
				else
				{
					if (i_sys_err == WSAEWOULDBLOCK)
						continue;
				}

				i_ret = mxErr_SockGeneric;
				break;
			}
			else
			{
				b_addr_valid = true;

				packlen += recvlen;
				p_buffer += recvlen;
				i_size -= recvlen;

				if(bTCP && (inFlags, mxSocketFlag_RecvFull) && i_size > 0)
				{
					nfound = 0;
					continue;
				}

				i_ret = 0;
				break;
			}
		}
	}

	if(i_ret == mxErr_None && b_stopped)
		i_ret = mxErr_SockStopByUser;

	if(outSysErr)
		*outSysErr = i_sys_err;
	if(outSize)
		*outSize = packlen;

	if(!bTCP && b_addr_valid)
	{
		if(outSrcAddr)
		{
			char* host_name = ::inet_ntoa(src_addr.sin_addr);
			::strcpy(outSrcAddr, host_name);
		}
		if(outSrcPort)
		{
			*outSrcPort = src_addr.sin_port;
		}
	}

	return i_ret;
}

int32_t
Receive(
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	int32_t*	outSysErr,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	return _ReceiveData(true, inSocket, inBuffer, inSize, outSize, outSysErr, NULL, NULL, inTimeout, inFlags);
}

int32_t
ReceiveDatagram(
	MxSockRef	inSocket,
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	int32_t*	outSysErr,
	char*		outSrcAddr,
	uint32_t*	outSrcPort,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	return _ReceiveData(false, inSocket, inBuffer, inSize, outSize, outSysErr, outSrcAddr, outSrcPort, inTimeout, inFlags);
}

int32_t
ReceiveBlock(
	MxSockRef	inSocket,
	MxBuffer*	inMem,
	int32_t*	outSysErr,
	mtime_t		inTimeout)
{
	int32_t i_err;
	int32_t msg_len, recv_len;

	i_err = Receive(inSocket, (char*)&msg_len, 4, &recv_len, outSysErr, inTimeout, mxSocketFlag_RecvTry | mxSocketFlag_RecvFull);

	if(i_err || recv_len != 4)
	{
		return (i_err) ? i_err : -1;
	}

	//msg_len = MxSwapInt32LittleToHost(msg_len);
	if(msg_len > 0)
	{
		inMem->Init();
		inMem->SetLimitSize(msg_len + 128);

		i_err = Receive(inSocket, inMem->GetBuffer(), msg_len, &recv_len, outSysErr, inTimeout, mxSocketFlag_RecvFull);

		if(recv_len == msg_len)
		{
			*(inMem->GetBuffer() + msg_len) = 0;
			inMem->SetDataSize(recv_len);
		}
	}

	return i_err;
}

}

