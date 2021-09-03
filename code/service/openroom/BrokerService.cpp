
#pragma warning (disable: 4267)
#pragma warning (disable: 4244)

#include "misc/types_sip.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <direct.h>
#	undef max
#	undef min
#else
#	include <unistd.h>
#	include <errno.h>
#endif

#include <string>
#include <list>

#include "misc/debug.h"
#include "misc/system_info.h"
#include "misc/config_file.h"
#include "misc/thread.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/common.h"
#include "misc/path.h"
#include "misc/value_smoother.h"
#include "misc/singleton.h"
#include "misc/file.h"
#include "misc/algo.h"
#include "misc/db_interface.h"

#include "net/service.h"
#include "net/unified_network.h"
#include "net/varpath.h"
#include "net/email.h"
#include "net/admin.h"

#include "net/UdpPeerBroker.h"

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../Common/Common.h"

#include "Family.h"
#include "outRoomKind.h"
#include "contact.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;
using namespace SIPP2P;

CUdpPeerBroker*		m_UdpPeerBroker = NULL;
bool				m_bTcp;

const int	PEER_BROKER_PORT	= 9000;
const int	PEER_RELAY_PORT		= 9600;

void delFromSession(uint32 idSession, uint32 idPeer);

struct CPeer
{
	TServiceId		tsid;
	uint32			partner;
	CInetAddress	pubaddr;
	CInetAddress	priaddr;
	uint32			idSession;
	string			hostname;

	CPeer&	operator=(CPeer& other);
	CPeer(TServiceId	_tsid, const string& _hostname);
};

CPeer::CPeer(TServiceId	_tsid, const string& _hostname)
{
	tsid = _tsid;
	hostname = _hostname;
	partner = 0;
	idSession = 0;
}

CPeer&	CPeer::operator=(CPeer& other)
{
	tsid = other.tsid;
	pubaddr = other.pubaddr;
	priaddr = other.priaddr;
	return *this;
}

typedef		std::map<uint32, CPeer*>		TPeers;
TPeers		Peers;

TPeers::iterator	findPeer(uint32 peerid)
{
	return Peers.find(peerid);
}

static void UnRegisterPeer(uint32 peerid)
{
	TPeers::iterator	itsrc = findPeer(peerid);
	if ( itsrc == Peers.end() )	return;

	CPeer*	peer = itsrc->second;

	sipinfo ("[%d] Peer is Disconnected. pub=%s, pri=%s", peerid, peer->pubaddr.asIPString().c_str(), peer->priaddr.asIPString().c_str());

	delFromSession(peer->idSession, peerid);

	delete peer;
	Peers.erase(itsrc);

}
static void cbUnRegisterPeer(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);
	UnRegisterPeer(peerid);
}

static void cbRegisterPeer(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);
	string	hostname;	_msgin.serial(hostname);

	CPeer*	peer = new CPeer(_tsid, hostname);
	Peers.insert( make_pair(peerid, peer) );
	
	CMessage	msgout(M_REGISTER_PEER);
	msgout.serial(peerid);
	msgout.serial(m_bTcp);
	msgout.serial(peerid);

	//UDP mode
	if ( ! m_bTcp )
	{
		//P2P broker information
		//CInetAddress	addr = CInetAddress::localHost();
		//addr.setPort(PEER_BROKER_PORT);
		//string	udpbroker = addr.asIPString();
		//msgout.serial(udpbroker);

		//RTP relay information
		string relayhost = IService::getInstance()->ConfigFile.exists("RelayHost") ? IService::getInstance()->ConfigFile.getVar("RelayHost").asString(): "localhost:9600";
		CInetAddress	relayaddr(relayhost);
		string	udprelay = relayaddr.asIPString();
		msgout.serial(udprelay);
	}
	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);

	sipinfo ("Peer registerd: id=%d, host=%s", peerid, hostname.c_str());
}

