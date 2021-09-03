
#include <misc/types_sip.h>

#if defined(SIP_OS_WINDOWS) && defined(_WINDOWS)
#	include <windows.h>
#	undef min
#	undef max
#endif

#include <string>
#include <vector>
#include <map>
#include <list>

#include <misc/debug.h>
#include <misc/ucstring.h>
#include <misc/i_xml.h>
#include <misc/sstring.h>
#include <misc/smart_ptr.h>

#include "Cell.h"

using namespace		SIPBASE;
using namespace		std;

CCellManager::CCellManager()
{
	m_cbNotifyPlayer = NULL;
	m_cbNotifyObject = NULL;
	m_cbNotifyTrigger = NULL;
	m_CbArg = NULL;
	m_MapX0 = 0, m_MapY0 = 0, m_MapW = 0, m_MapH = 0;
}

CCellManager::~CCellManager()
{
	m_Cells.clear();
}

bool	CCellManager::init(uint32 map_id, T_MAPUNIT view_dist)
{
	m_ViewDist = view_dist;
	if ( ! loadMap(map_id) )
	{
		siperrornoex("Load lobby map failed: map_id=%d", map_id);
		return false;
	}
	return true;
}

static	bool	checkPointInRect(T_MAPUNIT x, T_MAPUNIT y, RECT_T& rc)
{
	T_MAPUNIT	x1 = rc.x, y1 = rc.y;
	T_MAPUNIT	x2 = rc.x + rc.w, y2 = rc.y + rc.h;
	if ( x > x1 && y > y1 && x < x2 && y < y2 )
		return true;
	return false;
}

static	bool	checkOverlap(RECT_T& rc, std::list<RECT_T>& rc_list)
{
	T_MAPUNIT	x, y;
	std::list<RECT_T>::iterator it;

	for ( it = rc_list.begin(); it != rc_list.end(); it ++ )
	{
		RECT_T &	rc1 = *it;
		x = rc.x, y = rc.y;
		if ( checkPointInRect(x, y, rc1) )	return true;

		x = rc.x + rc.w, y = rc.y + rc.h;
		if ( checkPointInRect(x, y, rc1) )	return true;

		x = rc.x + rc.w / 2, y = rc.y + rc.h / 2;
		if ( checkPointInRect(x, y, rc1) )	return true;

		x = rc1.x, y = rc1.y;
		if ( checkPointInRect(x, y, rc) )	return true;

		x = rc1.x + rc1.w, y = rc1.y + rc1.h;
		if ( checkPointInRect(x, y, rc) )	return true;

		x = rc1.x + rc1.w / 2, y = rc1.y + rc1.h / 2;
		if ( checkPointInRect(x, y, rc) )	return true;
	}

	return false;
}

