#pragma once

// DB의 현재시간 얻기
#define MAKEQUERY_GETCURRENTTIME(strQ)						\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_getCurTime}"));

// 써비스 기동시에 DB 초기화
#define MAKEQUERY_InitDB(strQ, ShardMainCode)						\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_InitDB1(%d)}"), ShardMainCode);

// 빠른 기념관진입 가능성을 검사한다.
#define MAKEQUERY_CheckFastEnterroom(strQ, UserID, RoomID)					\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_FastLogin(%d, %d, ?)}"), UserID, RoomID);

//// 가족 login/out
//#define MAKEQUERY_LOGINFAMILY(strQ, fid, bOnline)		\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_setonline(%d, %d)}"), fid, bOnline);

// 가족 Login
#define	MAKEQUERY_FAMILYLOGIN(strQ, idUser)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_family_login1 %u"), idUser);

// Set app type of user                by krs
#define  MAKEQUERY_SetAppType(strQ, user_id, app_type)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Main_OnlineUser_SetAppType(%d, %d)}"), user_id, app_type );

// 가족 Logout
#define	MAKEQUERY_FAMILYLOGOUT(strQ, idUser)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_family_Logout1 %u"), idUser);

// 가족ID 생성
#define MAKEQUERY_GetNewFamilyID(strQ, newFID)				\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_GetNewID1(%d, ?)}"), newFID);

//// 가족정보 얻기
//#define MAKEQUERY_GetFamilyOnUserID(strQ, uID)				\
//	tchar strQ[500] = _t("");								\
//	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_getFromUserID(%d)}"), uID);

//// 로그인시에 가족정보 얻기
//#define MAKEQUERY_GetFamilyOnUserIDForLogin(strQ, uID)				\
//	tchar strQ[500] = _t("");								\
//	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_getFromUserIDForLogin3(%d, ?, ?)}"), uID);

// 가족의 기초정보 모두 load
#define MAKEQUERY_LOADFAMILYFORBASE(strQ, fid)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_loadHisFamilyBase(%d)}"), fid);

// Family정보 load
#define MAKEQUERY_LoadFamilyInfoForEnter(strQ, fid)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_LoadFamilyInfoForEnter2(%d)}"), fid);

// 가족창조
#define MAKEQUERY_InsertFamily(strQ, FID, ucFamilyName, userid, modelid, sexid, dressid, faceid, hairid, city) \
	tchar strQ[700] = _t("");								\
	SIPBASE::smprintf(strQ, 700, _t("{call Shrd_family_insert_Aud(%d, N'%s', %d, %d, %d, %d, %d, %d, %d)}"), FID, ucFamilyName.c_str(), userid, modelid, sexid, dressid, faceid, hairid, city);

//// 가족삭제
//#define MAKEQUERY_DELFAMILY(strQ, uAccountID)			\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_del(%d)}"), uAccountID);

// 가족설명설정
//#define MAKEQUERY_SET_FAMILYCOMMENT(strQ, FID, ucFamilyComment) \
//	tchar strQ[700] = _t("");								\
//	SIPBASE::smprintf(strQ, 700, _t("{call Shrd_family_setcomment(%d, N'%s')}"), FID, ucFamilyComment);

// 캐릭터정보 얻기
#define MAKEQUERY_GetFamilyCharacters(strQ, uFID)			\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_getFromFamilyID(%d)}"), uFID);
//// 캐릭터창조
//#define MAKEQUERY_NEWCHARACTER(strQ, uAccountID, ucName, bMaster, modelid, dressid, faceid, hairid)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_character_insert(%d, N'%s', %u, %d, %d, %d, %d)}"), uAccountID, ucName, bMaster, modelid, dressid, faceid, hairid);
//// 캐릭터삭제
//#define MAKEQUERY_DELCHARACTER(strQ, uAccountID, uCHID)			\
//	tchar strQ[300] = _t("");											\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_del(%d, %d)}"), uAccountID, uCHID);
// 캐릭터외형변경
#define MAKEQUERY_UpdateCharacter(strQ, uFID, uCHID, uModelID, sexid, uDressID, uFaceID, uHairID, city, ucChName)			\
	tchar strQ[300] = _t("");											\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_UpdateType_Aud(%d, %d, %d, %d, %d, %d, %d, %d, N'%s')}"), uFID, uCHID, uModelID, sexid, uDressID, uFaceID, uHairID, city, ucChName);

//// 캐릭터테이블 load
//#define MAKEQUERY_LOADCHFORBASE(strQ, chid)			\
//	tchar strQ[300] = _t("");											\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_loadHisBaseInfo(%d)}"), chid);
// 가족명중복검사
//#define MAKEQUERY_FAMILYNAMECHECK(strQ, fid)			\
//	tchar strQ[300] = _t("");											\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_checkName(%d)}"), fid);

//#define MAKEQUERY_GetCHInfo(strQ, chid, flag)			\
//	tchar strQ[300] = _t("");											\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_List(%d, %d)}"), chid, flag);

// update the part of character info
#define MAKEQUERY_UpdateCHInfo(strQ, FID, Birthday, City, Comment)						\
	tchar strQ[700] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 700, _t("{call Shrd_Character_Update_Birthdate_Aud(%d, %d, %d, N'%s')}"), FID, Birthday, City, Comment.c_str() );



/************************************************************************/
/*			Family Inven	                                            */
/************************************************************************/

//// Get Inven Items of Family
//#define MAKEQUERY_ReadFamilyInven(strQ, FID)															\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Item_List(%d, ?)}"), FID);

// Save Inven Items of Family
#define MAKEQUERY_SaveFamilyInven(strQ, FID, Data)														\
	tchar strQ[4500] = _t("");																		\
	SIPBASE::smprintf(strQ, 4500, _t("{call Shrd_Item_InsertAndUpdate(%d, 0x%S, ?)}"), FID, Data);

// Add Item SellCount
#define MAKEQUERY_AddItemSellCount(strQ, ItemID, SellCount)															\
	tchar strQ[300] = _t("");																		\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Item_AddSellCount(%d, %d)}"), ItemID, SellCount);

// Get Bestsell Item List
#define MAKEQUERY_GetBestsellItemList(strQ)															\
	tchar strQ[300] = _t("");																		\
	SIPBASE::smprintf(strQ, 300, _t("{call Shard_RecommendItem_Get}"));


/************************************************************************/
/*			Family Exp History Data                                     */
/************************************************************************/

// Save Family Exp History Data
#define MAKEQUERY_SaveFamilyExpHisData(strQ, FID, Data)														\
	tchar strQ[1024] = _t("");																		\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Family_UpdateExpHisData(%d, 0x%s, ?)}"), FID, Data);

// Save Family Level & Exp
#define MAKEQUERY_SaveFamilyLevelAndExp(strQ, FID, Level, Exp, LevelChanged)														\
	tchar strQ[300] = _t("");																		\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_UpdateLevelExpData3(%d, %d, %d, %d)}"), FID, Level, Exp, LevelChanged);

//// Save Family Level & Exp & Exp History Data
//#define MAKEQUERY_SaveFamilyExpData(strQ, FID, Level, Exp, Data)														\
//	tchar strQ[1024] = _t("");																		\
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Family_UpdateLevelExpData(%d, %d, %d, 0x%s)}"), FID, Level, Exp, Data);

// Save Family GMoney
#define MAKEQUERY_SaveFamilyGMoney(strQ, FID, GMoney)														\
	tchar strQ[300] = _t("");																		\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Update_Gmoney(%d, %d, ?)}"), FID, GMoney);

// Save Room Level & Exp
#define MAKEQUERY_SaveRoomLevelAndExp(strQ, RoomID, Level, Exp)														\
	tchar strQ[300] = _t("");																		\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_UpdateLevelExp(%d, %d, %d, ?)}"), RoomID, Level, Exp);

//// Add Room Exp when room not opened
//#define MAKEQUERY_AddRoomExp(strQ, RoomID, Exp)														\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_Update_Exp(%d, %d)}"), RoomID, Exp);



//// 가족item삭제
//#define MAKEQUERY_DELFAMILYITEM(strQ, fid, ItemPos, ItemNum)												\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_deleteItem(%d, %d, %d)}"), fid, ItemPos, ItemNum);





// 가족GMoney추가
//#define MAKEQUERY_ADDMONEY(strQ, fid, umoney, gmoney)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_addmoney_GM(%d, %d, %d)}"), fid, umoney, gmoney);
// 가족이름으로부터 기초정보 얻기
//#define MAKEQUERY_FAMILYINFOFROMNAME(strQ, fid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_infofromname_GM(%d)}"), fid);

//#define MAKEQUERY_FAMILYINFOFROMNAME_USERID(strQ, userid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_infofromname_GM_UID(%d)}"), userid);
// 가족이름으로부터 기초정보 얻기
//#define MAKEQUERY_FAMILYITEMINFOFROMNAME(strQ, fid, itypeid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_fitem_infofromname_GM(%d, %d)}"), fid, itypeid);
// 가족이름으로부터 배낭빈자리얻기
//#define MAKEQUERY_EMPTYPOSFOFROMNAME(strQ, fid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_fitem_emptyposfromname_GM(%d)}"), fid);
// 가족이름으로부터 급수설정하기
//#define MAKEQUERY_VDAYSFROMNAME(strQ, fid, vdays)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_vdaysfromname_GM(%d, %d)}"), fid, vdays);

// 사적공간사기
#define MAKEQUERY_BuyHisSpace(strQ, add_space, rid)						\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_buyspace(%d, %d)}"), add_space, rid);

//// 사적공간소비
//#define MAKEQUERY_EXPENDSPACE(strQ, fid, space)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_expendspace(%d, %d)}"), fid, space);

//// 사적공간복구
//#define MAKEQUERY_RESTORESPACE(strQ, fid, space)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_restorespace(%d, %d)}"), fid, space);

//// 사적공간사용문의
//#define MAKEQUERY_CANEXPENDSPACE(strQ, fid, space)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_canexpendspace(%d, %d)}"), fid, space);

//#define MAKEQUERY_SETEXPERIENCE(strQ, UserId)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_setexperience(%d)}"), UserId);

// 캐릭터dress변경
#define MAKEQUERY_UPDATECHDRESS(strQ, fid, chid, dressid)						\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Character_UpdateDress(%d, %d, %d)}"), fid, chid, dressid);

