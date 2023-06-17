#ifndef _MX_SOCKET_H_
#define _MX_SOCKET_H_
#include <inttypes.h>

#include "MxSocketUtils.h"

struct auth_socket_t
{
	void*	sock;
	int32_t (*auth_send_proc)(void* sock, void* send_buf, int32_t send_len);
	int32_t (*auth_recv_proc)(void* sock, void* recv_buf, int32_t recv_len);
	char*	self_addr;
	int32_t	sin_port;
	char*	server_addr;
	int32_t	server_port;
	int32_t	user_kind;
	int32_t	relay_auth;
	int32_t	auth_mode;
};

struct auth_api_t 
{
	void	(*cipher_dispose) (void* cipher, void* custom_data);

	int32_t	(*doauth_proxy) (auth_socket_t* sock, void* custom_data);
	int32_t	(*doauth_host) (auth_socket_t* sock, void** out_cipher, void* custom_data);
	int32_t	(*doauth_server) (auth_socket_t* sock, void** out_cipher, void* custom_data);

	int32_t	(*data_encrypt) (void* cipher, void* buf, int32_t* io_size, void* custom_data);
	int32_t	(*data_decrypt) (void* cipher, void* buf, int32_t* io_size, void* custom_data);

	int32_t	proxymsg_header_minsize;
	bool	(*proxymsg_parseheader)(char* buf, int32_t* out_bodylen, void* custom_data);
	bool	(*proxymsg_parsebody)(char* buf, void** out_proxyinfo, void* custom_data);
	void	(*proxyinfo_dispose)(void* proxyinfo, void* custom_data);


	char*	(*getlocalusername) (void* cipher, void* custom_data);
	char*	(*getpeerusername) (void* cipher, void* custom_data);
};

enum {
	eAuthAPI_SM = 0,
	eAuthAPI_91,
};

enum {
	eAuthKind_Ticket = 1,
	eAuthKind_PreAlias,
	eAuthKind_Pre_DontServerAuth,
	eAuthKind_Pre_AppName,
};

enum {
	eAuthUserKind_Default = 0,
	eAuthUserKind_Super,
};

// ****************************************************************
//                      MxSocket Class
// ****************************************************************

struct _MxSocket;

class MxSocket
{
public:
						MxSocket();

	virtual				~MxSocket();

	// --------------------------------------------------------

	MxSockRef			GetSockRef();

	void				SetSockRef(
							MxSockRef	inSockRef,
							bool		bClose = false);

	void				SetAuthority(
							int32_t		inAuthKind,
							int32_t		inDataEncFlag,
							char*		inRelayAddress = NULL,	// if eAuthKind_Pre_AppName, this is NULL
							int32_t		inRelayPort = 0);		// if eAuthKind_Pre_AppName, this is ReceptPort

	void				SetUserKind(
							int32_t		inUserKind);
							
	void				SetAuthMode(
							int32_t		inMode);
							
	void				SetDataEncFlag(
							int32_t		inFlag);

	void				SetTcpPeerIP(
							char*		inTcpPeerIP);

	void				GetTcpPeerIP(
							char*		outTcpPeerIP);

	void				GetUdpSendIP(
							char*		outAddress);
	
	const char*			GetLocalUserName( );
	
	const char*			GetPeerUserName( );
	
	static void			GetTcpPeerTokenPattern(
							char*		outPattern);
	
	static void			GetUdpSendIP(
							char*		outAddr,
							int32_t		inClientID,
							char*		inTcpPeerIP,
							char*		inRelayIP,
							int32_t		inRelayPort);

	// --------------------------------------------------------

	int32_t				CreateTcpSocket(
							char*		inBindAddress = NULL,
							uint32_t	inBindPort = 0,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP);
	
	int32_t				CreateAndConnectByTcp(
							char*		inServerAddress,	// if eAuthKind_Pre_AppName, this is AppName
							uint32_t	inServerPort,		// if eAuthKind_Pre_AppName, this is AppIndex
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP);

	int32_t				CreateUdpSocket(
							char*		inBindAddress,
							uint32_t	inBindPort,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	int32_t				CreateMultiSendSocket(
							char*		inBindAddress,
							int32_t		inTTL = 0,
							int32_t		inSendBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	int32_t				CreateMultiRecvSocket(
							char*		inMultiAddress,
							char*		inBindAddress,
							uint32_t	inBindPort,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultUDP);

	void				Close();

	// --------------------------------------------------------
	
	int32_t				SetOption(
							int32_t		inSocketFlags,
							int32_t		inSendBufSize,
							int32_t		inRecvBufSize);

	int32_t				Connect(
							char*		inServerAddress,
							uint32_t	inServerPort,
							mtime_t		inTimeout = mxSocket_Timeout);

	int32_t				Listen(
							int32_t		inMaxConn);

	MxSocket*			Accept(
							mtime_t		inTimeout,
							char*		outClientAddr,
							int32_t		inSendBufSize = 0,
							int32_t		inRecvBufSize = 0,
							int32_t		inSocketFlags = mxSocketFlag_DefaultTCP,
							int32_t		inAuthKind = 0, 
							int32_t		inDataEncFlag = 0);
	
	// --------------------------------------------------------
	
	int32_t				Send(
							char*	inBuffer,
							int32_t		inSize,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inSendUnit = mxSocket_SendUnitTCP,
							int32_t		inFlags = 0);
	
	int32_t				SendBlock(
							char*	inBuffer,
							int32_t		inSize,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inSendUnit = mxSocket_SendUnitTCP,
							int32_t		inFlags = 0);
	
	int32_t				SendDatagram(
							char*	inBuffer,
							int32_t		inSize,
							char*		inDestAddr,
							uint32_t	inDestPort,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = 0);

	/*int32_t				SendFileData(
							char*		inFilePath,
							char*		inDestName,
							int32_t		inBlockSize,
							mx_proc_t*	inIdleProc = NULL);*/

	// --------------------------------------------------------
	
	int32_t				SelectRecv(
							mtime_t		inTimeout = 1000000);

	int32_t				Receive(
							char*	inBuffer,
							int32_t		inSize,
							int32_t*	outSize,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = mxSocketFlag_RecvFull);

	int32_t				ReceiveDatagram(
							char*	inBuffer,
							int32_t		inSize,
							int32_t*	outSize,
							char*		outSrcAddr = NULL,
							uint32_t*	outSrcPort = NULL,
							mtime_t		inTimeout = mxSocket_Timeout,
							int32_t		inFlags = mxSocketFlag_RecvFull);

	int32_t				ReceiveBlock(
							MxBuffer*	inMem,
							mtime_t		inTimeout = mxSocket_Timeout);
	
	int32_t				ReceiveBlockEx(
							MxBuffer*	inMem,
							mtime_t		inTimeout = mxSocket_Timeout);
	
	/*int32_t				RecvFileData(
							char*		inSavePath,
							char*		outFilePath,
							mx_proc_t*	inIdleProc = NULL);*/

private:
	
public:
	_MxSocket*			GetSysRef();

private:
	_MxSocket*			m_pMembers;
};

#endif

