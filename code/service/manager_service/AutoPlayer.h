#ifndef TIAN_AUTOPLAY_CAUTOPLAYER_H
#define TIAN_AUTOPLAY_CAUTOPLAYER_H

// Autoplay夸没磊丰甫 焊包窍绰 努扼胶
class AutoPlayUnit
{
public:
	uint32			m_UnitID;

	T_FAMILYID		m_FID;
	T_FAMILYNAME	m_FamilyName;
	T_MODELID		m_ModelID;
	T_DRESS			m_DressID;
	T_FACEID		m_FaceID;
	T_HAIRID		m_HairID;
	T_ROOMID		m_RoomID;
	uint8			m_ActPos;
	uint32			m_IsXDG;
	ucstring		m_Pray;
	uint8			m_SecretFlag;
	uint32			m_ItemID[9];
	uint32			m_UsedItemID;
	uint8			m_UsedItemMoneyType;

//	uint32			m_LastTryTime;

	AutoPlayUnit& operator=(AutoPlayUnit& other)
	{
		m_FID = other.m_FID;
		m_FamilyName = other.m_FamilyName;
		m_ModelID = other.m_ModelID;
		m_DressID = other.m_DressID;
		m_FaceID = other.m_FaceID;
		m_HairID = other.m_HairID;
		m_RoomID = other.m_RoomID;
		m_ActPos = other.m_ActPos;
		m_IsXDG = other.m_IsXDG;
		m_Pray = other.m_Pray;
		m_SecretFlag = other.m_SecretFlag;
		memcpy(m_ItemID, other.m_ItemID, sizeof(m_ItemID));
		m_UsedItemID = other.m_UsedItemID;
		m_UsedItemMoneyType = other.m_UsedItemMoneyType;
//		m_LastTryTime = other.m_LastTryTime;
		return *this;
	}

	AutoPlayUnit() {};

	AutoPlayUnit(T_FAMILYID _FID, T_FAMILYNAME &_FamilyName, T_MODELID _ModelID, T_DRESS _DressID, T_FACEID _FaceID, T_HAIRID _HairID,
		T_ROOMID _RoomID, uint8 _ActPos, uint32 _IsXDG, ucstring &_Pray, uint8 _SecretFlag, uint32 *_ItemID, uint32 _UsedItemID, uint8 _UsedItemMoneyType)
		:	m_FID(_FID), m_RoomID(_RoomID), m_ActPos(_ActPos), m_IsXDG(_IsXDG), m_SecretFlag(_SecretFlag),
		m_ModelID(_ModelID), m_DressID(_DressID), m_FaceID(_FaceID), m_HairID(_HairID), m_UsedItemID(_UsedItemID), m_UsedItemMoneyType(_UsedItemMoneyType)
	{
		m_FamilyName = _FamilyName;
		m_Pray = _Pray;
		memcpy(m_ItemID, _ItemID, sizeof(m_ItemID));
//		m_LastTryTime = 0;
	}
};

typedef std::list<AutoPlayUnit>	TAutoPlayList;
extern	TAutoPlayList	AutoPlayUnitList;

class CAutoPlayPacketBase
{
public:
	//enum
	//{
	//	Item_GongPin1,
	//	Item_GongPin2,
	//	Item_GongPin3,
	//	Item_GongPin4,
	//	Item_GongPin5,
	//	Item_GongPin6,
	//	Item_Hua,
	//	Item_Xiang,
	//	Item_Jiu,
	//	Item_TianTeshi0 = 0,
	//	Item_TianTeshi1 = 1,
	//	Item_TianTeshi2 = 2,
	//	Item_TianTeshi3 = 3,
	//};

	MsgNameType	m_PacketName;
	uint32		m_Time;

	/*
	static string toMBCSString(tchar *usctr)
	{
		char	str[256];
		WideCharToMultiByte(CP_ACP, 0, usctr, -1, str, sizeof(str), 0, 0);
		return string(str);
	}
	*/

	static uint32 fromFloatStringToUint32(tchar *usctr)
	{
		float	fValue = std::wcstof(usctr, NULL); // (float)_wtof(usctr);
		uint32	nValue = *((uint32*)(&fValue));
		return nValue;
	}

	static float fromFloatStringToFloat(tchar *usctr)
	{
		float	fValue = std::wcstof(usctr, NULL); // (float)_wtof(usctr);
		return  fValue;
	}

	virtual void serial(CMessage &msgOut, uint32 *pItemID, string *pNPCNames)
	{
		sipwarning("====");
	}
};

// M_CS_ACT_START: Start Activity ([b:ActPos][d:Param 특수한 용도에 사용, 조상탑 고인행사인경우 1:꽃드리기 2:향드리기][b:GongpinCount][[d:GongpinItemID]][b:BowType])
class CAutoPlayPacket_M_CS_ACT_START : public CAutoPlayPacketBase
{
public:
	uint8	m_ActPos;
	uint32	m_Param;
	uint8	m_GongpinCount;
	uint8	m_BowType;

	CAutoPlayPacket_M_CS_ACT_START(uint32 time)
	{
		m_PacketName = M_CS_ACT_START;
		m_Time = time;
	}