extern	ucstring	OutDataRootPath;
bool	CCellManager::loadMap(uint32 map_id)
{
	ucstring scopePath = OutDataRootPath + toStringW(map_id) + L".scene";
	CIXml xml;
	if ( ! xml.init(scopePath) )
		return false;

	xmlNodePtr	pRoot = xml.getRootNode();
	if ( pRoot == NULL)
		return false;

	CSStringA	sPageSize = CIXml::getStringProperty(pRoot, "pageSize", "1000 1000 1000");
	CVectorSStringA	psv;
	if ( ! sPageSize.splitWords(psv) )
		return false;
	sipassert( psv.size() == 3 );
	int	pagesize_x = psv[0].atoi();
	int	pagesize_y = psv[2].atoi();

	xmlNodePtr	pPages = CIXml::getFirstChildNode(pRoot, "PageTable");
	if ( pPages == NULL )
		return false;

	//////////////////////////////////////////////////////////////////////////
	m_MapX0 = 0, m_MapY0 = 0, m_MapW = 0, m_MapH = 0;
	T_MAPUNIT	x_max = -1000000, y_max = -1000000;
	std::list<RECT_T>	rcPages;
	int	page_count = 0;

	xmlNodePtr	pPage = CIXml::getFirstChildNode(pPages, "Page");
	while ( pPage )
	{
		CSStringA	strX	= CIXml::getStringProperty(pPage,	"pageX",	"");
		CSStringA	strZ	= CIXml::getStringProperty(pPage,	"pageZ",	"");
		int	px = strX.atoi();
		int	py = strZ.atoi();

		T_MAPUNIT	x1 = (T_MAPUNIT)pagesize_x * px;
		T_MAPUNIT	z1 = (T_MAPUNIT)pagesize_y * py;
		T_MAPUNIT	x2 = (T_MAPUNIT)pagesize_x * (px + 1);
		T_MAPUNIT	z2 = (T_MAPUNIT)pagesize_y * (py + 1);
		RECT_T	rc(x1, z1, x2-x1, z2-z1);
		rcPages.push_back(rc);

		if ( x2 > x_max )	x_max = x2;
		if ( z2 > y_max )	y_max = z2;
		if ( x1 < m_MapX0 )	m_MapX0 = x1;
		if ( z1 < m_MapY0 )	m_MapY0 = z1;

		pPage = CIXml::getNextChildNode(pPage, "Page");
		page_count ++;
	}
	if ( page_count == 0 )	return false;
	m_MapW = x_max - m_MapX0;
	m_MapH = y_max - m_MapY0;
	m_CellSize = m_ViewDist;
	m_nCellXCount = (T_CELLID)(m_MapW / m_ViewDist) + 1;
	m_nCellYCount = (T_CELLID)(m_MapH / m_ViewDist) + 1;

	// calculate cell
	int	count = 0;
	for ( T_CELLID i = 0; i < m_nCellXCount; i ++ )
	for ( T_CELLID j = 0; j < m_nCellYCount; j ++ )
	{
		T_MAPUNIT	w = m_CellSize;
		T_MAPUNIT	x1 = m_MapX0 + i * w;
		T_MAPUNIT	y1 = m_MapY0 + j * w;
		RECT_T	rc(x1, y1, w, w);
		if ( ! checkOverlap(rc, rcPages) )
			continue;

		m_Cells[m_nCellXCount * j + i] = new CCell;
		count ++;
	}
	sipinfo("Cell calculated: cell_count=%d, x_count=%d, y_count=%d, cell_size=%.2f", count, m_nCellXCount, m_nCellYCount, m_CellSize);

	return true;
}

bool	CCellManager::calcCellNum(T_MAPUNIT x, T_MAPUNIT y, T_CELLID *pCellX, T_CELLID *pCellY)
{
	T_CELLID cell_x = (T_CELLID)((x - m_MapX0) / m_CellSize);
	T_CELLID cell_y = (T_CELLID)((y - m_MapY0) / m_CellSize);
	if ( cell_x >= m_nCellXCount || cell_y >= m_nCellYCount )
		return false;

	if ( pCellX )	*pCellX = cell_x;
	if ( pCellY )	*pCellY = cell_y;
	return true;
}

PCell	CCellManager::getCell(T_CELLID cell_x, T_CELLID cell_y)
{
	if ( ( cell_x < 0 || cell_x >= m_nCellXCount ) || ( cell_y < 0 || cell_y >= m_nCellYCount ) )
		return NULL;

	T_CELLID n = m_nCellXCount * cell_y + cell_x;
	TCells::iterator it = m_Cells.find(n);
	if ( it == m_Cells.end() )
		return NULL;
	return it->second;
}

struct CELL_POS_T
{
	int	x, y;
};
struct NOTIFY_CELL_MOVE_T 
{
	CELL_POS_T 		add_list[6];
	CELL_POS_T 		del_list[6];
	CELL_POS_T 		mov_list[9];
};