static	void	confirmConnectPeer(uint32 src_peer, uint32 dst_peer)
{
	TPeers::iterator	itsrc = findPeer(src_peer);
	TPeers::iterator	itdst = findPeer(dst_peer);
	if ( itsrc == Peers.end() || itdst == Peers.end() )
	{
		sipwarning ("Invalid: Request peer connect to %d from %d", dst_peer, src_peer);
		return;
	}

	uint32	peerid;
	string	pubaddr, priaddr;

	bool	bSameLocal = false;
	bool	bSamePublic = itdst->second->pubaddr.ipAddress() == itsrc->second->pubaddr.ipAddress();
	if (bSamePublic)
	{
		/*
		const sockaddr_in*	src = itsrc->second->pubaddr.sockAddr();
		const sockaddr_in*	dst = itdst->second->pubaddr.sockAddr();

		u_char	sb1 = src->sin_addr.S_un.S_un_b.s_b1;
		u_char	sb2 = src->sin_addr.S_un.S_un_b.s_b2;
		u_char	sb3 = src->sin_addr.S_un.S_un_b.s_b3;
		u_char	db1 = dst->sin_addr.S_un.S_un_b.s_b1;
		u_char	db2 = dst->sin_addr.S_un.S_un_b.s_b2;
		u_char	db3 = dst->sin_addr.S_un.S_un_b.s_b3;

		if ( sb1 == db1 && sb2 == db2 && sb3 == db3 )
			bSameLocal = true;
		*/

		bSameLocal = itsrc->second->pubaddr == itdst->second->pubaddr;
	}

	//send pub/pri address of dest to source peer
	CMessage	msgout_src(M_SC_CONNECT_PEER);
	pubaddr = itdst->second->pubaddr.asIPString();
	priaddr = itdst->second->priaddr.asIPString();
	peerid = itdst->first;
	msgout_src.serial(src_peer);		//peer id
	msgout_src.serial(peerid);		//peer id
	msgout_src.serial(pubaddr);		//public address
	msgout_src.serial(priaddr);		//private address
	msgout_src.serial(bSameLocal);
	CUnifiedNetwork::getInstance ()->send(itsrc->second->tsid, msgout_src);

	//send pub/pri address of source to dest peer
	CMessage	msgout_dst(M_SC_CONNECT_PEER);
	pubaddr = itsrc->second->pubaddr.asIPString();
	priaddr = itsrc->second->priaddr.asIPString();
	peerid = itsrc->first;
	msgout_dst.serial(dst_peer);	//peer id
	msgout_dst.serial(peerid);		//peer id
	msgout_dst.serial(pubaddr);		//public address
	msgout_dst.serial(priaddr);		//private address
	msgout_dst.serial(bSameLocal);
	CUnifiedNetwork::getInstance ()->send(itdst->second->tsid, msgout_dst);

	sipinfo ("confirmConnectPeer: %d, %d", src_peer, dst_peer);
}

//TCP mode
static void cbRequestConnectPeer(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	src_peer;
	uint32	dst_peer;
	string	sz_srcpriaddr, sz_pubaddr;
	_msgin.serial(src_peer, dst_peer, sz_srcpriaddr, sz_pubaddr);

	CInetAddress	src_priaddr(sz_srcpriaddr);
	CInetAddress	src_pubaddr(sz_pubaddr);

	TPeers::iterator	itsrc = findPeer(src_peer);
	TPeers::iterator	itdst = findPeer(dst_peer);
	if ( itsrc == Peers.end() || itdst == Peers.end() )
	{
		sipwarning ("Invalid: Request peer connect: %d, %d", dst_peer, src_peer);
		return;
	}

	itsrc->second->pubaddr = src_pubaddr;
	itsrc->second->priaddr = src_priaddr;

	sipinfo ("<%s><%s> Request peer connect: %d, %d", src_pubaddr.asIPString().c_str(),
		src_priaddr.asIPString().c_str(), src_peer, dst_peer);

	CMessage	msgout(M_REQ_CONNECT_PEER);
	msgout.serial(dst_peer, src_peer);
	CUnifiedNetwork::getInstance ()->send(itdst->second->tsid, msgout);
}

static void cbResponseConnectPeer(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	src_peer;
	uint32	dst_peer;
	string	sz_srcpriaddr, sz_pubaddr;
	_msgin.serial(src_peer, dst_peer, sz_srcpriaddr, sz_pubaddr);

	CInetAddress	src_priaddr(sz_srcpriaddr);
	CInetAddress	src_pubaddr(sz_pubaddr);

	TPeers::iterator	itsrc = findPeer(src_peer);
	TPeers::iterator	itdst = findPeer(dst_peer);
	if ( itsrc == Peers.end() || itdst == Peers.end() )
	{
		sipwarning ("Invalid: Response peer connect: %d, %d", dst_peer, src_peer);
		return;
	}

	itsrc->second->pubaddr = src_pubaddr;
	itsrc->second->priaddr = src_priaddr;

	sipinfo ("<%s><%s> Response peer connect: %d, %d", src_pubaddr.asIPString().c_str(),
		src_priaddr.asIPString().c_str(), src_peer, dst_peer);

	confirmConnectPeer(src_peer, dst_peer);
}