// add new memorial
#define MAKEQUERY_ADDROOMMEMORIAL(strQ, FID, RoomID, Memo, Secret)						\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_room_addmemorial(%d, %d, N'%s', %d)}"), FID, RoomID, Memo.c_str(), Secret);

//// modify the room's memorial
//#define MAKEQUERY_ModifyMemo(strQ, FID, MemoID, Memo, Secret)	\
//	tchar strQ[512] = _t("");						\
//	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_room_memorial_Update(%d, %d, N'%s', %d, ?)}"), FID, MemoID, Memo.c_str(), Secret);

//// 방글 삭제(방주인용)
//#define MAKEQUERY_DELROOMMEMORIALFORMASTER(strQ, memoid, roomid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_delmemorial(%d, %d)}"), memoid, roomid);
// 방글 삭제
//#define MAKEQUERY_DELROOMMEMORIAL(strQ, memoid, fid)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_delMemorialOfVisitor(%d, %d)}"), fid, memoid);
// 방에 남긴글 얻기
//#define MAKEQUERY_ROOMMEMORIAL(strQ)						\
//	tchar strQ[512] = _t("");						\
//	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Memorial(?, ?, ?, ?, ?, ?, ?, ?, ?)}"));
//// 길상등자료 얻기
//#define MAKEQUERY_GetOneMemorisl(strQ, MemoID)						\
//	tchar strQ[512] = _t("");						\
//	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_room_GetOneMemorial(%d)}"), MemoID);

//// 제물올리기
//#define MAKEQUERY_SETSACRIFICE(strQ, roomid, iid, num, x, y, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_setsacrifice(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), roomid, iid, num, x, y, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc);

// 방정보얻기
#define MAKEQUERY_GetRoomInfoForOpen(strQ, RoomID)						\
	tchar strQ[800] = _t("");								\
	SIPBASE::smprintf(strQ, 800, _t("{call Shrd_room_getRoomForOpen1(%d, ?)}"), RoomID);

//// 방정보얻기
//#define MAKEQUERY_GETROOM(strQ, uID)						\
//	tchar strQ[800] = _t("");								\
//	SIPBASE::smprintf(strQ, 800, _t("{call Shrd_room_getRoom(%d)}"), uID);
//// 방의부품정보load
//#define MAKEQUERY_GETROOMPART(strQ, RoomID)					\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getRoompartId(%d)}"), RoomID);
//// 방의제물목록load
//#define MAKEQUERY_GETROOMSACRIFICE(strQ, RoomID)					\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getSacrifice(%d)}"), RoomID);
//// 방의제물삭제
//#define MAKEQUERY_DELROOMSACRIFICE(strQ, ID)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_delSacrifice(%d)}"), ID);
//// 방의비석목록load
//#define MAKEQUERY_GETROOMTOMB(strQ, RoomID)					\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getTomb(%d)}"), RoomID);

//// 방의페트목록load
//#define MAKEQUERY_GETROOMPET(strQ, RoomID)					\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getPet(%d)}"), RoomID);
//// 페트추가
//#define MAKEQUERY_ADDPET(strQ, roomid, pettype, petid, petcount, petstep, petlife, petregion, petpos, gmoney, umoney, fid)						\
//	tchar strQ[600] = _t("");								\
//	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_addpet(%d, %d, %d, %d, %d, %d, %d, N'%s', %d, %d, %d)}"), roomid, pettype, petid, petcount, petstep, petlife, petregion, petpos, gmoney, umoney, fid);
// 방의 Default페트추가
//#define MAKEQUERY_ADDDEFAULTPET(strQ, roomid, pettype, petid, petcount, petstep, petlife, petregion, petpos)						\
//	tchar strQ[600] = _t("");								\
//	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_addDefaultPet(%d, %d, %d, %d, %d, %d, %d, N'%s')}"), roomid, pettype, petid, petcount, petstep, petlife, petregion, petpos);
//// 페트속성변경
//#define MAKEQUERY_UPDATEPET(strQ, id, petstep, petlife)						\
//	tchar strQ[600] = _t("");								\
//	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_updatePet(%d, %d, %d)}"), petstep, petlife, id);
//// 페트삭제
//#define MAKEQUERY_DELETEPET(strQ, id)						\
//	tchar strQ[600] = _t("");								\
//	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_deletePet(%d)}"), id);

//// 배경음악설정
//#define MAKEQUERY_SETROOMMUSIC(strQ, roomid, musicid, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc)		\
//	tchar strQ[600] = _t("");								\
//	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_getmusic(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), roomid, musicid, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc);



//// 방부품교체
//#define MAKEQUERY_CHANGEROOMPART(strQ, RoomID, newPartID, oldPartID, itemtypeID, FID, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc)		\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_roompart_change(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), RoomID, newPartID, oldPartID, itemtypeID, FID, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc);
// 방의 유효방문일수 증가
//#define MAKEQUERY_INCROOMVISITSDAYS(strQ, RoomID)		\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Rombaseinfo_UpdateVisitsDays(%d)}"), RoomID);
//// 방의 유효방문일수 증가
//#define MAKEQUERY_INCROOMVISITS(strQ, RoomID)		\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Rombaseinfo_UpdateVisits(%d)}"), RoomID);

//#define MAKEQUERY_UPDATEVISIT_TY(strQ, RoomID, nFamilyID, nVisit)		\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_ROombaseinfo_UpdateVisits_TY(%d, %d, %d)}"), RoomID, nFamilyID, nVisit);

#define MAKEQUERY_LOADMSTDATA1(strQ)							\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_loadmstdata1}"));
#define MAKEQUERY_LOADMSTDATA2(strQ)							\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_loadmstdata2}"));
#define MAKEQUERY_LOADLUCKHISTORY(strQ)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_LuckHis_load}"));
#define MAKEQUERY_RESETLUCKHISTORY(strQ)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_LuckHis_reset}"));
#define MAKEQUERY_ADDLUCKHISTORY(strQ, FID, RoomID, LuckLevel, LuckID, LuckDBTime)		\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_LuckHis_add(%d, %d, %d, %d, %d)}"), FID, RoomID, LuckLevel, LuckID, LuckDBTime);
#define MAKEQUERY_LOADBLESSHIS(strQ)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_BlessHis_load}"));
#define MAKEQUERY_RESETBLESSHIS(strQ)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_BlessHis_reset}"));
#define MAKEQUERY_ADDBLESSHIS(strQ, FID, RoomID, RecvDBTime)		\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_BlessHis_add(%d, %d, %d)}"), FID, RoomID, RecvDBTime);

// 방의 관리자인가
//#define MAKEQUERY_ISROOMMANAGER(strQ, roomid, fid)							\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_GetManagerID(%d, %d)}"), roomid, fid);
// 련계인관계를 얻는다
//#define MAKEQUERY_GETNEIGHBOR(strQ, id1, id2)							\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_contact_getNeighborType(%d, %d)}"), id1, id2);
//// 방의 관리자정보를 얻는다
//#define MAKEQUERY_GETROOMMANAGERS(strQ, roomid)				\
//	tchar strQ[500] = _t("");								\
//	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_room_getManager(%d)}"), roomid);
//// 방의 관리자를 추가한다
//#define MAKEQUERY_ADDROOMMANAGER(strQ, fid, roomid, managerid)				\
//	tchar strQ[500] = _t("");								\
//	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_RoomManager_Add(%d, %d, %d)}"), fid, roomid, managerid);
//// 방의 관리자를 삭제한다
//#define MAKEQUERY_DELROOMMANAGER(strQ, fid, roomid, managerid)				\
//	tchar strQ[500] = _t("");								\
//	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_room_delManager(%d, %d, %d)}"), fid, roomid, managerid);
// 방의 비석을 갱신한다
#define MAKEQUERY_UPDATETOMB(strQ, roomid, tombid, tomb)				\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_updateTomb(%d, %d, N'%s')}"), roomid, tombid, tomb);
// 방의 비석을 추가한다
#define MAKEQUERY_ADDTOMB(strQ, roomid, tombid, tomb)				\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_room_addTomb(%d, %d, N'%s')}"), roomid, tombid, tomb);

// 방의 비석을 추가한다
#define MAKEQUERY_MODIFYRUYIBEI(strQ, roomid, itemid, ruyibei)				\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Room_Ruyibei_InsertOrUpdate(%d, %d, N'%s')}"), roomid, itemid, ruyibei);
//// 방동결
//#define MAKEQUERY_CLOSEROOM(strQ, roomid)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_close(%d)}"), roomid);

//// 인벤의 아이템을 제상에 놓는다
//#define MAKEQUERY_PUTITEM(strQ, roomid, invenid, num, x, y, fid)												\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_putitem(%d, %d, %d, %d, %d, %d)}"), roomid, invenid, num, x, y, fid);
//// 인벤의 아이템을 태운다
//#define MAKEQUERY_BURNITEM(strQ, roomid, invenpos, num, fid)												\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_burnitem(%d, %d, %d, %d)}"), roomid, invenpos, num, fid);

// 아이템을 직접 태운다
//#define MAKEQUERY_BURNITEMINSTANT(strQ, roomid, iid, num, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc)												\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_burnitemInstant(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), roomid, iid, num, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc);

//// 페트키우기
//#define MAKEQUERY_CAREPET(strQ, roomid, careid, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc)		\
//	tchar strQ[300] = _t("");																		\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_carepet(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), roomid, careid, fid, GMoney, UMoney, ExpInc, VirtueInc, FameInc, RoomExpInc, RoomVirtueInc, RoomFameInc, RoomApprInc);
// 개인기념관창조
#define MAKEQUERY_CREATEROOM(strQ, sortID, masterID, price, years, months, days, hs, ShardCode)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_create %d, %d, %d, %d, %d, %d, %d, %d"), sortID, masterID, price, years, months, days, hs, ShardCode);
// 체험방창조
#define MAKEQUERY_CREATEROOMEXP(strQ, sortID, masterID)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_create_EXP %d, %d"), sortID, masterID);

// 방사용기간을 연장한다
#define MAKEQUERY_RenewRoom(strQ, masterID, idRoom, SceneID, years, months, days, UMoneyReal, AddHisSpace)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_renew %d, %d, %d, %d, %d, %d, %d, %d"), masterID, idRoom, SceneID, years, months, days, UMoneyReal, AddHisSpace);