static	NOTIFY_CELL_MOVE_T	s_NotifyCellMoveTable[3][3] = 
{
	{
		// 0, 0		(-1, -1)
		{
			{ {-2,-2},{-1,-2},{0,-2},{-2,-1},{-2,0},{10,10}	},						// add
			{ {1,-1},{1,0},{-1,1},{0,1},{1,1},{10,10}	},							// del
			{ {-1,-1},{0,-1},{-1,0},{0,0},{10,10},{10,10},{10,10},{10,10}	},		// move
		},
		// 0, 1		(-1, 0)
		{
			{ {-2,-1},{-2,0},{-2,1},{10,10},{10,10},{10,10}	},						// add
			{ {1,-1},{1,0},{1,1},{10,10},{10,10},{10,10}	},						// del
			{ {-1,-1},{0,-1},{-1,0},{0,0},{-1,1},{0,1},{10,10},{10,10},{10,10}	},	// move
		},
		// 0, 2		(-1, 1)
		{
			{ {-2,0},{-2,1},{-2,2},{-1,2},{0,2},{10,10}	},							// add
			{ {-1,-1},{0,-1},{1,-1},{1,0},{1,1},{10,10}	},							// del
			{ {-1,0},{0,0},{-1,1},{0,1},{10,10},{10,10},{10,10},{10,10},{10,10}	},	// move
		},
	},
	{
		// 1, 0		(0, -1)
		{
			{ {-1,-2},{0,-2},{1,-2},{10,10},{10,10},{10,10}	},						// add
			{ {-1,1},{0,1},{1,1},{10,10},{10,10},{10,10}	},						// del
			{ {-1,-1},{0,-1},{1,-1},{-1,0},{0,0},{1,0},{10,10},{10,10},{10,10}	},	// move
		},
		// 1, 1		(0, 0)
		{
			{ {10,10},{10,10},{10,10},{10,10},{10,10},{10,10}	},					// add
			{ {10,10},{10,10},{10,10},{10,10},{10,10},{10,10}	},					// del
			{ {-1,-1},{0,-1},{1,-1},{-1,0},{0,0},{1,0},{-1,1},{0,1},{1,1}	},		// move
		},
		// 1, 2		(0, 1)
		{
			{ {-1,2},{0,2},{1,2},{10,10},{10,10},{10,10}	},						// add
			{ {-1,-1},{0,-1},{1,-1},{10,10},{10,10},{10,10}	},						// del
			{ {-1,0},{0,0},{1,0},{-1,1},{0,1},{1,1},{10,10},{10,10},{10,10}	},		// move
		},
	},
	{
		// 2, 0		(1, -1)
		{
			{ {0,-2},{1,-2},{2,-2},{2,-1},{2,0},{10,10}	},							// add
			{ {-1,-1},{-1,0},{-1,1},{0,1},{1,1},{10,10}	},							// del
			{ {0,-1},{1,-1},{0,0},{1,0},{10,10},{10,10},{10,10},{10,10},{10,10}	},	// move
		},
		// 2, 1		(1, 0)
		{
			{ {2,-1},{2,0},{2,1},{10,10},{10,10},{10,10}	},						// add
			{ {-1,-1},{-1,0},{-1,1},{10,10},{10,10},{10,10}	},						// del
			{ {0,-1},{1,-1},{0,0},{1,0},{0,1},{1,1},{10,10},{10,10},{10,10}	},		// move
		},
		// 2, 2		(1, 1)
		{
			{ {2,0},{2,1},{0,2},{1,2},{2,2},{10,10}	},								// add
			{ {-1,-1},{0,-1},{1,-1},{-1,0},{-1,1},{10,10}	},						// del
			{ {0,0},{1,0},{0,1},{1,1},{10,10},{10,10},{10,10},{10,10},{10,10}	},	// move
		},
	},
};

bool	CCellManager::getNotifyPlayers(T_CELLID cell_x, T_CELLID cell_y, T_CELLID dst_cell_x, T_CELLID dst_cell_y,
									   TCellPlayerIds& add_chs, TCellPlayerIds& del_chs, TCellPlayerIds& mov_chs)
{
	int dx = dst_cell_x - cell_x;
	int dy = dst_cell_y - cell_y;
	if ( dx < -1 || dx > 1 || dy < -1 || dy > 1 )
	{
		getAroundCellPlayers(cell_x, cell_y, del_chs);
		getAroundCellPlayers(dst_cell_x, dst_cell_y, add_chs);
		return true;
	}

	int i = 0;
	NOTIFY_CELL_MOVE_T&	cell_mov = s_NotifyCellMoveTable[dx+1][dy+1];

	for ( i = 0; i < 6; i ++ )
	{
		int xx = cell_mov.add_list[i].x;
		int yy = cell_mov.add_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		T_CELLID	x = cell_x + xx;
		T_CELLID	y = cell_y + yy;
		PCell	pCell = getCell(x, y);
		if ( ! pCell.isNull() )	add_chs.insert(add_chs.end(), pCell->players.begin(), pCell->players.end());
	}
	for ( i = 0; i < 6; i ++ )
	{
		int xx = cell_mov.del_list[i].x;
		int yy = cell_mov.del_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		T_CELLID	x = cell_x + xx;
		T_CELLID	y = cell_y + yy;
		PCell	pCell = getCell(x, y);
		if ( ! pCell.isNull() )	del_chs.insert(del_chs.end(), pCell->players.begin(), pCell->players.end());
	}
	for ( i = 0; i < 9; i ++ )
	{
		int xx = cell_mov.mov_list[i].x;
		int yy = cell_mov.mov_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		T_CELLID	x = cell_x + xx;
		T_CELLID	y = cell_y + yy;
		PCell	pCell = getCell(x, y);
		if ( ! pCell.isNull() )	mov_chs.insert(mov_chs.end(), pCell->players.begin(), pCell->players.end());
	}
	return true;
}

