#ifndef DBLOG_H
#define DBLOG_H

#include "misc/db_interface.h"
#include "../Common/Common.h"

// log main type
enum	ENUM_DBLOG_MAINTYPE {
	DBLOG_MAINTYPE_SHARDLOGIN = 1,				// Shard Login
	DBLOG_MAINTYPE_SHARDLOGOUT,					// Shard Logout
	DBLOG_MAINTYPE_CREATEFAMILY,				// Create family
	DBLOG_MAINTYPE_DELFAMILY,					// Delete family
	DBLOG_MAINTYPE_CHANE_CH,					// Change Character
	DBLOG_MAINTYPE_CHFGMoney,                   // change family's GMoney
	DBLOG_MAINTYPE_CHFAMILYEXP,                 // Change family's Exp
	DBLOG_MAINTYPE_CHFAMILYMONEY,               // Change family's UMoney
	DBLOG_MAINTYPE_BUYROOM = 21,				// Buy room
	DBLOG_MAINTYPE_DELROOM,						// Delete room
	DBLOG_MAINTYPE_ENTERROOM,					// Enter room
	DBLOG_MAINTYPE_OUTROOM,						// Out room
	DBLOG_MAINTYPE_ADDDEAD,						// Add dead
	DBLOG_MAINTYPE_DELDEAD,						// Delete dead
	DBLOG_MAINTYPE_CHROOMMASTER,				// Change room master	
	DBLOG_MAINTYPE_UNDELROOM,					// UnDelete room
	DBLOG_MAINTYPE_PROMOTIONROOM,				// Promotion room
	DBLOG_MAINTYPE_CHROOMEXP,                   // Change room' Exp
	DBLOG_MAINTYPE_BUYITEM = 40,				// Buy item
	DBLOG_MAINTYPE_USEITEM,						// Use item
	DBLOG_MAINTYPE_BUYUSEITEM,					// Buy and use item
	DBLOG_MAINTYPE_USENOBODYITEM,				// Use nobody item
	DBLOG_MAINTYPE_BUYSPACE = 60,				// Buy space
	DBLOG_MAINTYPE_ADDPHOTO,					// Add Photo
	DBLOG_MAINTYPE_DELPHOTO,					// Delete Photo
	DBLOG_MAINTYPE_ADDVIDEO,					// Add Video
	DBLOG_MAINTYPE_DELVIDEO,					// Delete Video
	DBLOG_MAINTYPE_ADDCONTACT = 80,				// Add a Contact
	DBLOG_MAINTYPE_DELCONTACT = 81,				// Add a Contact
//	DBLOG_MAINTYPE_GMMONEY = 1000,
};