#define MAKEQUERY_PROLONGFREEROOM(strQ, idRoom, years, months, days)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_prolong %d, %d, %d, %d"), idRoom, years, months, days);

// 어떤 방의 정보얻기
#define MAKEQUERY_GetRoomInfo(strQ, idUser, idRoom)		\
	tchar strQ[300] = _t("");				\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_getInfo %d, %d"), idUser, idRoom);

// look over the room's info by other user
//#define MAKEQUERY_FINDROOMINFO(strQ)					\
//	tchar strQ[300] = _t("");						            \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_List(?, ?, ?, ?)}"));

// 방삭제
#define MAKEQUERY_DelRoom(strQ, FID, roomID, deleteDay)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_deleteRoom %d, %d, %d"), FID, roomID, deleteDay);

// 
#define MAKEQUERY_UnDelRoom(strQ, FID, roomID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_undeleteRoom %d, %d"), FID, roomID);


//#define MAKEQUERY_LoadFavorroomGroupList(strQ, FID)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Favorite_Group_list2(%d)}"), FID);

//#define MAKEQUERY_SaveFavorroomGroupList(strQ, FID, GroupList)				\
//	tchar strQ[1024] = _t("");						\
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Favorite_Group_Update2(%d, 0x%S, ?)}"), FID, GroupList);

//#define MAKEQUERY_InsertFavorroom(strQ, FID, RoomID, GroupID)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Favorite_Data_Insert2(%d, %d, %d, ?)}"),  FID, RoomID, GroupID);

//#define MAKEQUERY_DeleteFavorroom(strQ, FID, RoomID, GroupID)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Favorite_data_delete(%d, %d, %d, ?)}"), FID, RoomID, GroupID);

//#define MAKEQUERY_ChangeGroupFavorroom(strQ, FID, RoomID, OldGroupID, NewGroupID)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Favorite_data_Move_one(%d, %d, %d, %d, ?)}"), FID, OldGroupID, RoomID, NewGroupID );

//#define MAKEQUERY_GetFavorroom(strQ, FID)	\
//	tchar strQ[1024] = _t("");								\
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Favorite_data_list1(%d)}"), FID);

//#define MAKEQUERY_MoveFavorroomToDefaultGroup(strQ, FID, GroupID)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Favorite_data_Update(%d, %d)}"), GroupID, FID);

//#define MAKEQUERY_LOADROOMORDERINFO(strQ, FID)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomOrderInfo_Get(%d)}"), FID);

#define MAKEQUERY_MAKEEMPTYROOMORDERINFO(strQ, FID)	\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomOrderInfo_MakeEmpty(%d)}"), FID);

#define MAKEQUERY_SAVEOWNROOMORDERINFO(strQ, FID, infolist)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_RoomOrderInfo_SaveOwn(%d, 0x%S)}"), FID, infolist);

#define MAKEQUERY_SAVEMANAGEROOMORDERINFO(strQ, FID, infolist)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_RoomOrderInfo_SaveManage(%d, 0x%S)}"), FID, infolist);

#define MAKEQUERY_SAVEFAVOROOMORDERINFO(strQ, FID, infolist)	\
	tchar strQ[5120] = _t("");								\
	SIPBASE::smprintf(strQ, 5120, _t("{call Shrd_RoomOrderInfo_SaveFavo(%d, 0x%S)}"), FID, infolist);


//#define MAKEQUERY_NEW_FAVORROOMGROUP(strQ, groupname, FID)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Room_Favorite_Insert(N'%s',%d,?)}"), groupname.c_str(), FID );

//#define MAKEQUERY_DEL_FAVORROOMGROUP(strQ)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Room_Favorite_Delete(?,?,?)}") );

//#define MAKEQUERY_REN_FAVORROOMGROUP(strQ, groupid, FID, groupname)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Room_Favorite_Update(%d,%d,N'%s',?)}"), groupid, FID, groupname.c_str() );

// 방정보를 설정한다
#define MAKEQUERY_SETROOMINFO(strQ, idUser, idRoom, Name, Power, RCom, pwd)	\
	tchar strQ[1024] = _t("");								\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_room_UpdateInfo_Aud(%u, %u, N'%s', %u, N'%s', '%S')}"), idUser, idRoom, Name.c_str(), Power, RCom.c_str(), pwd.c_str());


/************************************************************************/
/*			Dead Info		                                            */
/************************************************************************/

// GetDeceasedInfo
#define MAKEQUERY_GetDeceasedForRoom(strQ, RoomID)						\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getDeceased4(%d)}"), RoomID);

// look over the room's deceased info
#define MAKEQUERY_GetDeceasedInfo(strQ, RoomID, FID, Password)					\
	tchar strQ[300] = _t("");						            \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_getDeceased3(%d, %d, '%S', ?)}"), RoomID, FID, Password.c_str());

// Set Deceased's Photo
#define MAKEQUERY_SetDeceasedPhoto(strQ, RoomID, DeadID, PhotoID)					\
	tchar strQ[300] = _t("");						            \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Deceased_Update_PhotoID(%d, %d, %d, ?)}"), RoomID, DeadID, PhotoID);

// 기념관에 고인을 추가한다
#define MAKEQUERY_AddDead(strQ, RoomID, RoomName, DeadID, DeadSurnameLen, DeadGivenname, Sex, Birthday, DeadDay, Brief, Domicile, Nation, PhotoType)					\
	tchar strQ[1300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1300, _t("{call Shrd_Deceased_InsertOrUpdate_Aud(%d, N'%s', %d, %d, N'%s', %d, %d, %d, N'%s', N'%s', N'%s', %d)}"), RoomID, RoomName.c_str(), DeadID, DeadSurnameLen, DeadGivenname.c_str(), Sex, Birthday, DeadDay, Brief.c_str(), Domicile.c_str(), Nation.c_str(), PhotoType );

//// Add Deceased History
//#define MAKEQUERY_DeadHisAdd(strQ, RoomID, DeadID, StartYear, EndYear, Title, Content)					\
//	tchar strQ[1300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 1300, _t("{call Shrd_Room_Deceased_History_insert1(%d, %d, %d, %d, N'%s', N'%s', ?)}"), RoomID, DeadID, StartYear, EndYear, Title.c_str(), Content.c_str());

//// Update Deceased History
//#define MAKEQUERY_DeadHisUpdate(strQ, HisID, StartYear, EndYear, Title, Content)					\
//	tchar strQ[1300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 1300, _t("{call Shrd_Room_Deceased_History_Update(%d,%d,%d,N'%s',N'%s',?)}"), HisID, StartYear, EndYear, Title.c_str(), Content.c_str());

//// Delete Deceased History
//#define MAKEQUERY_DeadHisDel(strQ, HisID)					\
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Deceased_History_Delete(%d,?)}"), HisID);

//// Get Deceased History
//#define MAKEQUERY_DeadHisGet(strQ, HisID)					\
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Deceased_History_Get(%d,?)}"), HisID);

//// Get Deceased History List
//#define MAKEQUERY_DeadHisList(strQ, RoomID, DeadID)					\
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Deceased_History_List1(%d,%d,?)}"), RoomID, DeadID);


//// 기념관에서 고인을 삭제한다
//#define MAKEQUERY_DelDead(strQ, userID, roomID, index)		\
//	tchar strQ[128] = _t("");						\
//	SIPBASE::smprintf(strQ, 128, _t("exec Shrd_Deceased_delete %u, %u, %u"), userID, roomID, index);





#define MAKEQUERY_GetFamilyCreateRooms(strQ, FID)	\
	tchar strQ[128] = _t("");				\
	SIPBASE::smprintf(strQ, 128, _t("exec Shrd_room_getOwnCreateRoom %u"), FID);

//#define MAKEQUERY_GetOwnCreateRooms(strQ, FID)	\
//	tchar strQ[128] = _t("");				\
//	SIPBASE::smprintf(strQ, 128, _t("{call Shrd_room_GetOwnRoomAndDeceased2(%d)}"), FID);

#define MAKEQUERY_GetOwnRoomOne(strQ, RoomID)	\
	tchar strQ[128] = _t("");				\
	SIPBASE::smprintf(strQ, 128, _t("exec Shrd_room_GetRoomID %d"), RoomID);

#define MAKEQUERY_FindRoom(strQ, target, flag, page, findstr, numPerPage)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_room_findRoom(%d, %d, %d, N'%s', %d, ?, ?, ?)}"), target, flag, page, findstr.c_str(), numPerPage);

//// modify the room's visit password
//#define MAKEQUERY_UPDATEPASSWORD(strQ)	\
//	tchar strQ[512] = _t("");				\
//	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_room_Update_Openpassword (?, ?, ?, ?)}"));


// 방관리자 추가
#define MAKEQUERY_RoomManager_Insert(strQ, OwnerFID, RoomID, targetFID)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_Insert(%d, %d, %d, ?)}"), OwnerFID, RoomID, targetFID);

// 방관리자 삭제
#define MAKEQUERY_RoomManager_Delete(strQ, OwnerFID, RoomID, targetFID)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_Delete(%d, %d, %d, ?)}"), OwnerFID, RoomID, targetFID);

// 방관리자 목록얻기
#define MAKEQUERY_RoomManager_List(strQ, OwnerFID, RoomID)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_List1(%d, %d, ?)}"), OwnerFID, RoomID);

// 방관리자 검사
#define MAKEQUERY_IsRoomManager(strQ, targetFID, RoomID)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_Check(%d, %d, ?)}"), targetFID, RoomID);

// 자기가 관리하는 방목록 얻기
#define MAKEQUERY_GetOwnManageRoom(strQ, OwnerFID, RoomID)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_Get(%d, %d)}"), OwnerFID, RoomID);

// 방관리자 권한변경
#define MAKEQUERY_ChangeRoomManagerPermission(strQ, RoomID, targetFID, Permission)	\
	tchar strQ[512] = _t("");				\
	SIPBASE::smprintf(strQ, 512, _t("{call Shrd_Room_Manager_SetPermission(%d, %d, %d)}"), RoomID, targetFID, Permission);


//// 련계인의 가족정보를 얻는다
//#define MAKEQUERY_CONTFAMILY(strQ, idUser)	\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_all(%d)}"), idUser);

