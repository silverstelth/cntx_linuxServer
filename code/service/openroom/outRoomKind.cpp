
#include "outRoomKind.h"
#include "../../common/Macro.h"
#include "../Common/Lang.h"
#include "misc/mem_stream.h"
#include "misc/command.h"
#include <net/service.h>
#include "mst_item.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

//////////////////////////////////////////////////////////////////////////
MAP_OUT_ANIMALPARAM	map_OUT_ANIMALPARAM;
bool	LoadAnimalParam(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_ANIMALPARAM.clear();
	START_SHEETANALYSE_3(ANIMAL, CharacterId, OneStep, Moveparam);
	if (strCharacterId != "")
	{
		OUT_ANIMALPARAM	newAnimal;
		newAnimal.TypeID = (T_ANIMALID)atoui(strCharacterId.c_str());
		newAnimal.MaxMoveDis = (T_DISTANCE)atoui(strOneStep.c_str());
		newAnimal.MoveParam = (uint32)atoui(strMoveparam.c_str());
		map_OUT_ANIMALPARAM.insert( make_pair(newAnimal.TypeID, newAnimal));
	}
	END_SHEETANALYSE();
	return true;
}
OUT_ANIMALPARAM		*GetOutAnimalParam(T_ANIMALID _ID)
{
	MAP_OUT_ANIMALPARAM::iterator it = map_OUT_ANIMALPARAM.find(_ID);
	if (it == map_OUT_ANIMALPARAM.end())
		return NULL;
	return &(it->second);
}


////////////////////////////////////////////////////////////////////
MAP_OUT_PETSET	map_OUT_PETSET;
bool	LoadPetData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_PETSET.clear();
	START_SHEETANALYSE_3(PETDATA, ItemId, PetTypeId, OrignalPet);
	if (strItemId != "")
	{
		//OUT_PETPARAM	newParam;
		//newParam.IncLifePer = (T_PETINCLIFEPER)atof(strIncPer.c_str());
		//newParam.OriginalLife = (T_PETLIFE)atoui(strOriginalLife.c_str());
		//newParam.DecLife = (T_PETLIFE)atoui(strDecVal.c_str());
		//newParam.DecInterval = (uint32)atoui(strDecInterval.c_str()) ;
		//newParam.DecInterval = newParam.DecInterval * 30; // 초

		T_PETID	PetID = (T_PETID)atoui(strOrignalPet.c_str());
		//T_PETGROWSTEP GrowStep = (T_PETGROWSTEP)atoui(strStep.c_str());

		MAP_OUT_PETSET::iterator itSet = map_OUT_PETSET.find(PetID);
		if (itSet == map_OUT_PETSET.end())
		{
			OUT_PETSET	newSet;
			newSet.TyepID = (T_PETTYPEID)atoui(strPetTypeId.c_str());
			//newSet.OriginStep = GrowStep;
			//newSet.OriginLife = newParam.OriginalLife;
			//newSet.MapPetParam.insert(make_pair(GrowStep, newParam));
			map_OUT_PETSET.insert(make_pair(PetID, newSet));
		}
		else
		{
			//itSet->second.MapPetParam.insert(make_pair(GrowStep, newParam));
		}
	}
	END_SHEETANALYSE();

	return true;
}
OUT_PETSET	*GetOutPetSet(T_PETID _ID)
{
	MAP_OUT_PETSET::iterator it = map_OUT_PETSET.find(_ID);
	if (it == map_OUT_PETSET.end())
		return NULL;
	return &(it->second);
}
//OUT_PETPARAM	*GetOutPetParam(T_PETID _ID, T_PETGROWSTEP _Step)
//{
//	MAP_OUT_PETSET::iterator it = map_OUT_PETSET.find(_ID);
//	if (it == map_OUT_PETSET.end())
//		return NULL;
//
//	MAP_OUT_PETPARAM::iterator param = it->second.MapPetParam.find(_Step);
//	if (param == it->second.MapPetParam.end())
//		return NULL;
//	
//	return &(param->second);
//}


////////////////////////////////////////////////////////////////////
MAP_OUT_PETCARE		map_OUT_PETCARE;
bool	LoadPetCare(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_PETCARE.clear();
	START_SHEETANALYSE_3(PETCARE, ItemId, PetTypeId, IncVal);
	if (strItemId != "")
	{
		OUT_PETCARE	newCare;
		newCare.CareID = (T_PETCAREID)atoui(strItemId.c_str());
		newCare.PetType = (T_PETTYPEID)atoui(strPetTypeId.c_str());
		newCare.IncLife = (T_PETLIFE)atoui(strIncVal.c_str());
		map_OUT_PETCARE.insert( make_pair(newCare.CareID, newCare));
	}
	END_SHEETANALYSE();
	return true;
}
OUT_PETCARE	*GetOutPetCare(T_PETCAREID _ID)
{
	MAP_OUT_PETCARE::iterator it = map_OUT_PETCARE.find(_ID);
	if (it == map_OUT_PETCARE.end())
		return NULL;
	return &(it->second);
}


//////////////////////////////////////////////////////////////////////////
MAP_OUT_MODELSEX			map_OUT_MODELSEX;
bool	LoadModelSex(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_MODELSEX.clear();
	START_SHEETANALYSE_2(MODELSEX, ModelId, SexId);
	if (strModelId != "")
	{
		uint32	ModelID = atoui(strModelId.c_str());
		uint32	SexID = atoui(strSexId.c_str());
		map_OUT_MODELSEX.insert( make_pair(ModelID, SexID));
	}
	END_SHEETANALYSE();
	return true;
}

uint32 GetModelSex(uint32 ModelID)
{
	MAP_OUT_MODELSEX::iterator it = map_OUT_MODELSEX.find(ModelID);
	if (it == map_OUT_MODELSEX.end())
		return 0;
	return it->second;
}