// DBLOG_MAINTYPE_BUYROOM's subtype
enum	ENUM_DBLSUB_BUYROOM {
	DBLSUB_BUYROOM_NEW = 1,						// Create room
	DBLSUB_BUYROOM_RENEW,						// Renew room
	DBLSUB_BUYROOM_NEW_CARD,					// Create room by card
	DBLSUB_BUYROOM_RENEW_CARD,					// Renew room by card
};
// DBLOG_MAINTYPE_BUYROOM's subtype
enum	DBLOG_MAINTYPE_PROMOTIONROOM {
	DBLSUB_PROMOTION = 1,						// Promotion room
	DBLSUB_PROMOTION_CARD,						// Promotion room by card
};
// DBLOG_MAINTYPE_BUYITEM's subtype
enum	ENUM_DBLSUB_BUYITEM {
	DBLSUB_BUYITEM_NEW = 1,						// Buy
	DBLSUB_GETMAILITEM,                         // get item from the mail
	DBLSUB_GETBANKITEM,                         // get item from the bank
};
// DBLOG_MAINTYPE_USEITEM's subtype
enum	ENUM_DBLSUB_USEITEM {
	DBLSUB_USEITEM_THROW = 1,					// Throw
	DBLSUB_USEITEM_PUT,							// Put
	DBLSUB_USEITEM_USE,							// Use
	DBLSUB_USEITEM_REFRESH,						// Refresh
	DBLSUB_USEITEM_SENDMAIL,					// Send to mail
	DBLSUB_USEITEM_SIMPLERITE,					// Simple rite
	DBLSUB_USEITEM_BURN,						// Burn
	DBLSUB_USEITEM_NEWFLOWER,					// New flower
	DBLSUB_USEITEM_FISHFOOD,					// Fish food
	DBLSUB_USEITEM_DRESS_FIRST_USE,				// Dress FirstUse
	DBLSUB_USEITEM_MULTIRITE,					// Multi rite
	DBLSUB_USEITEM_R0_TAOCAN,					// taocan in ReligionRoom 0
	DBLSUB_USEITEM_R0_XIANG,					// xiang in ReligionRoom 0
	DBLSUB_USEITEM_R0_HUA,						// hua in ReligionRoom 0
	DBLSUB_USEITEM_R0_XUYUAN,					// xuyuan in ReligionRoom 0
	DBLSUB_USEITEM_AUTOPLAY,					// Autoplay
	DBLSUB_USEITEM_SIMPLEJISI,					// Simple Jisi
	DBLSUB_USEITEM_MULTIJISI,					// Multi Jisi
	DBLSUB_USEITEM_ANCESTOR_HUA,				// Ancestor Hua
	DBLSUB_USEITEM_ANCESTOR_XIANG,				// Ancestor Xiang
	DBLSUB_USEITEM_RUYIBEI,						// Ruyibei
};
// DBLOG_MAINTYPE_BUYUSEITEM's subtype
enum	ENUM_DBLSUB_BUYUSEITEM {
	DBLSUB_BUYUSEITEM_CHANGE = 1,					
	DBLSUB_BUYUSEITEM_SAC,							
	DBLSUB_BUYUSEITEM_MUSIC,
	DBLSUB_BUYUSEITEM_NEWPET,
	DBLSUB_BUYUSEITEM_CAREPET,
	DBLSUB_BUYUSEITEM_BURN,
};

enum	ENUM_DBLSUB_GMMONEY {
	DBLSUB_GMMONEY_ADD = 1,					
	DBLSUB_GMMONEY_DEL,					
};
// DBLOG_MAINTYPE_USENOBODYITEM's subtype
enum	ENUM_DBLSUB_USENOBODYITEM {
	DBLSUB_USENOBODYITEM_GIVEWATER = 1,					
};