//// 해당 가족에게 온 쪽지목록을 적재한다
//#define MAKEQUERY_LOADCHITLISTTOHIM(strQ, FID)			\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_chit_load %u"), FID);
//// 쪽지를추가한다
//#define MAKEQUERY_ADDCHIT(strQ, idSender, idReciever, nType, p1, p2, p3, p4)			\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_chit_add %u, %u, %u, %u, %u, N'%s', N'%s'"), idSender, idReciever, nType, p1, p2, p3.c_str(), p4.c_str());
//// 쪽지정보를 얻는다
//#define MAKEQUERY_GETCHIT(strQ, chitid, recvid, delchit)			\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_chit_get %u, %u, %u"), chitid, recvid, delchit);



/************************************************************************/
/*			Friend			                                            */
/************************************************************************/

//#define MAKEQUERY_LoadFriendGroupList(strQ, FID)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_Group_list2(%d)}"), FID);

//#define MAKEQUERY_SaveFriendGroupList1(strQ, FID, Data)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_Group_Update2(%d, 0x%S, ?)}"), FID, Data);

//#define MAKEQUERY_GetFriendList(strQ, FID)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_List(%d)}"), FID);

//#define MAKEQUERY_InsertFriend(strQ, FID, FriendFID, GroupID, FriendIndex)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_Insert2(%d, %d, %d, %d, ?)}"), FID, FriendFID, GroupID, FriendIndex);

//#define MAKEQUERY_DeleteFriend(strQ, FID, FriendFID)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_delete2(%d, %d, ?)}"), FID, FriendFID);

//#define MAKEQUERY_ChangeFriendIndex(strQ, FID, FriendFID, NewGroupID, NewIndex)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_Move_Group(%d, %d, %d, %d, ?)}"), FID, FriendFID, NewGroupID, NewIndex );

#define MAKEQUERY_FriendDeleted(strQ, FID, FriendFID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Friend_deleted(%d, %d)}"), FID, FriendFID);

#define MAKEQUERY_GetFamilyModelID(strQ, FID, friendFID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_GetModelID(%d, %d)}"), FID, friendFID);



// reserve festival
#define	MAKEQUERY_ADDFESTIVAL(strQ, FID, nameFestival, date, alarmdays)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_festival_add(%d, N'%s', %d, %d, ?)}"), FID, nameFestival.c_str(), date, alarmdays );

#define	MAKEQUERY_MODIFYFESTIVAL(strQ, idFestival, nameFestival, date, alarmdays)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_festival_update(%d, N'%s', %d, %d, ?)}"), idFestival, nameFestival.c_str(), date, alarmdays );	

#define	MAKEQUERY_DELFESTIVAL(strQ)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_festival_del(?, ?)}") );	

//#define	MAKEQUERY_GETALLFESTIVAL(strQ)	\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_festival_Remind_getall(?, ?)}") );	

//#define	MAKEQUERY_GETDEADMEMORIALDAY(strQ, idUser)	\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Deceased_getMemorialDay %u"), idUser);

// log
#define	MAKEQUERY_CREATELOG(strQ)	\
	tchar strQ[1000] = _t("");						\
	SIPBASE::smprintf(strQ, 1000, _t("{call ShrdLog_Create_tbl}"));

#define	MAKEQUERY_CREATELOG1(strQ)	\
	tchar strQ[1000] = _t("");						\
	SIPBASE::smprintf(strQ, 1000, _t("{call ShrdLog_Create_tbl1}"));

#define	MAKEQUERY_NEWLOG(strQ, MainType, SubType, PID, UID, ItemID, ItemTypeID, VI1, VI2, VI3, VI4, PName, ItemName)	\
	tchar strQ[1000] = _t("");						\
	SIPBASE::smprintf(strQ, 1000, _t("{call ShrdLog_ShardLog_Insert(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, N'%s', N'%s')}"), MainType, SubType, PID, UID, ItemID, ItemTypeID, VI1, VI2, VI3, VI4, PName, ItemName);

// Unlaw log
#define	MAKEQUERY_UNLAWLOG(strQ, UnalwType, UnlawContent, FID)	\
	tchar strQ[1000] = _t("");						\
	SIPBASE::smprintf(strQ, 1000, _t("{call ShrdLog_Unlaw_Insert(%d, N'%s', %d)}"), UnalwType, UnlawContent, FID);

// gm log
#define	MAKEQUERY_GMNEWLOG(strQ, MainType, PID, UID, PID2, UID2, VI1, VI2, VI3, VU1, UName, OName)	\
	tchar strQ[1000] = _t("");						\
	SIPBASE::smprintf(strQ, 1000, _t("{call ShrdLog_gmlog_Insert(%d, %d, %d, %d, %d, %d, %d, %d, N'%s', N'%s', N'%s')}"), MainType, PID, UID, PID2, UID2, VI1, VI2, VI3, VU1, UName, OName);

//#define MAKEQUERY_PORTRAITINFO(strQ, RoomID)     \
//	tchar strQ[300] = _t("");                    \
//	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Room_PortraitInfo(%d)}"), RoomID );

/************************************************************************/
/*			Lobby			                                            */
/************************************************************************/

// add a new wish in lobby
//#define MAKEQUERY_ADDWISHINLOBBY(strQ)          \
//    tchar strQ[128] = _t("");                     \
//	SIPBASE::smprintf( strQ, 128, _t("{call Shrd_Pubilc_wish_Insert(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)}") );

// get the wish list in lobby
//#define MAKEQUERY_LOADWISHLIST(strQ)     \
//	tchar strQ[128] = _t("");                    \
//	SIPBASE::smprintf( strQ, 128, _t("{call Shrd_Pubilc_Wish_List(?, ?)}") );

// delete the wish in lobby which is out of date
//#define MAKEQUERY_DELETELOBBYWISH(strQ)						\
//	tchar strQ[128] = _t("");								\
//	SIPBASE::smprintf(strQ, 128, _t("{call Shrd_Pubilc_Wish_Delete(?, ?)}") );

/************************************************************************/
/*			History Manager                                             */
/************************************************************************/
// get the photo frame information for a room
#define MAKEQUERY_LoadPhotoFrame(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Room_PhotoFrame_List(%d,?)}"), RoomID );

// set a data for the certain frame(photo/Video)
#define MAKEQUERY_SetFrame(strQ, RoomID, FrameID, DataID, OffsetX, OffsetY, Scale)				\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Room_PhotoFrame_insert(%d, %d, %d, %d, %d, %d, ?)}"), RoomID, FrameID, DataID, OffsetX, OffsetY, Scale );

// cancel a data set for the frame(photo/Video)
#define MAKEQUERY_CancelSetFrame(strQ, RoomID, FrameID)				\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Room_PhotoFrame_Delete(%d, %d, ?)}"), RoomID, FrameID );

// Get Photo Frame's PhotoInfo
#define MAKEQUERY_GetFrame(strQ, RoomID, FrameID)				\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Room_PhotoFrame_Get(%d, %d)}"), RoomID, FrameID );

// get the figure frame information for a room
#define MAKEQUERY_LoadFigureFrame(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Room_GetFigureFrame(%d)}"), RoomID );

// set the figure frame information for a room
#define MAKEQUERY_SetFigureFrame(strQ, RoomID, FrameID, PhotoType, PhotoID1, PhotoID2)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Room_SetFigureFrame(%d, %d, %d, %d, %d)}"), RoomID, FrameID, PhotoType, PhotoID1, PhotoID2 );

// load data server list
#define MAKEQUERY_LoadDataServers(strQ)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_mstDataServers_List}") );

// check if rest space is insufficient
#define MAKEQUERY_CheckSpace(strQ, RoomID, DataSize)						\
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Room_BaseInfo_SpaceSize(%d,%d,?)}"), RoomID, DataSize );

//#define MAKEQUERY_LoadVideoFrame(strQ, RoomID, FID)     \
//	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Room_VideoFrame_List(%d,%d,?)}"), RoomID, FID );




// Add New: Load PhotoAlbumList BinaryData
#define MAKEQUERY_LoadPhotoAlbumList(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoAlbumInfo_Page_List(%d, ?)}"), RoomID );

// Add New: Save PhotoAlbumList BinaryData
#define MAKEQUERY_SavePhotoAlbumList(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoAlbumInfo_Page_insert(%d, ?, ?)}"), RoomID );

// Add New: Change Photo's AlbumID after a album deleted
#define MAKEQUERY_ChangePhotoAlbumIDAfterAlbumDeleted(strQ, RoomID, AlbumID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoAlbumInfo_delete2(%d, %d)}"), RoomID, AlbumID );

// Add New: Load PhotoList of a PhotoAlbum
#define MAKEQUERY_LoadPhotoList(strQ, RoomID, AlbumID, checkflag)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_RoomAndAlbumID_List(%d, %d, %d)}"), RoomID, AlbumID, checkflag );

// Add New: Insert a Photo
#define MAKEQUERY_InsertPhoto(strQ, WebID, RoomID, AlbumID, Desc, Crc32, FileSize, Type)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_Insert1(%d, %d, %d, N'%s', %d, %d, %d)}"), WebID, RoomID, AlbumID, Desc.c_str(), Crc32, FileSize, Type);

// Add New: Get a PhotoInfo
#define MAKEQUERY_GetPhotoInfo(strQ, RoomID, FileID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_PhotoID_List2(%d, %d, ?)}"), RoomID, FileID );

// Add New: Delete a Photo
#define MAKEQUERY_DeletePhoto(strQ, RoomID, FileID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_Delete(%d, %d, ?)}"), RoomID, FileID );

// Add New: Change Photo's AlbumID
#define MAKEQUERY_ChangePhotoAlbumID(strQ, RoomID, FileID, NewAlbumID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_PhotoAlbumID_update2(%d, %d, %d, ?)}"), RoomID, FileID, NewAlbumID );

// Add New: Change Photo's Desc
#define MAKEQUERY_ChangePhotoDesc(strQ, RoomID, FileID, NewDesc)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_PhotoInfo_NameAndDesc_Update2(%d, %d, N'%s', ?)}"), RoomID, FileID, NewDesc.c_str() );




// Add New: Load VideoAlbumList BinaryData
#define MAKEQUERY_LoadVideoAlbumList(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Video_Group_Pack_List(%d, ?)}"), RoomID );

