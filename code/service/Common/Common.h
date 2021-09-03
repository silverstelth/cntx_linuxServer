#ifndef TIAN_SERVER_COMMON_H
#define TIAN_SERVER_COMMON_H

#include "../../common/Macro.h"

// 써비스이름들
#define AS_S_NAME					"AS"
#define AS_L_NAME					"admin_main_service"
#define AES_S_NAME					"AES"
#define AES_L_NAME					"admin_executor_service"
#define LS_S_NAME					"LS"
#define LS_L_NAME					"login_service"
#define SS_S_NAME					"SS"
#define SS_L_NAME					"start_service"
#define NS_S_NAME					"NS"
#define NS_L_NAME					"naming_service"
#define WELCOME_SHORT_NAME			"WS"						// Welcome service 짧은 이름
#define WELCOME_LONG_NAME			"welcome_service"			// Welcome service 긴 이름
#define FRONTEND_SHORT_NAME			"FS"						// 접속써비스 짧은 이름
#define FRONTEND_LONG_NAME			"frontend_service"			// 접속써비스 긴 이름
#define OROOM_SHORT_NAME			"OROOM_S"					// 열린방관리써비스 짧은 이름
#define OROOM_LONG_NAME				"openroom_service"			// 열린방관리써비스 긴 이름
#define RROOM_SHORT_NAME			"RROOM_S"					// 종교방써비스 짧은 이름
#define RROOM_LONG_NAME				"religionroom_service"		// 종교방써비스 긴 이름
#define LOBBY_SHORT_NAME			"LOBBY"						// 로비써비스의 짧은 이름
#define LOBBY_LONG_NAME				"lobby_service"				// 로비써비스
#define RELAY_SHORT_NAME			"RS"						// 중계써비스의 짧은 이름
#define RELAY_LONG_NAME				"relay_service"				// 중계써비스의 긴 이름
#define HISMANAGER_SHORT_NAME		"HIS_S"						// 사적자료완충써비스 짧은 이름
#define HISMANAGER_LONG_NAME		"history_service"			// 사적자료완충써비스 긴 이름
#define STREAM_SHORT_NAME			"SMS"						//
#define STREAM_LONG_NAME			"stream_service"			//
#define LOG_SHORT_NAME				"LOG_S"						// 로그관리써비스 짧은 이름
#define LOG_LONG_NAME				"log_service"				// 로그관리써비스 긴 이름
#define MANAGER_SHORT_NAME			"MS"						// ManagerPlay써비스 짧은 이름
#define MANAGER_LONG_NAME			"manager_service"			// ManagerPlay써비스 긴 이름
// For test
#define VIRTUAL_SHORT_NAME			"VS"						//
#define VIRTUAL_LONG_NAME			"virtual_service"			//

// 불법활동기록
extern	void RECORD_UNLAW_EVENT(uint8 UnlawType, ucstring UnlawContent, uint32 ID);


#define		TID					uint32									// GeneralId
#define		T_ID				uint32									// GeneralId
#define		T_NAME				ucstring								// GeneralName
#define		T_M1970				uint32									// Minutes since 1970
#define		T_S1970				uint32									// Seconds since 1970
#define		T_DATE4				uint32									// [w:year(16th bit:era, 15th bit:lunar)][b:month][b:day]