extern	void	DBLOG_ShardLogin(T_FAMILYID _FID);
extern	void	DBLOG_ShardLogout(T_FAMILYID _FID);
extern	void	DBLOG_CreateFamily(T_FAMILYID _FID, T_FAMILYNAME _FName, T_ACCOUNTID _UserId, ucstring _UserName);
extern	void	DBLOG_DelFamily(T_FAMILYID _FID);
extern	void	DBLOG_ChangeCh(T_FAMILYID _FID, T_PRICE _UPrice, T_PRICE _GPrice);
extern  void    DBLOG_CHFamilyExp(T_FAMILYID _FID, T_FAMILYNAME _FName, T_F_EXP oldExp, T_F_EXP newExp, T_F_LEVEL level, uint8 FamilyExpType, T_ROOMID roomId, T_ROOMNAME roomName, ucstring ItemInfo);
extern  void    DBLOG_CHFamilyGMoney(T_FAMILYID _FID, T_FAMILYNAME _FName, uint32 oldGMoney, uint32 newGMoney);
extern	void	DBLOG_CreateRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days, T_PRICE _UMoney, T_PRICE _GMoney);
extern	void	DBLOG_CreateRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days);
extern	void	DBLOG_RenewRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days, T_PRICE _UMoney, T_PRICE _GMoney);
extern	void	DBLOG_RenewRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days);
extern	void	DBLOG_PromotionRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ID _NewSceneId, T_PRICE _UMoney, T_PRICE _GMoney);
extern	void	DBLOG_PromotionRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_ID _NewSceneId);
extern	void	DBLOG_DelRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	DBLOG_UnDelRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	DBLOG_EnterRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	DBLOG_OutRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern  void    DBLOG_ChangeRoomExp(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_ROOMEXP oldExp, T_ROOMEXP newExp, T_ROOMLEVEL roomLevel, uint8 RoomExpType, ucstring itemInfo = "");
extern	void	DBLOG_AddDead(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_DECEASED_POS _DeadPos);
extern	void	DBLOG_DelDead(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_DECEASED_POS _DeadPos);
extern	void	DBLOG_CHRoomMaster(T_FAMILYID _FID, T_FAMILYNAME _FName, ucstring _UName, T_ACCOUNTID _UID, T_FAMILYID _FID2, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	DBLOG_BuyItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney);
extern	void	DBLOG_GetMailItem(T_FAMILYID _FID, uint32 _MailID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_FAMILYID _fromFID, T_ACCOUNTID _fromUserId, uint32 _mainType, T_FAMILYNAME _fromFname );
extern	void	DBLOG_GetBankItem(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name);
extern	void	DBLOG_ThrowItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name);
extern	void	DBLOG_UseItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name);
extern	void	DBLOG_SendMailItem(T_FAMILYID _FID, uint32 _MailID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_FAMILYID _toFID, T_ACCOUNTID _toUserId, T_FAMILYNAME _toFname );
//extern	void	DBLOG_PutItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_RefreshItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name);
extern	void	DBLOG_DressFirstUse(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, ucstring _Name);