// Add New: Save VideoAlbumList BinaryData
#define MAKEQUERY_SaveVideoAlbumList(strQ, RoomID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Video_Group_Pack_insert(%d, ?, ?)}"), RoomID );

// Add New: Change Video's AlbumID after a album deleted
#define MAKEQUERY_ChangeVideoAlbumIDAfterAlbumDeleted(strQ, RoomID, AlbumID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_Video_Group_Pack_delete(%d, %d)}"), AlbumID, RoomID );

// Add New: Load VideoList of a Album
#define MAKEQUERY_LoadVideoList(strQ, RoomID, AlbumID, checkflag)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_RoomAndGroupID_List(%d, %d, %d)}"), RoomID, AlbumID, checkflag );

// Add New: Insert a Video
#define MAKEQUERY_InsertVideo(strQ, WebID, RoomID, AlbumID, Desc, Crc32, FileSize, TotalTime)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_Insert(%d, %d, N'%s', %d, %d, %d, %d)}"), WebID, RoomID, Desc.c_str(), FileSize, TotalTime, Crc32, AlbumID);

// Add New: Get a VideoInfo
#define MAKEQUERY_GetVideoInfo(strQ, RoomID, FileId)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_List2(%d, %d, ?)}"), RoomID, FileId );

// Add New: Delete a Video
#define MAKEQUERY_DeleteVideo(strQ, RoomID, FileID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_Delete(%d, %d, ?)}"), RoomID, FileID );

// Add New: Change Video's AlbumID
#define MAKEQUERY_ChangeVideoAlbumID(strQ, RoomID, FileId, NewAlbumID)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_VideoGroupID_update2(%d, %d, %d, ?)}"), RoomID, FileId, NewAlbumID );

// Add New: Change Video's Desc
#define MAKEQUERY_ChangeVideoDesc(strQ, RoomID, FileId, NewDesc)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_Desc_Update2(%d, %d, N'%s', ?)}"), RoomID, FileId, NewDesc.c_str() );

// Add New: Change Video's Index
#define MAKEQUERY_ChangeVideoIndex(strQ)     \
	SIPBASE::smprintf( strQ, 1024, _t("{call Shrd_VideoInfo_Update_IndexID(?, ?)}") ); //by krs







// find family info according to familyId or fName
#define  MAKEQUERY_FINDFAMILY(strQ, Page, FamilyName, CountPerPage)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_findWithName(%d, N'%s', %d, ?, ?, ?)}"), Page, FamilyName.c_str(), CountPerPage );

// find family info according to familyId
#define  MAKEQUERY_FINDFAMILYWITHID(strQ, FID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_findWithID(%d, ?)}"), FID );

// insert the new festival remind
#define  MAKEQUERY_INSERTFESTIVALREMIND(strQ, FID, type, festivalid, reminddays)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Festival_Remind_InsertAndUpdate(%d, %d, %d, %d ,?)}"), FID, type, festivalid, reminddays);

//// update the festival remind
//#define  MAKEQUERY_UPDATEFESTIVALREMIND(strQ)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Festival_Remind_Update(?, ?, ?)}") );

//// check in the black list or not
//#define  MAKEQUERY_CheckBlacklist(strQ, FID1, FID2)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Blacklist_Check(%d, %d, ?)}"), FID1, FID2 );

//// check in the frient list or not
//#define  MAKEQUERY_CheckFriendlist(strQ, FID1, FID2)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Friendlist_Check(%d, %d, ?)}"), FID1, FID2 );


/************************************************************************/
/*			Visit List                                                  */
/************************************************************************/
#define  MAKEQUERY_Delete30DayDatas(strQ)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Data_Delete30Day}") );

// get visitor list
#define  MAKEQUERY_GetVisitList(strQ, RoomID, page, pagesize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_List_year2(%d, %d, %d)}"), RoomID, page, pagesize);

// get visitor list FID only
#define  MAKEQUERY_GetVisitList_FID(strQ, RoomID, page, FID, pagesize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_List_Fid_year2(%d, %d, %d, %d)}"), RoomID, page, FID, pagesize);

// get visitor group
#define  MAKEQUERY_GetVisitGroup1(strQ, GroupID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_List_Get1_year(%d)}"), GroupID);

// add visitor group
#define  MAKEQUERY_Shrd_Visit_Insert(strQ, RoomID, FID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Insert_year(%d, %d, ?, ?)}"), RoomID, FID );

// add visitor memo
#define  MAKEQUERY_AddVisitMemo(strQ, ActionGroup, ActionType, Pray, Secret)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Insert_year(%d, %d, N'%s', %d, ?, ?)}"), ActionGroup, ActionType, Pray.c_str(), Secret );

#define  MAKEQUERY_AddVisitMemo_Aud(strQ, ActionGroup, ActionType, Pray, Secret)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Insert_year_Aud(%d, %d, N'%s', %d, ?, ?)}"), ActionGroup, ActionType, Pray.c_str(), Secret );

#define  MAKEQUERY_AddVisitMemo1(strQ, ActionGroup, ActionType, Pray, Secret, ItemData)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Insert_year(%d, %d, N'%s', %d, 0x%S, ?)}"), ActionGroup, ActionType, Pray.c_str(), Secret, ItemData );

#define  MAKEQUERY_AddVisitMemo1_Aud(strQ, ActionGroup, ActionType, Pray, Secret, ItemData)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Insert_year_Aud(%d, %d, N'%s', %d, 0x%S, ?)}"), ActionGroup, ActionType, Pray.c_str(), Secret, ItemData );
//// delete visitor memo
//#define  MAKEQUERY_DeleteVisitMemo(strQ, group_id, action_id)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Delete(%d, %d, ?)}"), action_id, group_id );

// add multilite memo
#define  MAKEQUERY_AddMultiVisitMemo(strQ, GroupID, VisitID, FID, Secret, ItemID, Pray)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Multi_year_Insert(%d, %d, %d, %d, %d, N'%s')}"), GroupID, VisitID, FID, Secret, ItemID, Pray.c_str());

#define  MAKEQUERY_AddMultiVisitMemo_Aud(strQ, GroupID, VisitID, FID, Secret, ItemID, Pray)                     \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_List_Multi_Insert_year_Aud(%d, %d, %d, %d, %d, N'%s')}"), GroupID, VisitID, FID, Secret, ItemID, Pray.c_str());

// get multilite memo
#define  MAKEQUERY_GetVisitData(strQ, VisitID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Data_Get_year(%d)}"), VisitID);

// get multilite memo's subid
#define  MAKEQUERY_GetVisitDataSubID(strQ, VisitID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Data_GetSubID_year(%d)}"), VisitID);

// shield visit memo
#define  MAKEQUERY_ShieldVisitData(strQ, VisitID, SubID, shieldflag)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Data_update_year(%d, %d, %d)}"), VisitID, SubID, shieldflag );

// delete visit memo
#define  MAKEQUERY_DeleteVisitData(strQ, VisitID, SubID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Data_Delete_year(%d, %d)}"), VisitID, SubID);

// modify visit memo
#define  MAKEQUERY_ModifyVisitData(strQ, FID, VisitID, SubID, Secret, pray)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_Data_modify_year(%d, %d, %d, %d, N'%s')}"), FID, VisitID, SubID, Secret, pray.c_str() );

#define  MAKEQUERY_ModifyVisitData_Aud(strQ, FID, VisitID, SubID, Secret, pray, atype)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_Data_modify_year_Aud(%d, %d, %d, %d, N'%s', %d)}"), FID, VisitID, SubID, Secret, pray.c_str(), atype );

// insert fish & flower data
#define  MAKEQUERY_AddFishFlowerVisitData(strQ, ActionGroup, Type, ItemID, OldFlowerID)                     \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_FishFlower_Insert_year(%d, %d, %d, %d)}"), ActionGroup, Type, OldFlowerID, ItemID );

// get visitor list FID only
#define  MAKEQUERY_GetRoomVisitFIDInOneDay(strQ, RoomID, year, month, day)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_Get_Fid_year(%d, %d, %d, %d)}"), RoomID, year, month, day );


/************************************************************************/
/*			Letter                                                      */
/************************************************************************/
// Clear tbl_Letter
#define  MAKEQUERY_ClearPaper(strQ)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Letter_Clear}"));

// get letter list
#define  MAKEQUERY_GetPaperList(strQ, RoomID, Year, FID, sortflag, page, pagesize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Letter_PagList3(%d, %d, %d, %d, %d, %d,  ?, ?, ?)}"), RoomID, Year, FID, sortflag, page, pagesize);

// get letter content
#define  MAKEQUERY_GetPaperContent(strQ, LetterID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Letter_List(%d)}"), LetterID );

// delete letter
#define  MAKEQUERY_DeletePaper(strQ, LetterID, FID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Letter_delete(%d, %d)}"), LetterID, FID );

// insert letter
#define  MAKEQUERY_InsertPaper(strQ, title, content, RoomID, opened, FID)                         \
	tchar strQ[1600] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1600, _t("{call Shrd_Letter_Insert(N'%s', N'%s', %d, %d, %d)}"), title.c_str(), content.c_str(), RoomID, opened, FID );

// modify letter
#define  MAKEQUERY_UpdatePaper(strQ, LetterID, title, content, opened, FID)                         \
	tchar strQ[1600] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1600, _t("{call Shrd_Letter_Update(%d, N'%s', N'%s', %d, %d)}"), LetterID, title.c_str(), content.c_str(), opened, FID );

/************************************************************************/
/*			Update Room                                                 */
/************************************************************************/
// get room renew list
#define  MAKEQUERY_GetRoomRenewHistory(strQ, RoomID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Renew_History_List(%d, ?, ?, ?)}"), RoomID );

// update room renew list
#define  MAKEQUERY_UpdateRoomRenewHistory(strQ, RoomID, NewSceneID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Renew_History_Update(%d, %d)}"), RoomID, NewSceneID );

// promotion room
#define  MAKEQUERY_PromotionRoom(strQ, RoomID, AddHisSpace, SceneID, IsSamePromotion)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Update_SceneId(%d, %d, %d, %d, ?)}"), RoomID, SceneID, AddHisSpace, IsSamePromotion );

