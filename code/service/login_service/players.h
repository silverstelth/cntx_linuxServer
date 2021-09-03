#ifndef SIP_PLAYERS_H
#define SIP_CONNECTSIP_PLAYERS_HION_WS_H

#include "misc/types_sip.h"
#include "net/callback_server.h"

bool ReadFriendInfo(T_FAMILYID FID);
bool SaveFriendGroupList(T_FAMILYID FID);
void SendFriendListToSS(uint32 userid, uint32 fid, uint16 sid);

void cb_M_CS_ADDCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_DELCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_RENCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_SETCONTGROUP_INDEX(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_ADDCONT(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_DELCONT(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_CHANGECONTGRP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_REQ_FRIEND_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_REQ_FAMILY_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);

void Send_M_NT_FRIEND_DELETED(T_FAMILYID FID, T_FAMILYID friendFID);
void Send_M_NT_FRIEND_SN(T_FAMILYID FID, uint32 sn, bool sendToMainShard, TServiceId tsid);



bool ReadFavorRoomInfo(T_FAMILYID FID);
bool SaveFavorRoomGroupList(T_FAMILYID FID);
void SendFavorRoomListToSS(uint32 userid, uint32 fid, uint16 sid);

void cb_M_CS_INSFAVORROOM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_DELFAVORROOM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_MOVFAVORROOM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_NEW_FAVORROOMGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_DEL_FAVORROOMGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_REN_FAVORROOMGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid);


// Chit
void cb_M_NT_ADDCHIT(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_RESCHIT(CMessage &msgin, const std::string &serviceName, TServiceId tsid);

// Yuyue
void SendYuyueListToSS(uint32 userid, uint32 fid, uint16 sid);
void cb_M_NT_AUTOPLAY3_REGISTER(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void LoadAllYuyueDatas();
void UpdateYuyueDatas();
void cb_M_NT_AUTOPLAY3_START_OK(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void UpdateYuyueExpDatas();
void cb_M_NT_AUTOPLAY3_EXP_ADD_OK(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_AUTOPLAY3_FAIL(CMessage &msgin, const std::string &serviceName, TServiceId tsid);

#endif // SIP_PLAYERS_H

/* End of players.h */
