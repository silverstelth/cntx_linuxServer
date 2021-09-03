#pragma once

#include "misc/vector.h"
#include "LoadOutData.h"

// 움직이는 동물파라메터
struct	OUT_ANIMALPARAM
{
	T_ANIMALID		TypeID;					// 종류ID
	uint32			MoveParam;				// 움직임특성을 나타내는 값
	T_DISTANCE		MaxMoveDis;				// 한번에 움직이는 최대거리
};
typedef		std::map<T_ANIMALID,	OUT_ANIMALPARAM>					MAP_OUT_ANIMALPARAM;
extern		MAP_OUT_ANIMALPARAM	map_OUT_ANIMALPARAM;
extern		bool				LoadAnimalParam(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern		OUT_ANIMALPARAM		*GetOutAnimalParam(T_ANIMALID _ID);

// 悼拱框流烙飞开
struct	REGIONSPACE
{
	enum REGIONSPACE_TYPE {SPACE_NONE, SPACE_BOX, SPACE_CYL, SPACE_SPH};
	REGIONSPACE_TYPE	m_RegionType;
	SIPBASE::CVector	m_CenterPos;
	float				m_Rot1, m_Rot2, m_Rot3, m_Rot4;
	SIPBASE::CVector	m_Size;
	uint32				m_UserData;

	REGIONSPACE() : m_UserData(0), m_Rot1(1.0f), m_Rot2(0), m_Rot3(0), m_Rot4(0) {};

	void				GetRandomPos(T_POSX *_X, T_POSY *_Y, T_POSZ *_Z)
	{
		SIPBASE::CVector radius = m_Size / 2;
		SIPBASE::CVector vPos(0,0,0);
		switch( m_RegionType )
		{
		case SPACE_BOX:
			{
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.x)) );
				vPos.x = SIPBASE::frand(-radius.x, radius.x);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.y)) );
				vPos.y = SIPBASE::frand(-radius.y, radius.y);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.z)) );
				vPos.z = SIPBASE::frand(-radius.z, radius.z);
				break;
			}
		case SPACE_SPH:
			{
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.y)) );
				float yaw = SIPBASE::frand(0, (float)(4 * SIPBASE::Pi));
				SIPBASE::Quaternion qYaw(yaw, SIPBASE::CVector(0,1,0));

				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.x)) );
				float pitch = SIPBASE::frand(0, (float)(4 * SIPBASE::Pi));
				SIPBASE::Quaternion qPitch(pitch, SIPBASE::CVector(1,0,0));

				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.z)) );
				float roll = SIPBASE::frand(0, (float)(4 * SIPBASE::Pi));
				SIPBASE::Quaternion qRoll(roll, SIPBASE::CVector(0,0,1));

				SIPBASE::Quaternion qRand = qYaw * qPitch * qRoll;

				vPos = qRand * SIPBASE::CVector(0,0,-1);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.x)) );
				vPos.x *= SIPBASE::frand(0, radius.x);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.y)) );
				vPos.y *= SIPBASE::frand(0, radius.y);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.z)) );
				vPos.z *= SIPBASE::frand(0, radius.z);

				break;
			}
		case SPACE_CYL:
			{
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.y)) );
				float yaw = SIPBASE::frand(0, (float)(4 * SIPBASE::Pi));
				SIPBASE::Quaternion qYaw(yaw, SIPBASE::CVector(0,1,0));
				vPos = qYaw * SIPBASE::CVector(0,0,-1);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.x)) );
				vPos.x *= SIPBASE::frand(0, radius.x);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.z)) );
				vPos.z *= SIPBASE::frand(0, radius.z);
				//srand( (uint)(SIPBASE::CTime::getLocalTime() * (uint)(radius.y)) );
				vPos.y = SIPBASE::frand(-radius.y, radius.y);

				break;
			}
		default:
			break;
		}
		vPos = SIPBASE::Quaternion(m_Rot1, m_Rot2, m_Rot3, m_Rot4) * vPos;
		vPos += m_CenterPos;
		*_X = vPos.x;	*_Y = vPos.y;	*_Z = vPos.z;
	}
	bool containPoint(const SIPBASE::CVector& point) const
	{
		SIPBASE::CVector radius = m_Size / 2;
		SIPBASE::CVector pt = point;
		// 痢狼 谅钎甫 Space狼 傍埃谅钎拌肺 函券茄促.
		pt -= m_CenterPos;
		SIPBASE::Quaternion qRot(m_Rot1, m_Rot2, m_Rot3, m_Rot4);
		pt = qRot.Inverse() * pt;
		switch ( m_RegionType )
		{
		case SPACE_BOX:
			{
				// xz谅钎蔼父 八荤茄促.
				if ( pt.x > -radius.x && pt.x < radius.x 
					&& pt.z > -radius.z && pt.z < radius.z )
					return true;
				break;
			}
		case SPACE_SPH:
			{
				// Sphere牢版快绰 xyz谅钎甫 农扁肺 唱穿菌阑锭 弊氦配福农扁啊 1焊促 奴啊甫 八荤茄促.
				pt /= radius;
				if ( pt.length() < 1 )
					return true;

				break;
			}
		case SPACE_CYL:
			{
				// 八荤窍妨绰 痢阑 xz乞搁俊 捧康矫虐绊 弊 捧康痢捞 xz馆版救俊 甸绢坷绰啊甫 八荤茄促.
				pt.y = 0;
				pt /= radius;
				if ( pt.length() < 1 )
					return true;
				break;
			}
		default:
			break;
		}

		return false;
	}
};
#define		UNKNOWN_POS						1000000.0
typedef	std::vector<REGIONSPACE>			LST_REGIONSPACE;
struct	REGIONSCOPE
{
	std::string			m_Name;
	LST_REGIONSPACE		m_lstSpace;
/*	SIPBASE::CVector	m_CenterPos;

	void	CalcCenterPos()
	{
		float	min_x = 1000000.0f, min_y = 1000000.0f, min_z = 1000000.0f;
		float	max_x = -1000000.0f, max_y = -1000000.0f, max_z = -1000000.0f;

		LST_REGIONSPACE::iterator it;
		for ( it = m_lstSpace.begin(); it != m_lstSpace.end(); it ++ )
		{
			REGIONSPACE &	sp = *it;
			float	x1, y1, z1, x2, y2, z2;
			x1 = sp.m_CenterPos.x - sp.m_Size.x / 2;
			y1 = sp.m_CenterPos.y - sp.m_Size.y / 2;
			z1 = sp.m_CenterPos.z - sp.m_Size.z / 2;
			x2 = sp.m_CenterPos.x + sp.m_Size.x / 2;
			y2 = sp.m_CenterPos.y + sp.m_Size.y / 2;
			z2 = sp.m_CenterPos.z + sp.m_Size.z / 2;
			if ( x1 < min_x )	min_x = x1;
			if ( y1 < min_y )	min_y = y1;
			if ( z1 < min_z )	min_z = z1;
			if ( x2 > max_x )	max_x = x2;
			if ( y2 > max_y )	max_y = y2;
			if ( z2 > max_z )	max_z = z2;
		}
		m_CenterPos.x = min_x + (max_x - min_x) / 2;
		m_CenterPos.y = min_y + (max_y - min_y) / 2;
		m_CenterPos.z = min_z + (max_z - min_z) / 2;
	}

	void	GetCenterPos(T_POSX *_X, T_POSY *_Y, T_POSZ *_Z)
	{
		if ( _X )	*_X = m_CenterPos.x;
		if ( _Y )	*_Y = m_CenterPos.y;
		if ( _Z )	*_Z = m_CenterPos.z;
	}
*/
	void	GetRandomPos(T_POSX *_X, T_POSY *_Y, T_POSZ *_Z)
	{
		uint32 uNum = static_cast<uint32>(m_lstSpace.size());
		if (uNum < 1)
			return;
		int nCandi = SIPBASE::Irand(0, (int)(uNum-1));
		if (nCandi < 0 || nCandi >= (int)uNum)
			nCandi = 0;
		int nCur = 0;
		LST_REGIONSPACE::iterator it;
		for(it = m_lstSpace.begin(); it != m_lstSpace.end(); it++)
		{
			if (nCur == nCandi)
			{
				(*it).GetRandomPos(_X, _Y, _Z);
				return;
			}
			nCur ++;
		}
	}

