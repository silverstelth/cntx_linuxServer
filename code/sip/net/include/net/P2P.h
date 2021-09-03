/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __P2P_H__
#define __P2P_H__

#include "net/PeerBase.h"
#include "net/UdpHolePunchingClient.h"

namespace SIPP2P	{

	class CP2P : public SIPNET::ICallbackNetPlugin
	{
		enum {P2P_NONE, P2P_HOLEPUNCHING, P2P_HOLEPUNCHED, P2P_WAIT_REGRELAY, P2P_RELAYED};
	public:
		CP2P();
		~CP2P();

		/*
		외부에서 리용할 인터페스
		*/
	public:

		/*
		초기화함수
		Broker써비스와의 통신을 담당하는 클라이언트오브젝트를 생성하고 접속성공한다음 파라메터로 준다
		써버와의 접속이 끊어지지 않는 한에는 프로그람종료할때까지 다시 호출하지 않는다
		*/
		bool	init(SIPNET::CCallbackClient* broker_client);

		void	registerPeer();
		void	unregisterPeer();

		/*
		파라메터로 주는 피어와의 P2P접속을 시도한다
		Broker써비스에 접속신청을 보내며 Broker써버스가 쌍방에 접속시도를 할데 대한 지령을 보내면
		NAT통과하여 접속을 이루기 위한 Hole-punching수법으로 접속시도를 하게 된다
		*/
		bool	connectPeer(uint32 ownid, uint32 peerid, std::string relayaddr);

		/*
		상대피어와 접속을 이루고 있었으면 접속을 종료한다
		*/
		void	disconnectPeer();

		/*
		상대피어와 접속이 이루어져 있으면 return true
		*/
		bool	isConnectedPeer();

		/*
		Relay서비스를 통한 통신부 가동
		*/
		bool	startRelayPeer();

		/*
		Relay서비스를 통한 통신부 중지
		*/
		void	stopRelayPeer();

		/*
		상대피어에게 파케트를 전송한다
		len를 따로 지정하지 않으면 파케트가 문자렬이라고 보고 문자렬길이보다 1바이트 더많게 보낸다
		마지막NULL까지 전송하기 위해서이다
		*/
		bool	sendPeer(char* msg, int len = 0);

		bool	sendBroker(SIPNET::CMessage& msgout);

		bool	sendRelay(char* msg, int len = 0);

		/*
		중계(Broker)써비스에 등록된 자기아이디를 얻는다
		*/
		uint32	getPeerId()	{ return m_peerid; }

		/*
		접속시도를 하거나 이미 접속이 이루어진 상대피어의 아이디를 돌려준다
		*/
		uint32	getPartnerId()	{ return m_partner_id; }

		/*
		상대피어로부터 파케트를 받으면 호출할 콜백을 등록한다
		*/
		void	setRecvCallback(TPeerRecvCallback cb, void* param = NULL);

		/*
		상대피어와 접속이 성공하면 호출할 콜백을 등록한다
		*/
		void	setConnectionCallback(TPeerConnectCallback cb);

		/*
		상대피어와의 접속이 끊어지면 호출할 콜백을 등록한다
		*/
		void	setDisconnectionCallback(TPeerDisconnectCallback cb);
	protected:
		/*
		P2P통신오브젝트를 해제한다
		대체로 프로그람완료나 써버와의 접속이 끊어졌을때 호출한다
		*/
		void	release();

		virtual void	Update();
		virtual void	DisconnectionCallback(SIPNET::TTcpSockId from);
		virtual void	DestroyCallback();

		void	SetState(int nState) { m_nState = nState; m_tmState = SIPBASE::CTime::getLocalTime(); }

	private:
		int							m_nState;
		SIPBASE::TTime				m_tmState;
		SIPBASE::TTime				m_tmLastUdpSend;
	
		SIPNET::CCallbackClient*	m_BaseClient;
		SIPNET::CUdpHolePunchingClient*		m_HolePunchingClient;

		SIPBASE::CSynchronized<SIPNET::CCallbackUdp*>		m_ConnectedUDPSock;
		SIPNET::CInetAddress		m_ConnectedAddr;

		SIPNET::CCallbackUdp		*m_UdpRelay;
		SIPNET::CInetAddress		m_RelayAddr;
		uint32						m_peerid;		//own peer id
		uint32						m_partner_id;	//partner peer id

		TPeerConnectCallback		m_cbPeerConnectCallback;
		TPeerDisconnectCallback		m_cbPeerDisconnectCallback;
		
		TPeerRecvCallback			m_cbRecv;
		void*						m_cbRecvParam;
		friend void HolePunchingEnd(SIPNET::EHolePunchingResult _HolePunchingResult, SIPNET::CCallbackUdp* _pUdpSock, SIPNET::CInetAddress _ConnectedAddr, SIPNET::CInetAddress _RelayAddr, void* _pArg);
		void	EndHolePunching(SIPNET::EHolePunchingResult _HolePunchingResult, SIPNET::CCallbackUdp* _pUdpSock, SIPNET::CInetAddress _ConnectedAddr, SIPNET::CInetAddress _RelayAddr);
		friend	void	RecvUDPPacket(char *pData, int nLength, SIPNET::CInetAddress &remote, void* param);
		void	RecvUDPPacket(char *pData, int nLength);
		friend	void	TargetExit(void* param);
		void	TargetExit();
		void	ResendRegisterRelay();

		friend	void	cbMessage(SIPNET::CMessage& msgin, SIPNET::TSockId from, SIPNET::CCallbackNetBase& clientcb);
		void	cbM_P2P_SC_RESRELAYADDR(SIPNET::CMessage& msgin);
		friend	void	cbUDPMessage(SIPNET::CMessage& msgin, SIPNET::CCallbackUdp& localudp, SIPNET::CInetAddress& remoteaddr, void* pArg);
		void	cbM_P2P_SC_REGISTERTORELAY(SIPNET::CMessage& msgin, SIPNET::CInetAddress& remoteaddr);
	};
}

#endif // end of guard __P2P_H__
