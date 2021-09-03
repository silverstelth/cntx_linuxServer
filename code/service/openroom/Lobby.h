#pragma once

#include "openroom.h"
#include "Cell.h"
//#include "Animal.h"
#include "World.h"

//class CLobbyManager;
class CLobbyWorld;

typedef	SIPBASE::CSmartPtr<CLobbyWorld>		TLobbyWorld;
typedef std::map<T_ROOMID, TLobbyWorld>		TLobbyWorlds;	//lobby id vs CLobbyWorld pointer
//typedef std::map<uint32, INFO_WISHINLOBBY>	TWishInLobby;
typedef std::map<T_ROOMID, ucstring>		TDefaultLobbys;

class CLobbyWorld : public CWorld
{
	/*            old                 */
//	ucstring		m_LobbyName;
//
//	CCellManager *	m_pCell;
//	bool			m_bCellMode;
//	LST_REGIONSCOPE	m_WishRegion;				// Wish regions
//
//	TAnimals		m_Animals;					// moving animals in lobby
//	MAP_REGIONSCOPE	m_RoomAnimalRegion;		
//
//private:
//	bool	loadMap(T_MSTROOM_ID SceneId);
//	bool    loadMoveAnimal(T_MSTROOM_ID SceneId);
//	bool    loadWishList(T_ROOMID lobby_id);

public:
	//bool	init(T_ROOMID lobby_id, const ucstring& lobbyname);
	//bool	update(SIPBASE::TTime curtime);
	//void	release();

	void    EnterFamilyInLobby(T_FAMILYID _FID, SIPNET::TServiceId _sid, uint32 bReqItemRefresh);
	void    EnterFamilyInLobby_After(T_FAMILYID _FID, uint32 bReqItemRefresh);
	void	SendAddedJifen(T_FAMILYID _FID);

	void	ch_leave(T_FAMILYID _FID);
	void	ch_move(T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_STATE _State, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z);

	/********************************** old **************************************/
	/*void	onCellNotifyPlayer(T_CELLPLAYERID playerid, ENUM_CELLNOTIFY_TYPE type, TCellPlayerIds& players);
	void	onCellNotifyWish(TCellObjIds& objs, ENUM_CELLNOTIFY_TYPE type, TCellPlayerIds& players);
	void    onCellNotifyAnimal(TCellObjIds& objs, ENUM_CELLNOTIFY_TYPE type, TCellPlayerIds& players);
	void    onCellNotifyTrigger(T_CELLPLAYERID playerid, T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT dist);*/

	SIPNET::TServiceId	getServiceId(T_FAMILYID _FID);
	uint32		getUserCount()	{ return static_cast<uint32>(m_mapFamilys.size()); }
	bool		isUserEmpty()	{ return m_mapFamilys.empty(); }
	bool		getCharacterState(T_CHARACTERID _chid, T_CH_STATE& _State, T_LEVEL& _Level, T_CH_DIRECTION& _Dir, T_CH_X& _X, T_CH_Y& _Y, T_CH_Z& _Z);
	INFO_CHARACTERINROOM *	getMainCh(T_FAMILYID _fid);

	/***************************** old *****************************************/

	//bool		isCellMode()	{ return m_bCellMode; }
	//CCellManager *	getCell()	{ return m_pCell; }
	//void getNewWishPos( T_CH_X _X, T_CH_Z _Z, float * Xpos, float * Ypos, float * Zpos, uint8 *indexScope, uint8 *indexSpace); 
	//void sendNewWishToAllInRoom(INFO_WISHINLOBBY &newWish, T_FAMILYID _FID);
	//// send all wish info to Client, when he enter the lobby
	//void sendAllWishToClient(T_FAMILYID _FID);
	//// check the wish in lobby is out of date or not
	//void refreshWish();
	//void refreshAnimal(SIPBASE::TTime curtime);
	//bool GetMoveablePos(std::string _strRegion, T_POSX _curX, T_POSY _curY, T_POSZ _curZ, T_DISTANCE _maxDis, T_POSX *_nextX, T_POSY *_nextY, T_POSZ *_nextZ);

	//// send the moving animals info to client, when he enter the lobby
	//void sendMoveAnimalToClient(T_FAMILYID _FID);

	//// all wish info in lobby
	//TWishInLobby m_mapWishList;


	CLobbyWorld() : CWorld(WORLD_LOBBY) { m_RoomName = L"Lobby"; };
	virtual ~CLobbyWorld() {};
};

extern	CLobbyWorld *	getLobbyWorld();
extern	void	cb_M_CS_PROMOTION_ROOM_Lobby(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMCARD_PROMOTION_Lobby(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_NEWROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMCARD_NEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_MS_ROOMCARD_USE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
