#if defined(SIP_OS_WINDOWS) && defined(_WINDOWS)
#	include <windows.h>
#	undef min
#	undef max
#endif

#include <misc/types_sip.h>
#include <string>
#include <vector>
#include <map>
#include <list>

#include <misc/smart_ptr.h>
#include "misc/ucstring.h"
#include "misc/common.h"
#include <net/service.h>
#include "../Common/Common.h"
#include "../../common/Macro.h"
#include "openroom.h"
#include "Cell.h"
#include "outRoomKind.h"
#include "Character.h"

#include "Animal.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

uint		CAnimal::m_nMoveStatistic = 80;
T_LEVEL		CAnimal::m_nLevelTh = 10;
T_DISTANCE	CAnimal::m_TriggerMinDist = 10;
T_DISTANCE	CAnimal::m_TriggerMaxDist = 100;

const	uint	REACTION_TIME = 3000;	//millisecond

CAnimal::CAnimal() :
	m_pScope(NULL), m_pCell(NULL), m_pParam(NULL),
	m_cbCh(NULL), m_CbArg(NULL),
	m_TargetChId(0),
	m_State(ANIMALSTATE_STOP)
{
	memset(&m_MoveObject, 0, sizeof(m_MoveObject));
}

CAnimal::~CAnimal()
{
}

bool	CAnimal::init(T_ANIMALID type, T_COMMON_ID id, REGIONSCOPE * scope)
{
	m_TypeID = type;
	m_ID = id;
	m_pScope = scope;
	m_pParam = GetOutAnimalParam(m_TypeID);
	if ( m_pParam == NULL )
		return false;

	float	randval = SIPBASE::frand(0, 1.0f);
//	m_Direction = randval * 6.0f;
	m_pScope->GetRandomPos(&m_PosX, &m_PosY, &m_PosZ);

	TTime	curtime = SIPBASE::CTime::getLocalTime();
	int	interval = m_pParam->MoveParam * 1000;
	m_NextMoveTime = (TTime)(curtime + interval + (int)(randval * interval * ANIMAL_MOVE_DELTATIME));
	sipassert( getCell() != NULL );
	getCell()->addObject(CELLOBJ_ANIMAL, m_ID, m_PosX * 1.0f/100, m_PosZ * 1.0f/100);
	T_DISTANCE	r = m_TriggerMaxDist * m_TriggerMaxDist;
	getCell()->setTriggerRadius(CELLOBJ_ANIMAL, r);
	return true;
}

bool	CAnimal::getCharacterInfo(T_CHARACTERID _chid, T_CH_STATE& _State, T_CH_DIRECTION& _Dir, T_CH_X& _X, T_CH_Y& _Y, T_CH_Z& _Z, T_LEVEL& _Level)
{
	sipassert(m_cbCh != NULL);
	return m_cbCh(m_CbArg, _chid, _State, _Dir, _X, _Y, _Z, _Level);
}