	void InitFromString(tchar *strLine)
	{
		int n = 0;
#ifdef SIP_OS_WINDOWS
		tchar	*token = wcstok(strLine, L",");
#else
		tchar *state;
		tchar	*token = wcstok(strLine, L",", &state);
#endif
		while (token != NULL)
		{
			switch (n)
			{
			case 0:		m_ActPos = (uint8)std::wcstod(token, NULL);	break;// _wtoi(token);			break;
			case 1:		m_Param = (uint32)std::wcstod(token, NULL);	break;// _wtoi(token);				break;
			case 2:		m_GongpinCount = (uint8)std::wcstod(token, NULL);	break;//_wtoi(token);		break;
			default:
				if (n == m_GongpinCount + 3)
					m_BowType = (uint8)std::wcstod(token, NULL);	// _wtoi(token);
				else if (n > m_GongpinCount + 3)
					sipwarning("Invalid Parameter.");
				break;
			}
			// token = wcstok(NULL, L",", &state);
			n ++;
			break;
		}
	}

	virtual void serial(CMessage &msgOut, uint32 *pItemID, string *pNPCNames)
	{
		msgOut.serial(m_ActPos, m_Param, m_GongpinCount);
		for (int i = 0; i < (int)m_GongpinCount; i ++)
		{
			msgOut.serial(pItemID[i]);
		}
		msgOut.serial(m_BowType);
	}
};

// M_CS_ACT_RELEASE: Release Activity
class CAutoPlayPacket_M_CS_ACT_RELEASE : public CAutoPlayPacketBase
{
public:
	CAutoPlayPacket_M_CS_ACT_RELEASE(uint32 time)
	{
		m_PacketName = M_CS_ACT_RELEASE;
		m_Time = time;
	}

	virtual void serial(CMessage &msgOut, uint32 *pItemID, string *pNPCNames)
	{
	}
};

// M_CS_ACT_STEP: Send Act Step Command ([b:ActPos][d:StepID])
class CAutoPlayPacket_M_CS_ACT_STEP : public CAutoPlayPacketBase
{
public:
	uint8	m_ActPos;
	uint32	m_StepID;

	CAutoPlayPacket_M_CS_ACT_STEP(uint32 time)
	{
		m_PacketName = M_CS_ACT_STEP;
		m_Time = time;
	}

	void InitFromString(tchar *strLine)
	{
		int n = 0;
#ifdef SIP_OS_WINDOWS
		tchar	*token = wcstok(strLine, L",");
#else
		tchar *state;
		tchar	*token = wcstok(strLine, L",", &state);
#endif
		while (token != NULL)
		{
			switch (n)
			{
			case 0:		m_ActPos = (uint8)std::wcstod(token, NULL);	// _wtoi(token);			break;
			case 1:		m_StepID = (uint32)std::wcstod(token, NULL);	// _wtoi(token);			break;
			case 2:		sipwarning("Invalid Parameter.");	break;
			}
			// token = wcstok(NULL, L",", &state);
			n ++;
			break;
		}
	}

	virtual void serial(CMessage &msgOut, uint32 *pItemID, string *pNPCNames)
	{
		msgOut.serial(m_ActPos, m_StepID);
	}
};

// 概 纠喊, 概 ActPos喊 Autoplay Packet格废阑 焊包窍绰 努扼胶
class CAutoPlayData
{
public:
//	uint16	m_SceneID;
//	uint8	m_ModelID;
	uint8	m_ActPos;
	uint32	m_IsXDG;
//	uint32	m_State;
//	uint32	m_StartX, m_StartY, m_StartZ, m_StartDir;

	std::vector<CAutoPlayPacketBase*>		m_AutoPlayPacketList;

	CAutoPlayData() {};
	~CAutoPlayData()
	{
		for (uint32 i = 0; i < m_AutoPlayPacketList.size(); i ++)
			delete m_AutoPlayPacketList[i];
		m_AutoPlayPacketList.clear();
	}

	void	Init(uint8 _ActPos, uint32 _IsXDG, ucstring *pAutoPlayData);
};


// Autoplay甫 角青窍绰 林眉
class CAutoPlayer
{
public:
	enum
	{
		STATUS_READY,
		STATUS_CHECKING,
		STATUS_FAIL,
		STATUS_PLAYING,
		STATUS_FINISHED,
	};

	CAutoPlayer(void);
	~CAutoPlayer(void);

	void init(T_FAMILYID _AutoPlayerFID);
	void start(AutoPlayUnit &data);
	void update();

	T_FAMILYID		m_AutoPlayerFID;
	uint32			m_Status;
	uint16			m_SceneID;
	uint8			m_RoomLevel;

	AutoPlayUnit	m_AutoPlayUnit;
	CAutoPlayData	*m_pAutoPlayData;
	//string			*m_pDeskName;

	TTime			m_StartTime;
	uint32			m_CurPacketIndex;

	// Packet Callback Functions
	void	on_M_NT_AUTOPLAY_ANS(uint32 result, uint32 roomID, uint16 sceneID, uint8 roomLevel);
};

extern CAutoPlayer		AutoPlayers[AUTOPLAY_FID_COUNT];

extern ucstring			g_AutoPlayData_ActPos1[];
extern ucstring			g_AutoPlayData_ActPos3[];
extern ucstring			g_AutoPlayData_ActPos83[];
extern ucstring			g_AutoPlayData_ActPos1_XDG[];

void InitAutoPlayers();
void UpdateAutoPayers();
void RegisterActionMemo(uint32 UnitID);

void cb_M_NT_AUTOPLAY_REGISTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_ADDACTIONMEMO_OK(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);

void cb_M_NT_AUTOPLAY3_START_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);

void RemoveAutoplayDatas(uint32 RoomID, uint32 Param);

#endif // TIAN_AUTOPLAY_CAUTOPLAYER_H
