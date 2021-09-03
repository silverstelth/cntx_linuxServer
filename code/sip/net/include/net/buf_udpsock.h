/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BUF_UDPSOCK_H__
#define __BUF_UDPSOCK_H__

#include "misc/types_sip.h"
#include "misc/hierarchical_timer.h"
#include "misc/mutex.h"

#include "buf_net_base.h"
#include "udp_sim_sock.h"


namespace SIPNET 
{

class CUdpSimSock;
class CServerReceiveTask;
class CBufNetBase;
class CServerBufSock;
// 송신및 수신한 파케트들을 저장하는 buffer
struct  TUDPBUFFER
{
	SIPBASE::CMemStream	buffer;
	SIPBASE::TTime		actiontime;
	uint8				uRepeatNum;
};

// 파케트type별로 송신하려는 자료를 림시 보관하는 queue
struct TUDPSENDQUEUE
{
	SIPBASE::CBufFIFO	buffer;			// buffer
	TUDPINDEX			curIndex;		// 다음동작의 index
};

typedef	std::vector<uint8>		TVU8;
typedef	std::list<TVU8>			TBUFFER;

struct TUDPRECVQUEUE
{
	TBUFFER				buffer;			// buffer
	TUDPINDEX			curIndex;		// 다음동작의 index
};

typedef	std::map<TUDPALLINDEX, TUDPBUFFER>	TLSTUDPBUFFER;
/**
* CBufUdpSock
* A socket and its sending buffer
*/
class CBufUdpSock
{
public:

	/** Constructor
	* \param sock To provide an external socket.
	sock for server is udp server socket
	sock for client is udp client socket
	addr for client must be invalid
	*/
	CBufUdpSock();

	/// Destructor
	virtual ~CBufUdpSock();


protected:


	// Update the network sending (call this method evenly). Returns false if an error occured.
	bool	update();

	void	setTimeFlushTrigger( sint32 ms );

	void	setSizeFlushTrigger( sint32 size ) { _TriggerSize = size; }

	bool	flush();

	virtual std::string typeStr() const { return "CLT "; }
	virtual void send(const uint8 *buffer, uint32 len) = 0;

	bool	pushBuffer( uint8 byType, const SIPBASE::CMemStream& buffer );

	void	AddRecvPacket(uint8 *mes, uint32 uSize);
	void	RefreshCompleteBuffer();
	void	AddPartPacket(std::vector<uint8> &mes, uint32 uSize);
	void	ProcResOfSended(TUDPALLINDEX resIndex);
	void	RefreshRecvAllPacket();
	
	bool						receivePart( uint32 nbExtraBytes );
	void						fillEventTypeOnly() { _ReceiveBuffer[_ReceiveBuffer.size()-1] = (uint8)CBufNetBase::User; }
	const std::vector<uint8>	receivedBuffer() const { return _ReceiveBuffer; }

	void	SendResponsePacket(TUDPALLINDEX allindex, TUDPINDEX pindex, TUDPALLINDEX resindex, CUdpSimSock *sock, CInetAddress addr);

	// Send queue
	TUDPSENDQUEUE								_SendFifo[UDPTYPENUM];
	SIPBASE::CMutex								_SendFifoMutex;

	// Sended buffer
	SIPBASE::CSynchronized<TLSTUDPBUFFER>		_SendedBuffer;		// 재전송Buffer
	uint8				_ReadyToSendBuffer[MAX_UDPDATAGRAMLEN];
	// Resend time
	SIPBASE::TTime		_ResendTime;
	TUDPALLINDEX		_CurAllIndex;

	// Received buffer
	SIPBASE::CSynchronized<TLSTUDPBUFFER>		_RecvedAllPacket;
	TUDPRECVQUEUE								_RecvedPacket[UDPTYPENUM];
	SIPBASE::CMutex								_RecvedQueueMutex;
	
	SIPBASE::CBufFIFO							_RecvComplete;
	std::vector<uint8>							_ReceiveBuffer;
	TBlockSize									_Length;


	// Receiv queue

	// Socket( for client )
//	CUdpSimSock			*Sock;

	// InetAddress( for server )
//	CInetAddress		AddressClient;

private:

	SIPBASE::TTime		_LastFlushTime; // updated only if time trigger is enabled (TriggerTime!=-1)
	SIPBASE::TTime		_TriggerTime;
	sint32				_TriggerSize;
};

// UDP통신에서 클라이언트 담당 클라스
class CBufUdpClient : public CBufUdpSock
{
public:	
	CBufUdpClient();
	/// Destructor
	virtual ~CBufUdpClient();
	
	/// get the UDP sock object
	CUdpSimSock			*getUdpSock() const { return Sock;}
	void	connect( const CInetAddress& addr ) { Sock->connect(addr); }
	void	setNonBlocking() { Sock->setNonBlocking(); }
protected:
	friend class CBufClient;
	friend class CClientReceiveTask;
	friend class CClientReceiveTaskUdp;
	uint32		idInServer;

	CUdpSimSock			*Sock;
	void send(const uint8 *buffer, uint32 len);

};

// UDP통신에서 써버 담당클라스
class CBufUdpServer : public CBufUdpSock
{
public:	
	CBufUdpServer( CUdpSimSock *server, CInetAddress addr):
	  ServerSock(server), ClientAddress(addr), TcpConfirm(false) {}
	/// Destructor
	virtual ~CBufUdpServer() { ServerSock = NULL; };
	bool	IsTcpConfirm() 
	{ 
		return TcpConfirm;
	}
	void			setClientAddress( CInetAddress addr ) { ClientAddress = addr; }
	CInetAddress&	getClientAddress()	{ return ClientAddress; }
	CUdpSimSock*	getUdpSock()		{ return ServerSock; }

protected:
	friend class CBufServer;
	friend class CListenTask;
	friend class CUdpReceiveTask;

	CUdpSimSock			*ServerSock;
	CInetAddress		ClientAddress;
	CServerBufSock		*TcpFriend;
	bool				TcpConfirm;
	void						fillSockIdAndEventType( const TTcpSockId sockId )
	{
		uint32 upos = _ReceiveBuffer.size() - 1 - sizeof(TTcpSockId);
		memcpy( (&*_ReceiveBuffer.begin()) + upos, &sockId, sizeof(TTcpSockId) );
		fillEventTypeOnly();
	}
	void send(const uint8 *buffer, uint32 len);

};

} // SIPNET

#endif // __BUF_UDPSOCK_H__