void	CAnimal::onCharacterMove(T_CHARACTERID chid)
{
	T_LEVEL			level;
	T_CH_STATE		state;
	T_CH_DIRECTION	dir;
	T_POS			x, y, z;
	if ( ! getCharacterInfo(chid, state, dir, x, y, z, level) )
		return;

	if ( state != moveStand )
	{
		if ( chid == m_TargetChId )
			m_TargetChId = 0;
		return;
	}

	bool	bFoundSeveral = ( m_TargetChId != 0 && chid != m_TargetChId );
	float	randvalue = SIPBASE::frand(0, 1.0f);
	if ( randvalue < 0.1f )
		return;
	if ( bFoundSeveral )
	{
		T_LEVEL	n1 = m_MoveObject.level, n2 = level;
		if ( randvalue < n1 * 1.0f / (n1 + n2) )
			return;
	}

	T_DISTANCE	dx = x - m_PosX;
	T_DISTANCE	dz = z - m_PosZ;
	T_DISTANCE dist = sqrt(dx * dx + dz * dz) + 0.01f;
	if ( dist > m_TriggerMaxDist )
		return;

	T_LEVEL	calc_level = (T_LEVEL)(level * (100 - (100 - m_nMoveStatistic) * randvalue) * 1.0f / 100);
	bool	bReplace = ( ! m_MoveObject.valid || ( m_MoveObject.valid && calc_level > m_MoveObject.level ) );
	T_DISTANCE	mindist = m_TriggerMinDist * (1 + randvalue);
	TTime	curtime = SIPBASE::CTime::getLocalTime();

	if ( dist < m_TriggerMinDist )
	{
		// deal with the moving close to player first
		if ( m_MoveObject.valid )			
			return;			
		if ( level >= m_nLevelTh )
		{
			if ( bReplace )
			{
				m_MoveObject.level = level;
				m_MoveObject.flag = MOVEFLAG_FLEE;
				m_MoveObject.des_X = x + m_TriggerMinDist * ( m_PosX - x ) / dist;
				m_MoveObject.des_Y = m_PosY;
				m_MoveObject.des_Z = z + m_TriggerMinDist * ( m_PosZ -z ) / dist;
				m_MoveObject.chid = chid;
				m_MoveObject.valid = true;
				m_NextMoveTime = 0;
				return;				
			}
			return;
		}
		else
		{
			if ( bReplace )
			{
				m_MoveObject.level = level;
				m_MoveObject.flag = MOVEFLAG_FLEE;
				m_MoveObject.des_X = x + m_TriggerMaxDist * ( m_PosX - x ) / dist;
				m_MoveObject.des_Y = m_PosY;
				m_MoveObject.des_Z = z + m_TriggerMaxDist * ( m_PosZ -z ) / dist;
				m_MoveObject.chid = chid;
				m_MoveObject.valid = true;
				m_NextMoveTime = 0;
				return;				
			}
			return;			
		}
	}
	else
	{
		if ( level >= m_nLevelTh )
		{
			// provide five destination position for choosing
			SIPBASE::CVector desPos1(x + mindist * ( m_PosX - x ) / dist, m_PosY, z + mindist * ( m_PosZ - z) / dist);
			SIPBASE::CVector desPos2;
			SIPBASE::CVector desPos3;
			SIPBASE::CVector desPos4;
			SIPBASE::CVector desPos5;

			if (x > m_PosX)
			{
				desPos2.set( x - mindist, m_PosY, z);
				desPos4.set( x + mindist, m_PosY, z);
			}
			else
			{
				desPos2.set( x + mindist, m_PosY, z);
				desPos4.set( x - mindist, m_PosY, z);
			}		

			if (m_PosZ > z)
			{
				desPos3.set(x, m_PosY, z + mindist);
				desPos5.set(x, m_PosY, z - mindist);
			}				
			else
			{
				desPos3.set(x, m_PosY, z - mindist);
				desPos5.set(x, m_PosY, z + mindist);
			}		

			std::vector<SIPBASE::CVector> desPosArray;
			desPosArray.push_back(desPos1);
			desPosArray.push_back(desPos2);
			desPosArray.push_back(desPos3);
			desPosArray.push_back(desPos4);
			desPosArray.push_back(desPos5);
			int index = (int)( 100 * randvalue ) % 5;
			
			if ( bReplace )
			{
				m_MoveObject.level = level;
				m_MoveObject.flag = MOVEFLAG_APPROACH;
				m_MoveObject.des_X = desPosArray[index].x;
				m_MoveObject.des_Y = desPosArray[index].y;
				m_MoveObject.des_Z = desPosArray[index].z;
				m_MoveObject.chid = chid;
				m_MoveObject.valid = true;
				m_NextMoveTime = (TTime)(curtime + REACTION_TIME * randvalue);
				return;
			}
			else
			{
				if ( m_MoveObject.flag == MOVEFLAG_FLEE )
				{
					m_MoveObject.level = level;
					m_MoveObject.flag = MOVEFLAG_APPROACH;
					m_MoveObject.des_X = desPosArray[index].x;
					m_MoveObject.des_Y = desPosArray[index].y;
					m_MoveObject.des_Z = desPosArray[index].z;
					m_MoveObject.chid = chid;
					m_MoveObject.valid = true;
					m_NextMoveTime = (TTime)(curtime + REACTION_TIME * randvalue);
					return;
				}
			}
			return;
		}
		else
		{
			// deal with the moving close to player first
			if ( m_MoveObject.valid )
			{
				return;
			}
			else
			{
				if ( bReplace )
				{
					m_MoveObject.level = level;
					m_MoveObject.flag = MOVEFLAG_FLEE;
					m_MoveObject.des_X = x + m_TriggerMaxDist * ( m_PosX - x ) / dist;
					m_MoveObject.des_Y = m_PosY;
					m_MoveObject.des_Z = z + m_TriggerMaxDist * ( m_PosZ -z ) / dist;
					m_MoveObject.chid = chid;
					m_MoveObject.valid = true;
					m_NextMoveTime = 0;
					return;				
				}
				else
					return;
			}
		}
	}
}

