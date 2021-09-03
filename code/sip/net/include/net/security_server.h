/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SECURITY_SERVER_H__
#define __SECURITY_SERVER_H__
#include "misc/types_sip.h"

#include "callback_net_base.h"
#include "buf_server.h"
#include "misc/CryptSystemManager.h"

namespace SIPNET 
{

// 특별한 송수신파케트들의 암호화를 요구하는 방식
enum E_SPECCRYPTPACKETREQUEST
{
	SCPM_NONE=0,
	SCPM_LOW=1000,
	SCPM_MEDIUM=2000,
	SCPM_HIGH=3000,
};

// 파케트 default암호화를 결정하는 방식값
enum E_DEFAULTCRYPTPACKETMODE
{
	DCPM_NONE,					// default암호화를 진행하지 않는다.
	DCPM_ALLEQUAL=1,			// 1부터 500사이값을 가진다.	해당수값의 id에 해당한 암호화방식으로 암호화를 진행한다.
	DCPM_RANDOM=1000,			// 클라이언트별로 random으로 임의의 low방식암호화를 선택하여 진행한다
};

struct	CSecClient
{
	CSecClient(uint32 _id, bool _bProcFlag, uint32 _nDefaultCryptPacketMethodID);
	virtual ~CSecClient();
	void						MakeCertificateData();
	bool						Certificate(SIPNET::CMessage& msg);
	bool						CheckClientCyclePing(SIPNET::CMessage& msg);
	void						FreeCertificateData();

	void						SerialCryptMethodKeys(OUT SIPNET::CMessage& msg);
	void						MakeCyclePingMsg(OUT SIPNET::CMessage& msg);
	
	void						EncodePacket(OUT CMessage& msgOriginal, uint32 cryptMethodID);
	void						DefaultEncodePacket(OUT CMessage& msgOriginal);
	void						DecodePacket(OUT CMessage& msgOriginal, uint32 cryptMethodID);
	bool						CheckPing(SIPBASE::TTime tmCur);
	uint32						m_ClientID;
	bool						m_bCryptPacketProcFlag;		// 파케트암호화처리에 대한 기발
	uint32						m_nDefaultCryptMethodID;	// default파케트암호화방식
	SIPBASE::TMapCryptMethod	m_mapCryptMethod;			// 암호화오브젝트
// Certificate	
	uint32						m_nCertificateDataLen;
	uint32						m_nCertificateMethodID;
	uint8*						m_pCertificateData;
	uint32						m_nEncodedCetiticateDataLen;
	uint8*						m_pEncodedCetiticateData;
// Ping
	uint32						m_nPingIntervalMS;
	SIPBASE::TTime				m_tmLastMakePing;
	sint32						m_nLastPingData;
};
typedef		std::map<TTcpSockId, CSecClient*>		TMapSecClient;

struct TSpecMsgCryptRequest 
{
	MsgNameType				mName;
	E_SPECCRYPTPACKETREQUEST	nRequest;
};

class	CCallbackServer;
class	CSecurityFunction
{
public:
	CSecurityFunction(bool bAllProc);
	virtual ~CSecurityFunction();
	void		AddNewClient(TTcpSockId from);
	void		DisConnectClient(TTcpSockId from);
	void		MakeCertificateMsg(OUT SIPNET::CMessage& msg, TTcpSockId from);
	void		MakeNotifyKeyMsg(OUT SIPNET::CMessage& msg, TTcpSockId from);
	void		MakeCyclePingMsg(OUT SIPNET::CMessage& msg, TTcpSockId from);
	bool		ClientCertificate(SIPNET::CMessage& msg, TTcpSockId from);
	bool		CheckClientCyclePing(SIPNET::CMessage& msg, TTcpSockId from);
	void		UpdatePing(std::vector<SIPNET::TTcpSockId>& uncheckBuffer);
	void		AddSendPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize);
	void		AddRecvPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize);

	void		EncodeSendPacket(CMessage &msgOriginal, TTcpSockId toClient);
	bool		DecodeRecvPacket(CMessage &msgOriginal, TTcpSockId from);

	inline bool	IsSecurityProcFlag() { return m_bAllSecurityProcFlag; };
	inline bool	IsSecurityConnectionProc() {return	(m_bAllSecurityProcFlag && m_bSecurityConnectionFlag);};
	inline bool	IsCryptPacketProcFlag() { return (m_bAllSecurityProcFlag && m_bCryptPacketProcFlag); };
	inline void	SetSecurityConnectionFlag(bool bF) { m_bAllSecurityProcFlag = bF; }
	inline void	SetPingFlag(bool bF) { m_bAllSecurityProcFlag = bF; }
	inline void	SetCryptPacketProcFlag(bool bF) { m_bAllSecurityProcFlag = bF; }
	inline void	SetDefaultCryptPacketMode(bool bF) { m_bAllSecurityProcFlag = bF; }
protected:
	bool							m_bAllSecurityProcFlag;			// 모든 보안관련처리에 대한 기발
	bool							m_bSecurityConnectionFlag;		// 보안접속처리에 대한 기발
	bool							m_bCryptPacketProcFlag;			// 파케트암호화처리에 대한 기발
	E_DEFAULTCRYPTPACKETMODE		m_nDefaultCryptPacketMode;		// default파케트암호화를 결정하는 방식

	TMapSecClient					m_mapSecClient;
	TMapSpecMsgCryptMethod			m_mapSpecSendMsgCryptMethod;
	TMapSpecMsgCryptMethod			m_mapSpecRecvMsgCryptMethod;

	SIPBASE::TTime					m_tmLastUpdatePing;
};
} // SIPNET

#endif // end of guard __SECURITY_SERVER_H__