//// promotion room by card
//#define  MAKEQUERY_PromotionRoomByCard(strQ, RoomID, CardType)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Update_ByCard(%d, %d, ?)}"), RoomID, CardType);

/************************************************************************/
/*			Mail                                                        */
/************************************************************************/

//#define  MAKEQUERY_GetMailboxStatus(strQ, FID)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_list_Receive_New(%d, ?, ?, ?, ?)}"), FID );

//// Get SendMail List
//#define  MAKEQUERY_GetSendMailList(strQ, FID, page, PageSize)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_list_Send_Page1(%d, %d, %d, ?, ?)}"), FID, page, PageSize );

//// Get ReceiveMail List
//#define  MAKEQUERY_GetReceiveMailList(strQ, FID, page, PageSize)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_list_Receive_Page1(%d, %d, %d, ?, ?, ?)}"), FID, page, PageSize );

//// Get a mail
//#define  MAKEQUERY_GetMail(strQ, mail_id)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_list(%d, ?)}"), mail_id );

//// Insert a mail
//#define  MAKEQUERY_InsertMail(strQ, FFID, RFID, Title, Content, MType, ItemExist)		\
//	tchar strQ[1024] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Mail_Insert(%d, %d, N'%s', N'%s', %d, %d, ?, ?)}"),	\
//	FFID, RFID, Title.c_str(), Content.c_str(), MType, ItemExist);

//// Delete a mail
//#define  MAKEQUERY_DeleteMail(strQ, mail_id, fid, mail_type)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_Delete(%d, %d, %d, ?)}"), mail_id, fid, mail_type );

//// Reject mail
//#define  MAKEQUERY_RejectMail(strQ, mail_id, fid)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_Return(%d, %d, ?)}"), mail_id, fid );

//// Set Mail Status
//#define  MAKEQUERY_SetMailStatus(strQ, mail_id, status)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_Read(%d, %d)}"), mail_id, status );

//// Get Mail Status
//#define  MAKEQUERY_GetMailStatus(strQ, mail_id)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_list_StatusAndMtype(%d, ?, ?, ?, ?, ?)}"), mail_id );

//// Insert a GM mail
//#define  MAKEQUERY_InsertMail_GM(strQ, toFID, Title, Content, ItemExist)		\
//	tchar strQ[1024] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Mail_Insert_GM(%d, N'%s', N'%s', %d, ?, ?)}"),	\
//	toFID, Title.c_str(), Content.c_str(), ItemExist);

//// Auto Back mail
//#define  MAKEQUERY_AutoReturnMail(strQ)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Mail_ReturnBySystem}"));




//// move item from oldPos to newPos
//#define  MAKEQUERY_MOVEITEM(strQ)                         \
//	tchar strQ[300] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Fitem_MovePos(?, ?, ?, ?, ?)}") );

// update the family's item's position
//#define  MAKEQUERY_UPDATEITEMPOS(strQ)                      \
//tchar strQ[500] = _t("");	                                             \
//SIPBASE::smprintf(strQ, 500, _t("{call Shrd_FamilyItem_UpdatePos(?, ?, ?, ?)}") );

//// get the recommend rooms or the top room
//#define  MAKEQUERY_GetJingPinTianYuanList(strQ)                      \
//	tchar strQ[500] = _t("");	                                             \
//	SIPBASE::smprintf(strQ, 500, _t("{call Shard_Room_Orderby_List1}")); 



// 소원추가
#define MAKEQUERY_AddWish(strQ, RoomID, FID, ItemID, TargetName, Pray, ItemTime)						\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_Public_Wish_Insert(%d, %d, N'%s', N'%s', %d, %d)}"), RoomID, FID, TargetName.c_str(), Pray.c_str(), ItemID, ItemTime);

// 소원목록얻기
#define MAKEQUERY_GetWishList(strQ, RoomID, Page, PageSize)						\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_Public_Wish_List(%d, %d, %d, ?, ?)}"), RoomID, Page, PageSize);

// 내 소원목록얻기
#define MAKEQUERY_GetMyWishList(strQ, RoomID, FID, Page, PageSize)						\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_Public_Wish_List_FID(%d, %d, %d, %d, ?, ?)}"), RoomID, FID, Page, PageSize);

// 소원의 기도내용 얻기
#define MAKEQUERY_GetWishPray(strQ, WishID)						\
	tchar strQ[600] = _t("");								\
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_Public_Wish_GetPray(%d)}"), WishID);



//// 열점추천방 설정
//#define MAKEQUERY_SetRecommendRoom(strQ, Index, RoomID, PhotoID, Date)						\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Recommend_InOrUpOrde(%d, %d, %d, %d)}"), Index, RoomID, PhotoID, Date);

//// 열점추천방 얻기
//#define MAKEQUERY_GetRecommendRoom(strQ)						\
//	tchar strQ[100] = _t("");								\
//	SIPBASE::smprintf(strQ, 100, _t("{call Shrd_Room_Recommend_Get}"));



// 사용자의 공덕값/공덕값리력 정보를 얻기
#define MAKEQUERY_ReadFamilyVirtueHisData(strQ, FID, ReligionRoomType)															\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Virtue_Get(%d, %d)}"), FID, ReligionRoomType);

// 사용자의 공덕값/공덕값리력 정보를 저장
#define MAKEQUERY_SaveFamilyVirtueData(strQ, FID, ReligionRoomType, AddVirtue, Data)														\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Virtue_UpdateVirtueData(%d, %d, %d, 0x%s)}"), FID, ReligionRoomType, AddVirtue, Data);

// 공덕비에 새겨질 목록 얻기
#define MAKEQUERY_GetVirtueList(strQ, ReligionRoomType)														\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_Virtue_Orderby_GetList(%d)}"), ReligionRoomType);



// 종교구역에서 행사 추가
#define  MAKEQUERY_Delete30DayReligionDatas(strQ, nowTime)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_ReligionAct_Delete30day(%d)}"), nowTime);

#define  MAKEQUERY_Religion_InsertAct(strQ, ReligionRoomType, FID, NpcID, ActionType, Secret)                         \
	tchar strQ[600] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 600, _t("{call Shrd_Visit_ReligionAct_Insert(%d, %d, %d, %d, %d)}"), ReligionRoomType, FID, NpcID, ActionType, Secret);

#define  MAKEQUERY_Religion_InsertPray(strQ, ActID, FID, Secret, Pray)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_ReligionPray_Insert_Aud(%d, %d, %d, N'%s', ?)}"), ActID, FID, Secret, Pray.c_str());

// 종교구역에서 행사의 기도문 수정
#define  MAKEQUERY_Religion_UpdatePray(strQ, ID, FID, bSecret, Pray)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Visit_ReligionPray_Update_Aud(%d, %d, %d, N'%s')}"), ID, FID, bSecret, Pray.c_str());

// 종교구역에서 행사목록 얻기
#define  MAKEQUERY_Religion_GetActList(strQ, ReligionRoomType, NpcID, Page, PageSize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_ReligionAct_List1(%d, %d, %d, %d, ?, ?)}"), ReligionRoomType, NpcID, Page, PageSize);

// 종교구역에서 내 행사목록 얻기
#define  MAKEQUERY_Religion_GetActList_FID(strQ, ReligionRoomType, FID, NpcID, Page, PageSize)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_ReligionAct_List_FID(%d, %d, %d, %d, %d, ?, ?)}"), ReligionRoomType, FID, NpcID, Page, PageSize);

// 종교구역에서 행사의 기도문 얻기
#define  MAKEQUERY_Religion_GetActPray(strQ, ID)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_ReligionPray_Get(%d)}"), ID);

#define  MAKEQUERY_Religion_GetAct_OnlyToday(strQ, ReligionRoomType)                         \
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Visit_ReligionAct_OnlyToday(%d)}"), ReligionRoomType);

#define  MAKEQUERY_GetReligionPBYHisList(strQ, Page, PageNum)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Religion_PBYMain_HisList(%d, %d)}"), Page, PageNum);

#define  MAKEQUERY_GetReligionMyPBYHisList(strQ, FID, Page, PageNum)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Religion_PBYMyMain_HisList(%d, %d, %d)}"), FID, Page, PageNum);

#define  MAKEQUERY_GetReligionPBYInfo(strQ, HisMainID)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Religion_PBYInfo(%d)}"), HisMainID);

#define  MAKEQUERY_Religion_PBYMain_Insert(strQ, FID, ActTarget, ActProperty, OpenProperty, ActText, BeginTime)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_PBYMain_Insert(%d, N'%s', N'%s', %d, N'%s', %d, ?)}"), FID, ActTarget, ActProperty, OpenProperty, ActText, BeginTime);

#define  MAKEQUERY_Religion_PBYMain_Modify(strQ, MainID, OpenProperty, ActText)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_PBYMain_Modify(%d, %d, N'%s')}"), MainID, OpenProperty, ActText);

#define  MAKEQUERY_Religion_PBYJoin_Insert(strQ, MainID, FID, OpenProperty, ActText, JoinTime, Item)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_PBYJoin_Insert(%d, %d, %d, N'%s', %d, %d)}"), MainID, FID, OpenProperty, ActText, JoinTime, Item);

#define  MAKEQUERY_Religion_PBYJoin_Modify(strQ, SubID, OpenProperty, ActText)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_PBYJoin_Modify(%d, %d, N'%s')}"), SubID, OpenProperty, ActText);

#define  MAKEQUERY_Religion_SBY_Insert(strQ, ActTarget, ActProperty, ActCycle, BeginTime, BackMusicID, ActText)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_SBY_Insert(N'%s', N'%s', %d, %d, %d, N'%s', ?)}"), ActTarget, ActProperty, ActCycle, BeginTime, BackMusicID, ActText);

#define  MAKEQUERY_Religion_SBY_Modify(strQ, EditedID, ActTarget, ActProperty, ActCycle, BeginTime, BackMusicID, ActText)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_SBY_Modify(%d, N'%s', N'%s', %d, %d, %d, N'%s', ?)}"), EditedID, ActTarget, ActProperty, ActCycle, BeginTime, BackMusicID, ActText);

#define  MAKEQUERY_Religion_SBY_Delete(strQ, EditedID)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_SBY_Delete(%d)}"), EditedID);

#define  MAKEQUERY_Religion_SBY_Load(strQ)                         \
	tchar strQ[1024] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1024, _t("{call Shrd_Religion_SBY_Load}"));

