
/************************************************************************/
/*        Cell process for support massive players in big map			*/
/*        2009.10.23	NamJuSong										*/
/************************************************************************/

#pragma once

#define	T_MAPUNIT			float
#define	T_CELLID			sint16
#define	T_CELLPLAYERID		uint32
#define	T_CELLOBJTYPE		uint16
#define	T_CELLOBJID			uint32

#define	INVALID_CELLID		(-1)

typedef std::list<T_CELLPLAYERID>	TCellPlayerIds;
typedef std::list<T_CELLOBJID>		TCellObjIds;

struct RECT_T
{
	T_MAPUNIT	x, y, w, h;

	RECT_T(T_MAPUNIT _x, T_MAPUNIT _y, T_MAPUNIT _w, T_MAPUNIT _h) : x(_x), y(_y), w(_w), h(_h)	{}
};

enum ENUM_CELLNOTIFY_TYPE {
	TYPE_CELL_ENTER,		// case of new object appear to map
	TYPE_CELL_LEAVE,		// case of a object disappear from map
	TYPE_CELL_ADD,			// case of some cell appear to player
	TYPE_CELL_DEL,			// case of some cell disappear from player
	TYPE_CELL_MOV,			// case of player move
};

struct	CellPlayer
{
	T_CELLID	cell_x, cell_y;
	T_MAPUNIT	x, y;

	CellPlayer() {}
	CellPlayer(T_CELLID _cell_x, T_CELLID _cell_y, T_MAPUNIT _x, T_MAPUNIT _y) : cell_x(_cell_x), cell_y(_cell_y), x(_x), y(_y) {};
};
typedef std::map<T_CELLPLAYERID, CellPlayer>	TTotalPlayers;

struct	CellObject
{
	T_CELLID	cell_x, cell_y;
	T_MAPUNIT	x, y;
	CellObject() {}
	CellObject(T_CELLID _cell_x, T_CELLID _cell_y, T_MAPUNIT _x, T_MAPUNIT _y) : cell_x(_cell_x), cell_y(_cell_y), x(_x), y(_y) {};
};
typedef std::map<T_CELLOBJID, CellObject>		TTypeObjects;
typedef std::map<T_CELLOBJTYPE, TTypeObjects>	TTotalObjects;

// Animal, Wish, ...
typedef std::map<T_CELLOBJTYPE, TCellObjIds>	TCellObjects;
typedef std::map<T_CELLOBJTYPE, T_MAPUNIT>		TTriggerRadius;		//Power of Radius

typedef	void	(*TCellNotifyPlayer)(void * arg, T_CELLPLAYERID playerid, ENUM_CELLNOTIFY_TYPE state, TCellPlayerIds& players);
typedef	void	(*TCellNotifyObject)(void * arg, T_CELLOBJTYPE obj_type, TCellObjIds& objs, ENUM_CELLNOTIFY_TYPE notify_type, TCellPlayerIds& players);
typedef	void	(*TCellNotifyTrigger)(void * arg, T_CELLPLAYERID playerid, T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT dist);

class  CCell : public SIPBASE::CRefCount
{
public:
	TCellPlayerIds	players;
	TCellObjects	objects;

	void	addPlayer(T_CELLPLAYERID id)
	{
		players.push_back(id);
	}

	void	delPlayer(T_CELLPLAYERID id)
	{
		players.remove(id);
	}

	void	addObject(T_CELLOBJTYPE type, T_CELLOBJID id)
	{
		objects[type].push_back(id);
	}

	void	delObject(T_CELLOBJTYPE type, T_CELLOBJID id)
	{
		TCellObjects::iterator it = objects.find(type);
		if ( it != objects.end() )
			it->second.remove(id);
	}
};

typedef SIPBASE::CSmartPtr<CCell>			PCell;
typedef std::map<T_CELLID, PCell>			TCells;

class	CCellManager
{
	T_MAPUNIT			m_MapX0, m_MapY0, m_MapW, m_MapH;
	T_MAPUNIT			m_ViewDist;

	T_CELLID			m_nCellXCount, m_nCellYCount;
	T_MAPUNIT			m_CellSize;
	
	TCells				m_Cells;
	TTotalObjects		m_Objects;
	TTotalPlayers		m_Players;

	TTriggerRadius		m_TriggerRadius;

	TCellNotifyPlayer	m_cbNotifyPlayer;
	TCellNotifyObject	m_cbNotifyObject;
	TCellNotifyTrigger	m_cbNotifyTrigger;
	void *				m_CbArg;

private:
	bool		loadMap(uint32 map_id);
	bool		calcCellNum(T_MAPUNIT x, T_MAPUNIT y, T_CELLID *pCellX = NULL, T_CELLID *pCellY = NULL);
	PCell		getCell(T_CELLID cell_x, T_CELLID cell_y);
	void		getAroundCellPlayers(T_CELLID cell_x, T_CELLID cell_y, TCellPlayerIds& notify_chs);
	bool		getNotifyPlayers(T_CELLID cell_x, T_CELLID cell_y, T_CELLID dst_cell_x, T_CELLID dst_cell_y,
								 TCellPlayerIds& add_chs, TCellPlayerIds& del_chs, TCellPlayerIds& mov_chs);
	bool		getNotifyObjects(T_CELLOBJTYPE obj_type,
								 T_CELLID cell_x, T_CELLID cell_y, T_CELLID dst_cell_x, T_CELLID dst_cell_y,
								 TCellObjIds& add_objs, TCellObjIds& del_objs);
	CellObject *	getObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id);
	T_MAPUNIT	getTriggerRadius(T_CELLOBJTYPE obj_type);
	bool		trigger(T_CELLPLAYERID playerid, T_MAPUNIT x, T_MAPUNIT y);

	void		onNotifyPlayer(T_CELLPLAYERID playerid, ENUM_CELLNOTIFY_TYPE notify_type, TCellPlayerIds& notify_players);
	void		onNotifyObject(T_CELLOBJTYPE obj_type, TCellObjIds& objs, ENUM_CELLNOTIFY_TYPE notify_type, TCellPlayerIds& notify_players);
	void		onNotifyTrigger(T_CELLPLAYERID playerid, T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT dist);

public:
	bool		init(uint32 map_id, T_MAPUNIT view_dist = 800);
	void		setNotifyCallback(TCellNotifyPlayer cb, void * arg = NULL, TCellNotifyObject cbo = NULL)	{ m_cbNotifyPlayer = cb; m_cbNotifyObject = cbo; m_CbArg = arg; }
	void		setTriggerCallback(TCellNotifyTrigger cb)	{ m_cbNotifyTrigger = cb; }
	void		setTriggerRadius(T_CELLOBJTYPE obj_type, T_MAPUNIT radius)	{ m_TriggerRadius[obj_type] = radius; }

	bool		addPlayer(T_CELLPLAYERID playerid, T_MAPUNIT x, T_MAPUNIT y);
	bool		delPlayer(T_CELLPLAYERID playerid);
	bool		movePlayer(T_CELLPLAYERID playerid, T_MAPUNIT x, T_MAPUNIT y);

	bool		addObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT x, T_MAPUNIT y);
	bool		delObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id);
	bool		moveObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT x, T_MAPUNIT y);

public:
	CCellManager();
	~CCellManager();
};