#define		T_COMMON_ID			uint32									// 일반적인 ID형
#define		T_COMMON_NAME		ucstring								// 일반적인 명칭형
#define		T_COMMON_CONTENT	ucstring								// 일반적인 설명문
#define		T_PRICE				uint32									// 금액
#define		T_PRICEDISC			uint32									// 금액할인페센트
#define		T_DATETIME			std::string								// 날자형
#define		T_SEX				uint8									// 성별
#define		T_AGE				uint8									// 나이구분
#define		T_FACEID			uint32									// 얼굴형태
#define		T_HAIRID			uint32									// 머리카락형태
#define		T_URL				ucstring								// URL
#define		T_MODELID			uint32									// 모델id
#define		T_DRESS				uint32									// 복장id
#define		T_ISMASTER			uint8									// 주캐릭터
#define		T_FLAG				uint8									// 기발값
#define		T_URLID				uint8									// URL id
#define		T_POS				float									// Position
#define		T_POSX				float									// 일반위치 X
#define		T_POSY				float									// 일반위치 Y
#define		T_POSZ				float									// 일반위치 Z
#define		T_DISTANCE			float									// 거리
#define		T_INROOMPOS			ucstring								// 방안에서 위치를 정의하는 문자렬
// 가족, 캐릭터
#define		T_ACCOUNTID			T_COMMON_ID								// 계정id type
#define		T_FAMILYID			T_ACCOUNTID								// 가족id type
#define		T_SESSIONSTATE		uint32								// client session state
#define		T_CHARACTERID		T_COMMON_ID								// 캐릭터id
#define     T_CHBIRTHDATE       T_COMMON_ID                             // charater's birthdate
#define     T_CHCITY            T_COMMON_ID                             // charater's city
#define     T_CHRESUME          T_COMMON_NAME                           // charater's resume
#define     T_DATATIME          T_COMMON_ID                             // [Data4]
#define		T_FAMILYNAME		T_COMMON_NAME							// 가족이름
#define		T_CHARACTERNAME		T_COMMON_NAME							// 캐릭터이름
#define		T_CH_DIRECTION		float									// 캐릭터의 방향
#define		T_LEVEL				uint32									// Level
#define		T_CH_POS			T_POSX									// 캐릭터의 위치
#define		T_CH_X				T_POSX									// 캐릭터의 X 위치
#define		T_CH_Y				T_POSY									// 캐릭터의 Y 위치
#define		T_CH_Z				T_POSZ									// 캐릭터의 Z 위치
#define		T_CH_STATE			uint32									// 캐릭터의 상태
#define		T_F_TAMPING			uint16									// 충전회수
#define		T_F_EXP				uint32									// 가족경험값
#define		T_F_VIRTUE			uint16									// 가족공덕값
#define		T_F_FAME			uint16									// 가족명망값
#define		T_F_LEVEL			uint8									// 가족레벨
#define		T_F_HONORID			uint8									// 가족칭호
#define		T_F_VISITDAYS		uint32									// 방문날자수
#define		T_HISSPACE			uint32									// 사적자료공간
#define		T_EXPERIENCE		uint8									// 체험기발
// tbl_mstRoom 형Mapping
#define		T_MSTROOM_ID		uint16									// 방종류id
#define		T_MSTROOM_NAME		T_COMMON_NAME							// 방종류이름
#define		T_MSTROOM_TYPEID	uint8									// 방형상id
#define		T_MSTROOM_FAITHID	uint8									// 신앙id
#define		T_MSTROOM_CHNUM		uint16									// 방에 들어올수 있는 최대캐릭수
#define		T_MSTROOM_LEVEL		uint8									// 방의 급수
#define		T_MSTROOM_PERIOD	uint32									// 사용기간
#define		T_MSTROOM_YEARS		uint8									// 사용년수
#define		T_MSTROOM_MONTHS	uint8									// 사용월수
#define		T_MSTROOM_DAYS		uint8									// 사용날자수

// tbl_Room 형Mapping
#define		T_SCENEID			uint16									// SceneId
#define		T_ROOMID			T_COMMON_ID								// 방id type
#define		T_ROOMNAME			T_COMMON_NAME							// 방이름
#define		T_ROOMPOWERID		uint8									// 방리용권한
#define		T_ROOMVISITS		uint32									// 방문회수
#define		T_ROOMVDAYS			uint32									// 유효입방날자
#define		T_ROOMLEVEL			uint8									// 방급수
#define		T_ROOMEXP			uint32									// 방경험값
#define		T_ROOMVIRTUE		uint16									// 방공덕값
#define		T_ROOMFAME			uint16									// 방명망값
#define		T_ROOMAPPRECIATION	uint16									// 방감은값
#define		T_ROOMMUSICID		uint32									// 방의배경음악id
#define		T_ROOMROLE			uint8									// 방에서의 역할
#define		T_ROOMCHANNELID		T_COMMON_ID								// 방의 Channel