	// *_NextX != UNKNOWN_POS 捞搁 *_NextX, *_NextY, *_NextZ 困摹俊 Family啊 乐栏骨肺 弊 困摹俊辑 档噶媚具 茄促.
	bool	GetMoveablePos(T_POSX _X, T_POSY _Y, T_POSZ _Z, T_DISTANCE _maxDis, T_POSX *_NextX, T_POSY *_NextY, T_POSZ *_NextZ) const
	{
		T_POSX		familyPosX = *_NextX, familyPosY = *_NextY, familyPosZ = *_NextZ;

		T_DISTANCE	fMoveX, fMoveY, fMoveZ;

		// familyPosX != UNKNOWN_POS 牢 版快俊父 蜡侩窍促.
		float		vx, vz;
		if(familyPosX != UNKNOWN_POS)
		{
			vx = _X - familyPosX;
			vz = _Z - familyPosZ;
			float	abs_vx = abs(vx), abs_vz = abs(vz), abs_max;
			if(abs_vx < 1 && abs_vz < 1)
			{
				// 某腐磐客 悼拱捞 呈公 啊鳖捞俊 乐绰 版快俊绰 酒公 规氢栏肺 啊档废 茄促.
				familyPosX = UNKNOWN_POS;
			}
			else
			{
				// vx客 vz甫 沥痹拳茄促.
				abs_max = (abs_vx > abs_vz) ? abs_vx : abs_vz;
				vx /= abs_max;
				vz /= abs_max;
			}
		}

		// 泅犁 悼拱捞 器窃登咯乐绰 space甸阑 茫绰促.
		LST_REGIONSPACE		lstAvailSpace;
		LST_REGIONSPACE::const_iterator it;
		for ( it = m_lstSpace.begin(); it != m_lstSpace.end(); it++ )
		{
			if ( (*it).containPoint(SIPBASE::CVector(_X, _Y, _Z)) )
				lstAvailSpace.push_back(*it);
		}

		const int MAX_CALC_COUNT = 3;
		for(int	count = 0; count < MAX_CALC_COUNT; count ++)
		{
			if(familyPosX == UNKNOWN_POS)
			{
				fMoveX = SIPBASE::frand(-_maxDis, _maxDis);
				fMoveY = 0.0f;
				fMoveZ = SIPBASE::frand(-_maxDis, _maxDis);
				*_NextX = _X + fMoveX;
				*_NextY = _Y + fMoveY;
				*_NextZ = _Z + fMoveZ;
			}
			else
			{
				// 荤恩栏肺何磐 档噶啊绰 谅钎甫 急琶秦具 茄促.
				fMoveX = SIPBASE::frand(0, _maxDis);
				fMoveZ = SIPBASE::frand(-_maxDis, _maxDis);
				*_NextX = _X + vx * fMoveX - vz * fMoveZ;
				*_NextY = _Y;
				*_NextZ = _Z + vz * fMoveX + vx * fMoveZ;
			}

			for ( it = lstAvailSpace.begin(); it != lstAvailSpace.end(); it++ )
			{
				if ( (*it).containPoint(SIPBASE::CVector(*_NextX, *_NextY, *_NextZ)) )
					return true;
			}
		}

		lstAvailSpace.clear();

		return false;
	}

