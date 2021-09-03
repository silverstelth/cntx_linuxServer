#pragma once

//#define MAKEQUERY_CHECKAUTH(strQ)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_checkAuth (?,?,?,?,?,?)}"));
#define MAKEQUERY_CHECKAUTH_2(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_checkAuth2 (?,?,?,?,?,?,?)}"));

#define MAKEQUERY_CheckAuth2(strQ, UserID, twopassword)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_checkAuth_Twopassword (%d, '%S', ?)}"), UserID, twopassword);

//#define MAKEQUERY_CHECKAUTH_GM(strQ)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_checkAuth_GM (?,?,?,?,?,?)}"));
#define MAKEQUERY_CHECKAUTH_GM_2(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_checkAuth_GM2 (?,?,?,?,?,?,?)}"));

#define MAKEQUERY_SetUserFID(strQ, UserID, FID, FamilyName)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_setFID2(%d, %d, N'%s')}"), UserID, FID, FamilyName.c_str());

#define MAKEQUERY_GetUserOnlineStatus(strQ, UserID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_UserBaseinfo_Oline_List(%d)}"), UserID);

#define MAKEQUERY_SetUserOnlineStatus(strQ, UserID, OnlineStatus)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_UserBaseinfo_Update_Oline(%d, %d)}"), UserID, OnlineStatus);

//#define MAKEQUERY_GETONLINESHARD(strQ)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_getSHARDonline"));

#define MAKEQUERY_INSERTUSER(strQ, UserID, strName, strPass, strPass2, UserMoney, strUserMoney, addJifen)	\
	tchar strQ[512] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_insertUSER (%d, '%S', '%S', '%S', %d, '%S', %d, ?)}"), UserID, strName, strPass, strPass2, UserMoney, strUserMoney, addJifen);

// Shard
#define MAKEQUERY_DISCONNECTSHARD(strQ, shardID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_disconSHARD %u"), shardID);

#define MAKEQUERY_ACCEPTNEWSHARD(strQ, shardID, strName, zoneId, nUserLimit, ucContent)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_acceptNewShard %u, N'%s', %u, %u, N'%s'"), shardID, strName, zoneId, nUserLimit, ucContent);

#define MAKEQUERY_UPDATESHARDINFO(strQ, shardID, strName, zoneId, nUserLimit, ucContent)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_updateSHARDinfo %u, N'%s', %u, %u, N'%s'"), shardID, strName, zoneId, nUserLimit, ucContent);

#define	MAKEQUERY_SHARDON(strQ, shardID)					\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_setOnline %u"), shardID);

#define MAKEQUERY_LOGINUSERTOSHARD(strQ, userid, shardID, strIp, nGMoney)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_user_loginUSERtoShard1 %u, %u, '%S', %u"), userid, shardID, strIp.c_str(), nGMoney);

#define MAKEQUERY_LOGOFFUSERTOSHARD(strQ, userid, shardID, nRooms, nGMoney, bRightValue)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_user_logoffUSERtoShard1 %u, %u, %u, %u, %u"), userid, shardID, nRooms, nGMoney, bRightValue);

#define MAKEQUERY_LOGINUSERTOSHARD_GM(strQ, userid, shardID, strIp, nGMoney)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_user_loginUSERtoShard_GM1 %u, %u, '%S', %u"), userid, shardID, strIp.c_str(), nGMoney);

#define MAKEQUERY_LOGOFFUSERTOSHARD_GM(strQ, userid, shardID, nRooms, nGMoney, bRightValue)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_user_logoffUSERtoShard_GM1 %u, %u, %u, %u, %u"), userid, shardID, nRooms, nGMoney, bRightValue);

//#define MAKEQUERY_GETSHARDINFO(strQ, shardID)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_getShardInfo %u"), shardID);

#define MAKEQUERY_CLEANDB(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_cleanInit}"));