//////////////////////////////////////////////////////////////////////////
MAP_OUT_TAOCAN_FOR_AUTOPLAY	map_OUT_TAOCAN_FOR_AUTOPLAY;
bool LoadTaocanForAutoplay(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_TAOCAN_FOR_AUTOPLAY.clear();

	char		buf[512];
	{
		START_SHEETANALYSE_4(TAOCANITEM_XIANBAO, ItemID, ItemSubtype, ItemList, PrayText);
		if (strItemSubtype != "")
		{
			uint32 TaocanType = atoui(strItemSubtype.c_str());
			if (TaocanType == SACRFTYPE_TAOCAN_XIANBAO)
			{
				OUT_TAOCAN_FOR_AUTOPLAY	data;
				data.TaocanType = TaocanType;

				uint32	TaocanItemID = atoui(strItemID.c_str());

				strcpy(buf, strItemList.c_str());
				int n = 0;
				char	*token = strtok(buf, "*");
				while (token != NULL)
				{
					data.ItemIDs[n] = atoui(token);
					token = strtok(NULL, "*");
					n ++;
				}

				map_OUT_TAOCAN_FOR_AUTOPLAY[TaocanItemID] = data;
			}
		}
		END_SHEETANALYSE();
	}
	{
		START_SHEETANALYSE_3(TAOCANITEM_YISI, ItemID, ItemSubtype, ItemList);
		if (strItemSubtype != "")
		{
			uint32 TaocanType = atoui(strItemSubtype.c_str());
			if (TaocanType == SACRFTYPE_TAOCAN_ROOM || TaocanType == SACRFTYPE_TAOCAN_BUDDISM || TaocanType == SACRFTYPE_TAOCAN_ROOM_XDG)
			{
				OUT_TAOCAN_FOR_AUTOPLAY	data;
				data.TaocanType = TaocanType;

				uint32	TaocanItemID = atoui(strItemID.c_str());

				strcpy(buf, strItemList.c_str());
				int n = 0;
				char	*token = strtok(buf, "*");
				while (token != NULL)
				{
					data.ItemIDs[n] = atoui(token);
					token = strtok(NULL, "*");
					n ++;
				}

				map_OUT_TAOCAN_FOR_AUTOPLAY[TaocanItemID] = data;
			}
		}
		END_SHEETANALYSE();
	}
	return true;
}

OUT_TAOCAN_FOR_AUTOPLAY* GetTaocanItemInfo(uint32 TaocanItemID)
{
	MAP_OUT_TAOCAN_FOR_AUTOPLAY::iterator it = map_OUT_TAOCAN_FOR_AUTOPLAY.find(TaocanItemID);
	if (it == map_OUT_TAOCAN_FOR_AUTOPLAY.end())
		return NULL;
	return &(it->second);
}


//////////////////////////////////////////////////////////////////////////
MAP_OUT_ROOMKIND			map_OUT_ROOMKIND;
bool	LoadRoomKind(const CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_ROOMKIND.clear();
//	START_SHEETANALYSE_9(ROOMKIND, ID, MaxChars, OwnChars, KinChars, FriendChars, NumChars, LiuYanDesks, BlessLampCount, AncDeadCount);
	START_SHEETANALYSE_5(ROOMKIND, ID, MaxChars, LiuYanDesks, BlessLampCount, AncDeadCount);
	if (strID != "")
	{
		OUT_ROOMKIND newKind;
		newKind.ID = (T_MSTROOM_ID)atoui(strID.c_str());
		newKind.MaxChars = (T_MSTROOM_CHNUM)atoui(strMaxChars.c_str());
//		newKind.OwnChars = (T_MSTROOM_CHNUM)atoui(strOwnChars.c_str());
//		newKind.KinChars = (T_MSTROOM_CHNUM)atoui(strKinChars.c_str());
//		newKind.FriendChars = (T_MSTROOM_CHNUM)atoui(strFriendChars.c_str());
//		newKind.NumChars = (T_MSTROOM_CHNUM)atoui(strNumChars.c_str());
		newKind.LiuYanDesks = (uint32)atoui(strLiuYanDesks.c_str());
		newKind.BlessLampCount = (uint32)atoui(strBlessLampCount.c_str());
		if (newKind.BlessLampCount > MAX_BLESS_LAMP_COUNT)
		{
			sipwarning("BlessLampCount > MAX_BLESS_LAMP_COUNT!!! KindID=%d", newKind.ID);
			return false;
		}
		newKind.AncestorDeceasedCount = (uint32)atoui(strAncDeadCount.c_str());
		if (newKind.LiuYanDesks > MAX_MEMO_DESKS_COUNT)
		{
			sipwarning("LiuYanDesks > MAX_MEMO_DESKS_COUNT!!! KindID=%d", newKind.ID);
			return false;
		}
		if (newKind.AncestorDeceasedCount > MAX_ANCESTOR_DECEASED_COUNT)
		{
			sipwarning("newKind.AncestorDeceasedCount > MAX_ANCESTOR_DECEASED_COUNT!!! KindID=%d", newKind.ID);
			return false;
		}
		map_OUT_ROOMKIND.insert( make_pair(newKind.ID, newKind));
	}
	END_SHEETANALYSE();
	return true;
}

// Get possible max character count
T_MSTROOM_CHNUM	GetMaxCharacterOfRoomKind(T_MSTROOM_ID _id)
{
	MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(_id);
	if (ItResult == map_OUT_ROOMKIND.end())
		return 0;
	return ItResult->second.MaxChars;
}

// Get possible max character count
uint32 GetMaxAncestorDeceasedCount(T_MSTROOM_ID _id)
{
	MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(_id);
	if (ItResult == map_OUT_ROOMKIND.end())
		return 0;
	return ItResult->second.AncestorDeceasedCount;
}

const OUT_ROOMKIND	*GetOutRoomKind(T_MSTROOM_ID _ID)
{
	MAP_OUT_ROOMKIND::iterator it = map_OUT_ROOMKIND.find(_ID);
	if (it == map_OUT_ROOMKIND.end())
		return NULL;
	return &(it->second);
}