bool	CCellManager::getNotifyObjects(T_CELLOBJTYPE obj_type,
									   T_CELLID cell_x, T_CELLID cell_y, T_CELLID dst_cell_x, T_CELLID dst_cell_y,
									   TCellObjIds& add_objs, TCellObjIds& del_objs)
{
	int dx = dst_cell_x - cell_x;
	int dy = dst_cell_y - cell_y;
	if ( dx < -1 || dx > 1 || dy < -1 || dy > 1 )
	{
		NOTIFY_CELL_MOVE_T&	cell_mov = s_NotifyCellMoveTable[1][1];

		for ( int i = 0; i < 9; i ++ )
		{
			int xx = cell_mov.mov_list[i].x;
			int yy = cell_mov.mov_list[i].y;
			if ( xx >= 10 || yy >= 10 )
				continue;
			
			{
				T_CELLID	x = cell_x + xx;
				T_CELLID	y = cell_y + yy;
				PCell	pCell = getCell(x, y);
				if ( pCell.isNull() )	
					continue;
				TCellObjects &	objs = pCell->objects;
				TCellObjects::iterator it = objs.find(obj_type);
				if ( it != objs.end() && ! it->second.empty() )
					del_objs.insert(del_objs.end(), it->second.begin(), it->second.end());
			}
			{
				T_CELLID	x = dst_cell_x + xx;
				T_CELLID	y = dst_cell_y + yy;
				PCell	pCell = getCell(x, y);
				if ( pCell.isNull() )	
					continue;
				TCellObjects &	objs = pCell->objects;
				TCellObjects::iterator it = objs.find(obj_type);
				if ( it != objs.end() && ! it->second.empty() )
					add_objs.insert(add_objs.end(), it->second.begin(), it->second.end());
			}
		}
		return true;
	}

	int i = 0;
	NOTIFY_CELL_MOVE_T&	cell_mov = s_NotifyCellMoveTable[dx+1][dy+1];

	for ( i = 0; i < 6; i ++ )
	{
		int xx = cell_mov.add_list[i].x;
		int yy = cell_mov.add_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		T_CELLID	x = cell_x + xx;
		T_CELLID	y = cell_y + yy;
		PCell	pCell = getCell(x, y);
		if ( pCell.isNull() )	
			continue;
		TCellObjects &	objs = pCell->objects;
		TCellObjects::iterator it = objs.find(obj_type);
		if ( it != objs.end() && ! it->second.empty() )
			add_objs.insert(add_objs.end(), it->second.begin(), it->second.end());
	}
	for ( i = 0; i < 6; i ++ )
	{
		int xx = cell_mov.del_list[i].x;
		int yy = cell_mov.del_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		T_CELLID	x = cell_x + xx;
		T_CELLID	y = cell_y + yy;
		PCell	pCell = getCell(x, y);
		if ( pCell.isNull() )	
			continue;

		TCellObjects &	objs = pCell->objects;
		TCellObjects::iterator it = objs.find(obj_type);
		if ( it != objs.end() && ! it->second.empty() )
			del_objs.insert(del_objs.end(), it->second.begin(), it->second.end());
	}
	return true;
}

#define	ADD_NOTIFY_PLAYERS(x, y)	\
	pCellT = getCell(x, y);	\
	if ( pCellT != NULL )	\
	{	\
		TCellPlayerIds &	players = pCellT->players;	\
		if ( ! players.empty() )	\
			notify_players.insert(notify_players.end(), players.begin(), players.end());	\
	}	\

void	CCellManager::getAroundCellPlayers(T_CELLID cell_x, T_CELLID cell_y, TCellPlayerIds& notify_players)
{
	CCell *	pCellT;
	ADD_NOTIFY_PLAYERS(cell_x-1, cell_y-1);
	ADD_NOTIFY_PLAYERS(cell_x, cell_y-1);
	ADD_NOTIFY_PLAYERS(cell_x+1, cell_y-1);
	ADD_NOTIFY_PLAYERS(cell_x-1, cell_y);
	ADD_NOTIFY_PLAYERS(cell_x, cell_y);
	ADD_NOTIFY_PLAYERS(cell_x+1, cell_y);
	ADD_NOTIFY_PLAYERS(cell_x-1, cell_y+1);
	ADD_NOTIFY_PLAYERS(cell_x, cell_y+1);
	ADD_NOTIFY_PLAYERS(cell_x+1, cell_y+1);
}