// 기념관의 행사결과물 보관
#define  MAKEQUERY_SaveRoomActionResult(strQ, RoomID, type, time, result)                         \
	tchar strQ[1000] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 1000, _t("{call Shrd_Room_Action_Result_IandU(%d, %d, %d, 0x%s)}"), RoomID, type, time, result);



// 방양도설정
#define MAKEQUERY_GiveRoomStart(strQ, RoomID, toFID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Give_Start(%d, %d, ?)}"), RoomID, toFID);
// 방양도완료
#define MAKEQUERY_SetRoomMaster(strQ, RoomID, newFID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_room_setMaster1(%d, %d, ?)}"), RoomID, newFID);
// 방양도해제
#define MAKEQUERY_GiveRoomCancel(strQ, RoomID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_Give_Cancel(%d, ?)}"), RoomID);

//// 양도받는 방목록 얻기
//#define MAKEQUERY_GetReceiveRooms(strQ, FID)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GetReceiveRooms(%d)}"), FID);


//// 체험방설정
//#define MAKEQUERY_SETEXPROOM(strQ, Roomid, FamilyID, bset)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_mstExpRoom_SET_GM(%d, %d, %d)}"), Roomid, FamilyID, bset);
//#define MAKEQUERY_EXPROOMDATA(strQ, Roomid, FamilyID, bData)	\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_mstExpRoom_Checked_GM(%d, %d, %d)}"), Roomid, FamilyID, bData);
//#define MAKEQUERY_LOADEXPROOM(strQ)							\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_mstExpRoom_load}"));
//#define MAKEQUERY_UPDATEEXPROOMINDEX(strQ, roomID, roomindex)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_room_updateexproomindex %d, %d"), roomID, roomindex);


// 체험방Group 얻기
#define MAKEQUERY_GetExpRoomGroup(strQ)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomGroup_GoodLuck_Get}"));

// 체험방Group 추가
#define MAKEQUERY_InsertExpRoomGroup(strQ, GroupName, GroupIndex, GroupID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomGroup_GoodLuck_Insert_GM(N'%s', %d, %d)}"), GroupName.c_str(), GroupIndex, GroupID);

// 체험방Group 수정
#define MAKEQUERY_ModifyExpRoomGroup(strQ, GroupID, GroupName, GroupIndex)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomGroup_GoodLuck_Update_GM(%d, N'%s', %d)}"), GroupID, GroupName.c_str(), GroupIndex);

// 체험방Group 삭제
#define MAKEQUERY_DeleteExpRoomGroup(strQ, GroupID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomGroup_GoodLuck_Delete_GM(%d)}"), GroupID);

// 체험방목록 얻기
#define MAKEQUERY_GetExpRoom(strQ)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GoodLuck_Get1}"));

// 체험방 추가
#define MAKEQUERY_InsertExpRoom(strQ, RoomID, GroupID, RoomIndex, PhotoID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GoodLuck_Insert_GM1(%d, %d, %d, %d)}"), RoomID, GroupID, RoomIndex, PhotoID);

// 체험방 수정
#define MAKEQUERY_ModifyExpRoom(strQ, RoomID, GroupID, RoomIndex, PhotoID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GoodLuck_Update_GM(%d, %d, %d, %d)}"), RoomID, GroupID, RoomIndex, PhotoID);

// 체험방 삭제
#define MAKEQUERY_DeleteExpRoom(strQ, RoomID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GoodLuck_Delete_GM(%d)}"), RoomID);



// TopList-경험값순위방목록 얻기
//#define MAKEQUERY_GetTopList_RoomExp(strQ)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_RoomExp1}"));
#define MAKEQUERY_GetTopList_RoomExp(strQ, roomCondition)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_RoomExp2(N'%s')}"), roomCondition.c_str());

// TopList-방문록순위방목록 얻기
//#define MAKEQUERY_GetTopList_RoomVisit(strQ)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_RoomVisit1}"));
#define MAKEQUERY_GetTopList_RoomVisit(strQ, roomCondition)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_RoomVisit2(N'%s')}"), roomCondition.c_str());

// TopList-경험값순위Family목록 얻기
//#define MAKEQUERY_GetTopList_FamilyExp(strQ)				\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_FamilyExp}"));
#define MAKEQUERY_GetTopList_FamilyExp(strQ, familyCondition)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_order_FamilyExp2(N'%s')}"), familyCondition.c_str());



//////// 꽃, 물고기 ////////////
#define MAKEQUERY_GetFishFlowerData(strQ, RoomID)					\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomFishFlower_List(%d)}"), RoomID);

#define MAKEQUERY_SaveFishExp(strQ, RoomID, FishLevel, FishExp, FishUpdateTime)					\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomFishFlower_Update_Fish(%d, %d, %d, %d)}"), RoomID, FishLevel, FishExp, FishUpdateTime);

#define MAKEQUERY_SaveFlowerLevel(strQ, RoomID, FlowerID, FlowerLevel, FlowerDelTime, FlowerFID)					\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomFishFlower_Update_Flower1(%d, %d, %d, %d, %d)}"), RoomID, FlowerID, FlowerLevel, FlowerDelTime, FlowerFID);

//#define MAKEQUERY_SaveRoomFishData(strQ, RoomID, FishData)					\
//	tchar strQ[300] = _t("");								\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_RoomFishFlower_Update_Fish1(%d, 0x%S)}"), RoomID, FishData);



// 조상탑기능
#define MAKEQUERY_SaveAncestorText(strQ, RoomID, text)					\
	tchar strQ[1700] = _t("");								\
	SIPBASE::smprintf(strQ, 1700, _t("{call Shrd_room_Ancestor_InsertOrUpdate(%d, N'%s')}"), RoomID, text.c_str());

#define MAKEQUERY_SaveAncestorDeceased(strQ, RoomID, AncestorID, DeadID, SurName, Name, SexID, BirthDate, DeadDate, BriefHistory, Domicile, Nation, PhotoID, PhotoType)					\
	tchar strQ[1300] = _t("");								\
	SIPBASE::smprintf(strQ, 1300, _t("{call Shrd_room_Ancestor_Deceased_InsertOrUpdate(%d, %d, %d, %d, N'%s', %d, %d, %d, N'%s', N'%s', N'%s', %d, %d)}"), RoomID, AncestorID, DeadID, SurName, Name.c_str(), SexID, BirthDate, DeadDate, BriefHistory.c_str(), Domicile.c_str(), Nation.c_str(), PhotoID, PhotoType);


//// 사용자형상사진
//#define MAKEQUERY_GetFamilyFigure(strQ, FID)				\
//	tchar strQ[300] = _t("");						\
//	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_GetFigureID(%d, ?, ?)}"), FID);

#define MAKEQUERY_SetFamilyFigure(strQ, FID, FigureID, FigureSecretFlag)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Family_UpdateFigureID(%d, %d, %d)}"), FID, FigureID, FigureSecretFlag);


// 금붕어정보 보관
#define MAKEQUERY_SaveGoldFishData(strQ, RoomID, GoldFishScopeID, GoldFishData, HistoryData)				\
	tchar strQ[2048] = _t("");						\
	SIPBASE::smprintf(strQ, 2048, _t("{call Shrd_RoomFishFlower_UpdateGoldFishData(%d, %d, 0x%s, 0x%s)}"), RoomID, GoldFishScopeID, GoldFishData, HistoryData);


// 보물창고정보
#define MAKEQUERY_SaveRoomStore(strQ, RoomID, StoreData)				\
	tchar strQ[1500] = _t("");						\
	SIPBASE::smprintf(strQ, 1500, _t("{call Shrd_Room_Store_InsertOrUpdate(%d, 0x%s)}"), RoomID, StoreData);

#define MAKEQUERY_SaveRoomStoreHistory(strQ, RoomID, FID, Flag, ItemData, Protected, Comment)				\
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_Room_Store_List_Insert1(%d, %d, %d, 0x%s, N'%s', %d, ?)}"), RoomID, FID, Flag, ItemData, Comment.c_str(), Protected);

#define MAKEQUERY_GetRoomStoreHistory(strQ, RoomID, Year, Month, Flag, actorFID, Page)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_GetStoreList1(%d, %d, %d, %d, %d, %d, ?, ?, ?)}"), RoomID, actorFID, Year, Month, Flag, Page);

#define MAKEQUERY_GetRoomStoreHistoryDetail(strQ, ListID)				\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_Room_getStoreDetail(%d)}"), ListID);

#define MAKEQUERY_ModifyRoomStoreHistory(strQ, ListID, Protected, Comment)				\
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_Room_Store_List_Modify(%d, N'%s', %d)}"), ListID, Comment.c_str(), Protected);

#define MAKEQUERY_DeleteRoomStoreHistory(strQ, ListID)				\
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_Room_Store_List_Delete(%d)}"), ListID);

#define MAKEQUERY_ShieldRoomStoreHistory(strQ, ListID, Shield)				\
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_Room_Store_update_Shield(%d, %d)}"), ListID, Shield);


// 경험값2배기념일
#define MAKEQUERY_GetHolidayList(strQ)				\
	tchar strQ[1500] = _t("");						\
	SIPBASE::smprintf(strQ, 1500, _t("{call Shrd_mst_Holiday_Get}"));


//// 축복카드기능
//#define MAKEQUERY_LoadBlessCard(strQ, FID)     \
//	tchar strQ[500] = _t("");						\
//	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_GetBlessCard(%d)}"), FID );

#define MAKEQUERY_SaveBlessCard(strQ, FID, BlessCardID, Count)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_SetBlessCard(%d, %d, %d)}"), FID, BlessCardID, Count );

#define MAKEQUERY_LoadCheckIn(strQ, FID)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_family_Signin_Day_list(%d)}"), FID );

#define MAKEQUERY_SaveCheckIn(strQ, FID, nCheckDays)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_family_Signin_Day_insert(%d, %d)}"), FID, nCheckDays );

#define  MAKEQUERY_GetPublicRoomLuckHisList(strQ, Page, PageNum)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_PublicRoomLuck_List1(%d, %d)}"), Page, PageNum);

#define  MAKEQUERY_GetPublicRoomLuckHisInsert(strQ, FID, LuckID)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_PublicRoomLuck_Insert(%d, %d)}"), FID, LuckID);

