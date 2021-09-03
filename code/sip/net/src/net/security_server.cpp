/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/security_server.h"

#include "net/callback_server.h"

using namespace std;
using namespace SIPBASE;

namespace SIPNET 
{
	CVariable<uint32>	ClientPingInterval("net", "ClientPingInterval", "Client's ping proc interval ms", 60000, 0, true);

//////////////////////////////////////////////////////////////////////////
CSecClient::CSecClient(uint32 _id, bool _bProcFlag, uint32 _nDefaultCryptPacketMethodID) :
	m_ClientID(_id),
	m_bCryptPacketProcFlag(_bProcFlag),
	m_nDefaultCryptMethodID(0),
	m_nCertificateMethodID(0),
	m_nCertificateDataLen(0),
	m_pCertificateData(NULL),
	m_nEncodedCetiticateDataLen(0),
	m_pEncodedCetiticateData(NULL)
{
	CCryptMethodTypeManager::getInstance().MakeAllCryptMethodObject(m_mapCryptMethod);
	
	TMapCryptMethod::iterator it;
	for (it = m_mapCryptMethod.begin(); it != m_mapCryptMethod.end(); it++)
	{
		ICryptMethod* cm = it->second;
		IKey* pKey = cm->GetCryptKey();
		if (pKey)
			pKey->GenerateKey(&m_ClientID);
	}
	m_nDefaultCryptMethodID = _nDefaultCryptPacketMethodID;

	m_nPingIntervalMS = SIPBASE::Irand(ClientPingInterval.get()/2, ClientPingInterval.get());
	m_tmLastMakePing = 0;
	m_nLastPingData = 0;
}
CSecClient::~CSecClient()
{
	TMapCryptMethod::iterator it;
	for (it = m_mapCryptMethod.begin(); it != m_mapCryptMethod.end(); it++)
	{
		ICryptMethod* cm = it->second;
		delete	cm;
	}
	m_mapCryptMethod.clear();

	delete [] m_pCertificateData;
	delete [] m_pEncodedCetiticateData;
}

void	CSecClient::MakeCertificateData()
{
	// 인증자료만들기
	srand ((uint32)time (NULL));
	m_nCertificateDataLen = SIPBASE::Irand(128, 256);
	m_pCertificateData = new uint8[m_nCertificateDataLen];
	GenerateRandomValue(m_pCertificateData, m_nCertificateDataLen);

	// 인증자료보관
	uint8*	pCopyCertificateData = new uint8[m_nCertificateDataLen];
	memcpy(pCopyCertificateData, m_pCertificateData, m_nCertificateDataLen);

	// 인증에 사용할 암호화방식 결정
	m_nCertificateMethodID = CCryptMethodTypeManager::getInstance().GetRandomCryptMethodIDOnlevel(CL_LOW);
	TMapCryptMethod::iterator it = m_mapCryptMethod.find(m_nCertificateMethodID);
	if	(it == m_mapCryptMethod.end())
		siperrornoex("There is no cryptmethod for Certificate.");

	// 암호화
	ICryptMethod*	cryptMethod = it->second;
	uint8*	pEncodedData = NULL;
	uint32	nEncodedLen;
	bool	bDelAfterUsing;
	cryptMethod->Encode(pCopyCertificateData, m_nCertificateDataLen, &pEncodedData, nEncodedLen, bDelAfterUsing);

	// 암호화된 자료보관
	m_nEncodedCetiticateDataLen = nEncodedLen;
	if (m_nEncodedCetiticateDataLen > 0)
	{
		m_pEncodedCetiticateData = new uint8[m_nEncodedCetiticateDataLen];
		memcpy(m_pEncodedCetiticateData, pEncodedData, m_nEncodedCetiticateDataLen);
	}
	else
		m_pEncodedCetiticateData = NULL;

	delete [] pCopyCertificateData;
	if (bDelAfterUsing)
		delete [] pEncodedData;

}

void	CSecClient::SerialCryptMethodKeys(OUT SIPNET::CMessage& msg)
{
	msg.serial(m_nDefaultCryptMethodID, m_nPingIntervalMS);
	uint32	mSize = m_mapCryptMethod.size();
	msg.serial(mSize);
	TMapCryptMethod::iterator it;
	for (it = m_mapCryptMethod.begin(); it != m_mapCryptMethod.end(); it++)
	{
		uint32	methodID = it->first;
		ICryptMethod* cm = it->second;
		IKey* pKey = cm->GetCryptKey();

		msg.serial(methodID);
		if (pKey)
		{
			pKey->serial(msg);
		}
	}	
}

void	CSecClient::MakeCyclePingMsg(OUT SIPNET::CMessage& msg)
{
	m_tmLastMakePing = CTime::getLocalTime();
	m_nLastPingData = (sint32)::rand();
	
	msg.serial(m_nLastPingData);
}

bool	CSecClient::Certificate(SIPNET::CMessage& msg)
{
	uint32 nClientDataLen;
	msg.serial(nClientDataLen);

	if (m_nEncodedCetiticateDataLen != nClientDataLen)
		return false;

	if (nClientDataLen == 0)
		return true;

	uint8 *pClientData = new uint8[nClientDataLen];
	msg.serialBuffer(pClientData, nClientDataLen);

	bool bEqual = false;
	if (memcmp(pClientData, m_pEncodedCetiticateData, nClientDataLen) == 0)
		bEqual = true;

	delete [] pClientData;
	return bEqual;

}

bool	CSecClient::CheckClientCyclePing(SIPNET::CMessage& msg)
{
	if (m_nPingIntervalMS == 0)
	{
		sipwarning("Failed CheckClientCyclePing for m_nPingIntervalMS = 0");
		return false;
	}

	sint32	certiData;
	msg.serial(certiData);

	if (certiData != m_nLastPingData)
	{
		sipwarning("Failed CheckClientCyclePing for data difference: ServerData=%d, ClientData=%d", m_nLastPingData, certiData);
		return false;
	}

	SIPBASE::TTime tmServer = CTime::getLocalTime();
	uint32 tmDelta = static_cast<uint32>(tmServer - m_tmLastMakePing);

	if (tmDelta < m_nPingIntervalMS && tmDelta < 9000)	// Mobile client send ping-pong in 10s, Modified by RSC 2015/04/24
	{
		if (tmDelta < (m_nPingIntervalMS - m_nPingIntervalMS/10))
		{
			sipwarning("Failed CheckClientCyclePing for received quickly: m_nPingIntervalMS=%d, DeltaTime=%d, ClientID=0x%x", m_nPingIntervalMS, tmDelta, m_ClientID);
			return false;
		}
	}
	return true;
	
}

void	CSecClient::FreeCertificateData()
{
	delete [] m_pCertificateData;
	delete [] m_pEncodedCetiticateData;

	m_pCertificateData = NULL;
	m_pEncodedCetiticateData = NULL;
	m_nCertificateDataLen = m_nEncodedCetiticateDataLen = 0;
}

void	CSecClient::EncodePacket(OUT CMessage& msgOriginal, uint32 cryptMethodID)
{
	if (!m_bCryptPacketProcFlag)
		return;

	TMapCryptMethod::iterator it = m_mapCryptMethod.find(cryptMethodID);
	if	(it == m_mapCryptMethod.end())
		return;

	uint8*	pOriginal = const_cast<uint8*>(msgOriginal.buffer());
	pOriginal += msgOriginal.getHeaderSize();
	uint32	nOriginalLen = msgOriginal.length() - msgOriginal.getHeaderSize();
	if (nOriginalLen == 0)
		return;

	ICryptMethod*	cryptMethod = it->second;
	uint8*	pEncodedData = NULL;
	uint32	nEncodedLen;
	bool	bDelAfterUsing;
	cryptMethod->Encode(pOriginal, nOriginalLen, &pEncodedData, nEncodedLen, bDelAfterUsing);

	if (pOriginal != pEncodedData || nOriginalLen != nEncodedLen)
		siperrornoex("There is mistaked crypt method.");

	msgOriginal.setCryptMode(static_cast<TCRYPTMODE>(cryptMethodID));
}

void	CSecClient::DecodePacket(OUT CMessage& msgOriginal, uint32 cryptMethodID)
{
	if (!m_bCryptPacketProcFlag)
		return;

	TMapCryptMethod::iterator it = m_mapCryptMethod.find(cryptMethodID);
	if	(it == m_mapCryptMethod.end())
		return;

	uint8*	pOriginal = const_cast<uint8*>(msgOriginal.buffer());
	pOriginal += msgOriginal.getHeaderSize();
	uint32	nOriginalLen = msgOriginal.length() - msgOriginal.getHeaderSize();
	if (nOriginalLen == 0)
		return;

	ICryptMethod*	cryptMethod = it->second;
	uint8*	pDecodedData = NULL;
	uint32	nDecodedLen;
	bool	bDelAfterUsing;
	cryptMethod->Decode(pOriginal, nOriginalLen, &pDecodedData, nDecodedLen, bDelAfterUsing);

	if (pOriginal != pDecodedData || nOriginalLen != nDecodedLen)
		siperrornoex("There is mistaked crypt method.");
}

void	CSecClient::DefaultEncodePacket(OUT CMessage& msgOriginal)
{
	if (m_nDefaultCryptMethodID == 0)
		return;

	EncodePacket(msgOriginal, m_nDefaultCryptMethodID);
}

bool	CSecClient::CheckPing(SIPBASE::TTime tmCur)
{
	if (m_tmLastMakePing > 0)
	{
		if (tmCur - m_tmLastMakePing > m_nPingIntervalMS * 10)
		{
			sipwarning("No response ping from client : time=%dMS, from=0x%x", tmCur - m_tmLastMakePing, m_ClientID);
			return true;
		}
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////
CSecurityFunction::CSecurityFunction(bool bAllProc) :
	m_bAllSecurityProcFlag(bAllProc),
	m_bSecurityConnectionFlag(bAllProc),
	m_bCryptPacketProcFlag(bAllProc)
{
	m_nDefaultCryptPacketMode = DCPM_NONE;
	if (IsCryptPacketProcFlag())
		m_nDefaultCryptPacketMode = DCPM_RANDOM;
	
	m_tmLastUpdatePing = CTime::getLocalTime();
}

CSecurityFunction::~CSecurityFunction()
{
	TMapSecClient::iterator it;
	for (it = m_mapSecClient.begin(); it != m_mapSecClient.end(); it++)
	{
		CSecClient* sc = it->second;
		delete	sc;
	}
	m_mapSecClient.clear();
}

void	CSecurityFunction::AddNewClient(TTcpSockId from)
{
	uint32 nDefaultCryptMethodID = 0;
	switch (m_nDefaultCryptPacketMode)
	{
	case DCPM_NONE:			
		nDefaultCryptMethodID = 0;					
		break;
	case DCPM_ALLEQUAL:		
		nDefaultCryptMethodID = DCPM_ALLEQUAL;		
		break;
	case DCPM_RANDOM:
		nDefaultCryptMethodID = CCryptMethodTypeManager::getInstance().GetRandomCryptMethodIDOnlevel(CL_LOW);
		break;
	}

	CSecClient	*pNew = new CSecClient( *((uint32 *)from),
									IsCryptPacketProcFlag(), 
									nDefaultCryptMethodID);

	m_mapSecClient.insert(make_pair(from, pNew));
}

void	CSecurityFunction::DisConnectClient(TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
		return;
	
	CSecClient*	sc = it->second;
	delete sc;
	m_mapSecClient.erase(from);
}

void	CSecurityFunction::MakeNotifyKeyMsg(OUT SIPNET::CMessage& msg, TTcpSockId from)
{
	{
		TMapSecClient::iterator it = m_mapSecClient.find(from);
		if (it == m_mapSecClient.end())
		{
			siperrornoex("There is no client for notifying key.");
		}

		CSecClient*	sc = it->second;
		sc->SerialCryptMethodKeys(msg);
	}

	{
		uint32	sSize = m_mapSpecRecvMsgCryptMethod.size();
		msg.serial(sSize);
		TMapSpecMsgCryptMethod::iterator	it;
		for (it = m_mapSpecRecvMsgCryptMethod.begin(); it != m_mapSpecRecvMsgCryptMethod.end(); it++)
		{
			MsgNameType mName = it->first;
			uint32		cryptMethod = it->second;
			msg.serial(mName, cryptMethod);
		}
	}

}

void	CSecurityFunction::MakeCertificateMsg(OUT SIPNET::CMessage& msg, TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
		siperrornoex("There is no client for certificating.");
	
	CSecClient*	sc = it->second;
	sc->MakeCertificateData();

	msg.serial(sc->m_nCertificateMethodID);
	msg.serial(sc->m_nCertificateDataLen);
	msg.serialBuffer(sc->m_pCertificateData, sc->m_nCertificateDataLen);
}

void	CSecurityFunction::MakeCyclePingMsg(OUT SIPNET::CMessage& msg, TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
	{
		siperrornoex("There is no client for cycle ping.");
	}

	CSecClient*	sc = it->second;
	sc->MakeCyclePingMsg(msg);
}

bool	CSecurityFunction::ClientCertificate(SIPNET::CMessage& msg, TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
		return false;
	
	CSecClient*	sc = it->second;
	
	bool bResult = sc->Certificate(msg);

	sc->FreeCertificateData();

	return bResult;

}

bool	CSecurityFunction::CheckClientCyclePing(SIPNET::CMessage& msg, TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
	{
		sipwarning("There is client %p", from);
		return false;
	}

	CSecClient*	sc = it->second;

	return (sc->CheckClientCyclePing(msg));
}

void	CSecurityFunction::UpdatePing(vector<TTcpSockId>& uncheckBuffer)
{
	TTime tmCur = CTime::getLocalTime();
	if (tmCur - m_tmLastUpdatePing < 60000)
		return;
	
	TMapSecClient::iterator it;
	for (it = m_mapSecClient.begin(); it != m_mapSecClient.end(); it++)
	{
		CSecClient* sc = it->second;
		
		if (sc->CheckPing(tmCur))
			uncheckBuffer.push_back(it->first);
	}
	m_tmLastUpdatePing = tmCur;
}

static void	AddSpecPacketCryptMethod(TMapSpecMsgCryptMethod &mapBuffer, const TSpecMsgCryptRequest *requestarray, uint32 arraysize)
{
	for (uint32 i = 0; i < arraysize; i++)
	{
		MsgNameType					mes = requestarray[i].mName;
		E_SPECCRYPTPACKETREQUEST	requestC = requestarray[i].nRequest;
		uint32						candiCryptMethod = 0;
		switch (requestC)
		{
		case SCPM_NONE:		candiCryptMethod = 0;	break;
		case SCPM_LOW:		
			candiCryptMethod = CCryptMethodTypeManager::getInstance().GetRandomCryptMethodIDOnlevel(CL_LOW);			
			if (candiCryptMethod == 0)
				siperrornoex("There is no low crypt request mode");
			break;
		case SCPM_MEDIUM:	
			candiCryptMethod = CCryptMethodTypeManager::getInstance().GetRandomCryptMethodIDOnlevel(CL_MEDIUM);		
			if (candiCryptMethod == 0)
				siperrornoex("There is no medium crypt request mode");
			break;
		case SCPM_HIGH:		
			candiCryptMethod = CCryptMethodTypeManager::getInstance().GetRandomCryptMethodIDOnlevel(CL_HIGH);		
			if (candiCryptMethod == 0)
				siperrornoex("There is no high crypt request mode");
			break;
		}
		mapBuffer.insert(make_pair(mes, candiCryptMethod));
	}
}

void	CSecurityFunction::AddSendPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize)
{
	AddSpecPacketCryptMethod(m_mapSpecSendMsgCryptMethod, requestarray, arraysize);
}

void	CSecurityFunction::AddRecvPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize)
{
	AddSpecPacketCryptMethod(m_mapSpecRecvMsgCryptMethod, requestarray, arraysize);
}

void	CSecurityFunction::EncodeSendPacket(CMessage &msgOriginal, TTcpSockId toClient)
{
	TMapSecClient::iterator it = m_mapSecClient.find(toClient);
	if (it == m_mapSecClient.end())
		return;
	
	CSecClient*	sc = it->second;
	TMapSpecMsgCryptMethod::iterator sm = m_mapSpecSendMsgCryptMethod.find(msgOriginal.getName());
	if (sm != m_mapSpecSendMsgCryptMethod.end())
	{
		uint32	cm = sm->second;
		if (cm != SCPM_NONE)
			sc->EncodePacket(msgOriginal, cm);
	}
	else
		sc->DefaultEncodePacket(msgOriginal);
}

bool	CSecurityFunction::DecodeRecvPacket(CMessage &msgOriginal, TTcpSockId from)
{
	TMapSecClient::iterator it = m_mapSecClient.find(from);
	if (it == m_mapSecClient.end())
		return true;
	CSecClient*	sc = it->second;

	MsgNameType	mName = msgOriginal.getName();
	uint32	cryptMethod = static_cast<uint32>(msgOriginal.getCryptMode());

	TMapSpecMsgCryptMethod::iterator sm = m_mapSpecRecvMsgCryptMethod.find(mName);
	if (sm != m_mapSpecRecvMsgCryptMethod.end())
	{
		uint32	mustCryptMethod = sm->second;
		if (mustCryptMethod != cryptMethod)
			return false;
	}
	else
	{
		if (sc->m_nDefaultCryptMethodID != cryptMethod)
			return false;
	}

	if (cryptMethod == 0)
		return true;

	sc->DecodePacket(msgOriginal, cryptMethod);
	return true;
}

} // SIPNET