static void cbConnectPeerOk(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);

	uint32	dst_peerid;
	_msgin.serial(dst_peerid);

	uint32	src_peerid = peerid;
	TPeers::iterator	itsrc = findPeer(src_peerid);
	TPeers::iterator	itdst = findPeer(dst_peerid);

	if ( itsrc == Peers.end() || itdst == Peers.end() )
	{
		sipinfo("Peers no exist in peer list: %d, %d", src_peerid, dst_peerid);
		return;
	}

	itsrc->second->partner = dst_peerid;
	sipinfo("Peer connection is completed: %d and %d", src_peerid, dst_peerid);
}

static void cbRequestChat(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);
	uint32	dst_peer, mode;
	_msgin.serial(dst_peer, mode);

//	TPeers::iterator	itdst = findPeer(dst_peer);
//	if ( itdst == Peers.end() )
//		return;
	FAMILY	* family = GetFamilyData(peerid);
	FAMILY	* familyDst = GetFamilyData(dst_peer);
	if (family == NULL || familyDst == NULL)
		return;

	string relayhost = IService::getInstance()->ConfigFile.exists("RelayHost") ? IService::getInstance()->ConfigFile.getVar("RelayHost").asString(): "localhost:9600";
	CInetAddress	relayaddr(relayhost);
	string	udprelay = relayaddr.asIPString();

	CMessage	msgout(M_SC_REQUEST_TALK);
	uint32	src_peer = peerid;
	msgout.serial(dst_peer);
	msgout.serial(src_peer, family->m_Name);
	msgout.serial(mode, udprelay);

	CUnifiedNetwork::getInstance ()->send(familyDst->m_FES, msgout);
	sipinfo("request talk: src=%d, dest=%d, relayaddr=%s", src_peer, dst_peer, udprelay.c_str());
}

static void cbResponseChat(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);

	uint32	dst_peer;
	sint8	ResponseCode;
	string	udprelay;

	_msgin.serial(dst_peer);
	_msgin.serial(ResponseCode);
	_msgin.serial(udprelay);

//	TPeers::iterator	itdst = findPeer(dst_peer);
//	if ( itdst == Peers.end() )
//		return;
	FAMILY	* familyDst = GetFamilyData(dst_peer);
	if (familyDst == NULL)
		return;

	CMessage	msgout(M_SC_RESPONSE_TALK);
	uint32	src_peer = peerid;
	msgout.serial(dst_peer);
	msgout.serial(src_peer);
	msgout.serial(ResponseCode, udprelay);
	
	CUnifiedNetwork::getInstance ()->send(familyDst->m_FES, msgout);
	sipinfo("response talk: src=%d, dest=%d, ResponseCode=%d, relayaddr=%s", src_peer, dst_peer, ResponseCode, udprelay.c_str());
}

static void cbHangupChat(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);

	uint32	dst_peer;
	_msgin.serial(dst_peer);

	uint32	hangupcode;
	_msgin.serial(hangupcode);

//	TPeers::iterator	itdst = findPeer(dst_peer);
//	if ( itdst == Peers.end() )	return;
	FAMILY	* familyDst = GetFamilyData(dst_peer);
	if (familyDst == NULL)
		return;

	CMessage	msgout(M_SC_HANGUP_TALK);
	uint32	src_peer = peerid;
	msgout.serial(dst_peer, src_peer, hangupcode);

	CUnifiedNetwork::getInstance ()->send(familyDst->m_FES, msgout);
	sipinfo("hangup talk: src=%d, dest=%d, hagupcode=%d", src_peer, dst_peer, hangupcode);
}

//////////////////////////////////////////////////////////////////////////
//Session
typedef	std::list<uint32>	TSessionMembers;

class CTalkSession
{
public:
	TSessionMembers	m_members;
	uint32			m_session_id;

