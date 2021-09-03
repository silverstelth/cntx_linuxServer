#ifndef TIAN_MANAGER_MAIN_H
#define TIAN_MANAGER_MAIN_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

void SendMsgToOpenroomForCreate(T_ROOMID RoomID, SIPNET::CMessage &msgOut);
void SendMsgToRoomID(T_ROOMID RoomID, SIPNET::CMessage &msgOut);
void SendChit(uint32 srcFID, uint32 targetFID, uint8 ChitType, uint32 p1, uint32 p2, ucstring &p3, ucstring &p4);

extern	uint8	ShardMainCode[25];
bool IsThisShardRoom(uint32 RoomID);
bool IsThisShardFID(uint32 FID);

#endif	// TIAN_MANAGER_MAIN_H