////////////////////////////////////////////////////////////////////
bool	LoadRoomScope(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	{   // PC for C version
		START_SHEETANALYSE_3(ROOMSCOPE, SceneId, PetPosId, BirthPosName);
		if (strSceneId != "" && strBirthPosName != "")
		{
			T_MSTROOM_ID SceneType = (T_MSTROOM_ID)atoui(strSceneId.c_str());
			T_PETREGIONID RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

			MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(SceneType);
			if (ItResult == map_OUT_ROOMKIND.end())
			{
				sipwarning("Invalid SceneType. SceneType=%d", SceneType);
				return false;
			}
			OUT_ROOMKIND* info = &(ItResult->second);
			REGIONSCOPE newscope;
			newscope.m_Name = strBirthPosName;
			info->RoomRegion_PC.insert(make_pair(RegionID, newscope));			
		}
		END_SHEETANALYSE();
	}

	{   //Mobile
		START_SHEETANALYSE_3(ROOMSCOPE_MOBILE, SceneId, PetPosId, BirthPosName);
		if (strSceneId != "" && strBirthPosName != "")
		{
			T_MSTROOM_ID SceneType = (T_MSTROOM_ID)atoui(strSceneId.c_str());
			T_PETREGIONID RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

			MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(SceneType);
			if (ItResult == map_OUT_ROOMKIND.end())
			{
				sipwarning("Invalid SceneType. SceneType=%d", SceneType);
				return false;
			}
			OUT_ROOMKIND* info = &(ItResult->second);
			REGIONSCOPE newscope;
			newscope.m_Name = strBirthPosName;
			info->RoomRegion_Mobile.insert(make_pair(RegionID, newscope));
		}
		END_SHEETANALYSE();
	}

	{   // PC for Unity  by krs
		START_SHEETANALYSE_3(ROOMSCOPE_UNITY, SceneId, PetPosId, BirthPosName);
		if (strSceneId != "" && strBirthPosName != "")
		{
			T_MSTROOM_ID SceneType = (T_MSTROOM_ID)atoui(strSceneId.c_str());
			T_PETREGIONID RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

			MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(SceneType);
			if (ItResult == map_OUT_ROOMKIND.end())
			{
				sipwarning("Invalid SceneType. SceneType=%d", SceneType);
				return false;
			}
			OUT_ROOMKIND* info = &(ItResult->second);
			REGIONSCOPE newscope;
			newscope.m_Name = strBirthPosName;			
			info->RoomRegion_PC_Unity.insert(make_pair(RegionID, newscope));
		}
		END_SHEETANALYSE();
	}

	return true;
}


////////////////////////////////////////////////////////////////////
//LST_OUT_ROOMPETMAX						lst_OUT_ROOMPETMAX;
//bool	LoadRoomPetMax(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
//{
//	lst_OUT_ROOMPETMAX.clear();
//	START_SHEETANALYSE_3(ROOMPETMAX, SceneId, PetPosId, MaxCount);
//	if (strSceneId != "")
//	{
//		OUT_ROOMPETMAX	newData;
//		newData.RoomID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
//		newData.RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());
//		newData.MaxCount = (T_PETCOUNT)atoui(strMaxCount.c_str());
//		lst_OUT_ROOMPETMAX.push_back(newData);
//	}
//	END_SHEETANALYSE();
//	return true;
//}
//T_PETCOUNT	GetRoomPetMax(T_MSTROOM_ID _RoomID, T_PETREGIONID _RegionID)
//{
//	for (LST_OUT_ROOMPETMAX::iterator it = lst_OUT_ROOMPETMAX.begin(); it != lst_OUT_ROOMPETMAX.end (); it++)
//	{
//		if ((*it).RoomID == _RoomID && (*it).RegionID == _RegionID)
//			return (*it).MaxCount;
//	}
//	return 0;
//}

static void	GetRegionScopeParam(xmlNodePtr	pSpace, REGIONSPACE *newSpace)
{
	string	strType	= CIXml::getStringProperty(pSpace,	"type",	"");
	string	strPos	= CIXml::getStringProperty(pSpace,	"pos",	"");
	string	strRot	= CIXml::getStringProperty(pSpace,	"rot",	"");
	string	strSize	= CIXml::getStringProperty(pSpace,	"size",	"");

	newSpace->m_RegionType = REGIONSPACE::SPACE_NONE;
	if (strType == "Cylinder")
		newSpace->m_RegionType = REGIONSPACE::SPACE_CYL;
	if (strType == "Box")
		newSpace->m_RegionType = REGIONSPACE::SPACE_BOX;
	if (strType == "Sphere")
		newSpace->m_RegionType = REGIONSPACE::SPACE_SPH;

	if (newSpace->m_RegionType != REGIONSPACE::SPACE_NONE)
	{
		// 谅钎沥焊啊福扁
		char seps[]   = " ";
		{
			char chTemp[100];	memset(chTemp, 0, sizeof(chTemp));
			strcpy(chTemp, strPos.c_str());
			char *token = strtok(chTemp, seps );
			int nTemp = 0;
			while( token != NULL )
			{
				if (nTemp == 0)
					newSpace->m_CenterPos.x = (T_POSX)atof(token);
				if (nTemp == 1)
					newSpace->m_CenterPos.y = (T_POSY)atof(token);
				if (nTemp == 2)
					newSpace->m_CenterPos.z = (T_POSZ)atof(token);
				nTemp ++;
				token = strtok( NULL, seps );
			}

		}
		// Rot沥焊啊福扁
		{
			char chTemp[100];	memset(chTemp, 0, sizeof(chTemp));
			strcpy(chTemp, strRot.c_str());
			char *token = strtok(chTemp, seps );
			int nTemp = 0;
			while( token != NULL )
			{
				if (nTemp == 0)
					newSpace->m_Rot1 = (float)atof(token);
				if (nTemp == 1)
					newSpace->m_Rot2 = (float)atof(token);
				if (nTemp == 2)
					newSpace->m_Rot3 = (float)atof(token);
				if (nTemp == 3)
					newSpace->m_Rot4 = (float)atof(token);
				nTemp ++;
				token = strtok( NULL, seps );
			}

		}
		// Size沥焊啊福扁
		{
			{
				char chTemp[100];	memset(chTemp, 0, sizeof(chTemp));
				strcpy(chTemp, strSize.c_str());
				char *token = strtok(chTemp, seps );
				int nTemp = 0;
				while( token != NULL )
				{
					if (nTemp == 0)
						newSpace->m_Size.x = (T_DISTANCE)atof(token);
					if (nTemp == 1)
						newSpace->m_Size.y = (T_DISTANCE)atof(token);
					if (nTemp == 2)
						newSpace->m_Size.z = (T_DISTANCE)atof(token);
					nTemp ++;
					token = strtok( NULL, seps );
				}
			}
		}
	}
}