	bool	IsInScope(T_POSX x, T_POSY y, T_POSZ z) const
	{
		LST_REGIONSPACE::const_iterator it;
		for(it = m_lstSpace.begin(); it != m_lstSpace.end(); it++)
		{
			if ( (*it).containPoint(SIPBASE::CVector(x, y, z)) )
				return true;
		}
		return false;
	}
};
typedef	std::map<T_PETREGIONID, REGIONSCOPE>		MAP_REGIONSCOPE;

struct	ROOMANIMALBASE
{
	T_COMMON_ID			m_ID;
	T_POSX				m_PosX;			
	T_POSY				m_PosY;			
	T_POSZ				m_PosZ;			
	T_CH_DIRECTION		m_Direction;	
};
struct	ROOMANIMAL : public ROOMANIMALBASE
{
	T_ANIMALID			m_TypeID;
	T_COMMON_ID			m_RegionID;
	uint8				m_ServerControlType;
	T_ANIMALSTATE		m_State;		
	SIPBASE::MSSQLTIME	m_NextMoveTime;	
	T_FAMILYID			m_SelFID;		
	ROOMANIMAL() : m_SelFID(0) {};
};
typedef	std::map<T_COMMON_ID, ROOMANIMAL>				MAP_ROOMANIMAL;
typedef	std::map<T_COMMON_ID, ROOMANIMALBASE>			MAP_ROOMEDITANIMAL;