extern	void	DBLOG_ItemUsed(int UseSubtype, T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
/*
extern	void	DBLOG_SimpleRiteItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_BurnItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_NewFlowerItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_FishFoodItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_MultiRiteItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
*/
//extern	void	DBLOG_BurnItemInstant(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_RoomPart(T_FAMILYID _FID, T_ROOMPARTID _PartID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_Sacrifice(T_FAMILYID _FID, T_COMMON_ID _SacID, uint32 _SacNumber, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_SetMusic(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_AddPet(T_FAMILYID _FID, T_PETID _PetID, uint32 _PetNumber, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);
extern	void	DBLOG_CarePet(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID);

extern	void	DBLOG_BuySpace(T_FAMILYID _FID, T_COMMON_ID _SpaceID, uint32 _byte, T_PRICE _UMoney, T_PRICE _GMoney, uint32 room_id);
extern	void	DBLOG_AddPhoto(T_FAMILYID _FID, uint32 PhotoID, uint32 _byte, uint32 RoomID);
extern	void	DBLOG_DelPhoto(T_FAMILYID _FID, uint32 PhotoID, uint32 _byte, uint32 RoomID);
extern	void	DBLOG_AddVideo(T_FAMILYID _FID, uint32 VideoID, uint32 _byte, uint32 RoomID);
extern	void	DBLOG_DelVideo(T_FAMILYID _FID, uint32 VideoID, uint32 _byte, uint32 RoomID);

extern	void	DBLOG_AddContact(T_FAMILYID _FID, T_FAMILYID _targetID);
extern	void	DBLOG_DelContact(T_FAMILYID _FID, T_FAMILYID _targetID);
extern	void	DBLOG_AddHis(T_FAMILYID _FID, T_ROOMID _RoomID, uint8 _uHisType, uint32 _uHisPos);
//extern	void	DBLOG_RoomEvent(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMEVENTTYPE _EventID);
extern	void	DBLOG_CreateCH(T_FAMILYID _FID, T_CHARACTERID _CHID, T_MODELID _MID, T_DRESS _DID, T_FACEID _FaceID, T_HAIRID _HairID);
extern	void	DBLOG_DelCH(T_FAMILYID _FID);

// GM log main type
enum	ENUM_GMLOG_MAINTYPE {
	GMLOG_MAINTYPE_NOTIC = 1,
	GMLOG_MAINTYPE_SHOW,
	GMLOG_MAINTYPE_PULLFAMILY,
	GMLOG_MAINTYPE_LOGOFFFAMILY,
	GMLOG_MAINTYPE_OUTFAMILY,
	GMLOG_MAINTYPE_SETLEVEL,
	GMLOG_MAINTYPE_GIVEITEM,
	GMLOG_MAINTYPE_GIVEMONEY,
	GMLOG_MAINTYPE_SETEXPROOM = 20,
	GMLOG_MAINTYPE_UNSETEXPROOM,
	GMLOG_MAINTYPE_EXPROOM_LEVEL,
	GMLOG_MAINTYPE_EXPROOM_EXP,
	GMLOG_MAINTYPE_EXPROOM_VISITS
};

extern	void	GMLOG_Show(T_FAMILYID _GMID, bool _bShow);
extern	void	GMLOG_PullFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	GMLOG_OutFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	GMLOG_SetLevel(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_F_LEVEL _Level);
extern	void	GMLOG_GiveItem(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_ITEMSUBTYPEID _ItemID, uint32 _ItemNumber, ucstring _ItemName);
extern	void	GMLOG_GiveMoney(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_PRICE _UMoney, T_PRICE _GMoney);

extern	void	GMLOG_SetExpRoom(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	GMLOG_UnsetExpRoom(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName);
extern	void	GMLOG_PropExpRoomLevel(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_LEVEL _oldLevel, T_F_LEVEL _Level);
extern	void	GMLOG_PropExpRoomExp(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_EXP _oldExp, T_F_EXP _Exp);
extern	void	GMLOG_PropExpRoomVisit(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_EXP _oldVisit, T_F_EXP _Visit);

#define		NONE_NUMBER				0
#define		NONE_NVARCHAR			L""

#define		DECLARE_LOG_VARIABLES()											\
	uint32				MainType = NONE_NUMBER, SubType = NONE_NUMBER;		\
	uint32				PID = NONE_NUMBER;									\
	uint32				UID = NONE_NUMBER;									\
	uint32				ItemID = NONE_NUMBER;								\
	uint32				ItemTypeID = NONE_NUMBER;							\
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER, VI4  =NONE_NUMBER;	\
	ucstring			PName = NONE_NVARCHAR;								\
	ucstring			ItemName = NONE_NVARCHAR;							\
	\
	uint32				UserID = NONE_NUMBER;								\
	ucstring			FamilyName = NONE_NVARCHAR;							\
	ucstring			UserName = NONE_NVARCHAR;							\
	FAMILY*				pFamily = GetFamilyData(_FID);						\
	if (pFamily != NULL)													\
{																		\
	FamilyName = pFamily->m_Name; UserID = pFamily->m_UserID;	UserName = pFamily->m_UserName;			\
}

#define		DOING_LOG()														\
	CMessage			mesLog(M_CS_SHARDLOG);								\
	mesLog.serial(MainType, SubType, PID, UID, ItemID, ItemTypeID);			\
	mesLog.serial(VI1, VI2, VI3, VI4, PName, ItemName);						\
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );		\

#define		DECLARE_LOG_VARIABLES_HIS()											\
	uint32				MainType = NONE_NUMBER, SubType = NONE_NUMBER;		\
	uint32				PID = NONE_NUMBER;									\
	uint32				UID = NONE_NUMBER;									\
	uint32				ItemID = NONE_NUMBER;								\
	uint32				ItemTypeID = NONE_NUMBER;							\
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER, VI4  =NONE_NUMBER;	\
	ucstring			PName = NONE_NVARCHAR;								\
	ucstring			ItemName = NONE_NVARCHAR;							\
	\
	uint32				UserID = NONE_NUMBER;								\
	ucstring			FamilyName = NONE_NVARCHAR;							\
	ucstring			UserName = NONE_NVARCHAR;


#endif	//DBLOG_H