void	CAnimal::onUpdate(SIPBASE::TTime curtime)
{
	if ( curtime < m_NextMoveTime )
		return;
	m_State = ANIMALSTATE_STOP;
	int	interval = m_pParam->MoveParam * 1000;
	m_NextMoveTime = (TTime)(curtime + interval + Irand(0, (int)(interval * ANIMAL_MOVE_DELTATIME)));

	if ( m_MoveObject.valid )
	{
		m_MoveObject.valid = false;
		if ( ! m_pScope->IsInScope(m_MoveObject.des_X, m_MoveObject.des_Y, m_MoveObject.des_Z) )
			return;

//		T_POSZ	deltaZ = (m_MoveObject.des_Z - m_PosZ) * (-1.0f);
//		T_POSX	deltaX = (m_MoveObject.des_X - m_PosX) * (-1.0f);
//		if ( deltaZ == 0 )
//			m_Direction = 0.0f;
//		else
//			m_Direction = atan2(deltaX, deltaZ);
		m_PosX = m_MoveObject.des_X;
		m_PosY = m_MoveObject.des_Y;
		m_PosZ = m_MoveObject.des_Z;
		m_TargetChId = m_MoveObject.chid;
	}
	else
	{
		T_LEVEL			level;
		T_CH_STATE		state;
		T_CH_DIRECTION	dir;
		T_POS			x, y, z;
		if ( ! getCharacterInfo(m_TargetChId, state, dir, x, y, z, level) || m_TargetChId == 0 )		
		{
			sipassert(m_pScope != NULL);
			T_POSX	NextX;	T_POSY	NextY;	T_POSZ	NextZ;
			if ( ! m_pScope->GetMoveablePos(m_PosX, m_PosY, m_PosZ, m_pParam->MaxMoveDis, &NextX, &NextY, &NextZ) )
				return;

//			T_POSZ	deltaZ = (NextZ - m_PosZ) * (-1.0f);
//			T_POSX	deltaX = (NextX - m_PosX) * (-1.0f);
//			if ( deltaZ == 0 )
//				m_Direction = 0.0f;
//			else
//				m_Direction = atan2(deltaX, deltaZ);
			m_PosX = NextX;
			m_PosY = NextY;
			m_PosZ = NextZ;
		}
		else
		{
			T_DISTANCE moveRadius = m_TriggerMaxDist / 2;
			T_POSX	NextX;	T_POSZ	NextZ;
			T_POSY	NextY = m_PosY;
			NextX = SIPBASE::frand( x - moveRadius, x + moveRadius );
			NextZ = SIPBASE::frand( z - moveRadius, z + moveRadius );
			if ( ! m_pScope->IsInScope(NextX, m_PosY, NextZ) )
			{
				if ( ! m_pScope->GetMoveablePos(m_PosX, m_PosY, m_PosZ, m_pParam->MaxMoveDis, &NextX, &NextY, &NextZ) )
					return;
			}
//			T_POSZ	deltaZ = (NextZ - m_PosZ) * (-1.0f);
//			T_POSX	deltaX = (NextX - m_PosX) * (-1.0f);
//			if ( deltaZ == 0 )
//				m_Direction = 0.0f;
//			else
//				m_Direction = atan2(deltaX, deltaZ);
			m_PosX = NextX;	
			m_PosY = NextY;
			m_PosZ = NextZ;		
		}	
	}

	move(m_PosX, m_PosZ);
	m_MoveObject.level = 0;
	m_State = ANIMALSTATE_MOVE;
}

void	CAnimal::move(T_POSX x, T_POSZ z)
{
	sipassert(m_pCell != NULL);
	getCell()->moveObject(CELLOBJ_ANIMAL, m_ID, x, z);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

CRabbit::CRabbit()
{
}

CRabbit::~CRabbit()
{
}

void	CRabbit::onCharacterMove(T_CHARACTERID chid)
{
}

void	CRabbit::onUpdate(SIPBASE::TTime curtime)
{
}