// 고인
#define		T_DECEASED_POS		uint8									// 고인위치
#define		T_DECEASED_NAME		T_COMMON_NAME							// 고인이름
#define		T_DECEASED_HIS		ucstring								// 고인략력
// 가족item
#define		T_FITEMID			T_COMMON_ID								// 가족item id
#define		T_ITEMTYPEID		uint8									// Item의 큰 분류id
#define		T_ITEMSUBTYPEID		uint32									// Item의 작은 분류 id
#define		T_FITEMNUM			uint16									// 가족item의 개수
#define		T_FITEMPOS			uint16									// 가족item 위치
#define		T_FITEMUSING		uint8									// 가족item사용여부
#define		T_USEDTIME			uint32									// 가족item사용시간
// 부품관련
#define		T_ROOMPARTID		uint16									// 기념관안에서 부품ID
#define		T_ROOMPARTPOS		std::string								// 부분품의 위치정보
// 채팅
#define		T_CHATTEXT			ucstring								// 채팅문자렬
// 제물아이템
#define		T_SACRIFICEITEMNUM	uint8									// 제물아이템개수
#define		T_SACRIFICEX		uint8									// X
#define		T_SACRIFICEY		uint8									// Y
// 련계인
#define		T_CONTACTTYPE		uint8									// 련계인종류
// 행사
//#define		T_ROOMEVENTTYPE		uint16									// 개별행사종류
// 비석
#define		T_TOMBID			uint16									// 방안에서 비석 ID
// Pet
#define		T_PETID				uint32									// 페트ID(ItemID)
#define		T_PETTYPEID			uint16									// 페트종류ID(물고기, 나무)
#define		T_PETREGIONID		uint32									// 페트령역ID
#define		T_PETGROWSTEP		uint8									// 페트단계
#define		T_PETLIFE			uint16									// 페트생명력
#define		T_PETCOUNT			uint16									// 페트개수
#define		T_PETPOS			ucstring								// 페트위치
#define		T_PETINCLIFEPER		float									// 페트생명력증가률
#define		T_PETCAREID			uint32									// 페트가꾸기id
// 소원
#define		T_WISHPOS			uint16									// 방에서 소원번호
// 움직이는 동물
#define		T_ANIMALID			uint32									// 종류id
#define		T_ANIMALSTATE		uint8									// 동물의 상태
#define		T_ANIMLAREGIONID	uint32									// 동물의 움직임령역ID

#define		NOFAMILYID			0										// 무효한 가족id
#define		NOROOMID			0										// 무효한 방id
#define		ISVALIDROOM(rid)	(rid != NOROOMID ? true : false)		// 유효한 방id인가
#define		NOFES				SIPNET::TServiceId(0)					// 무효한 FES
#define		NOITEMID			0										// 무효한 Item ID
#define		NOITEMPOS			0										// 무효한 Item Pos
#define		NOVALUE				0										// 무효한 값

// History data
#define		T_DATAID			uint32									// 사적자료ID
#define     T_ALBUMID           uint32                                  // albun id
#define     T_FRAMEID           uint32                                  // frame Id
#define		T_DATATYPE			uint8									// 사적자료Type (TPhotoType)
#define		T_AUTHKEY			uint32									// 사적자료Upload/Download요청 보안키
#define		T_DATASERVERID		uint8									// 사적자료서버 DB ID
#define		INVALID_DATASERVERID	0
#define		INVALID_ROOMID		0

enum{
	ROOMACT_NEWFLOWER		= 0x00000001,//
	ROOMACT_GIVEWATER		= 0x00000002,//
	ROOMACT_FOODFISH		= 0x00000004,//
	ROOMACT_XIANHUAXINGLI	= 0x00000008,//
	ROOMACT_YISHI			= 0x00000010,//
	ROOMACT_XIANBAO			= 0x00000020,//
	ROOMACT_XIEXIN			= 0x00000040,//
	ROOMACT_JIXIANGDENG		= 0x00000080,//
	ROOMACT_KONGMINGDENG	= 0x00000100,//
	ROOMACT_GUANSHANGYU		= 0x00000200,//
	ROOMACT_TESHIITEM		= 0x00000400,//
	ROOMACT_TESHICARD		= 0x00000800,//
	ROOMACT_MUSIC			= 0x00001000,//
	ROOMACT_ITEMUSED		= 0x00002000,//
	ROOMACT_ROOMSTORE		= 0x00004000,//
};
struct	PRICE_T
{
	T_PRICE		gmoney;
	T_PRICE		umoney;