struct ROOMEDITANIMALACTIONINFO 
{
	T_COMMON_ID				m_GroupID;
	std::vector<uint8>		m_ActionInfo;
};
typedef	std::map<T_COMMON_ID, ROOMEDITANIMALACTIONINFO>			MAP_ROOMEDITANIMALACTIONINFO;

struct OUT_ROOMKIND
{
	OUT_ROOMKIND() : EditAnimalIniFlag_PC(false), EditAnimalIniFlag_Mobile(false) {}
	T_MSTROOM_ID							ID;						// 종류id
	T_MSTROOM_CHNUM							MaxChars;				// 최대허용캐릭수
//	T_MSTROOM_CHNUM							OwnChars;				// 주인허용캐릭수
//	T_MSTROOM_CHNUM							KinChars;				// 친인허용캐릭수
//	T_MSTROOM_CHNUM							FriendChars;			// 친구허용캐릭수
//	T_MSTROOM_CHNUM							NumChars;				// 기타허용캐릭수
	uint32									LiuYanDesks;			// 방문록남기기 책상수
	uint32									BlessLampCount;			// 길상등 수
	uint32									AncestorDeceasedCount;	// 조상탑 고인수

	// for C_PC
	MAP_REGIONSCOPE							RoomRegion_PC;				// region scope자료
	MAP_ROOMANIMAL							RoomAnimalDefault_PC;		// 동물들의 초기위치
	// for Unity_PC
	MAP_REGIONSCOPE							RoomRegion_PC_Unity;				// region scope자료
	MAP_ROOMANIMAL							RoomAnimalDefault_PC_Unity;		// 동물들의 초기위치
	// for Mobile
	MAP_REGIONSCOPE							RoomRegion_Mobile;				// mobile region scope자료
	MAP_ROOMANIMAL							RoomAnimalDefault_Mobile;		// mobile 동물들의 초기위치	

	std::string GetRegionName(T_PETREGIONID pid, int RegionType)	// 0 : C Veirsion for PC, 1 : Mobile, 2 : Unity Version for PC   by krs
	{
		if (RegionType == 0)	// C Version for PC
		{
			MAP_REGIONSCOPE::iterator itregion = RoomRegion_PC.find(pid);
			if (itregion == RoomRegion_PC.end())
				return "";
			return itregion->second.m_Name;
		}
		if (RegionType == 2)	// Unity Version for PC  by krs
		{
			MAP_REGIONSCOPE::iterator itregion = RoomRegion_PC_Unity.find(pid);
			if (itregion == RoomRegion_PC_Unity.end())
				return "";
			return itregion->second.m_Name;
		}

		// Mobile
		MAP_REGIONSCOPE::iterator itregion = RoomRegion_Mobile.find(pid);
		if (itregion == RoomRegion_Mobile.end())
			return "";
		return itregion->second.m_Name;
	}

	const REGIONSCOPE*		GetRegionScope(T_PETREGIONID pid, int RegionType)	// 0 : C_PC, 1 : Mobile, 2 : Unity_PC by krs
	{
		if (RegionType == 0)	// PC
		{
			MAP_REGIONSCOPE::iterator itregion = RoomRegion_PC.find(pid);
			if (itregion == RoomRegion_PC.end())
				return NULL;
			return &(itregion->second);
		}
		if (RegionType == 2)	// Unity for PC  by krs
		{
			MAP_REGIONSCOPE::iterator itregion = RoomRegion_PC_Unity.find(pid);
			if (itregion == RoomRegion_PC_Unity.end())
				return NULL;
			return &(itregion->second);
		}

		// Mobile
		MAP_REGIONSCOPE::iterator itregion = RoomRegion_Mobile.find(pid);
		if (itregion == RoomRegion_Mobile.end())
			return NULL;
		return &(itregion->second);
	}

