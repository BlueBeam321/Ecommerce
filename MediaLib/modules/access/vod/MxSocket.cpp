#include <inttypes.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include "MxErrCodes.h"
#include "MxSocket.h"
#include "MxArray.h"

#ifdef _MX_MAC_
typedef void*		FSRef;
typedef	uint16_t	UniChar;
#endif


// ------------------------------------------------------

struct _MxSocket
{
	MxSockRef	i_socket;
	bool		i_closeSocket;

    vlc_mutex_t	p_send_mutex;
    vlc_mutex_t	p_recv_mutex;
    vlc_mutex_t	p_close_mutex;

	char		multi_addr[128];

	auth_api_t*	auth_api;
	void*		custom_data;
	int32_t		auth_api_kind;

	int32_t		i_authFlag;
	int32_t		i_proxyFlag;
	int32_t		i_proxyServerAuth;
	int32_t		i_encflag;
	int32_t		i_authUserKind;
	int32_t		i_authmode;
	char		relay_addr[128];
	int32_t		i_relayPort;

	void*		p_authObj;
	int32_t		i_preAccept;
	MxArray<char>*	recv_queue;

	char		tcp_peer_ip[24];
	void*		proxy_info;

	bool		bind_address_is_null;
	char		bind_address[128];
	int32_t		bind_port;
	int32_t		send_buf_size;
	int32_t		recv_buf_size;
	int32_t		socket_flags;
};

// ****************************************************************
//                      MxSocket Class
// ****************************************************************

MxSocket::MxSocket()
{
	//SMSecurity_InitProcs();

    m_pMembers = new _MxSocket;
	memset(m_pMembers, 0, sizeof(_MxSocket));
	m_pMembers->i_socket = mxSocket_Invalid;

    vlc_mutex_init(&m_pMembers->p_send_mutex);
	vlc_mutex_init(&m_pMembers->p_recv_mutex );
	vlc_mutex_init(&m_pMembers->p_close_mutex);

	m_pMembers->i_closeSocket = true;
	m_pMembers->i_preAccept = true;
	m_pMembers->i_authUserKind = eAuthUserKind_Super;

}

MxSocket::~MxSocket()
{
	Close();
	vlc_mutex_destroy(&m_pMembers->p_send_mutex);
	vlc_mutex_destroy(&m_pMembers->p_recv_mutex);
	vlc_mutex_destroy(&m_pMembers->p_close_mutex);

	free(m_pMembers->recv_queue);

	delete (m_pMembers);
}

// --------------------------------------------------------

void MxSocket::Close()
{
	vlc_mutex_lock(&m_pMembers->p_close_mutex);

	if (m_pMembers->i_closeSocket)
		MxSocketUtils::CloseMultiRecvSocket(m_pMembers->i_socket, m_pMembers->multi_addr);

	m_pMembers->i_socket = mxSocket_Invalid;
	strcpy(m_pMembers->multi_addr, "");

	vlc_mutex_unlock(&m_pMembers->p_close_mutex);
}

MxSockRef
MxSocket::GetSockRef()
{
	return m_pMembers->i_socket;
}

_MxSocket*
MxSocket::GetSysRef()
{
	return m_pMembers;
}

void
MxSocket::SetSockRef(
	MxSockRef	inSockRef,
	bool		bClose)
{
	m_pMembers->i_socket = inSockRef;
	m_pMembers->i_closeSocket = bClose;
}

void
MxSocket::SetTcpPeerIP(
	char*		inTcpPeerIP)
{
	strcpy(m_pMembers->tcp_peer_ip, inTcpPeerIP);
}

void
MxSocket::GetTcpPeerIP(
	char*		outTcpPeerIP)
{
	strcpy(outTcpPeerIP, m_pMembers->tcp_peer_ip);
}

void
MxSocket::GetUdpSendIP(
	char*		outAddress)
{

}

const char*
MxSocket::GetLocalUserName()
{
	return NULL;
}

const char*
MxSocket::GetPeerUserName()
{
	return NULL;
}

void
MxSocket::GetTcpPeerTokenPattern(
	char*		outPattern)
{
	strcpy(outPattern, "@");
}