#define  MAKEQUERY_GetPublicRoomLuckRefresh(strQ)	\
	tchar strQ[300] = _t("");	                                             \
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_PublicRoomLuck_Refresh}"));

//// 사용후기
//#define MAKEQUERY_LoadReviewList(strQ)     \
//	tchar strQ[500] = _t("");						\
//	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Room_Focus_Get}") );

//#define MAKEQUERY_InsertReview(strQ, ReviewID, Name, Content, Url)     \
//	tchar strQ[500] = _t("");						\
//	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Room_Focus_Insert(%d, N'%s', N'%s', N'%s')}"), ReviewID, Name.c_str(), Content.c_str(), Url.c_str() );

//#define MAKEQUERY_ModifyReview(strQ, ReviewID, Name, Content, Url)     \
//	tchar strQ[500] = _t("");						\
//	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Room_Focus_Modify(%d, N'%s', N'%s', N'%s')}"), ReviewID, Name.c_str(), Content.c_str(), Url.c_str() );

//#define MAKEQUERY_DeleteReview(strQ, ReviewID)     \
//	tchar strQ[500] = _t("");						\
//	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Room_Focus_Del(%d)}"), ReviewID );


// 검사에서 불합격된 방목록을 얻는다.
#define MAKEQUERY_GetAuditingErrList(strQ)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Auditing_Err_List}") );

// 공공구역에서 벽에 붙이는 체험방자료를 얻는다.
#define MAKEQUERY_GetPublicroomFrameList(strQ)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Publicroom_GetFrameList}") );

// 공공구역에서 벽에 붙이는 체험방자료를 설정한다.
#define MAKEQUERY_SetPublicroomFrame(strQ, Index, PhotoID, FrameID)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_Publicroom_UpdateFrame(%d, %d, %d)}"), Index, PhotoID, FrameID );

// 공공구역에서 대형행사목록 얻기
#define MAKEQUERY_GetLargeActList(strQ, RoomID)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_LargeAct_GetList(%d)}"), RoomID );

// 공공구역에서 대형행사 추가
#define MAKEQUERY_InsertLargeAct(strQ, RoomID, Title, Comment, FlagText, ActMean, Option, AcceptTime, StartTime, ItemIDs, PhotoIDs, Prizes, Vips, ONotice, ENotice)     \
	tchar strQ[4096] = _t("");						\
	SIPBASE::smprintf( strQ, 4096, _t("{call Shrd_LargeAct_Insert(%d, N'%s', N'%s', N'%s', N'%s', %d, N'%s', N'%s', 0x%s, 0x%s, 0x%s, 0x%s, N'%s', N'%s', ?)}"), RoomID, Title.c_str(), Comment.c_str(), FlagText.c_str(), ActMean.c_str(), Option, AcceptTime.c_str(), StartTime.c_str(), ItemIDs, PhotoIDs, Prizes, Vips, ONotice.c_str(), ENotice.c_str() );

// 공공구역에서 대형행사 수정
#define MAKEQUERY_ModifyLargeAct(strQ, ActID, Title, Comment, FlagText, ActMean, Option, AcceptTime, StartTime, ItemIDs, PhotoIDs, Prizes, Vips, ONotice, ENotice)     \
	tchar strQ[2048] = _t("");						\
	SIPBASE::smprintf( strQ, 2048, _t("{call Shrd_LargeAct_Modify(%d, N'%s', N'%s', N'%s', N'%s', %d, N'%s', N'%s', 0x%s, 0x%s, 0x%s, 0x%s, N'%s', N'%s')}"), ActID, Title.c_str(), Comment.c_str(), FlagText.c_str(), ActMean.c_str(), Option, AcceptTime.c_str(), StartTime.c_str(), ItemIDs, PhotoIDs, Prizes, Vips, ONotice.c_str(), ENotice.c_str() );

// 공공구역에서 대형행사 삭제
#define MAKEQUERY_DeleteLargeAct(strQ, ActID)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_LargeAct_Delete(%d)}"), ActID );

// 공공구역에서 대형행사의 참가자수 설정
#define MAKEQUERY_SetLargeActUserCount(strQ, ActID, UserCount)     \
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf( strQ, 500, _t("{call Shrd_LargeAct_SetUserCount(%d, %d)}"), ActID, UserCount );


// 금언사용자 목록얻기
#define MAKEQUERY_GetSilenceList(strQ, CurTime)     \
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Silence_Get(%d)}"), CurTime );

// 금언사용자 설정
#define MAKEQUERY_SetSilence(strQ, FID, SilenceTime)     \
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf( strQ, 300, _t("{call Shrd_Silence_Set(%d, %d)}"), FID, SilenceTime );

// 3달동안 사용되지 않은 무료방을 삭제한다.
#define MAKEQUERY_DeleteNonusedFreeRoom(strQ)		\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_DeleteNonusedFreeRoom}"));

// 지역써버이동시 이동하여야 할 자료를 얻는다.
#define	MAKEQUERY_GetUserMoveData(strQ, FID)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_GetMovableInfo %u"), FID);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - 초기화
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_reset(strQ, FID)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_reset %u"), FID);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Family
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family(strQ, FID, UID, GMoney, UserExp, FLevel)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_tbl_Family %u, %u, %d, %d, %d"), FID, UID, GMoney, UserExp, FLevel);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Family_ExpHis
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_ExpHis(strQ, FID, ExpHisData)	\
	tchar strQ[500] = _t("");						\
	SIPBASE::smprintf(strQ, 500, _t("exec Shrd_Family_SaveMovableInfo_tbl_Family_ExpHis %u, 0x%s"), FID, ExpHisData);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Character
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Character(strQ, FID, Name, ModelID, SexID, DefaultDressID, FaceID, HairID, DressID, BirthDate, City, Resume)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_tbl_Character %u, N'%s', %d, %d, %d, %d, %d, %d, %d, %d, N'%s'"), FID, Name.c_str(), ModelID, SexID, DefaultDressID, FaceID, HairID, DressID, BirthDate, City, Resume.c_str());

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_BlessCard
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_BlessCard(strQ, FID, BlessCardID, BCount)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_tbl_BlessCard %u, %d, %d"), FID, BlessCardID, BCount);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Family_Virtue
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_Virtue(strQ, FID, QFVirtue, QFVirtueHis)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_tbl_Family_Virtue %u, %d, 0x%s"), FID, QFVirtue, QFVirtueHis);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Item
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Item(strQ, FID, ItemData)	\
	tchar strQ[4500] = _t("");						\
	SIPBASE::smprintf(strQ, 4500, _t("exec Shrd_Family_SaveMovableInfo_tbl_Item %u, 0x%s"), FID, ItemData);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Family_RoomOrderInfo
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_RoomOrderInfo(strQ, FID, info1, info2, info3)	\
	tchar strQ[8192] = _t("");						\
	SIPBASE::smprintf(strQ, 8192, _t("exec Shrd_Family_SaveMovableInfo_tbl_Family_RoomOrderInfo %u, 0x%s, 0x%s, 0x%s"), FID, info1, info2, info3);

// 지역써버이동시 이동하여야 할 자료를 쓰기 - tbl_Family_Signin_Day
#define	MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_Signin_Day(strQ, FID, SigninData, SigninCount)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_Family_SaveMovableInfo_tbl_Family_Signin_Day %u, N'%S', %d"), FID, SigninData.c_str(), SigninCount);

// 캐릭터의 이름이 변경되였을 때 다른 지역써버들에서 DB에서 수정한다.
#define	MAKEQUERY_Shrd_Merge_Character_UpdateName(strQ, FID, chName)	\
	tchar strQ[256] = _t("");						\
	SIPBASE::smprintf(strQ, 256, _t("exec Shrd_Merge_Character_UpdateName %u, N'%s'"), FID, chName.c_str());

// FavorRoom정보를 얻기
#define	MAKEQUERY_Shrd_GetRoomListInfo(strQ, RoomStr)	\
	tchar strQ[8192] = _t("");						\
	SIPBASE::smprintf(strQ, 8192, _t("exec Shrd_GetRoomListInfo '%s'"), RoomStr.c_str());

// 로그인시에 가족정보 얻기
#define MAKEQUERY_GetFamilyAllInfoForLogin_Lobby(strQ, uID)				\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_getAllInfo_Login_lobby(%d, ?, ?)}"), uID);

// 로그인시에 가족정보 얻기
#define MAKEQUERY_GetFamilyAllInfoForLogin_MS(strQ, FID)				\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_getAllInfo_Login_ms(%d)}"), FID);

// 지역진입시에 가족정보 얻기
#define MAKEQUERY_GetFamilyAllInfoForEnterShard_MS(strQ, FID)				\
	tchar strQ[500] = _t("");								\
	SIPBASE::smprintf(strQ, 500, _t("{call Shrd_family_getAllInfo_EnterShard_ms(%d)}"), FID);

// Family Exp정보 load - offline상태에서의 경험값증가를 위해서.
#define MAKEQUERY_LoadFamilyExpInfo(strQ, FID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_LoadFamilyExpInfo(%d)}"), FID);

#define MAKEQUERY_GetFamilyNengLiang(strQ, FID)				\
	tchar strQ[300] = _t("");								\
	SIPBASE::smprintf(strQ, 300, _t("{call Shrd_family_GetNengLiang(%d)}"), FID);

// point아이템을 일반아이템으로 바꾸기 위하여 자료 읽기
#define	MAKEQUERY_Test_GetFamilyInven(strQ)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_test_GetFamilyInven"));

// point아이템을 일반아이템으로 바꾼것을 표시
#define	MAKEQUERY_Test_SetFamilyInvenChecked(strQ, FID)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_test_SetFamilyInvenChecked %u"), FID);

// 방주인에게 point를 주기 위하여 위하여 자료 읽기
#define	MAKEQUERY_Test_GetRoom(strQ)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_test_GetRoom"));

// 방주인에게 point를 준것을 표시
#define	MAKEQUERY_Test_SetRoomChecked(strQ, RoomID, FID, GMoney)	\
	tchar strQ[300] = _t("");						\
	SIPBASE::smprintf(strQ, 300, _t("exec Shrd_test_SetRoomChecked %u, %u, %u"), RoomID, FID, GMoney);

