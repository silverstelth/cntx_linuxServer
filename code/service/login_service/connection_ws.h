#ifndef SIP_CONNECTION_WS_H
#define SIP_CONNECTION_WS_H

#include "misc/types_sip.h"
#include "net/callback_server.h"

//extern SIPNET::CCallbackServer *WSServer;

void connectionWSInit ();
void connectionWSUpdate ();
void connectionWSRelease ();

void RefreshMoveDataUserList();

uint32 SendMsgToShardByCode(CMessage &msg, uint8 ShardCode);
uint32 SendMsgToShardByID(CMessage &msg, uint32 ShardID);
bool SendPacketToFID(T_FAMILYID FID, CMessage msgOut);

CShardInfo* findShardWithSId(uint16 sid);

void ResetSomeVariablesWhenLogin(uint32 UID);

uint32 GetCurrentShardID(uint32 UID);

bool GetCurrentSafetyServer(uint32 FID, uint16 &OnlineWsSid, uint32 &OfflineShardID);

#endif // SIP_CONNECTION_WS_H

/* End of connection_ws.h */