void	CCellManager::onNotifyPlayer(T_CELLPLAYERID playerid, ENUM_CELLNOTIFY_TYPE notify_type, TCellPlayerIds& notify_players)
{
	sipassert(m_cbNotifyPlayer != NULL);
	m_cbNotifyPlayer(m_CbArg, playerid, notify_type, notify_players);
}

void	CCellManager::onNotifyObject(T_CELLOBJTYPE obj_type, TCellObjIds& objs, ENUM_CELLNOTIFY_TYPE notify_type, TCellPlayerIds& notify_players)
{
	if ( objs.empty() )	return;
	sipassert(m_cbNotifyObject != NULL);
	m_cbNotifyObject(m_CbArg, obj_type, objs, notify_type, notify_players);
}

void	CCellManager::onNotifyTrigger(T_CELLPLAYERID playerid, T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT dist)
{
	sipassert(m_cbNotifyTrigger != NULL);
	m_cbNotifyTrigger(m_CbArg, playerid, obj_type, obj_id, dist);
}

bool	CCellManager::addPlayer(T_CELLPLAYERID playerid, T_MAPUNIT x, T_MAPUNIT y)
{
	if ( m_Players.find(playerid) != m_Players.end() )
		return false;

	T_CELLID cell_x = INVALID_CELLID, cell_y = INVALID_CELLID;
	if ( ! calcCellNum(x, y, &cell_x, &cell_y) )
		return false;

	PCell	pCell = getCell(cell_x, cell_y);
	if ( pCell.isNull() )
		return false;

	pCell->addPlayer(playerid);
	m_Players.insert( make_pair(playerid, CellPlayer(cell_x, cell_y, x, y)) );

	// Notify one info to one and others
	TCellPlayerIds	notify_chs;
	getAroundCellPlayers(cell_x, cell_y, notify_chs);
	onNotifyPlayer(playerid, TYPE_CELL_ENTER, notify_chs);

	// Notify info of objects to one
	TCellPlayerIds	mych;
	mych.push_back(playerid);
	NOTIFY_CELL_MOVE_T&	cell_mov = s_NotifyCellMoveTable[1][1];
	for ( TTotalObjects::iterator it = m_Objects.begin(); it != m_Objects.end(); it ++ )
	{
		T_CELLOBJTYPE obj_type = it->first;
		TCellObjIds	add_objs;
		for ( int i = 0; i < 9; i ++ )
		{
			int xx = cell_mov.mov_list[i].x;
			int yy = cell_mov.mov_list[i].y;
			if ( xx >= 10 || yy >= 10 )
				continue;
			T_CELLID	x = cell_x + xx;
			T_CELLID	y = cell_y + yy;
			PCell	cc = getCell(x, y);
			if ( cc.isNull() )
				continue;

			TCellObjects &	objs = cc->objects;
			TCellObjects::iterator it1 = objs.find(obj_type);
			if ( it1 != objs.end() )
				add_objs.insert(add_objs.end(), it1->second.begin(), it1->second.end());
		}
		onNotifyObject(obj_type, add_objs, TYPE_CELL_ADD, mych);
	}

	trigger(playerid, x, y);

	return true;
}

bool	CCellManager::delPlayer(T_CELLPLAYERID playerid)
{
	if ( m_Players.find(playerid) == m_Players.end() )
		return false;

	CellPlayer &	ch = m_Players[playerid];
	PCell	pCell = getCell(ch.cell_x, ch.cell_y);
	if ( pCell.isNull() )
		return false;

	// Notify to others
	TCellPlayerIds	notify_players;
	getAroundCellPlayers(ch.cell_x, ch.cell_y, notify_players);
	onNotifyPlayer(playerid, TYPE_CELL_LEAVE, notify_players);

	pCell->delPlayer(playerid);
	m_Players.erase(playerid);
	return true;
}

