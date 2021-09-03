#ifndef OPENROOM_CHARACTER_H
#define OPENROOM_CHARACTER_H

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/common.h"
#include "../Common/Common.h"

// 某腐磐磊丰
struct CHARACTER 
{
	CHARACTER()	
		: m_DressID(0) {};
	CHARACTER(T_CHARACTERID _ID, T_CHARACTERNAME _Name, T_FAMILYID _Familyid, T_ISMASTER _IsMaster, T_MODELID _modelid, T_DRESS _ddid, T_FACEID _faceid, T_HAIRID _hairid, T_DRESS _dressid, T_CHBIRTHDATE _BirthDate, T_CHCITY _City, T_CHRESUME _Resume )
		:m_ID(_ID), m_Name(_Name), m_FamilyID(_Familyid), m_IsMaster(_IsMaster), m_ModelId(_modelid), m_DefaultDressID(_ddid), m_FaceID(_faceid), m_HairID(_hairid), m_DressID(_dressid), m_BirthDate(_BirthDate), m_City(_City), m_Resume(_Resume) {}
	T_CHARACTERID							m_ID;						// id
	T_CHARACTERNAME							m_Name;						// 捞抚
	T_FAMILYID								m_FamilyID;					// 啊练id
	T_ISMASTER								m_IsMaster;
	T_MODELID								m_ModelId;
	T_DRESS									m_DefaultDressID;
	T_FACEID								m_FaceID;
	T_HAIRID								m_HairID;
	T_DRESS									m_DressID;
	T_CHBIRTHDATE                           m_BirthDate;
	T_CHCITY                                m_City;
	T_CHRESUME                              m_Resume;
};

typedef		std::map<T_CHARACTERID,	CHARACTER>					MAP_CHARACTER;
extern		MAP_CHARACTER				map_character;

// 방안에 있는 캐릭터정보
struct INFO_CHARACTERINROOM 
{
	INFO_CHARACTERINROOM()
		:m_CHID(0), m_HoldItemID(0)	{}
	INFO_CHARACTERINROOM(T_CHARACTERID _CHID, T_MODELID _ModelID)
		:m_CHID(_CHID), m_ModelID(_ModelID), m_HoldItemID(0), m_AniState(0){}

	T_CHARACTERID		m_CHID;					// 캐릭터id
	T_MODELID			m_ModelID;				// 캐릭터모델ID
	T_CH_DIRECTION		m_Direction;			// 캐릭터의 방향
	T_CH_X				m_X;					// 캐릭터의 X 위치
	T_CH_Y				m_Y;					// 캐릭터의 Y 위치
	T_CH_Z				m_Z;					// 캐릭터의 Z 위치
	ucstring			m_ucAniName;			// Animation name
	T_CH_STATE			m_AniState;				// 상태
	T_FITEMID			m_HoldItemID;			// 손에 든 아이템ID(DB에서)
};

//extern		void			LoadCharacterData(T_CHARACTERID _CHID);
extern		void			AddCharacterData(CHARACTER& _newCH);
extern		void			DelCharacterData(T_CHARACTERID _CHID);

extern		bool			IsInFamily(T_FAMILYID _FID, T_CHARACTERID _CHID);
extern		CHARACTER		*GetCharacterData(T_CHARACTERID _CHID);
extern		void			SendCharacterInfo(T_FAMILYID _FID, SIPNET::TServiceId _tsid, T_FAMILYID _TargetId = 0);

// callback
//extern void	cb_M_CS_ALLCHARACTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_NEWCHARACTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_DELCHARACTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_CHCHARACTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_CHANGE_CHDESC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_CHDESC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_REQ_FAMILYCHS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

#endif // OPENROOM_CHARACTER_H
