/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NATTYPEDETECTIONCLIENT_H__
#define __NATTYPEDETECTIONCLIENT_H__

#include <misc/types_sip.h>
#include "net/callback_client.h"
#include "net/callback_udp.h"
#include "net/syspacket.h"
namespace	SIPNET	
{
//////////////////////////////////////////////////////////////////////////
class CNatType
{
public:
	// Nat 종류 enum
	enum { NAT_DONTKNOW=0, NAT_DONTHAVE, NAT_ADDRESSCONE, NAT_PORTCONE, NAT_SYMMETRIC };
	// Constructor
	CNatType() { m_nNatType = NAT_DONTKNOW; }
	CNatType(uint32 nType) { m_nNatType = nType; }

	bool	IsValid()	const { return (m_nNatType == NAT_DONTKNOW) ? false : true; }

	// Nat종류값
	uint32		m_nNatType;
};

//////////////////////////////////////////////////////////////////////////
// NAT검출이 완료된후 결과를 통지하기 위한 callback함수 type
typedef void (*TDetectionNotifyCallback) ( const CNatType &natType, const CInetAddress &serverAddress, void *arg );

class CNatTypeDetectionClient : public ICallbackNetPlugin
{
	enum { NTDC_NONE=0, NTDC_WAIT_REQUESTNATHAVE, NTDC_WAIT_NATTYPE, NTDC_WAIT_CONETYPE};
public:	
	// constructor
	CNatTypeDetectionClient();
	// destructor
	virtual ~CNatTypeDetectionClient();
	
	// Nat검출시작
	// 반드시 검출용써버에 이미 련결되여 있는 CCallbackClient를 파라메터로 주어야 한다.
	void	StartNatDetection(CCallbackClient *pBaseConnection, TDetectionNotifyCallback detectionNotifyCallback, void *detectionNotifyCbArg);
	void	StopNatDetection();
protected:
	virtual	void	Update();
	virtual	void	DisconnectionCallback(TTcpSockId from);
	virtual	void	DestroyCallback();
private:
	// 모든 Nat검출결과를 이 map에 보관한다.
	// 이 map의 key는 검출과정에 사용한 검출용써버의 IP주소이다.
	static	std::map<std::string, CNatType>		m_mapDetectedType;
	static	const CNatType*		GetAlreadyDetectedNatType(std::string strDetectionServerAddress);
	static	void				RegisterDetectedNatType(std::string strDetectionServerAddress, const CNatType& natType);

	// 검출과정에 사용할 Base connection
	CCallbackClient								*m_pBaseConnection;
	// 검출과정에 사용할 Udp 소켓
	CCallbackUdp								*m_UDPSock;
	// 검출결과를 통지하기 위한 callback함수와 파라메터
	TDetectionNotifyCallback					m_DetectionNotifyCallback;
	void										*m_DetectionNotifyCbArg;
	// 검출결과
	CNatType									m_NatTypeForBaseConnection;

	// NatClient의 GUID(써버로부터 받는다)
	uint32										m_NatClientGUID;
	uint16										m_ServerUDPPort1;
	uint16										m_ServerUDPPort2;
	uint16										m_ServerPortconeUDPPort;
	// 검출과정상태값
	int											m_nCurrentState;
	SIPBASE::TTime								m_tmState;
	SIPBASE::TTime								m_tmLastUdpSend;
		

	// 검출결과를 통보한다
	void		NotifyDectectionResult(const CNatType &natType);

	// 상태설정
	void	SetState(int nState) { m_nCurrentState = nState; m_tmState = SIPBASE::CTime::getLocalTime(); }

	// Detection써버의 net주소를 얻는다
	CInetAddress GetDetectionServerNetAddr() const;
	// 써버의 UPD1 net주소를 얻는다
	CInetAddress GetServerUDP1NetAddr() const;
	// 써버의 UPD2 net주소를 얻는다
	CInetAddress GetServerUDP2NetAddr() const;
	// 써버의 PortConeAddr UPD net주소를 얻는다
	CInetAddress GetServerPortConeUDPNetAddr() const;
	// Base connection의 net주소를 얻는다
	CInetAddress GetMyBasePrivateNetAddr() const;
	// UDP client의 private주소를 얻는다
	CInetAddress GetMyUDPPrivateNetAddr() const;

	// send
	void	SendToUDP(CMessage& msgout, CInetAddress& destAddr);
	void	ResendReqNatType();
	void	ResendReqConeType();

	// callback
	friend	void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
	friend	void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg);
	void	cbM_NAT_SC_REQUESTNATHAVE(CMessage& msgin);
	void	cbM_NAT_SC_REQNATTYPE(CMessage& msgin);
	void	cbM_NAT_SC_REQCONETYPE(CMessage& msgin, CInetAddress& remoteaddr);

};

} // namespace	SIPNET
#endif // end of guard __NATTYPEDETECTIONCLIENT_H__