bool	CCellManager::movePlayer(T_CELLPLAYERID _playerid, T_MAPUNIT _x, T_MAPUNIT _y)
{
	TTotalPlayers::iterator it = m_Players.find(_playerid);
	if ( it == m_Players.end() )	return false;
	CellPlayer &	ch = it->second;
	T_CELLID cell_x0 = ch.cell_x, cell_y0 = ch.cell_y;

	T_CELLID cell_x = INVALID_CELLID, cell_y = INVALID_CELLID;
	if ( ! calcCellNum(_x, _y, &cell_x, &cell_y) )
		return false;

	// Notify to others
	bool	bCellChanged = ( cell_x != cell_x0 || cell_y != cell_y0 );
	if ( ! bCellChanged )
	{
		TCellPlayerIds	mov_players;
		getAroundCellPlayers(cell_x, cell_y, mov_players);
		onNotifyPlayer(_playerid, TYPE_CELL_MOV, mov_players);
	}
	else
	{
		TCellPlayerIds	add_players, del_players, mov_players;
		if ( getNotifyPlayers(cell_x0, cell_y0, cell_x, cell_y, add_players, del_players, mov_players) )
		{
			add_players.remove(_playerid);
			del_players.remove(_playerid);
			if ( mov_players.empty() )	mov_players.push_back(_playerid);

			onNotifyPlayer(_playerid, TYPE_CELL_ADD, add_players);
			onNotifyPlayer(_playerid, TYPE_CELL_DEL, del_players);
			onNotifyPlayer(_playerid, TYPE_CELL_MOV, mov_players);
		}
	}

	// Notify object info
	TCellPlayerIds	mych;
	mych.push_back(_playerid);
	for ( TTotalObjects::iterator it = m_Objects.begin(); it != m_Objects.end(); it ++ )
	{
		T_CELLOBJTYPE obj_type = it->first;
		TCellObjIds	add_objs, del_objs;
		if ( getNotifyObjects(obj_type, cell_x0, cell_y0, cell_x, cell_y, add_objs, del_objs) )
		{
			onNotifyObject(obj_type, add_objs, TYPE_CELL_ADD, mych);
			onNotifyObject(obj_type, del_objs, TYPE_CELL_DEL, mych);
		}
	}

	trigger(_playerid, _x, _y);

	ch.cell_x = cell_x, ch.cell_y = cell_y;
	ch.x = _x, ch.y = _y;

	//if according cell changed
	if ( bCellChanged )
	{
		PCell	pCell;
		pCell = getCell(cell_x0, cell_y0);
		if ( ! pCell.isNull() )	pCell->delPlayer(_playerid);
		pCell = getCell(cell_x, cell_y);
		if ( ! pCell.isNull() )	pCell->addPlayer(_playerid);
	}
	return true;
}

T_MAPUNIT	CCellManager::getTriggerRadius(T_CELLOBJTYPE obj_type)
{
	TTriggerRadius::iterator it = m_TriggerRadius.find(obj_type);
	if ( it != m_TriggerRadius.end() )
		return it->second;
	return 0;
}

bool	CCellManager::trigger(T_CELLPLAYERID _playerid, T_MAPUNIT _x, T_MAPUNIT _y)
{
	T_CELLID cell_x = INVALID_CELLID, cell_y = INVALID_CELLID;
	if ( ! calcCellNum(_x, _y, &cell_x, &cell_y) )
		return false;

	NOTIFY_CELL_MOVE_T&	cell_mov = s_NotifyCellMoveTable[1][1];

	for ( int i = 0; i < 9; i ++ )
	{
		int xx = cell_mov.mov_list[i].x;
		int yy = cell_mov.mov_list[i].y;
		if ( xx >= 10 || yy >= 10 )
			continue;
		PCell	pCell = getCell(cell_x + xx, cell_y + yy);
		if ( pCell.isNull() )	
			continue;

		TCellObjects &	objects = pCell->objects;
		for ( TCellObjects::iterator it1 = objects.begin(); it1 != objects.end(); it1 ++ )
		{
			T_CELLOBJTYPE	obj_type = it1->first;
			T_MAPUNIT	r = getTriggerRadius(obj_type);
			if ( r == 0 )	continue;

			TTypeObjects &	typeobjs = m_Objects[obj_type];
			TCellObjIds &	ids = it1->second;
			for ( TCellObjIds::iterator it2 = ids.begin(); it2 != ids.end(); it2 ++ )
			{
				T_CELLOBJID id = *it2;
				TTypeObjects::iterator it3 = typeobjs.find(id);
				if ( it3 == typeobjs.end() )
					continue;
				CellObject &	obj = it3->second;
				T_MAPUNIT	dx = _x - obj.x;
				T_MAPUNIT	dy = _y - obj.y;
				T_MAPUNIT	dist = dx * dx + dy * dy;
				if ( dist > r )		continue;

				onNotifyTrigger(_playerid, obj_type, id, dist);
			}
		}
	}
	return true;
}

