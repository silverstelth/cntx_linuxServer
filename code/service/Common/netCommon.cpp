//////////////////////////////////////////////////////////////////////////
/* Common User file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <vector>
#include <map>

#include "net/service.h"
#include "net/SecurityServer.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "../Common/netCommon.h"
#include "../../common/Macro.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

// When send to Client of CallbackServer
void SendFailMessageToClient(uint16 errNo, TTcpSockId to, CCallbackNetBase& clientcb )
{
	CMessage msgout(M_SC_FAIL);
	msgout.serial(errNo);

	CCallbackServer * server = dynamic_cast<CCallbackServer *>( &clientcb );
	server->send(msgout, to);
}

// When send to Client of Service
void SendFailMessageToServer(ucstring reason, TServiceId sid, uint32 userid)
{
	CMessage msgout(M_SC_FAIL);
	msgout.serial(reason);
	msgout.serial(userid);
	CUnifiedNetwork::getInstance()->send((TServiceId)sid, msgout);
}

// When send to Client of Service
void SendFailMessageToServer(uint16 errNo, TServiceId sid, uint32 userid)
{
	CMessage msgout(M_SC_FAIL);
	msgout.serial(errNo);
	msgout.serial(userid);
	CUnifiedNetwork::getInstance()->send((TServiceId)sid, msgout);
}

// When send to Client of Service
void SendFailMessageToServer(uint16 errNo, string sname, uint32 userid)
{
	CMessage msgout(M_SC_FAIL);
	msgout.serial(errNo);
	msgout.serial(userid);
	CUnifiedNetwork::getInstance()->send(sname, msgout);
}

void SendSystemNotice(uint8 uType, ucstring text)
{
	CMessage	msgout(M_SC_CHATSYSTEM);
	msgout.serial(uType, text);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgout);
}

bool GoldEncodeByAccount(const char* id, uint64& value1, uint64& value2)
{
	long key1 = 0;
	long key2 = 0;
	int length = static_cast<int>(strlen(id));
	for(int i = 0; i < length; i ++)
	{
		if(i % 2)
		{
			key2 += (long)id[i];
			if(id[i] >= 97 && id[i] <= 122)
			{
				key2 -= 32;
			}
		}
		else
		{
			key1 += (long)id[i];
			if(id[i] >= 97 && id[i] <= 122)
			{
				key1 -= 32;
			}
		}
	}
	key1 = (key1 % 128) + 1;
	key2 = (key2 % 128) + 1;

	value1 = value1 * key1 + 1;
	value2 = value2 * key2 + 1;
	return true;
}

string GetGoldEncodeStringByAccount(string sAccountID, uint32 CurMoney)
{
	uint64 gold1 = CurMoney;
	uint64 gold2 = CurMoney;

	GoldEncodeByAccount(sAccountID.c_str(), gold1, gold2);

	char strGold[21];
	for(long i = 0; i < 20; i ++)
	{
		strGold[i] = '0';
	}
	strGold[20] = 0;

	char tempStr[11];
	size_t strlenth;
#ifdef SIP_OS_WINDOWS
	_i64toa(gold1, tempStr, 16);
#else
	sprintf(tempStr, "%lx", gold1);
#endif
	strlenth = strlen(tempStr);
	memcpy(&strGold[10 - strlenth], tempStr, strlenth);
#ifdef SIP_OS_WINDOWS
	_i64toa(gold2, tempStr, 16);
#else
	sprintf(tempStr, "%lx", gold2);
#endif
	strlenth = strlen(tempStr);
	memcpy(&strGold[20 - strlenth], tempStr, strlenth);

	return string(strGold);
}

bool GoldDecodeByAccount(const char* id, uint64& value1, uint64& value2)
{
	long key1 = 0;
	long key2 = 0;
	int length = static_cast<int>(strlen(id));
	for( int i = 0; i < length; i++ )
	{
		if(i % 2)
		{
			key2 += (long)id[i];
			if(id[i] >= 97 && id[i] <= 122)
			{
				key2 -= 32;
			}
		}
		else
		{
			key1 += (long)id[i];
			if(id[i] >= 97 && id[i] <= 122)
			{
				key1 -= 32;
			}
		}
	}
	key1 = (key1 % 128) + 1;
	key2 = (key2 % 128) + 1;

	value1 -= 1;
	value2 -= 1;
	if((value1 % key1) || (value2 % key2))
	{
		return false;
	}

	value1 /= key1;
	value2 /= key2;
	return true;
}

bool	IsSystemAccount(T_ACCOUNTID _id)
{
	if (_id >= SYSTEMUSERIS_START &&
		_id <= SYSTEMUSERIS_END)
		return true;
	return false;
}

bool	IsSystemRoom(T_ROOMID _roomid)
{
	if (_roomid >= SYSTEMROOM_START &&	_roomid <= SYSTEMROOM_END)
		return true;
	if (_roomid >= MIN_RELIGIONROOMID && _roomid <= MAX_RELIGIONROOMID)
		return true;

	return false;
}

bool IsSameDay(uint64 second1, uint64 second2)
{
	second1 /= SECOND_PERONEDAY;
	second2 /= SECOND_PERONEDAY;
	return (second1 == second2);
}

ucstring GetTimeStrFromM1970(uint32 timeM)
{
	sint64	timeS = timeM;
	timeS *= 60;
	return GetTimeStrFromS1970(timeS);
}

ucstring GetTimeStrFromS1970(sint64 timeS)
{
	struct tm *time = gmtime((time_t*)&timeS);

	ucchar	buf[50];
	SIPBASE::smprintf(buf, 50, L"%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

	return ucstring(buf);
}

sint8	SITE_CHECKIN::GetNextCheckDays(uint32 today, sint8 currentCheckDays, uint32 lastCheckDay)
{
	sint8		nCheckinDays = CIS_CANT_CHECK;

	if (m_ActID != 0)
	{
		uint32 deltaDays = today - lastCheckDay;

		if (deltaDays == 0)	// possible to checkin one time per one day.
		{
			nCheckinDays = CIS_CANT_CHECK;
		}
		else
		{
			sint8 nCandiIndex = -1;
			if (deltaDays == 1)	// If serial checkin, set next index
			{
				nCandiIndex = currentCheckDays;
			}
			else	// If no serial checkin, reset
			{
				nCandiIndex = 0;
			}

			if (nCandiIndex == 0)	// If starting checkin
			{
				if (today < m_BeginTime.GetDaysFrom1970() ||
					today > m_EndTime.GetDaysFrom1970()) 
				{
					nCheckinDays = CIS_CANT_CHECK;
				}
				else
				{
					if (!m_bActive)	
					{
						nCheckinDays = CIS_CANT_CHECK;
					}
					else 
					{
						nCheckinDays = 1;
					}
				}

			}
			else // If serial checkin, success without fail
			{
				nCheckinDays = nCandiIndex + 1;
				if (nCheckinDays >= CIS_MAX_DAYS)
					nCheckinDays = 0;
			}
		}
	}
	return nCheckinDays;
}
