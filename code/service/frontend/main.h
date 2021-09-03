#ifndef TIAN_FRONTEND_MAIN_H
#define TIAN_FRONTEND_MAIN_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include <map>

extern SIPNET::CCallbackServer		*Clients;

extern	uint32				GetIdentifiedID(SIPNET::TTcpSockId _sid);
extern	SIPNET::TTcpSockId	GetSocketForIden(uint32 _uid);
extern	bool				IsGM(uint32 _uid);
extern	bool				IsGMActive(uint32 _uid);
extern	bool				AbleGMPacketAccept(uint32 _uid, MsgNameType mesName);
extern	void				SendGMFail(T_ERR err, SIPNET::TTcpSockId from);
extern	ucstring			GetUserName(uint32 _uid);
extern	void				SetGMActive(uint32 _uid, uint8 _bOn);
extern	uint8				GetUserType(uint32 _uid);

struct CPlayer
{
	CPlayer(){};
	CPlayer(uint32 Id, SIPNET::TSockId Con, uint8 uType, ucstring name, uint8 bGM) : 
	id(Id), con(Con), UserType(uType), UserName(name), GMActive(0), bIsGM(bGM) { }
	uint32		id;
	SIPNET::TSockId		con;
	uint8		UserType;
	ucstring	UserName;
	uint8		GMActive;
	uint8		bIsGM;
};

typedef std::map<uint32, CPlayer> _pmap;

extern	_pmap localPlayers;

#endif	// TIAN_FRONTEND_MAIN_H