	PRICE_T(T_PRICE gm = 0, T_PRICE um = 0) : gmoney(gm), umoney(um)	{}
	PRICE_T&	operator=(const PRICE_T& other)
	{
		gmoney = other.gmoney;
		umoney = other.umoney;
		return *this;
	}
};

// Inven과 Mail의 추가아이템목록을 관리하기 위한 구조체, 템플레이트
struct ITEM_INFO
{
	uint32	ItemID;			// 4 bytes
	uint16	ItemCount;		// 2 bytes
	uint64	GetDate;		// 8 bytes 복장아이템이 아닌 경우에는 0, 복장인 경우에도 전혀 사용하지 않은 새 아이템인 경우 0, 아닌 경우 사용시작시간
	uint32	UsedCharID;		// 4 bytes
	uint8	MoneyType;		// 1 bytes, MONEYTYPE_UMONEY = 1, MONEYTYPE_POINT = 2
};

template <int _MaxItemCount>
struct ITEM_LIST
{
	ITEM_INFO	Items[_MaxItemCount];

	ITEM_LIST()
	{
		memset(Items, 0, sizeof(Items));
	}

	bool fromBuffer(uint8 *buf, uint32 bufsize)
	{
		uint32	count = (uint32)(*buf);					buf ++;
		if((count > _MaxItemCount) || (count * 20 + 1 != bufsize))
			return false;

		uint8	pos;
		memset(Items, 0, sizeof(Items));
		for(uint32 i = 0; i < count; i ++)
		{
			pos = *((uint8*)buf);						buf += sizeof(uint8);
			if(Items[pos].ItemID != 0)
				return false;
			Items[pos].ItemID = *((uint32*)buf);		buf += sizeof(uint32);
			Items[pos].ItemCount = *((uint16*)buf);		buf += sizeof(uint16);
			Items[pos].GetDate = *((uint64*)buf);		buf += sizeof(uint64);
			Items[pos].UsedCharID = *((uint32*)buf);	buf += sizeof(uint32);
			Items[pos].MoneyType = *((uint8*)buf);		buf += sizeof(uint8);
			if (Items[pos].MoneyType == 0)
			{
				Items[pos].MoneyType = MONEYTYPE_UMONEY;
			}
		}
		return true;
	}

	bool toBuffer(uint8 *buf, uint32 *bufsize)
	{
		uint32	maxsize = (*bufsize);
		uint8*	buf0 = buf;			buf ++;
		(*buf0) = 0;
		uint32	realsize = 1;

		for(int i = 0; i < _MaxItemCount; i ++)
		{
			if(Items[i].ItemID == 0)
				continue;

			realsize += 20;
			if(realsize > maxsize)
				return false;

			if (Items[i].MoneyType == 0)
			{
				Items[i].MoneyType = MONEYTYPE_UMONEY;
			}

			*((uint8*)buf) = (uint8)i;				buf += sizeof(uint8);
			*((uint32*)buf) = Items[i].ItemID;		buf += sizeof(uint32);
			*((uint16*)buf) = Items[i].ItemCount;	buf += sizeof(uint16);
			*((uint64*)buf) = Items[i].GetDate;		buf += sizeof(uint64);
			*((uint32*)buf) = Items[i].UsedCharID;	buf += sizeof(uint32);
			*((uint8*)buf) = Items[i].MoneyType;	buf += sizeof(uint8);

			(*buf0) ++;
		}

		(*bufsize) = realsize;

		return true;
	}

	void CopyFrom(ITEM_LIST *pSrc)
	{
		memcpy(Items, pSrc->Items, sizeof(Items));
	}
};