void
MxSocket::GetUdpSendIP(
	char*		outAddr,
	int32_t		inClientID,
	char*		inTcpPeerIP,
	char*		inRelayIP,
	int32_t		inRelayPort)
{
	sprintf(outAddr, "%s@%d@%s@%d", inTcpPeerIP, inClientID, inRelayIP, inRelayPort);
}

void
MxSocket::SetAuthority(
	int32_t		inAuthKind,
	int32_t		inDataEncFlag,
	char*		inRelayAddress,
	int32_t		inRelayPort)
{
	if (inRelayAddress)
		strcpy(m_pMembers->relay_addr, inRelayAddress);
	m_pMembers->i_relayPort = inRelayPort;
}

void
MxSocket::SetUserKind(
	int32_t		inUserKind)
{
	m_pMembers->i_authUserKind = inUserKind;
}

void
MxSocket::SetAuthMode(
	int32_t		inMode)
{
	m_pMembers->i_authmode = inMode;
}

void
MxSocket::SetDataEncFlag(
	int32_t		inFlag)
{
	m_pMembers->i_encflag = inFlag;
}

// --------------------------------------------------------

int32_t
MxSocket::CreateTcpSocket(
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err;

	m_pMembers->bind_address_is_null = (inBindAddress) ? false : true;
	strcpy(m_pMembers->bind_address, inBindAddress);
	m_pMembers->bind_port = inBindPort;
	m_pMembers->send_buf_size = inSendBufSize;
	m_pMembers->recv_buf_size = inRecvBufSize;
	m_pMembers->socket_flags = inSocketFlags;
	m_pMembers->bind_port = inBindPort;

	i_ret = MxSocketUtils::CreateTcpSocket(&m_pMembers->i_socket, &i_sys_err, inBindAddress, inBindPort, inSendBufSize, inRecvBufSize, inSocketFlags);
#ifdef _WIN32
	SetLastError(i_sys_err);
#endif

	return i_ret;
}