	// for PC
	bool									EditAnimalIniFlag_PC;
	MAP_ROOMEDITANIMAL						RoomEditAnimalDefault_PC;
	MAP_ROOMEDITANIMALACTIONINFO			RoomEditAnimalActionInfo_PC;
	// for Mobile
	bool									EditAnimalIniFlag_Mobile;
	MAP_ROOMEDITANIMAL						RoomEditAnimalDefault_Mobile;
	MAP_ROOMEDITANIMALACTIONINFO			RoomEditAnimalActionInfo_Mobile;
};
typedef		std::map<T_MSTROOM_ID,	OUT_ROOMKIND>					MAP_OUT_ROOMKIND;
extern		MAP_OUT_ROOMKIND			map_OUT_ROOMKIND;

extern	bool				LoadRoomKind(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	T_MSTROOM_CHNUM		GetMaxCharacterOfRoomKind(T_MSTROOM_ID _id);
extern	const OUT_ROOMKIND	*GetOutRoomKind(T_MSTROOM_ID _ID);
extern	uint32				GetMaxAncestorDeceasedCount(T_MSTROOM_ID _id);


// 방의 페트지역에서 키울수 있는 최대 페트수
//struct OUT_ROOMPETMAX
//{
//	T_MSTROOM_ID							RoomID;
//	T_PETREGIONID							RegionID;
//	T_PETCOUNT								MaxCount;
//};
//typedef		std::list<OUT_ROOMPETMAX>		LST_OUT_ROOMPETMAX;
//extern		LST_OUT_ROOMPETMAX				lst_OUT_ROOMPETMAX;
//extern		bool LoadRoomPetMax(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
//extern		T_PETCOUNT	GetRoomPetMax(T_MSTROOM_ID _RoomID, T_PETREGIONID _RegionID);

//// Default로 생성하여야 하는 페트목록
//struct	OUT_ROOMDEFAULTPET
//{
//	T_MSTROOM_ID							RoomID;
//	T_PETREGIONID							RegionID;
//	T_PETID									PetID;
//	T_PETCOUNT								PetNum;
//};
extern		bool LoadRoomDefaultPet(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);

extern		bool LoadRoomScope(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);

// 페트자료(PARAM)
//struct	OUT_PETPARAM
//{
//	T_PETINCLIFEPER							IncLifePer;					// 생명력증가비률
//	T_PETLIFE								OriginalLife;				// 초기생명값
//	T_PETLIFE								DecLife;					// 생명력감소량
//	uint32									DecInterval;				// 생명력감소기간(초)
//};
//typedef		std::map<T_PETGROWSTEP,	OUT_PETPARAM>					MAP_OUT_PETPARAM;

// 페트쎄트
struct	OUT_PETSET
{
	T_PETTYPEID								TyepID;						// 페트종류
	//T_PETGROWSTEP							OriginStep;					// 초기단계
	//T_PETLIFE								OriginLife;					// 초기생명력
	//MAP_OUT_PETPARAM						MapPetParam;				// 페트속성

	//T_PETLIFE		GetOriginalLife(T_PETGROWSTEP _Step)
	//{
	//	MAP_OUT_PETPARAM::iterator it;
	//	for (it = MapPetParam.begin(); it != MapPetParam.end(); it++)
	//	{
	//		if (_Step == it->first)
	//			return it->second.OriginalLife;
	//	}
	//	return 0;
	//}
	//T_PETGROWSTEP	OwnStep(T_PETLIFE _CurLife)
	//{
	//	T_PETGROWSTEP	FindStep = OriginStep;
	//	T_PETLIFE		MinLifeDelta;
	//	bool bFirst = true;
	//	MAP_OUT_PETPARAM::iterator it;
	//	for (it = MapPetParam.begin(); it != MapPetParam.end(); it++)
	//	{
	//		if (_CurLife < it->second.OriginalLife)
	//			continue;
	//		if (bFirst)
	//		{
	//			FindStep = it->first;
	//			MinLifeDelta = _CurLife - it->second.OriginalLife;
	//			bFirst = false;
	//			continue;
	//		}
	//		T_PETLIFE	Delta = _CurLife - it->second.OriginalLife;
	//		if (Delta <= MinLifeDelta)
	//		{
	//			FindStep = it->first;
	//			MinLifeDelta = Delta;
	//		}
	//	}
	//	return FindStep;
	//}
	//T_PETGROWSTEP	LowestStep(T_PETGROWSTEP _CurStep)
	//{
	//	T_PETGROWSTEP LimitStep;
	//	if (_CurStep % 10 != 0)
	//		LimitStep = (_CurStep / 10) * 10;
	//	else
	//		LimitStep = _CurStep - 10;
	//	
	//	T_PETGROWSTEP loweststep = _CurStep;
	//	T_PETGROWSTEP maxdelta = 0;
	//	MAP_OUT_PETPARAM::iterator it;
	//	for (it = MapPetParam.begin(); it != MapPetParam.end(); it++)
	//	{
	//		if (it->first <= LimitStep || it->first >= _CurStep)
	//			continue;
	//		if (_CurStep - it->first >= maxdelta)
	//		{
	//			maxdelta = _CurStep - it->first;
	//			loweststep = it->first;
	//		}
	//	}
	//	return loweststep;
	//}
	//T_PETGROWSTEP	NextStep(T_PETGROWSTEP _CurStep, T_PETLIFE &_CurLife)
	//{
	//	T_PETGROWSTEP	NextPetStep = OwnStep(_CurLife);
	//	if (NextPetStep == _CurStep)
	//		return NextPetStep;

	//	bool	bGrowStep = false;
	//	if (_CurStep % 10 == 0)
	//		bGrowStep = true;

	//	T_PETGROWSTEP milestonestep = _CurStep / 10;
	//	if (bGrowStep)	// 성장단계이면
	//	{
	//		if (NextPetStep >= _CurStep + 10)
	//		{
	//			T_PETGROWSTEP temp = _CurStep + 10;
	//			_CurLife = GetOriginalLife(temp);
	//			return temp;
	//		}
	//		if (NextPetStep > _CurStep)
	//			return _CurStep;
	//		// 작은 단계이면
	//		if (NextPetStep > _CurStep - 10)
	//			return NextPetStep;
	//		return LowestStep(_CurStep);
	//	}
	//	else
	//	{
	//		if (NextPetStep >= ((milestonestep+1) * 10))
	//		{
	//			T_PETGROWSTEP temp = (milestonestep+1) * 10;
	//			_CurLife = GetOriginalLife(temp);
	//			return (temp);
	//		}
	//		if (NextPetStep > _CurStep)
	//			return NextPetStep;
	//		if (NextPetStep > milestonestep * 10)
	//			return NextPetStep;
	//		return LowestStep((milestonestep+1) * 10);
	//	}
	//	return NextPetStep;
	//}
};
typedef		std::map<T_PETID,	OUT_PETSET>					MAP_OUT_PETSET;
extern	MAP_OUT_PETSET	map_OUT_PETSET;
extern	bool				LoadPetData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	OUT_PETSET			*GetOutPetSet(T_PETID _ID);
//extern	OUT_PETPARAM		*GetOutPetParam(T_PETID _ID, T_PETGROWSTEP _Step);

// 페트가꾸기자료
struct	OUT_PETCARE
{
	T_PETCAREID		CareID;					// 아이템ID
	T_PETTYPEID		PetType;				// 페트종류
	T_PETLIFE		IncLife;				// 생명력증가량
};
typedef		std::map<T_PETCAREID,	OUT_PETCARE>					MAP_OUT_PETCARE;
extern	MAP_OUT_PETCARE		map_OUT_PETCARE;
extern	bool				LoadPetCare(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	OUT_PETCARE			*GetOutPetCare(T_PETCAREID _ID);

// 신비문출현목록
// 신비문출현목록
//struct OUT_ROOMTGATE
//{
//	T_MSTROOM_ID		RoomID;				// 방id
//	T_ROOMVIRTUE		RoomVirtue;			// 방의 공덕값
//	float				GateFre;			// 출현빈도수
//	uint32				GateTime;			// 출현시간(초)
//	std::string			RegionName;			// 령역이름
//};
//typedef	std::list<OUT_ROOMTGATE>		LST_OUT_ROOMTGATE;
//extern		LST_OUT_ROOMTGATE			lst_OUT_ROOMTGATE;
//extern		bool				LoadRoomGate(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);

//enum	ENUM_SERVERSCOPE_TYPE {
//	POSTYPE_MYSTICROOM = 1,
//	POSTYPE_ANIMAL = 2,
//	POSTYPE_XUYUAN,
//};
//struct OUT_SERVERSCOPE
//{
//	T_MSTROOM_ID			SceneID;
//	ENUM_SERVERSCOPE_TYPE	Type;
//	std::string				ScopeName;
//};
//typedef	std::vector<OUT_SERVERSCOPE>	LST_OUT_SERVERSCOPE;
//extern		LST_OUT_SERVERSCOPE			lst_OUT_SERVERSCOPE;
//extern		bool				LoadServerScope(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);

// 모델-성별대응표
typedef	std::map<uint32, uint32>	MAP_OUT_MODELSEX;
extern		MAP_OUT_MODELSEX		map_OUT_MODELSEX;
extern		bool		LoadModelSex(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern		uint32		GetModelSex(uint32 ModelID);

// Luck자료
struct OUT_LUCK
{
	uint32		LuckID;
	uint32		LuckPercent;
	uint8		LuckLevel;
};
typedef	std::vector<OUT_LUCK>	LST_OUT_LUCK;
extern		LST_OUT_LUCK			lst_OUT_LuckData;
extern	bool		LoadLuckData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	OUT_LUCK	*GetLuck(bool bPublicRoom = false);
//extern	OUT_LUCK	*GetLuck4();

// Prize자료
struct OUT_PRIZE
{
	uint32		PrizeID;
	uint8		PrizeLevel;
	uint32		PrizeItemID;
};
typedef	std::vector<OUT_PRIZE>	LST_OUT_PRIZE;
extern		LST_OUT_PRIZE			lst_OUT_PrizeData;
extern	bool		LoadPrizeData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	OUT_PRIZE	*GetPrizeData(uint32 PrizeID);
extern	void		CheckValidItemData();

struct OUT_PRIZE_LEVEL
{
	uint8		PrizeLevel;
	std::vector<uint32>	PrizeIDs;
};
typedef	std::vector<OUT_PRIZE_LEVEL>	LST_OUT_PRIZE_LEVEL;
extern		LST_OUT_PRIZE_LEVEL			lst_OUT_PrizeLevelData;
extern	OUT_PRIZE	*GetPrize(uint8 PrizeLevel);
extern	int GetPrizeItemLevel(uint32 ItemID);

// 종교구역자료
struct OUT_RELIGIONROOM
{
	uint32		ReligionRoomID;
	uint16		ReligionRoomSceneID;
};
typedef	std::vector<OUT_RELIGIONROOM>	LST_OUT_RELIGIONROOM;
extern		LST_OUT_RELIGIONROOM		lst_OUT_RELIGIONROOM;
extern		bool	LoadReligionRoom(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
uint16		GetReligionRoomSceneID(uint32 RRoomID);

// AutoPlay를 위한 Taocan(종합)아이템자료
struct OUT_TAOCAN_FOR_AUTOPLAY
{
	uint32		TaocanType;
	uint32		ItemIDs[9];
};
typedef	std::map<uint32, OUT_TAOCAN_FOR_AUTOPLAY>	MAP_OUT_TAOCAN_FOR_AUTOPLAY;
extern		MAP_OUT_TAOCAN_FOR_AUTOPLAY	map_OUT_TAOCAN_FOR_AUTOPLAY;
extern		bool	LoadTaocanForAutoplay(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
OUT_TAOCAN_FOR_AUTOPLAY*		GetTaocanItemInfo(uint32 TaocanItemID);
uint32		GetMinPriceOfTaocanForYishi();
uint32		GetMinPriceOfTaocanForXianbao();

// PointCard자료
struct OUT_POINTCARD
{
	uint32		CardID;
	uint32		AddPoint;
};
typedef	std::vector<OUT_POINTCARD>	LST_OUT_POINTCARD;
extern		LST_OUT_POINTCARD		lst_OUT_PointCardData;
extern	bool		LoadPointCardData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern	OUT_POINTCARD	*GetPointCardData();

// 물고기레벨별 YuWangTeXiao자료
struct	OUT_FISHYUWANGTEXIAO
{
	uint32		FishLevel;
	uint32		TeXiaoPercent;
};
typedef	std::map<uint32, OUT_FISHYUWANGTEXIAO>	MAP_OUT_FISHYUWANGTEXIAO;
extern		MAP_OUT_FISHYUWANGTEXIAO	map_OUT_FISHYUWANGTEXIAO;
extern		bool	LoadFishYuWangTeXiao(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile);
extern		uint32	GetYuWangTeXiaoPercent(uint32 level);