//#define MAKEQUERY_LOADZONE(strQ)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_loadZonetbl"));

#define MAKEQUERY_LOADGMADDR(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_mstGMAddress_LoadInfo_GM"));

//#define MAKEQUERY_LOADSHARDSTATE(strQ)	\
//	tchar strQ[128] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_shard_loadShardState}")); \

//#define MAKEQUERY_LOADSHARDVIPS(strQ)	\
//	tchar strQ[128] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_loadShardVIPs"));





/////////////////////////


//#define MAKEQUERY_LOADSERVERLIST(strQ, userid, privilege)  \
//	tchar strQ[128] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_getSHARDonlinenew %u, %u"), userid, privilege);

//#define MAKEQUERY_LOADSERVERLIST_GM(strQ, userid, privilege)  \
//	tchar strQ[128] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_getSHARDonlinenew_GM %u, %u"), userid, privilege);


///////////////////////

// Update
#define MAKEQUERY_LOADLASTVERSIONS(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_mstApplications_LoadInfo"));

#define MAKEQUERY_LOADUPDATESERVERS(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_mstUpdateServers_Loadinfo"));

#define MAKEQUERY_LOADUPDATEINFO(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_mstUpdateAppsInfo_LoadInfo"));

//#define MAKEQUERY_CLEANTABLESFORUPDATE(strQ)	\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_update_cleanTablesForUpdate"));

//#define	MAKEQUERY_USERMONEY(strQ, flag, moneyExpend, maintype, subtype, userid, familyid, itemid, itemtypeid, v1, v2, v3, shardId, usertypeid)	\
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("exec Main_user_money %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u"), flag, moneyExpend, maintype, subtype, userid, familyid, itemid, itemtypeid, v1, v2, v3, shardId, usertypeid);

//// check the user's money is enough or not
//#define MAKEQUERY_CANUSEMONEY(strQ, flag, moneyExpend, userid)   \
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("exec Main_user_money_One %u, %u, %u"), flag, moneyExpend, userid); 

//#define	MAKEQUERY_GetUserMoney(strQ, idUser)	\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_user_getMoney(%u, ?)}"), idUser);

#define	MAKEQUERY_GetUserMoney(strQ, UserID)	\
	tchar strQ[300] = _t("");			\
	SIPBASE::smprintf(strQ, 128, _t("exec Main_user_money_get %d"), UserID);

#define	MAKEQUERY_SetUserMoney(strQ, UserID, curMoney, sMoney, OriginalMoney, sOriginalMoney, MainType, SubType, FamilyID, ItemID, ItemTypeID, v1, v2, v3, ShardID, UserTypeID)	\
	tchar strQ[300] = _t("");			\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_user_money_set(%d, %d, %d, '%S', '%S', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ?)}"), UserID, curMoney, OriginalMoney, sMoney.c_str(), sOriginalMoney.c_str(), MainType, SubType, FamilyID, ItemID, ItemTypeID, v1, v2, v3, ShardID, UserTypeID);

// get all the shards' info from DB
#define MAKEQUERY_GETSHARDLIST(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_shard_list;"));

#define MAKEQUERY_LOADIPMNT(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("EXEC Main_mstIPMnt_List;"));

/////////////////////
// for User Item
/////////////////////

//// Get NewUser's Item
//#define MAKEQUERY_GetNewUserItem(strQ, userid)   \
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_Shard_Items_Get_NewUser(%d, ?)}"), userid);

//// Get User's Item List
//#define MAKEQUERY_GetUserItemList(strQ, userid)   \
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_Shard_Items_List(%d, ?)}"), userid);

//// Get User's Item
//#define MAKEQUERY_GetUserItem(strQ, userid, IndexID, ItemID, Count)   \
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_Shard_Items_Get(%d, %d, %d, %d, ?)}"), userid, IndexID, ItemID, Count);