	CTalkSession(uint32 idSession)
	{
		m_session_id = idSession;
	}

	~CTalkSession()
	{
	}

	int	getMemberCount()
	{
		return m_members.size();
	}

	void addMember(uint32 id)
	{
		TSessionMembers::iterator	it;
		for ( it = m_members.begin(); it != m_members.end(); it ++ )
		{
			if ( *it == id )
				return;
		}
		m_members.push_back(id);
	}

	void delMember(uint32 id)
	{
		TSessionMembers::iterator	it;
		for ( it = m_members.begin(); it != m_members.end(); it ++ )
		{
			if ( *it == id )
			{
				m_members.erase(it);
				return;
			}
		}
	}
};

typedef	std::map<uint32, CTalkSession*>	TTalkSessions;
TTalkSessions	TalkSessions;

static void cbJoinSession(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);
	
	uint32	idSession, idPeer;
	_msgin.serial(idSession, idPeer);
	
	idPeer = peerid;

	sipinfo("join session: %d, %d", idSession, idPeer);

	TPeers::iterator	itp = Peers.find(idPeer);
	if ( itp == Peers.end() )
	{
		sipinfo("error: join session from unregisterd peer: %d", idPeer);
		return;
	}
	itp->second->idSession = idSession;

	CTalkSession*	session;
	TTalkSessions::iterator	it;
	it = TalkSessions.find(idSession);
	if ( it == TalkSessions.end() )
	{
		session = new CTalkSession(idSession);
		session->addMember(idPeer);
		TalkSessions.insert( make_pair(idSession, session) );
	}
	else
	{
		//\C0̹\CC \BC\BC\BCǿ\A1 \C0ִ\C2 \BC\BA\BF\F8\B5鿡\B0\D4 \BB\F5\B7\CE \B5\E9\BE\EE\BF\C2 \BC\BA\BF\F8\C0\BB \C5\EB\C1\F6\C7Ѵ\D9
		session = it->second;
		TSessionMembers&	members = session->m_members;
		TSessionMembers::iterator	itm;
		for ( itm = members.begin(); itm != members.end(); itm ++ )
		{
			uint32	peeri = *itm;
			TPeers::iterator	itp = Peers.find(peeri);
			if ( itp == Peers.end() )
			{
				members.erase(itm);
			}
			else
			{
				CPeer*	peer = itp->second;

				CMessage	msgout1(M_SC_SESSION_ADDMEMBER);
				msgout1.serial(peeri, idPeer);
				CUnifiedNetwork::getInstance ()->send(peer->tsid, msgout1);
			}
		}

		session->addMember(idPeer);
	}

	CMessage	msgout2(M_SC_JOIN_SESSION);
	bool	bOK = true;
	msgout2.serial(peerid);
	msgout2.serial(bOK);

	TSessionMembers&	members = session->m_members;
	TSessionMembers::iterator	itm;
	for ( itm = members.begin(); itm != members.end(); itm ++ )
	{
		uint32	peeri = *itm;
		msgout2.serial(peeri);
	}
	CUnifiedNetwork::getInstance ()->send(itp->second->tsid, msgout2);
	
}

void delFromSession(uint32 idSession, uint32 idPeer)
{
	TTalkSessions::iterator	it;
	it = TalkSessions.find(idSession);
	if ( it == TalkSessions.end() )
		return;

	CTalkSession*	session = it->second;
	session->delMember(idPeer);

	if ( session->getMemberCount() == 0 )
	{
		sipinfo("delete session: %d", session->m_session_id);
		delete it->second;
		TalkSessions.erase(it);
	}
	sipinfo("del from session: %d, %d", idSession, idPeer);
}

static void cbExitSession(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);

	uint32	idSession, idPeer;
	_msgin.serial(idSession, idPeer);
	
	idPeer = peerid;
	sipinfo("exit session: %d, %d", idSession, idPeer);

	delFromSession(idSession, idPeer);
}

