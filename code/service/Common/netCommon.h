//////////////////////////////////////////////////////////////////////////
/* Common User file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#ifndef _NETCOMMON_H_
#define _NETCOMMON_H_

#include "../../common/Macro.h"
#define	SECOND_PERONEDAY		86400

void SendFailMessageToClient(uint16 errNo, SIPNET::TTcpSockId to, SIPNET::CCallbackNetBase& clientcb );
void SendFailMessageToServer(ucstring reason, SIPNET::TServiceId sid, uint32 dbid);
void SendFailMessageToServer(uint16 errNo, SIPNET::TServiceId sid, uint32 dbid);
void SendFailMessageToServer(uint16 errNo, std::string sname, uint32 dbid);
void SendSystemNotice(uint8 uType, ucstring text);

bool GoldEncodeByAccount(const char* id, uint64& value1, uint64& value2);
std::string GetGoldEncodeStringByAccount(std::string sAccountID, uint32 CurMoney);
bool GoldDecodeByAccount(const char* id, uint64& value1, uint64& value2);
bool	IsSystemAccount(uint32 _id);
bool	IsSystemRoom(uint32 _roomid);
bool	IsSameDay(uint64 second1, uint64 second2);
ucstring GetTimeStrFromM1970(uint32 timeM);
ucstring GetTimeStrFromS1970(sint64 timeS);

struct SITE_CHECKIN
{
	uint32		m_ActID;
	bool		m_bActive;
	ucstring	m_Text;
	SIPBASE::MSSQLTIME	m_BeginTime;
	SIPBASE::MSSQLTIME	m_EndTime;

	struct CHECKINITEM
	{
		uint8		m_Count;
		uint32		m_ItemID[MAX_SITE_CHECKIN_ITEMCOUNT];
		uint8		m_ItemCount[MAX_SITE_CHECKIN_ITEMCOUNT];
	};
	CHECKINITEM		m_CheckInItems[CIS_MAX_DAYS];

	SITE_CHECKIN() { Reset(); }
	void	Reset() { m_ActID = 0; m_bActive = false; memset(m_CheckInItems, 0, sizeof(m_CheckInItems)); }
	sint8	GetNextCheckDays(uint32 today, sint8 currentCheckDays, uint32 lastCheckDay);
};

#endif //_NETCOMMON_H_