//// Back User's Item
//#define MAKEQUERY_BackUserItem(strQ, IndexID, Count)   \
//	tchar strQ[300] = _t("");			\
//	SIPBASE::smprintf(strQ, 128, _t("{call Main_Shard_Items_Back(%d, %d, ?)}"), IndexID, Count);

//// Tianyuan Card Check
//#define MAKEQUERY_RoomCard_Check(strQ, CardID)	\
//	tchar strQ[256] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("{CALL CEUserDB_Card_Room_checkAuth('%S', ?, ?)}"), CardID.c_str());
//
//// Tianyuan Card Use
//#define MAKEQUERY_RoomCard_Use(strQ, CardID, UserID, UserName, UserIP, flag)	\
//	tchar strQ[256] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("{CALL CEUserDB_Card_Room_Use('%S', %d, '%s', '%S', %d, ?, ?)}"), CardID.c_str(), UserID, UserName.c_str(), UserIP.c_str(), flag);

// Tianyuan Card Use
#define MAKEQUERY_RoomCard_Use(strQ, CardID, UserID, UserIP)	\
	tchar strQ[256] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL CEUserDB_Card_Room_Use('%S', %d, '%S', ?, ?)}"), CardID.c_str(), UserID, UserIP.c_str());

#define MAKEQUERY_GetCurrentCheckIn(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_mstSignin_day_List}"));

// 劝悼包访
#define MAKEQUERY_GetCurrentActivity(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_Activity_Items_List}"));

#define MAKEQUERY_GetUserLastActivity(strQ, UserID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_User_ActIDID_Get(%d)}"), UserID);

#define MAKEQUERY_SetUserLastActivity(strQ, UserID, ActID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_User_ActIDID_Update1(%d, %d, ?)}"), UserID, ActID);

#define MAKEQUERY_GetBeginnerMstItem(strQ)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_mstRegister_items_list}"));

#define MAKEQUERY_GetUserLastActivityAndIsGetItem(strQ, UserID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_User_ActIDIDAndIsGetItem_Get(%d)}"), UserID);

#define MAKEQUERY_SetUserBeginnerItem(strQ, UserID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_User_IsGetItem_Update1(%d, ?)}"), UserID);


#define MAKEQUERY_GetAddedJifen(strQ, UserID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_AwardWeb_Get(%d)}"), UserID);

#define MAKEQUERY_UpdateAddedJifen(strQ, ID)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_AwardWeb_Update(%d)}"), ID);

// Crash结滚沥焊 结持扁
#define MAKEQUERY_SERVER_CRASH_LOG(strQ, strLog)					\
	tchar strQ[1000] = _t("");																	\
	SIPBASE::smprintf(strQ, 128, _t("EXEC CrashLogWrite N'%S'"), strLog.c_str());


// User狼 付瘤阜DB磊丰乐绰 ShardID 汲沥
#define MAKEQUERY_SetUserShardID(strQ, UID, ShardID)					\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_User_SetShardId(%d, %d)}"), UID, ShardID);

// User狼 付瘤阜DB磊丰乐绰 ShardID 掘扁
#define MAKEQUERY_GetUserShardID(strQ, UID)					\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_GetShardID(%d, ?)}"), UID);

// Family捞抚函版
#define MAKEQUERY_ChangeFamilyName(strQ, FID, FamilyName, ModelId)	\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_user_setFamilyName(%d, N'%s', %d)}"), FID, FamilyName.c_str(), ModelId);

// 模备弊缝 掘扁
#define MAKEQUERY_LoadFriendGroupList(strQ, FID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_Group_list2(%d)}"), FID);

// 模备弊缝 函版 焊包
#define MAKEQUERY_SaveFriendGroupList(strQ, FID, Data)				\
	tchar strQ[1024] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_Group_Update2(%d, 0x%S)}"), FID, Data);

// 模备格废 掘扁
#define MAKEQUERY_GetFriendList(strQ, FID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_List(%d)}"), FID);