static void cbSpeakToSession(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	peerid;		_msgin.serial(peerid);
	uint32	idSession, idPeer;
	_msgin.serial(idSession, idPeer);

	idPeer = peerid;
	sipinfo("speak to session: %d, %d", idSession, idPeer);

	TTalkSessions::iterator	it;
	it = TalkSessions.find(idSession);
	if ( it == TalkSessions.end() )
		return;

	CTalkSession*	session = it->second;
	if ( session->getMemberCount() < 2 )
		return;

	TSessionMembers&	members = session->m_members;
	TSessionMembers::iterator	itm;
	for ( itm = members.begin(); itm != members.end(); itm ++ )
	{
		uint32	peeri = *itm;
		TPeers::iterator	itp = Peers.find(peeri);
		if ( itp != Peers.end() )
		{
			CPeer*	peer = itp->second;
			CMessage	msgout(M_SC_SPEAK_TO_SESSION);
			msgout.serial(peeri);
			msgout.serial(idPeer);
			CUnifiedNetwork::getInstance ()->send(peer->tsid, msgout);
		}
	}
}

void	FamilyLogoutForBroker(T_FAMILYID _FID)
{
	TPeers::iterator	itsrc = findPeer(_FID);
	if ( itsrc == Peers.end() )	return;
	CPeer*	peer = itsrc->second;
	if ( peer->partner )
	{
		FAMILY	* family = GetFamilyData(peer->partner);
		if (family != NULL)
		{
			CMessage	msgout(M_SC_DISCONNECT_PEER);
			msgout.serial(peer->partner);
			msgout.serial(_FID);
			CUnifiedNetwork::getInstance ()->send(family->m_FES, msgout);
		}
	}
	UnRegisterPeer(_FID);
}

static TUnifiedCallbackItem B_CallbackArray[] =
{
	{ M_REGISTER_PEER,			cbRegisterPeer },
	{ M_CS_UNREGISTER_PEER,		cbUnRegisterPeer },
	{ M_REQ_CONNECT_PEER,		cbRequestConnectPeer },
	{ M_CS_RES_CONNECT_PEER,	cbResponseConnectPeer },
	{ M_CS_CONNECT_PEER_OK,		cbConnectPeerOk },

	{ M_CS_JOIN_SESSION,		cbJoinSession },
	{ M_CS_EXIT_SESSION,		cbExitSession },
	{ M_CS_SPEAK_TO_SESSION,	cbSpeakToSession },

	{ M_CS_REQUEST_TALK,		cbRequestChat },
	{ M_CS_RESPONSE_TALK,		cbResponseChat },
	{ M_CS_HANGUP_TALK,			cbHangupChat },
};

void	InitBroker()
{
	CUnifiedNetwork::getInstance ()->addCallbackArray(B_CallbackArray, sizeof(B_CallbackArray)/sizeof(B_CallbackArray[0]));

	m_bTcp = false;

	if ( m_bTcp )
		sipinfo("TCP mode");
	else
		sipinfo("UDP mode");
}

void	UpdateBroker()
{
	if ( m_UdpPeerBroker )
	{
		m_UdpPeerBroker->update();
	}
}

void	ReleaseBroker()
{
	if ( m_UdpPeerBroker )
	{
		delete m_UdpPeerBroker;
		m_UdpPeerBroker = NULL;
	}
}

//class CBrokerService :	public IService
//{
//public:
//	void init()
//	{
//		InitBroker();
//	}
//
//	bool update ()
//	{
//		UpdateBroker();
//		return true;
//	}
//
//	void release()
//	{         
//		ReleaseBroker();
//	}
//};
//
//void run_broker_service()
//{
//	CBrokerService *scn = new CBrokerService;
//	createDebug (NULL, false);
//	scn->setCallbackArray (B_CallbackArray, sizeof(B_CallbackArray)/sizeof(B_CallbackArray[0]));
//	sint retval = scn->main ("BS", "broker_service", "", 0, "", "", __DATE__" "__TIME__);
//	delete scn;
//}

//***************************************************************************************************************************************

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setp2p, "set mode of P2P with TCP or UDP", "<text>")
{
	string	mode = args[0].c_str();
	if ( mode == "tcp" )
	{
		m_bTcp = true;
		sipinfo("P2P mode set to TCP mode");
	}
	else if ( mode == "udp" )
	{
		m_bTcp = false;
		sipinfo("P2P mode set to UDP mode");
	}
	else
		sipinfo("bad command: setP2P TCP/UDP");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, getp2p, "get mode of P2P with TCP or UDP", "")
{
	if ( m_bTcp  )
		sipinfo("P2P mode is TCP");
	else
		sipinfo("P2P mode is UDP");

	return true;
}
