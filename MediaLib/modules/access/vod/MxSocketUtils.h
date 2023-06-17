#ifndef _MX_SOCKET_UTILS_H_
#define _MX_SOCKET_UTILS_H_
#include <vlc_fixups.h>
#include <vlc_common.h>

#include "MxBuffer.h"


/* socket option flags */
#define	mxSocketFlag_SockReuseAddr	0x00000001
#define	mxSocketFlag_SockNonBlock	0x00000002
#define	mxSocketFlag_SockKeepAlive	0x00000004
#define	mxSocketFlag_SockNoDelay	0x00000008

#define	mxSocketFlag_DefaultTCP		(mxSocketFlag_SockReuseAddr|mxSocketFlag_SockNonBlock|mxSocketFlag_SockKeepAlive)
#define	mxSocketFlag_DefaultUDP		(mxSocketFlag_SockReuseAddr)

/* socket receive flags */
#define	mxSocketFlag_RecvTry		0x00010000	// if there is no incoming data, return mxErr_SockIdle
#define	mxSocketFlag_RecvFull		0x00020000

/* socket constants */
#define	mxSocket_Invalid			(0xFFFFFFFF)
#define	mxSocket_Timeout			INT64_C(5000000)	// 5s

#define	mxSocket_SendUnitTCP		(256 * 1024)		// 256K

// ****************************************************************
//                      MxSocketUtils
// ****************************************************************

#ifdef _MX_ARCH_64_
typedef uint64_t	MxSockRef;
#else
typedef uint32_t	MxSockRef;
#endif

namespace MxSocketUtils
{
	int32_t		InitializeSocket(
							int32_t*	outSysErr = NULL);

	void		GetLocalIPAddress(
							char*		outLocalAddr,
							MxSockRef	inSocket = mxSocket_Invalid);

	int32_t		GetSockSinPort(
							MxSockRef	inSocket);

	uint32_t	ConvertIPAddrToLong(
							char*		inIPAddr);

	void		ConvertLongToIPAddr(
							uint32_t	inNumAddr,
							char*		outIPAddr);

	int32_t		SetKeepAliveTime(
							MxSockRef	inSocket,
							bool		bSetKeepAlive,
							mtime_t		inKeepAliveTime,
							mtime_t		inKeepAliveInterval,
							int32_t*	outSysErr);

	bool		IsMulticastAddress(
							char*		inMulticastAddr);

// --------------------------------------------------------

	int32_t		CreateTcpSocket(
							MxSockRef*	outSocket,
							int32_t*	outSysErr,
							char*		inBindAddress = NULL,
							uint32_t	inBindPort = 0,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP);

	int32_t		CreateUdpSocket(
							MxSockRef*	outSocket,
							int32_t*	outSysErr,
							char*		inBindAddress,
							uint32_t	inBindPort,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	int32_t		CreateMultiSendSocket(
							MxSockRef*	outSocket,
							int32_t*	outSysErr,
							char*		inBindAddress,
							int32_t		inTTL = 0,
							int32_t		inSendBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	int32_t		CreateMultiRecvSocket(
							MxSockRef*	outSocket,
							int32_t*	outSysErr,
							char*		inMultiAddress,
							char*		inBindAddress,
							uint32_t	inBindPort,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	void		CloseSocket(
							MxSockRef	inSocket);

	void		CloseMultiRecvSocket(
							MxSockRef	inSocket,
							char*		inMultiAddress);

// --------------------------------------------------------

	int32_t		SetSocketOption(
							MxSockRef	inSocket,
							int32_t		inSocketFlags,
							int32_t		inSendBufSize,
							int32_t		inRecvBufSize,
							int32_t*	outSysErr);

	int32_t		Connect(
							MxSockRef	inSocket,
							char*		inServerAddress,
							uint32_t	inServerPort,
							int32_t*	outSysErr,
							mtime_t		inTimeout = mxSocket_Timeout);

	int32_t		CreateAndConnectByTcp(
							MxSockRef*	outSocket,
							int32_t*	outSysErr,
							char*		inServerAddress,
							uint32_t	inServerPort,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP);

	int32_t		Listen(
							MxSockRef	inSocket,
							int32_t		inMaxConn,
							int32_t*	outSysErr);

	MxSockRef	Accept(
							MxSockRef	inSocket,
							mtime_t		inTimeout,
							char*		outClientAddr,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP);

// --------------------------------------------------------

	int32_t		Send(
							MxSockRef	inSocket,
							char*	inBuffer,
							int32_t		inSize,
							int32_t*	outSysErr,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inSendUnit = mxSocket_SendUnitTCP,
							int32_t		inFlags = 0);

	int32_t		SendDatagram(
							MxSockRef	inSocket,
							char*	inBuffer,
							int32_t		inSize,
							char*		inDestAddr,
							uint32_t	inDestPort,
							int32_t*	outSysErr,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = 0);

// --------------------------------------------------------

	int32_t		SelectRecv(
							MxSockRef	inSocket,
							int32_t*	outSysErr,
							mtime_t		inTimeout = 1000000);

	int32_t		Receive(
							MxSockRef	inSocket,
							char*	inBuffer,
							int32_t		inSize,
							int32_t*	outSize,
							int32_t*	outSysErr,
							
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = mxSocketFlag_RecvFull);

	int32_t		ReceiveDatagram(
							MxSockRef	inSocket,
							char*	inBuffer,
							int32_t		inSize,
							int32_t*	outSize,
							int32_t*	outSysErr,
							char*		outSrcAddr = NULL,
							uint32_t*	outSrcPort = NULL,
							
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = mxSocketFlag_RecvFull);

	int32_t		ReceiveBlock(
							MxSockRef	inSocket,
							MxBuffer*	inMem,
							int32_t*	outSysErr,
							
							mtime_t		inTimeout = mxSocket_Timeout);

// --------------------------------------------------------
}

#endif