// 模备眠啊
#define MAKEQUERY_InsertFriend(strQ, FID, FriendFID, GroupID, FIndex)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_Insert2(%d, %d, %d, %d)}"), FID, FriendFID, GroupID, FIndex);

// 模备昏力
#define MAKEQUERY_DeleteFriend(strQ, FID, FriendFID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_delete2(%d, %d)}"), FID, FriendFID);

// 模备 鉴辑/弊缝 函版
#define MAKEQUERY_ChangeFriendIndex(strQ, FID, FriendFID, NewGroupID, NewIndex)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_Move_Group(%d, %d, %d, %d)}"), FID, FriendFID, NewGroupID, NewIndex );

// 模备眠啊甫 困秦 荤侩磊沥焊 掘扁
#define MAKEQUERY_GetFamilyInfo(strQ, FID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Friend_GetFamilyInfo(%d)}"), FID);


// 荐笼规弊缝掘扁
#define MAKEQUERY_LoadFavorroomGroupList(strQ, FID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_Group_list2(%d)}"), FID);

// 荐笼规弊缝函版焊包
#define MAKEQUERY_SaveFavorroomGroupList(strQ, FID, GroupList)				\
	tchar strQ[1024] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_Group_Update2(%d, 0x%S)}"), FID, GroupList);

// 荐笼规格废 掘扁
#define MAKEQUERY_GetFavorroom(strQ, FID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_data_list(%d)}"), FID);

// 荐笼规 眠啊
#define MAKEQUERY_InsertFavorroom(strQ, FID, RoomID, GroupID)	\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_Data_Insert2(%d, %d, %d)}"),  FID, RoomID, GroupID);

// 荐笼规 昏力
#define MAKEQUERY_DeleteFavorroom(strQ, FID, RoomID)	\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_data_delete(%d, %d)}"), FID, RoomID);

// 荐笼规 鉴辑/弊缝 函版
#define MAKEQUERY_ChangeGroupFavorroom(strQ, FID, RoomID, OldGroupID, NewGroupID)	\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_data_Move_one(%d, %d, %d, %d)}"), FID, OldGroupID, RoomID, NewGroupID );

// 荐笼规弊缝 昏力矫俊 Default弊缝栏肺 颗扁扁
#define MAKEQUERY_MoveFavorroomToDefaultGroup(strQ, FID, GroupID)	\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Favorite_data_Update(%d, %d)}"), GroupID, FID);

/************************************************************************/
/*			Mail                                                        */
/************************************************************************/

// Auto Back mail
#define  MAKEQUERY_AutoReturnMail(strQ)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_ReturnBySystem}"));

// Insert a GM mail
#define  MAKEQUERY_InsertMail_GM(strQ, toFID, Title, Content, ItemExist)		\
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Insert_GM(%d, N'%s', N'%s', %d, ?, ?)}"), toFID, Title.c_str(), Content.c_str(), ItemExist);

#define  MAKEQUERY_GetMailboxStatus(strQ, FID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_GetMailBoxStatus(%d, ?, ?, ?)}"), FID);

// Get SendMail List
#define  MAKEQUERY_GetSendMailList(strQ, FID, page, PageSize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_list_Send_Page(%d, %d, %d, ?, ?)}"), FID, page, PageSize );

// Get ReceiveMail List
#define  MAKEQUERY_GetReceiveMailList(strQ, FID, page, PageSize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_list_Receive_Page(%d, %d, %d, ?, ?, ?)}"), FID, page, PageSize );

// Get a mail
#define  MAKEQUERY_GetMail(strQ, mail_id)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Get(%d, ?)}"), mail_id );

// Set Mail Status
#define  MAKEQUERY_SetMailStatus(strQ, mail_id, status)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_SetStatus(%d, %d)}"), mail_id, status );