CellObject *	CCellManager::getObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id)
{
	if ( m_Objects.find(obj_type) != m_Objects.end() )
	{
		TTypeObjects &	objs = m_Objects[obj_type];
		if ( objs.find(obj_id) != objs.end() )
			return &objs[obj_id];
	}
	return NULL;
}

bool	CCellManager::addObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT x, T_MAPUNIT y)
{
	if ( getObject(obj_type, obj_id) != NULL )
		return false;

	T_CELLID cell_x = INVALID_CELLID, cell_y = INVALID_CELLID;
	if ( ! calcCellNum(x, y, &cell_x, &cell_y) )
		return false;

	PCell	pCell = getCell(cell_x, cell_y);
	if ( pCell.isNull() )
		return false;

	pCell->addObject(obj_type, obj_id);
	m_Objects[obj_type][obj_id] = CellObject(cell_x, cell_y, x, y);

	// Notify to others
	TCellPlayerIds	notify_players;
	getAroundCellPlayers(cell_x, cell_y, notify_players);

	TCellObjIds	objs;
	objs.push_back(obj_id);
	onNotifyObject(obj_type, objs, TYPE_CELL_ENTER, notify_players);
	return true;
}

bool	CCellManager::delObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id)
{
	CellObject *	obj = getObject(obj_type, obj_id);
	if ( obj == NULL )
		return false;
	PCell	pCell = getCell(obj->cell_x, obj->cell_y);
	if ( pCell.isNull() )
		return false;

	// Notify to others
	TCellPlayerIds	notify_players;
	getAroundCellPlayers(obj->cell_x, obj->cell_y, notify_players);

	TCellObjIds	objs;
	objs.push_back(obj_id);
	onNotifyObject(obj_type, objs, TYPE_CELL_LEAVE, notify_players);

	pCell->delObject(obj_type, obj_id);
	if ( m_Objects.find(obj_type) != m_Objects.end() )
		m_Objects[obj_type].erase(obj_id);
	return true;
}

bool	CCellManager::moveObject(T_CELLOBJTYPE obj_type, T_CELLOBJID obj_id, T_MAPUNIT x, T_MAPUNIT y)
{
	CellObject *	obj = getObject(obj_type, obj_id);
	if ( obj == NULL )
		return false;

	T_CELLID cell_x0 = obj->cell_x, cell_y0 = obj->cell_y;
	T_CELLID cell_x = INVALID_CELLID, cell_y = INVALID_CELLID;
	if ( ! calcCellNum(x, y, &cell_x, &cell_y) )
		return false;

	// Notify to others
	TCellObjIds	objs;
	objs.push_back(obj_id);
	bool	bCellChanged = ( cell_x != cell_x0 || cell_y != cell_y0 );
	if ( ! bCellChanged )
	{
		TCellPlayerIds	mov_players;
		getAroundCellPlayers(cell_x, cell_y, mov_players);
		onNotifyObject(obj_type, objs, TYPE_CELL_MOV, mov_players);
	}
	else
	{
		TCellPlayerIds	add_players, del_players, mov_players;
		if ( getNotifyPlayers(cell_x0, cell_y0, cell_x, cell_y, add_players, del_players, mov_players) )
		{
			onNotifyObject(obj_type, objs, TYPE_CELL_ADD, add_players);
			onNotifyObject(obj_type, objs, TYPE_CELL_DEL, del_players);
			onNotifyObject(obj_type, objs, TYPE_CELL_MOV, mov_players);
		}
	}

	obj->cell_x = cell_x, obj->cell_y = cell_y;
	obj->x = x, obj->y = y;

	//if according cell changed
	if ( bCellChanged )
	{
		PCell	pCell;
		pCell = getCell(cell_x0, cell_y0);
		if ( ! pCell.isNull() )	pCell->delObject(obj_type, obj_id);
		pCell = getCell(cell_x, cell_y);
		if ( ! pCell.isNull() )	pCell->addObject(obj_type, obj_id);
	}
	return true;
}