// 움직이는 동물생성파라메터
struct	OUT_ROOMANIMALREGION
{
	T_COMMON_ID			ID;							// 유일구분ID
	T_MSTROOM_ID		SceneID;					// Sceneid
	T_PETREGIONID		RegionID;					// 령역 ID
	std::string			RegionName;					// 방안에서 령역이름
	T_ANIMALID			TypeID;						// 종류ID
	uint32				AnimalNum;					// 동물의 개수
	uint32				ServerControlType;			// 써버에서 조종하는 방식, 
};
typedef		std::map<T_COMMON_ID,	OUT_ROOMANIMALREGION>					MAP_OUT_ROOMANIMALREGION;

////////////////////////////////////////////////////////////////////
bool	LoadRoomDefaultPet(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	{ // For C version
		MAP_OUT_ROOMANIMALREGION	mapAnimals;

		START_SHEETANALYSE_6(ROOMDPET, SceneId, PetPosId, ItemId, DefaultCount, ModelId, ServerControl);
		T_PETCOUNT petCount = (T_PETCOUNT)atoui(strDefaultCount.c_str());
		if (petCount > 0)
		{
			if (strItemId != "")
			{
			}
			else
			{
				static T_COMMON_ID CurID = 1;
				uint32 isServerControl = atoui(strServerControl.c_str());
				if ( (isServerControl > 0) && (strModelId != "") )
				{// Animals controlled by server
					OUT_ROOMANIMALREGION	newAnimal;
					newAnimal.ID = CurID;
					newAnimal.SceneID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
					newAnimal.AnimalNum = (T_PETCOUNT)atoui(strDefaultCount.c_str());
					newAnimal.TypeID = (T_PETID)atoui(strModelId.c_str());
					newAnimal.ServerControlType = isServerControl;
					newAnimal.RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

					MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(newAnimal.SceneID);
					if (ItResult == map_OUT_ROOMKIND.end())
					{
						sipwarning("ItResult == map_OUT_ROOMKIND.end(). newAnimal.SceneID=%d", newAnimal.SceneID);
						return false;
					}
					OUT_ROOMKIND* info = &(ItResult->second);
					newAnimal.RegionName = info->GetRegionName(newAnimal.RegionID, 0);
					if (newAnimal.RegionName != "")
					{
						mapAnimals.insert(make_pair(CurID, newAnimal));
						CurID ++;
					}
					else
					{
						sipwarning("newAnimal.RegionName empty. SceneID=%d, RegionID=%d", newAnimal.SceneID, newAnimal.RegionID);
					}
				}
			}
		}
		END_SHEETANALYSE();

		extern	ucstring				OutDataRootPath;
		MAP_OUT_ROOMKIND::iterator itroomkind;
		for (itroomkind = map_OUT_ROOMKIND.begin(); itroomkind != map_OUT_ROOMKIND.end(); itroomkind ++)
		{
			TTime	curtime = SIPBASE::CTime::getLocalTime();
			T_MSTROOM_ID	RoomKind = itroomkind->first;
			OUT_ROOMKIND*	RoomInfo = &(itroomkind->second);

			ucstring scopePath = OutDataRootPath + toStringW(RoomKind) + L".scope";
			CIXml xmlScope;
			if (xmlScope.init(scopePath))
			{
				T_COMMON_ID		NextAnimalID = 10000;
				MAP_OUT_ROOMANIMALREGION::iterator	regionit;
				for(regionit = mapAnimals.begin(); regionit != mapAnimals.end(); regionit++)
				{
					const OUT_ROOMANIMALREGION *region = &(regionit->second);
					if (region->SceneID != RoomKind)
						continue;

					MAP_REGIONSCOPE::iterator itRoomRegion = RoomInfo->RoomRegion_PC.find(region->RegionID);
					if (itRoomRegion == RoomInfo->RoomRegion_PC.end())
					{
						sipwarning("itRoomRegion == RoomInfo->RoomRegion_PC.end(). RoomKind=%d, RegionID=%d", RoomKind, region->RegionID);
						return false;
					}

					if (itRoomRegion->second.m_lstSpace.size() == 0)
					{
						xmlNodePtr	pRoot = xmlScope.getRootNode();
						if (pRoot == NULL)
						{
							sipwarning("pRoot == NULL");
							continue;
						}
						xmlNodePtr	pChild = CIXml::getChildNodeOfStringProperty(pRoot, "Scope", "name", region->RegionName.c_str());
						if (pChild == NULL)
						{
							sipwarning(L"There is no animal scope info of room : KindID=%d, Scope=%s", RoomKind, ucstring(region->RegionName).c_str());
							continue;
						}
						REGIONSCOPE		newScope;
						newScope.m_Name = region->RegionName;
						xmlNodePtr	pSpace = CIXml::getFirstChildNode(pChild, "Space");
						while(pSpace)
						{
							REGIONSPACE newSpace;
							GetRegionScopeParam(pSpace, &newSpace);
							newScope.m_lstSpace.push_back(newSpace);
							pSpace = CIXml::getNextChildNode(pChild, "Space");
						}
						if (newScope.m_lstSpace.size() < 1)
						{
							sipwarning(L"There is no animal space info of room : KindID=%d, Scope=%s", RoomKind, ucstring(region->RegionName).c_str());
							continue;
						}

						itRoomRegion->second = newScope;
					}

					OUT_ANIMALPARAM			*AnimalParam = GetOutAnimalParam(region->TypeID);
					if (AnimalParam != NULL)
					{
						// 悼拱檬扁硅摹
						for (uint32 i = 0; i < region->AnimalNum; i++)
						{
							ROOMANIMAL newAnimal;
							newAnimal.m_ID = NextAnimalID++;			newAnimal.m_RegionID = region->RegionID;
							newAnimal.m_TypeID = region->TypeID;		newAnimal.m_ServerControlType = (uint8)region->ServerControlType;

							newAnimal.m_Direction = frand(0, 6.0f);
							itRoomRegion->second.GetRandomPos(&newAnimal.m_PosX, &newAnimal.m_PosY, &newAnimal.m_PosZ);

							int	interval = AnimalParam->MoveParam * 1000;
							newAnimal.m_NextMoveTime.timevalue = 0;
							RoomInfo->RoomAnimalDefault_PC.insert(make_pair(newAnimal.m_ID, newAnimal));
						}
//						sipinfo(L"Room animal init pos : RoomKind=%d, RegionID=%d, Number=%d", region->RoomID, region->ID, region->AnimalNum);
					}
					else
						sipwarning(L"There is no animal param : type=%d", region->TypeID);
				}
			}
//			else
//				sipwarning(L"There is no animal moveable scope file KindID=%d", RoomKind);
		}
	}

	{
		MAP_OUT_ROOMANIMALREGION	mapAnimals;

		START_SHEETANALYSE_6(ROOMDPET_MOBILE, SceneId, PetPosId, ItemId, DefaultCount, ModelId, ServerControl);
		T_PETCOUNT petCount = (T_PETCOUNT)atoui(strDefaultCount.c_str());
		if (petCount > 0)
		{
			if (strItemId != "")
			{
			}
			else
			{
				static T_COMMON_ID CurID = 1;
				uint32 isServerControl = atoui(strServerControl.c_str());
				if ( (isServerControl > 0) && (strModelId != "") )
				{// Animals controlled by server
					OUT_ROOMANIMALREGION	newAnimal;
					newAnimal.ID = CurID;
					newAnimal.SceneID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
					newAnimal.AnimalNum = (T_PETCOUNT)atoui(strDefaultCount.c_str());
					newAnimal.TypeID = (T_PETID)atoui(strModelId.c_str());
					newAnimal.ServerControlType = isServerControl;
					newAnimal.RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

					MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(newAnimal.SceneID);
					if (ItResult == map_OUT_ROOMKIND.end())
					{
						sipwarning("ItResult == map_OUT_ROOMKIND.end(). newAnimal.SceneID=%d", newAnimal.SceneID);
						return false;
					}
					OUT_ROOMKIND* info = &(ItResult->second);
					newAnimal.RegionName = info->GetRegionName(newAnimal.RegionID, 1);
					if (newAnimal.RegionName != "")
					{
						mapAnimals.insert(make_pair(CurID, newAnimal));
						CurID ++;
					}
					else
					{
						sipwarning("newAnimal.RegionName empty. SceneID=%d, RegionID=%d", newAnimal.SceneID, newAnimal.RegionID);
					}
				}
			}
		}
		END_SHEETANALYSE();

		extern	ucstring				OutDataRootPath;
		MAP_OUT_ROOMKIND::iterator itroomkind;
		for (itroomkind = map_OUT_ROOMKIND.begin(); itroomkind != map_OUT_ROOMKIND.end(); itroomkind ++)
		{
			TTime	curtime = SIPBASE::CTime::getLocalTime();
			T_MSTROOM_ID	RoomKind = itroomkind->first;
			OUT_ROOMKIND*	RoomInfo = &(itroomkind->second);

			ucstring scopePath = OutDataRootPath + toStringW(RoomKind + 1000) + L".scope";
			CIXml xmlScope;
			if (xmlScope.init(scopePath))
			{
				T_COMMON_ID		NextAnimalID = 10000;
				MAP_OUT_ROOMANIMALREGION::iterator	regionit;
				for(regionit = mapAnimals.begin(); regionit != mapAnimals.end(); regionit++)
				{
					const OUT_ROOMANIMALREGION *region = &(regionit->second);
					if (region->SceneID != RoomKind)
						continue;

					MAP_REGIONSCOPE::iterator itRoomRegion = RoomInfo->RoomRegion_Mobile.find(region->RegionID);
					if (itRoomRegion == RoomInfo->RoomRegion_Mobile.end())
					{
						sipwarning("itRoomRegion == RoomInfo->RoomRegion_M.end(). RoomKind=%d, RegionID=%d", RoomKind, region->RegionID);
						return false;
					}

					if (itRoomRegion->second.m_lstSpace.size() == 0)
					{
						xmlNodePtr	pRoot = xmlScope.getRootNode();
						if (pRoot == NULL)
						{
							sipwarning("pRoot == NULL");
							continue;
						}
						xmlNodePtr	pChild = CIXml::getChildNodeOfStringProperty(pRoot, "Scope", "name", region->RegionName.c_str());
						if (pChild == NULL)
						{
							sipwarning(L"There is no animal scope info of room : KindID=%d, Scope=%s", RoomKind + 1000, ucstring(region->RegionName).c_str());
							continue;
						}
						REGIONSCOPE		newScope;
						newScope.m_Name = region->RegionName;
						xmlNodePtr	pSpace = CIXml::getFirstChildNode(pChild, "Space");
						while(pSpace)
						{
							REGIONSPACE newSpace;
							GetRegionScopeParam(pSpace, &newSpace);
							newScope.m_lstSpace.push_back(newSpace);
							pSpace = CIXml::getNextChildNode(pChild, "Space");
						}
						if (newScope.m_lstSpace.size() < 1)
						{
							sipwarning(L"There is no animal space info of room : KindID=%d, Scope=%s", RoomKind, ucstring(region->RegionName).c_str());
							continue;
						}

						itRoomRegion->second = newScope;
					}

					OUT_ANIMALPARAM			*AnimalParam = GetOutAnimalParam(region->TypeID);
					if (AnimalParam != NULL)
					{
						// 悼拱檬扁硅摹
						for (uint32 i = 0; i < region->AnimalNum; i++)
						{
							ROOMANIMAL newAnimal;
							newAnimal.m_ID = NextAnimalID++;			newAnimal.m_RegionID = region->RegionID;
							newAnimal.m_TypeID = region->TypeID;		newAnimal.m_ServerControlType = (uint8)region->ServerControlType;

							newAnimal.m_Direction = frand(0, 6.0f);
							itRoomRegion->second.GetRandomPos(&newAnimal.m_PosX, &newAnimal.m_PosY, &newAnimal.m_PosZ);

							int	interval = AnimalParam->MoveParam * 1000;
							newAnimal.m_NextMoveTime.timevalue = 0;
							RoomInfo->RoomAnimalDefault_Mobile.insert(make_pair(newAnimal.m_ID, newAnimal));
						}
//						sipinfo("Room animal init pos : RoomKind=%d, RegionID=%d, Number=%d", region->SceneID, region->ID, region->AnimalNum);
					}
					else
						sipwarning(L"There is no animal param : type=%d", region->TypeID);
				}
			}
//			else
//				sipwarning(L"There is no animal moveable scope file KindID=%d", RoomKind + 1000);
		}		
	}

	// For Unity Version  by krs
	{
		MAP_OUT_ROOMANIMALREGION	mapAnimals;

		START_SHEETANALYSE_6(ROOMDPET_UNITY, SceneId, PetPosId, ItemId, DefaultCount, ModelId, ServerControl);
		T_PETCOUNT petCount = (T_PETCOUNT)atoui(strDefaultCount.c_str());
		if (petCount > 0)
		{
			if (strItemId != "")
			{
			}
			else
			{
				static T_COMMON_ID CurID = 1;
				uint32 isServerControl = atoui(strServerControl.c_str());
				if ((isServerControl > 0) && (strModelId != ""))
				{// Animals controlled by server
					OUT_ROOMANIMALREGION	newAnimal;
					newAnimal.ID = CurID;
					newAnimal.SceneID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
					newAnimal.AnimalNum = (T_PETCOUNT)atoui(strDefaultCount.c_str());
					newAnimal.TypeID = (T_PETID)atoui(strModelId.c_str());
					newAnimal.ServerControlType = isServerControl;
					newAnimal.RegionID = (T_PETREGIONID)atoui(strPetPosId.c_str());

					MAP_OUT_ROOMKIND::iterator ItResult = map_OUT_ROOMKIND.find(newAnimal.SceneID);
					if (ItResult == map_OUT_ROOMKIND.end())
					{
						sipwarning("ItResult == map_OUT_ROOMKIND.end(). newAnimal.SceneID=%d", newAnimal.SceneID);
						return false;
					}
					OUT_ROOMKIND* info = &(ItResult->second);
					newAnimal.RegionName = info->GetRegionName(newAnimal.RegionID, 2);
					if (newAnimal.RegionName != "")
					{
						mapAnimals.insert(make_pair(CurID, newAnimal));
						CurID++;
					}
					else
					{
						sipwarning("newAnimal.RegionName empty. SceneID=%d, RegionID=%d", newAnimal.SceneID, newAnimal.RegionID);
					}
				}
			}
		}
		END_SHEETANALYSE();

		extern	ucstring				OutDataRootPath;
		MAP_OUT_ROOMKIND::iterator itroomkind;
		for (itroomkind = map_OUT_ROOMKIND.begin(); itroomkind != map_OUT_ROOMKIND.end(); itroomkind++)
		{
			TTime	curtime = SIPBASE::CTime::getLocalTime();
			T_MSTROOM_ID	RoomKind = itroomkind->first;
			OUT_ROOMKIND*	RoomInfo = &(itroomkind->second);

			ucstring scopePath = OutDataRootPath + toStringW(RoomKind) + L".scope_unity"; // by krs
			CIXml xmlScope;
			if (xmlScope.init(scopePath))
			{
				T_COMMON_ID		NextAnimalID = 10000;
				MAP_OUT_ROOMANIMALREGION::iterator	regionit;
				for (regionit = mapAnimals.begin(); regionit != mapAnimals.end(); regionit++)
				{
					const OUT_ROOMANIMALREGION *region = &(regionit->second);
					if (region->SceneID != RoomKind)
						continue;

					MAP_REGIONSCOPE::iterator itRoomRegion = RoomInfo->RoomRegion_PC_Unity.find(region->RegionID);
					if (itRoomRegion == RoomInfo->RoomRegion_PC_Unity.end())
					{
						sipwarning("itRoomRegion == RoomInfo->RoomRegion_PC.end(). RoomKind=%d, RegionID=%d", RoomKind, region->RegionID);
						return false;
					}

					if (itRoomRegion->second.m_lstSpace.size() == 0)
					{
						xmlNodePtr	pRoot = xmlScope.getRootNode();
						if (pRoot == NULL)
						{
							sipwarning("pRoot == NULL");
							continue;
						}
						xmlNodePtr	pChild = CIXml::getChildNodeOfStringProperty(pRoot, "Scope", "name", region->RegionName.c_str());
						if (pChild == NULL)
						{
							sipwarning(L"There is no animal scope info of room : KindID=%d, Scope=%s", RoomKind, ucstring(region->RegionName).c_str());
							continue;
						}
						REGIONSCOPE		newScope;
						newScope.m_Name = region->RegionName;
						xmlNodePtr	pSpace = CIXml::getFirstChildNode(pChild, "Space");
						while (pSpace)
						{
							REGIONSPACE newSpace;
							GetRegionScopeParam(pSpace, &newSpace);
							newScope.m_lstSpace.push_back(newSpace);
							pSpace = CIXml::getNextChildNode(pChild, "Space");
						}
						if (newScope.m_lstSpace.size() < 1)
						{
							sipwarning(L"There is no animal space info of room : KindID=%d, Scope=%s", RoomKind, ucstring(region->RegionName).c_str());
							continue;
						}

						itRoomRegion->second = newScope;
					}

					OUT_ANIMALPARAM			*AnimalParam = GetOutAnimalParam(region->TypeID);
					if (AnimalParam != NULL)
					{
						// 悼拱檬扁硅摹
						for (uint32 i = 0; i < region->AnimalNum; i++)
						{
							ROOMANIMAL newAnimal;
							newAnimal.m_ID = NextAnimalID++;			newAnimal.m_RegionID = region->RegionID;
							newAnimal.m_TypeID = region->TypeID;		newAnimal.m_ServerControlType = (uint8)region->ServerControlType;

							newAnimal.m_Direction = frand(0, 6.0f);
							itRoomRegion->second.GetRandomPos(&newAnimal.m_PosX, &newAnimal.m_PosY, &newAnimal.m_PosZ);

							int	interval = AnimalParam->MoveParam * 1000;
							newAnimal.m_NextMoveTime.timevalue = 0;
							RoomInfo->RoomAnimalDefault_PC_Unity.insert(make_pair(newAnimal.m_ID, newAnimal));
						}
						//						sipinfo(L"Room animal init pos : RoomKind=%d, RegionID=%d, Number=%d", region->RoomID, region->ID, region->AnimalNum);
					}
					else
						sipwarning(L"There is no animal param : type=%d", region->TypeID);
				}
			}
			//			else
			//				sipwarning(L"There is no animal moveable scope file KindID=%d", RoomKind);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
//LST_OUT_ROOMTGATE			lst_OUT_ROOMTGATE;
//bool	LoadRoomGate(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
//{
//	lst_OUT_ROOMTGATE.clear();
//	START_SHEETANALYSE_5(TGATE, SceneId, Virtue, Scope, Fre, Second);
//	if (strSceneId != "")
//	{
//		OUT_ROOMTGATE	newGate;
//		newGate.RoomID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
//		newGate.RoomVirtue = (T_ROOMVIRTUE)atoui(strVirtue.c_str());
//		newGate.RegionName = strScope;
//		newGate.GateFre = (float)atof(strFre.c_str());
//		newGate.GateTime = (uint32)atoui(strSecond.c_str());
//		lst_OUT_ROOMTGATE.push_back(newGate);
//	}
//	END_SHEETANALYSE();
//	return true;
//}

//////////////////////////////////////////////////////////////////////////
//LST_OUT_SERVERSCOPE			lst_OUT_SERVERSCOPE;
//bool	LoadServerScope(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
//{
//	lst_OUT_SERVERSCOPE.clear();
//	START_SHEETANALYSE_3(SERVERSCOPE, SceneId, PosType, PosName);
//	if (strSceneId != "")
//	{
//		OUT_SERVERSCOPE	newScope;
//		newScope.SceneID = (T_MSTROOM_ID)atoui(strSceneId.c_str());
//		newScope.Type = (ENUM_SERVERSCOPE_TYPE)atoui(strPosType.c_str());
//		newScope.ScopeName = strPosName;
//		lst_OUT_SERVERSCOPE.push_back(newScope);
//	}
//	END_SHEETANALYSE();
//	return true;
//}


//////////////////////////////////////////////////////////////////////////
LST_OUT_RELIGIONROOM	lst_OUT_RELIGIONROOM;
bool LoadReligionRoom(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	lst_OUT_RELIGIONROOM.clear();
	START_SHEETANALYSE_2(RELIGIONROOMDATA, RoomId, RoomSceneId);
	if (strRoomId != "")
	{
		OUT_RELIGIONROOM	data;
		data.ReligionRoomID = atoui(strRoomId.c_str());
		data.ReligionRoomSceneID = (uint16)atoui(strRoomSceneId.c_str());
		lst_OUT_RELIGIONROOM.push_back(data);
	}
	END_SHEETANALYSE();
	return true;
}

uint16 GetReligionRoomSceneID(uint32 RRoomID)
{
	for(LST_OUT_RELIGIONROOM::iterator it = lst_OUT_RELIGIONROOM.begin(); it != lst_OUT_RELIGIONROOM.end(); it ++)
	{
		if(it->ReligionRoomID == RRoomID)
			return it->ReligionRoomSceneID;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
LST_OUT_LUCK	lst_OUT_LuckData;
uint32			LuckData_Percent_Max;
bool LoadLuckData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	LuckData_Percent_Max = 0;

	lst_OUT_LuckData.clear();
	START_SHEETANALYSE_3(LUCKDATA, LuckId, LuckPercent, LuckLevel);
	if (strLuckId != "")
	{
		OUT_LUCK	data;
		data.LuckID = atoui(strLuckId.c_str());
		data.LuckPercent = atoui(strLuckPercent.c_str());
		data.LuckLevel = (uint8)atoui(strLuckLevel.c_str());
		lst_OUT_LuckData.push_back(data);

		LuckData_Percent_Max += data.LuckPercent;
	}
	END_SHEETANALYSE();
	return true;
}

uint32 nAlwaysLuck3ID = 0;
uint32 nAlwaysLuck4ID = 0;

OUT_LUCK	*GetLuck(bool bPublicRoom)
{
	if (nAlwaysLuck4ID > 0)
	{
		LST_OUT_LUCK::iterator it;
		for(it = lst_OUT_LuckData.begin(); it != lst_OUT_LuckData.end(); it ++)
		{
			if (it->LuckLevel == 4 && it->LuckID == nAlwaysLuck4ID)
				return &(*it);
		}
		return NULL;
	}

	if (nAlwaysLuck3ID > 0)
	{
		LST_OUT_LUCK::iterator it;
		for(it = lst_OUT_LuckData.begin(); it != lst_OUT_LuckData.end(); it ++)
		{
			if (it->LuckLevel == 3 && it->LuckID == nAlwaysLuck3ID)
				return &(*it);
		}
	}
	else
	{
		uint32	rnd_value = rand() * 10000 / RAND_MAX;

		// 공공구역에서의 선밍은 3->2배 잘 나오게 하도록 한다. (Lv4만 유효)
		if (bPublicRoom)
			rnd_value /= 2;

		if(rnd_value >= LuckData_Percent_Max)
			return NULL;

		LST_OUT_LUCK::iterator it;
		for(it = lst_OUT_LuckData.begin(); it != lst_OUT_LuckData.end(); it ++)
		{
			if(rnd_value < it->LuckPercent)
				return &(*it);
			rnd_value -= it->LuckPercent;
		}
	}

	return NULL;
}

//OUT_LUCK	*GetLuck4()
//{
//	int nCount = 0;
//	LST_OUT_LUCK::iterator it;
//	for(it = lst_OUT_LuckData.begin(); it != lst_OUT_LuckData.end(); it ++)
//	{
//		if(it->LuckLevel == 4)
//			nCount++;
//	}
//	if (nCount == 0)
//	{
//		siperrornoex("There is no luck4 data");
//		return NULL;
//	}
//
//	int nPos = Irand(0, nCount-1);
//	int nCurPos = 0;
//
//	for(it = lst_OUT_LuckData.begin(); it != lst_OUT_LuckData.end(); it ++)
//	{
//		if(it->LuckLevel != 4)
//			continue;
//
//		if(nCurPos == nPos)
//			return &(*it);
//		nCurPos++;
//	}
//	return NULL;
//}


//////////////////////////////////////////////////////////////////////////
LST_OUT_PRIZE	lst_OUT_PrizeData;
LST_OUT_PRIZE_LEVEL			lst_OUT_PrizeLevelData;
bool LoadPrizeData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	lst_OUT_PrizeData.clear();
	START_SHEETANALYSE_3(PRIZEDATA, PrizeID, PrizeLevel, PrizeItemID);
	if (strPrizeID != "")
	{
		OUT_PRIZE	data;
		data.PrizeID = atoui(strPrizeID.c_str());
		data.PrizeLevel = (uint8)atoui(strPrizeLevel.c_str());
		data.PrizeItemID = atoui(strPrizeItemID.c_str());
		lst_OUT_PrizeData.push_back(data);
	}
	END_SHEETANALYSE();

	lst_OUT_PrizeLevelData.clear();
	for (LST_OUT_PRIZE::iterator it = lst_OUT_PrizeData.begin(); it != lst_OUT_PrizeData.end(); it ++)
	{
		uint8	level = it->PrizeLevel;
		uint32	prizeid = it->PrizeID;
		bool	bFound = false;
		for (LST_OUT_PRIZE_LEVEL::iterator it1 = lst_OUT_PrizeLevelData.begin(); it1 != lst_OUT_PrizeLevelData.end(); it1 ++)
		{
			if (it1->PrizeLevel == level)
			{
				bFound = true;
				it1->PrizeIDs.push_back(prizeid);
				break;
			}
		}
		if (!bFound)
		{
			OUT_PRIZE_LEVEL	data;
			data.PrizeLevel = level;
			data.PrizeIDs.push_back(prizeid);

			lst_OUT_PrizeLevelData.push_back(data);
		}
	}

	return true;
}

OUT_PRIZE *GetPrizeData(uint32 PrizeID)
{
	for (LST_OUT_PRIZE::iterator it = lst_OUT_PrizeData.begin(); it != lst_OUT_PrizeData.end(); it ++)
	{
		if (it->PrizeID == PrizeID)
			return &(*it);
	}
	return NULL;
}

OUT_PRIZE *GetPrize(uint8 PrizeLevel)
{
	for (LST_OUT_PRIZE_LEVEL::iterator it = lst_OUT_PrizeLevelData.begin(); it != lst_OUT_PrizeLevelData.end(); it ++)
	{
		if (it->PrizeLevel == PrizeLevel)
		{
			size_t	rnd_value = rand() * it->PrizeIDs.size() / RAND_MAX;
			if (rnd_value == it->PrizeIDs.size())
			{
				if (rnd_value == 0)
					return NULL;
				rnd_value --;
			}

			return GetPrizeData(it->PrizeIDs[rnd_value]);
		}
	}
	return NULL;
}

void CheckValidItemData()
{
	for (LST_OUT_PRIZE::iterator it = lst_OUT_PrizeData.begin(); it != lst_OUT_PrizeData.end(); it ++)
	{
		uint32 ItemID = it->PrizeItemID;
		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if (mstItem == NULL)
		{
			siperrornoex("There is no prize item:ID=%d", ItemID);
			 return;
			//sipwarning("There is no prize item:ID=%d", ItemID);
		}
	}
}

int GetPrizeItemLevel(uint32 ItemID)
{
	for (LST_OUT_PRIZE::iterator it = lst_OUT_PrizeData.begin(); it != lst_OUT_PrizeData.end(); it ++)
	{
		if (ItemID == it->PrizeItemID)
		{
			return it->PrizeLevel;
		}
	}
	return -1;
}


//////////////////////////////////////////////////////////////////////////
LST_OUT_POINTCARD	lst_OUT_PointCardData;
bool LoadPointCardData(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	lst_OUT_PointCardData.clear();
	START_SHEETANALYSE_2(POINTCARD, CardID, AddPoint);
	if (strCardID != "")
	{
		OUT_POINTCARD	data;
		data.CardID = atoui(strCardID.c_str());
		data.AddPoint = atoui(strAddPoint.c_str());
		lst_OUT_PointCardData.push_back(data);
	}
	END_SHEETANALYSE();

	return true;
}
OUT_POINTCARD *GetPointCardData()
{
	size_t	rnd_value = rand() * lst_OUT_PointCardData.size() / RAND_MAX;

	return &lst_OUT_PointCardData[rnd_value];
}


//////////////////////////////////////////////////////////////////////////
MAP_OUT_FISHYUWANGTEXIAO	map_OUT_FISHYUWANGTEXIAO;
bool	LoadFishYuWangTeXiao(const SIPBASE::CIXml *xmlInfo, ucstring ucOutIniFile)
{
	map_OUT_FISHYUWANGTEXIAO.clear();
	START_SHEETANALYSE_2(YUWANGXINXI, FLevel, XPercent);
	if (strFLevel != "")
	{
		OUT_FISHYUWANGTEXIAO	data;
		data.FishLevel = atoui(strFLevel.c_str());
		data.TeXiaoPercent = atoui(strXPercent.c_str());
		map_OUT_FISHYUWANGTEXIAO[data.FishLevel] = data;
	}
	END_SHEETANALYSE();

	return true;
}
uint32	GetYuWangTeXiaoPercent(uint32 level)
{
	MAP_OUT_FISHYUWANGTEXIAO::iterator it = map_OUT_FISHYUWANGTEXIAO.find(level);
	if (it == map_OUT_FISHYUWANGTEXIAO.end())
	{
		return 0;
	}
	return it->second.TeXiaoPercent;
}


SIPBASE_CATEGORISED_COMMAND(OROOM_S, setAlwaysLuck3, "Set always luck3 for dev", "LuckID (0:No, 16~20)")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	nAlwaysLuck3ID  = atoui(v.c_str());
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setAlwaysLuck4, "Set always luck4 for dev", "LuckID (0:No, 21~24)")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 uv = atoui(v.c_str());
	nAlwaysLuck4ID = uv;
	return true;
}