// Delete a mail
#define  MAKEQUERY_DeleteMail(strQ, mail_id, fid, mail_type)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Delete(%d, %d, %d, ?)}"), mail_id, fid, mail_type );

// Reject mail
#define  MAKEQUERY_RejectMail(strQ, mail_id, fid)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Return(%d, %d, ?)}"), mail_id, fid );

#define  MAKEQUERY_GetMailboxStatusForSend(strQ, fromFID, toFID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_GetMailBoxStatusForSend(%d, %d, ?, ?, ?)}"), fromFID, toFID);

// Insert a mail
#define  MAKEQUERY_InsertMail(strQ, FFID, RFID, Title, Content, MType, ItemExist)		\
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Insert(%d, %d, N'%s', N'%s', %d, %d, ?, ?)}"), FFID, RFID, Title.c_str(), Content.c_str(), MType, ItemExist);

/************************************************************************/
/*			Chit                                                        */
/************************************************************************/

// 秦寸 啊练俊霸 柯 率瘤格废阑 利犁茄促
#define MAKEQUERY_LoadChitList(strQ, FID)			\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("exec Main_chit_load %u"), FID);

// 率瘤甫眠啊茄促
#define MAKEQUERY_AddChit(strQ, idSender, idReciever, nType, p1, p2, p3, p4)			\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("exec Main_chit_add %u, %u, %u, %u, %u, N'%s', N'%s'"), idSender, idReciever, nType, p1, p2, p3.c_str(), p4.c_str());

// 率瘤甫 昏力茄促
#define MAKEQUERY_DeleteChit(strQ, chitid)			\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 128, _t("exec Main_chit_delete %u"), chitid);

/************************************************************************/
/*			抗距狼侥									                                                      */
/************************************************************************/

// 荤侩磊狼 抗距狼侥格废 掘扁
#define MAKEQUERY_GetYuyueList_FID(strQ, FID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Yuyue_list_FID2(%d)}"), FID);

// 抗距狼侥殿废
#define MAKEQUERY_AddYuyue(strQ, RoomID, RoomName, FID, FamilyName, ModelID, DressID, FaceID, HairID,	\
		StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Yuyue_add2(%d, N'%s', %d, N'%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), \
		RoomID, RoomName.c_str(), FID, FamilyName.c_str(), ModelID, DressID, FaceID, HairID, \
		StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID);

// 傈眉 抗距狼侥格废 掘扁
#define MAKEQUERY_GetYuyueList(strQ)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Yuyue_list2}"));

// 抗距狼侥 窍风 柳青窃
#define MAKEQUERY_YuyuePlayed(strQ, YuyueID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Yuyue_Played(%d)}"), YuyueID);

// Family狼 付瘤阜DB磊丰乐绰 ShardID 掘扁
#define MAKEQUERY_GetFamilyShardID(strQ, UID)					\
	tchar strQ[128] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{CALL Main_family_GetShardID(%d, ?)}"), UID);

// 抗距狼侥 坷蜡
#define MAKEQUERY_YuyueFailed(strQ, YuyueID, ChitType, Reason)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Yuyue_Failed(%d, %d, %d)}"), YuyueID, ChitType, Reason);

// 捞傈俊 备涝茄 规捞 乐绰 荤侩磊甫 (Notice甫 困窍咯) 殿废茄促.
#define MAKEQUERY_Temp_RegisterNoticeUser(strQ, FID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_temp_RegisterNoticeUser(%d)}"), FID);

// 荤侩磊啊 (Notice甫 困窍咯) 殿废登咯乐绰瘤 八荤茄促s.
#define MAKEQUERY_Temp_CheckNoticeUser(strQ, FID)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_temp_CheckNoticeUser(%d)}"), FID);

// 메일과 치트표에서 90일이전의 자료를 삭제한다.
#define MAKEQUERY_Main_Mail_Chit_Delete90days(strQ)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 128, _t("{call Main_Mail_Chit_Delete90days}"));