template <class S_DATA, int _MaxCount>
class SDATA_LIST
{
public:
	uint32	DataCount;
	S_DATA	Datas[_MaxCount];

	SDATA_LIST()
	{
		DataCount = 0;
		memset(Datas, 0, sizeof(Datas));
	}

	bool fromBuffer(uint8 *buf, uint32 bufsize)
	{
		if (bufsize == 0)
		{
			DataCount = 0;
			memset(Datas, 0, sizeof(Datas));
			return true;
		}

		DataCount = (uint32)(*buf); buf ++;
#ifdef SIP_OS_WINDOWS
		if((DataCount > _MaxCount) || (1 + DataCount * sizeof(S_DATA) != bufsize))
			return false;

		memcpy(Datas, buf, DataCount * sizeof(S_DATA));
#else
		bool bDiffSize = S_DATA::isDiffSize();
		if (bDiffSize)
		{
			uint32 size = S_DATA::getSize();
			if((DataCount > _MaxCount) || (1 + DataCount * size != bufsize))
				return false;

			uint8 *temp = (uint8 *)calloc(0, size);
			for (int i = 0; i < DataCount; i++) {
				memcpy(temp, buf + i * size, size);
				Datas[i].serialize(temp);
			}
		} 
		else
		{
			if((DataCount > _MaxCount) || (1 + DataCount * sizeof(S_DATA) != bufsize))
				return false;

			memcpy(Datas, buf, DataCount * sizeof(S_DATA));
		}
#endif
		return true;
	}

	bool toBuffer(uint8 *buf, uint32 *bufsize)
	{
		uint32	maxsize = (*bufsize);
		uint32	realsize = 1 + DataCount * sizeof(S_DATA);

		if(realsize > maxsize)
			return false;

		(*buf) = (uint8)DataCount;	buf ++;
		memcpy(buf, Datas, realsize - 1);
		(*bufsize) = realsize;

		return true;
	}
};

struct SITE_ACTIVITY
{
	uint32		m_ActID;
	ucstring	m_Text;
	uint8		m_Count;
	uint32		m_ActItemID[MAX_SITE_ACTIVITY_ITEMCOUNT];
	uint8		m_ActItemCount[MAX_SITE_ACTIVITY_ITEMCOUNT];
	SIPBASE::MSSQLTIME	m_BeginTime;
	SIPBASE::MSSQLTIME	m_EndTime;
	SITE_ACTIVITY() : m_ActID(0), m_Count(0) {}
	void	Reset() { m_ActID = 0; m_Count = 0;}
};

struct SYSTEM_GIFTITEM
{
	uint32		m_GiftID;
	ucstring	m_Title;
	ucstring	m_Content;
	uint8		m_Count;
	uint32		m_ItemID[MAX_SITE_ACTIVITY_ITEMCOUNT];
	uint8		m_ItemCount[MAX_SITE_ACTIVITY_ITEMCOUNT];
	SYSTEM_GIFTITEM() : m_GiftID(0) {}
};

struct LuckHistory
{
	LuckHistory(T_FAMILYID _FID, T_ROOMID _RoomID, uint8 _LuckLevel, uint32 _LuckID, uint64 _LuckDBTime)
	{
		FID			= _FID;
		RoomID		= _RoomID;
		LuckLevel	= _LuckLevel;
		LuckID		= _LuckID;
		LuckDBTime	= _LuckDBTime;
	}
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	uint8			LuckLevel;
	uint32			LuckID;
	uint64			LuckDBTime;
};

enum	_CHITADDTYPE{
	CHITADDTYPE_ADD_IF_OFFLINE = 1,		// targetFID가 온라인이면 직접 보내고, 오프라인이면 DB에 추가한다.
	CHITADDTYPE_ADD_ONLY,				// DB에 추가한다.
	CHITADDTYPE_ADD_AND_SEND,			// DB에 추가한 다음 그 Chit를(ChitID와 함께) targetFID에게 보낸다.
};

enum USERCODE
{
	UNKOWN_USER,
	SYSTEM_USER,
	COMMON_USER
};

#endif // TIAN_SERVER_COMMON_H
