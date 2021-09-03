/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPHOLEPUNCHINGCLIENT_H__
#define __UDPHOLEPUNCHINGCLIENT_H__

#include <misc/types_sip.h>
#include "net/callback_client.h"
#include "net/callback_udp.h"
#include "net/syspacket.h"
namespace	SIPNET	
{
	// HolePunching의 결과에 대한 상수값
	enum	EHolePunchingResult	{ HP_FALIED, HP_SUCCESS };

	// HolePunching결과를 알려줄 callback함수의 prototype
	typedef void (*THolePunchingNotifyCallback)(EHolePunchingResult _HolePunchingResult, 
												CCallbackUdp*		_pUdpSock, 
												CInetAddress		_ConnectedAddr, 
												CInetAddress		_RelayAddr, 
												void*				_pArg);

	// HolePunching상대가 탈퇴하였을때 통보를 위한 callback함수의 prototype
	typedef void (*TTargetExitCallback)(void* _pArg);
	
	// HolePunching client
	class CUdpHolePunchingClient : public ICallbackNetPlugin
	{
	private:
		// 클라스의 상태값
		enum EUDPHPCState{ UDPHPC_NONE=0, UDPHPC_WAIT_CLIENTREGISTER, UDPHPC_WAIT_UDPPORTEXCHANGE, UDPHPC_WAIT_HOLEPUNCHING, UDPHPC_WAIT_HOLEPUNCHED};
	public:	
		// constructor
		CUdpHolePunchingClient();
		// destructor
		virtual ~CUdpHolePunchingClient();

		// HolePunching시작, 반드시 써버에 이미 련결되여 있는 CCallbackClient를 파라메터로 주어야 한다.
		void	StartHolePunching(CCallbackClient* _pBaseConnection, uint32 _OwnGUID, uint32 _TargetHPClientGUID, THolePunchingNotifyCallback _HolePunchingNotifyCallback, void* _pHolepunchingNotifyCbArg);
		// HolePunching정지
		void	StopHolePunching();
		// HolePunching이 완료된후 상대가 탈퇴하였을때를 위한 callback을 등록한다.
		void	SetTargetExitCallback(TTargetExitCallback _CB, void* _pArg)
		{
			m_TargetExitCallback		= _CB;
			m_pTargetExitCallbackArg	= _pArg;
		}

	protected:
		// Update함수
		virtual	void	Update();
		// Base connection이 끊어졌을때의 callback
		virtual	void	DisconnectionCallback(TTcpSockId _From);
		// Base client가 파괴되였을때의 callback
		virtual	void	DestroyCallback();
	private:
		// Base connection의 써버주소 얻기
		CInetAddress GetHPServerNetAddr() const
		{
			if (m_pBaseConnection)
			{
				return (m_pBaseConnection->remoteAddress());
			}
			return (CInetAddress());
		}
		// Base connection의 private주소 얻기
		CInetAddress GetMyBasePrivateNetAddr() const
		{
			if (m_pBaseConnection)
			{
				return (m_pBaseConnection->id()->getTcpSock()->localAddr());
			}
			return (CInetAddress());
		}
		// HolePunching을 위한 UDP써버주소 얻기
		CInetAddress GetServerUDPNetAddr() const
		{
			CInetAddress	addrServer = GetHPServerNetAddr();
			addrServer.setPort(m_ServerUDPPort);
			return (addrServer);
		}
		// HolePunching을 위한 자기의 UDP private주소 얻기
		CInetAddress GetMyUDPPrivateNetAddr() const
		{
			return (m_pUDPSock->localAddr());
		}
		// UDP파케트전송
		void	SendToUDP(CMessage& _rOutMsg, CInetAddress& _rDestAddr)
		{
			try
			{
				m_pUDPSock->sendTo(_rOutMsg.buffer(), _rOutMsg.length(), _rDestAddr);
			}
			catch (...)
			{
			}
		}
		// 오브젝트의 상태설정
		void	SetState(EUDPHPCState _State) 
		{ 
			m_nCurrentState = _State; 
			m_tmState		= SIPBASE::CTime::getLocalTime(); 
		}
		
		void	CheckCompleteHolePunching();
		void	ProcFailed();
		void	ResendUdpPortExchange();
		void	ResendHolePunchingSYN();

		friend	void	_cbTcpMessage(CMessage& _Msg, TSockId _From, CCallbackNetBase& _rClientCB);
		void	MsgCB_M_UHP_SC_CLIENTREGISTER(CMessage& _Msg);
		void	MsgCB_M_UHP_SC_UDPPORTEXCHANGE(CMessage& _Msg);
		void	MsgCB_M_UHP_SC_CONNECTPRIVATE(CMessage& _Msg);
		void	MsgCB_M_UHP_SC_CONNECTPUBLIC(CMessage& _Msg);
		void	MsgCB_M_UHP_SC_TARGETEXIT(CMessage& _Msg);

		friend	void	_cbUDPMessage(CMessage& _Msg, CCallbackUdp& _rLocalUdp, CInetAddress& _rRemoteaddr, void* _pArg);
		void	MsgCB_M_UHP_CS_CONNECTPRIVATE(CMessage& _Msg, CInetAddress& _rRemoteaddr);
		void	MsgCB_M_UHP_CS_CONNECTPUBLIC(CMessage& _Msg, CInetAddress& _rRemoteaddr);
		void	NotifyHolePunchedResult(EHolePunchingResult _Result);
		void	NotifyHolePunchingEndToServer();

		// HP과정에 사용할 Base connection
		CCallbackClient*							m_pBaseConnection;
		// HP과정에 사용할 Udp 소켓
		CCallbackUdp*								m_pUDPSock;
		uint16										m_ServerUDPPort;

		CInetAddress								m_RelayAddress;
		// HP결과를 통지하기 위한 callback함수와 파라메터
		THolePunchingNotifyCallback					m_HolePuncingNotifyCallback;
		void*										m_pHolePuncingNotifyCbArg;
		
		// HolePunching성공후 상대방이 exit하였을때의 처리를 위한 callback
		TTargetExitCallback							m_TargetExitCallback;
		void*										m_pTargetExitCallbackArg;
		
		// 상태변수들
		EUDPHPCState								m_nCurrentState;
		SIPBASE::TTime								m_tmState;
		SIPBASE::TTime								m_tmLastUdpSend;
		bool										m_bSuccessToTarget;	
		bool										m_bSuccessToMe;	

		// 이 id는 써버로부터 받는다. 등록할때 클라이언트에서 자기의 id를 써버에 보내지만
		// 써버는 이id를 다시 검증하여 클라이언트에 확정적으로 통지하게 된다.
		uint32										m_MyHPClientGUID;
		CInetAddress								m_MyRegisteredPublicNetAddr;

		uint32										m_TargetHPClientGUID;
		CInetAddress								m_TargetPrivateNetAddr;
		CInetAddress								m_TargetRegisteredPublicNetAddr;
		CInetAddress								m_TargetHolePublicNetAddr;

		CInetAddress								m_TargetHolePunchedNetAddr;

	};

} // namespace	SIPNET
#endif // end of guard __UDPHOLEPUNCHINGCLIENT_H__