int32_t
MxSocket::CreateAndConnectByTcp(
	char*		inServerAddress,
	uint32_t	inServerPort,
	mtime_t		inTimeout,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{

	int32_t i_ret;

	i_ret = CreateTcpSocket(NULL, 0, inSendBufSize, inRecvBufSize, inSocketFlags);
	if (i_ret == mxErr_None)
		i_ret = Connect(inServerAddress, inServerPort, inTimeout);

	return i_ret;
}

int32_t
MxSocket::CreateUdpSocket(
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err;
	
	i_ret = MxSocketUtils::CreateUdpSocket(&m_pMembers->i_socket, &i_sys_err, inBindAddress, inBindPort, inSendBufSize, inRecvBufSize, inSocketFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
	return i_ret;
}

int32_t
MxSocket::CreateMultiSendSocket(
	char*		inBindAddress,
	int32_t		inTTL,
	int32_t		inSendBufSize,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err;
	
	i_ret = MxSocketUtils::CreateMultiSendSocket(&m_pMembers->i_socket, &i_sys_err, inBindAddress, inTTL, inSendBufSize, inSocketFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
	return i_ret;
}

int32_t
MxSocket::CreateMultiRecvSocket(
	char*		inMultiAddress,
	char*		inBindAddress,
	uint32_t	inBindPort,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags)
{
	int32_t i_ret, i_sys_err;
	
	i_ret = MxSocketUtils::CreateMultiRecvSocket(&m_pMembers->i_socket, &i_sys_err, inMultiAddress, inBindAddress, inBindPort, inRecvBufSize, inSocketFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif

	if(i_ret == mxErr_None)
		strcpy(m_pMembers->multi_addr, inMultiAddress);
	
	return i_ret;
}

int32_t
MxSocket::SetOption(
	int32_t		inSocketFlags,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize)
{
	int32_t i_ret, i_sys_err;
	
	i_ret = MxSocketUtils::SetSocketOption(m_pMembers->i_socket, inSocketFlags, inSendBufSize, inRecvBufSize, &i_sys_err);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
	return i_ret;
}

int32_t
MxSocket::Connect(
	char*		inServerAddress,
	uint32_t	inServerPort,
	mtime_t		inTimeout)
{
	int32_t i_ret = 0, i_sys_err = 0;
	
	i_ret = MxSocketUtils::Connect(m_pMembers->i_socket, inServerAddress, inServerPort, &i_sys_err, inTimeout);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif

	if(i_ret)
		Close();
	
	return i_ret;
}

int32_t MxSock_AuthSendDataProc(void* inSocket, void* inDataBuff, int32_t inDataLen)
{
	int32_t i_err = 0;
	MxSocket* pSocket = (MxSocket*)inSocket;

	if (pSocket)
	{
		_MxSocket* pMembers = pSocket->GetSysRef();

		i_err = pSocket->Send((char*)inDataBuff, inDataLen, NULL, INT64_C(30000000));
	}

	return i_err;
}

int32_t MxSock_AuthRecvDataProc(void* inSocket, void* inDataBuff, int32_t inDataLen)
{
	int32_t i_err = 0;
	int32_t i_recv_size = 0;
	MxSocket* pSocket = (MxSocket*)inSocket;

	if (pSocket)
	{
		_MxSocket* pMembers = pSocket->GetSysRef();
		i_err = pSocket->Receive((char*)inDataBuff, inDataLen, &i_recv_size, NULL, INT64_C(30000000));
	}

	return i_err;
}

int32_t
MxSocket::Listen(
	int32_t		inMaxConn)
{
	int32_t i_ret, i_sys_err;
	
	i_ret = MxSocketUtils::Listen(m_pMembers->i_socket, inMaxConn, &i_sys_err);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
	if(i_ret)
		Close();
	
	return i_ret;
}

namespace MxSocketUtils
{
MxSockRef
_AcceptWithLog(
	MxSockRef	inSocket,
	mtime_t		inTimeout,
	char*		outClientAddr,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags);
};

MxSocket*
MxSocket::Accept(
	mtime_t		inTimeout,
	char*		outClientAddr,
	int32_t		inSendBufSize,
	int32_t		inRecvBufSize,
	int32_t		inSocketFlags,
	int32_t		inAuthKind,
	int32_t		inDataEncFlag)
{
	MxSockRef i_sock;
	MxSocket* p_obj = NULL;
	
	i_sock = MxSocketUtils::_AcceptWithLog(m_pMembers->i_socket, inTimeout, outClientAddr, inSendBufSize, inRecvBufSize, inSocketFlags);
	if(i_sock != mxSocket_Invalid)
	{
		p_obj = new MxSocket();
		p_obj->SetSockRef(i_sock, true);
		p_obj->SetTcpPeerIP(outClientAddr);
	}
	
	return p_obj;
}

int32_t
MxSocket::Send(
	char*	inBuffer,
	int32_t		inSize,
	mtime_t		inTimeout,
	int32_t		inSendUnit,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err;

	vlc_mutex_lock(&m_pMembers->p_send_mutex);

	i_ret = 0;
	i_sys_err = 0;

    if (i_ret == 0)
        i_ret = MxSocketUtils::Send(m_pMembers->i_socket, inBuffer, inSize, &i_sys_err, inTimeout, inSendUnit, inFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
    if (i_ret)
        Close();
	vlc_mutex_unlock(&m_pMembers->p_send_mutex);
	return i_ret;
}

int32_t
MxSocket::SendBlock(
	char*	inBuffer,
	int32_t		inSize,
	mtime_t		inTimeout,
	int32_t		inSendUnit,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err = 0;

	vlc_mutex_lock(&m_pMembers->p_send_mutex);

	int32_t i_size = (inSize);

	i_ret = MxSocketUtils::Send(m_pMembers->i_socket, (char*)&i_size, 4, &i_sys_err, inTimeout, inSendUnit, inFlags);
	if (i_ret == 0)
	{
		if (i_ret == 0)
			i_ret = MxSocketUtils::Send(m_pMembers->i_socket, inBuffer, inSize, &i_sys_err, inTimeout, inSendUnit, inFlags);
	}
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif

	if (i_ret)
		Close();
		
	vlc_mutex_unlock(&m_pMembers->p_send_mutex);
	return i_ret;
}

int32_t
MxSocket::SendDatagram(
	char*	inBuffer,
	int32_t		inSize,
	char*		inDestAddr,
	uint32_t	inDestPort,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err;
	char		DestAddr[256];
	int32_t		DestPort = inDestPort;

	strcpy(DestAddr, inDestAddr);
	
	vlc_mutex_lock(&m_pMembers->p_send_mutex);
	i_ret = MxSocketUtils::SendDatagram(m_pMembers->i_socket, inBuffer, inSize, DestAddr, DestPort, &i_sys_err, inTimeout, inFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
    vlc_mutex_unlock(&m_pMembers->p_send_mutex);

	return i_ret;
}

int32_t
MxSocket::SelectRecv(
	mtime_t		inTimeout )
{
	int32_t i_sys_err = 0, i_found = 0;
	i_found = MxSocketUtils::SelectRecv(m_pMembers->i_socket, &i_sys_err, inTimeout);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
	return i_found;
}

int32_t
MxSocket::Receive(
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	int32_t i_ret = 0, i_sys_err;

	vlc_mutex_lock(&m_pMembers->p_recv_mutex);

	if (i_ret == 0)
	{
		if (m_pMembers->recv_queue && m_pMembers->recv_queue->GetCount() > 0)
		{
			int32_t	len = m_pMembers->recv_queue->GetCount();
			if (len > inSize)
				len = inSize;
			memcpy(inBuffer, m_pMembers->recv_queue->GetBuffer(), len);
			m_pMembers->recv_queue->Remove(0, len);

			if (inSize > len)
			{
				i_ret = MxSocketUtils::Receive(m_pMembers->i_socket, inBuffer+len, inSize-len, outSize, &i_sys_err, inTimeout, inFlags);
#ifdef _WIN32
                SetLastError(i_sys_err);
#endif

				if (i_ret == 0)
					*outSize += len;
			}
			else
				*outSize = len;
		}
		else
		{
			i_ret = MxSocketUtils::Receive(m_pMembers->i_socket, inBuffer, inSize, outSize, &i_sys_err, inTimeout, inFlags);
#ifdef _WIN32
            SetLastError(i_sys_err);
#endif
		}
	}

    vlc_mutex_unlock(&m_pMembers->p_recv_mutex);
	if (i_ret && i_ret != mxErr_SockIdle && i_ret != mxErr_SockStopByUser)
		Close();

	return i_ret;
}

int32_t
MxSocket::ReceiveDatagram(
	char*	inBuffer,
	int32_t		inSize,
	int32_t*	outSize,
	char*		outSrcAddr,
	uint32_t*	outSrcPort,
	mtime_t		inTimeout,
	int32_t		inFlags)
{
	int32_t i_ret, i_sys_err;
	vlc_mutex_lock(&m_pMembers->p_recv_mutex);
	
	i_ret = MxSocketUtils::ReceiveDatagram(m_pMembers->i_socket, inBuffer, inSize, outSize, &i_sys_err, outSrcAddr, outSrcPort, inTimeout, inFlags);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif
	
//	if (i_ret && i_ret != mxErr_SockIdle && i_ret != mxErr_SockStopByUser)
//		Close();
	vlc_mutex_unlock(&m_pMembers->p_recv_mutex);
	return i_ret;
}

int32_t
MxSocket::ReceiveBlock(
	MxBuffer*	inMem,
	mtime_t		inTimeout)
{
	int32_t i_ret, i_sys_err;

	vlc_mutex_lock(&m_pMembers->p_recv_mutex);
	
	i_ret = MxSocketUtils::ReceiveBlock(m_pMembers->i_socket, inMem, &i_sys_err, inTimeout);
#ifdef _WIN32
    SetLastError(i_sys_err);
#endif

	if (i_ret && i_ret != mxErr_SockIdle && i_ret != mxErr_SockStopByUser)
		Close();

    vlc_mutex_unlock(&m_pMembers->p_recv_mutex);

	return i_ret;
}

int32_t
MxSocket::ReceiveBlockEx(
	MxBuffer*	inMem,
	mtime_t		inTimeout)
{
	int32_t i_ret = mxErr_Generic;
	mtime_t time = mdate();
	
	bool b_stopped = false;
	while (1)
	{
		i_ret = ReceiveBlock(inMem, inTimeout);
		
		if (i_ret == mxErr_SockIdle && (mdate() - time) < (uint64_t)inTimeout)
			msleep(10000);
		else
		{
			if (i_ret == mxErr_SockIdle)
				i_ret = mxErr_SockTimeout;
			break;
		}
	}
	
	if (i_ret != mxErr_None && b_stopped)
		i_ret = mxErr_SockStopByUser;
	
	return i_ret;
}
