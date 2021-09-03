
#include <misc/types_sip.h>
#include <net/service.h>

#include "../../common/Packet.h"
#include "Family.h"

#include "../Common/DBLog.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

bool	g_bDBLogAction;


void RECORD_UNLAW_EVENT(uint8 UnlawType, ucstring UnlawContent, T_FAMILYID FID)
{
	sipwarning(L"Detect Unlaw action(%d) = %s", FID, UnlawContent.c_str());

	CMessage			mesLog(M_CS_UNLAWLOG);
	mesLog.serial(UnlawType, UnlawContent, FID);
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );

	FAMILY*				pFamily = GetFamilyData(FID);
	if (pFamily != NULL)
	{
		CMessage mes(M_FORCEDISCONNECT);
		mes.serial(FID, UnlawContent);
		CUnifiedNetwork::getInstance ()->send( pFamily->m_FES, mes );
	}
}

void	DBLOG_CreateFamily(T_FAMILYID _FID, T_FAMILYNAME _FName, T_ACCOUNTID _UserId, ucstring _UserName)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_CREATEFAMILY;
	PID = _FID;
	UID = _UserId;
	PName = _FName;
	ItemName = _UserName;
	DOING_LOG();
}

void	DBLOG_DelFamily(T_FAMILYID _FID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_DELFAMILY;
	PID = _FID;
	UID = UserID;
	PName = FamilyName;
	ItemName = UserName;
	DOING_LOG();
}

void	DBLOG_ChangeCh(T_FAMILYID _FID, T_PRICE _UPrice, T_PRICE _GPrice)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_CHANE_CH;
	PID = _FID;
	VI2 = _UPrice;
	VI3 = _GPrice;
	DOING_LOG();
}

void    DBLOG_CHFamilyExp(T_FAMILYID _FID, T_FAMILYNAME _FName, T_F_EXP oldExp, T_F_EXP newExp, T_F_LEVEL level, uint8 FamilyExpType, T_ROOMID roomId, T_ROOMNAME roomName, ucstring ItemInfo)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_CHFAMILYEXP;
	SubType = FamilyExpType;
	PID = _FID;
	UID = UserID;
	VI1 = oldExp;
	VI2 = newExp;
	VI3 = level;
	VI4 = roomId;
	PName = L"[" + UserName + L"]" + FamilyName + L"[" + roomName + L"]" ;
	ItemName = ItemInfo;
	DOING_LOG();
}

void DBLOG_CHFamilyGMoney(T_FAMILYID _FID, T_FAMILYNAME _FName, uint32 oldGMoney, uint32 newGMoney)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_CHFGMoney;
	PID = _FID;
	UID = UserID;
	VI1 = oldGMoney;
	VI2 = newGMoney;
	PName = FamilyName;
	ItemName = UserName;
	DOING_LOG();
}

void	DBLOG_CreateRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days, T_PRICE _UMoney, T_PRICE _GMoney)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYROOM;
	SubType = DBLSUB_BUYROOM_NEW;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _RoomType;
	VI1 = _Days;
	VI2 = _UMoney;
	VI3 = _GMoney;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_CreateRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYROOM;
	SubType = DBLSUB_BUYROOM_NEW_CARD;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _RoomType;
	VI1 = _Days;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_PromotionRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ID _NewSceneId, T_PRICE _UMoney, T_PRICE _GMoney)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_PROMOTIONROOM;
	SubType = DBLSUB_PROMOTION;
	PID = _FID;
	UID = UserID;
	VI1 = _NewSceneId;
	VI2 = _UMoney;
	VI3 = _GMoney;
	VI4 = _RoomID;
	DOING_LOG();
}

void	DBLOG_PromotionRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_ID _NewSceneId)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_PROMOTIONROOM;
	SubType = DBLSUB_PROMOTION;
	PID = _FID;
	UID = UserID;
	VI1 = _NewSceneId;
	VI4 = _RoomID;
	DOING_LOG();
}

void	DBLOG_RenewRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days, T_PRICE _UMoney, T_PRICE _GMoney)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYROOM;
	SubType = DBLSUB_BUYROOM_RENEW;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _RoomType;
	VI1 = _Days;
	VI2 = _UMoney;
	VI3 = _GMoney;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_RenewRoomByCard(T_FAMILYID _FID, T_ROOMID _RoomID, T_MSTROOM_ID _RoomType, T_ROOMNAME _RoomName, uint32 _Days)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYROOM;
	SubType = DBLSUB_BUYROOM_RENEW_CARD;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _RoomType;
	VI1 = _Days;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_DelRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_DELROOM;
	PID = _FID;
	UID = UserID;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_UnDelRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_UNDELROOM;
	PID = _FID;
	UID = UserID;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_EnterRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_ENTERROOM;
	PID = _FID;
	UID = UserID;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_OutRoom(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_OUTROOM;
	PID = _FID;
	UID = UserID;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void    DBLOG_ChangeRoomExp(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_ROOMEXP oldExp, T_ROOMEXP newExp, T_ROOMLEVEL roomLevel,  uint8 RoomExpType, ucstring itemInfo)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_CHROOMEXP;
	SubType = RoomExpType;
	PID = _FID;
	UID = UserID;
	VI1 = oldExp;
	VI2 = newExp;
	VI3 = roomLevel;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName + L"[" + _RoomName + L"]";
	ItemName = itemInfo;
	DOING_LOG();
}


void	DBLOG_AddDead(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_DECEASED_POS _DeadPos)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_ADDDEAD;
	PID = _FID;
	UID = UserID;
	ItemID = _DeadPos;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_DelDead(T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_DECEASED_POS _DeadPos)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_DELDEAD;
	PID = _FID;
	UID = UserID;
	ItemID = _DeadPos;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_CHRoomMaster(T_FAMILYID _FID, T_FAMILYNAME _FName, ucstring _UName, T_ACCOUNTID _UID, T_FAMILYID _FID2, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_LOG_VARIABLES();
	FAMILY*	pFamily2 = GetFamilyData(_FID2);
	uint32 UserID2;
	ucstring	UserName2, FamilyName2;
	if (pFamily2 != NULL)
	{
		UserID2 = pFamily2->m_UserID;
		UserName2 = pFamily2->m_UserName;
		FamilyName2 = pFamily2->m_Name;
	}

	MainType = DBLOG_MAINTYPE_CHROOMMASTER;
	PID = _FID;
	UID = _UID;
	ItemID = _FID2;
	ItemTypeID = UserID2;
	VI4 = _RoomID;
	PName = L"[" + _UName + L"]" + _FName + L"[" + UserName2 + L"]" + FamilyName2;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_BuyItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYITEM;
	SubType = DBLSUB_BUYITEM_NEW;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI2 = _UMoney;
	VI3 = _GMoney;
	
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_GetMailItem(T_FAMILYID _FID, uint32 _MailID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_FAMILYID _fromFID, T_ACCOUNTID _fromUserId, uint32 _mainType, T_FAMILYNAME _fromFname )
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYITEM;
	SubType = DBLSUB_GETMAILITEM;
	PID = _FID;
	UID = UserID;
	ItemID = _MailID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI2 = _fromFID;
	VI3 = _fromUserId;
	VI4 = _mainType;
	PName = L"[" + UserName + L"]" + FamilyName + L"[" + _fromFname + L"]";
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_GetBankItem(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYITEM;
	SubType = DBLSUB_GETBANKITEM;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_ThrowItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_THROW;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_UseItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_USE;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_SendMailItem(T_FAMILYID _FID, uint32 _MailID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_FAMILYID _toFID, T_ACCOUNTID _toUserId, T_FAMILYNAME _toFname )
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_SENDMAIL;
	PID = _FID;
	UID = UserID;
	ItemID = _MailID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI2 = _toFID;
	VI3 = _toUserId;
	PName = L"[" + UserName + L"]" + FamilyName + L"[" + _toFname + L"]";
	ItemName = _Name;
	DOING_LOG();
}

//void	DBLOG_PutItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_USEITEM;
//	SubType = DBLSUB_USEITEM_PUT;
//	PID = _FID;
//	UID = UserID;
//	ItemID = _ItemID;
//	ItemTypeID = _TypeID;
//	VI1 = _Number;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

void	DBLOG_RefreshItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_REFRESH;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

//void	DBLOG_BurnItemInstant(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_BUYUSEITEM;
//	SubType = DBLSUB_BUYUSEITEM_BURN;
//	PID = _FID;
//	UID = UserID;
//	ItemTypeID = _TypeID;
//	VI1 = _Number;
//	VI2 = _UMoney;
//	VI3 = _GMoney;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

void	DBLOG_DressFirstUse(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, ucstring _Name)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_DRESS_FIRST_USE;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _Name;
	DOING_LOG();
}

void	DBLOG_ItemUsed(int UseSubtype, T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = UseSubtype;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}
/*
void	DBLOG_SimpleRiteItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_SIMPLERITE;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}

void	DBLOG_BurnItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_BURN;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}

void	DBLOG_NewFlowerItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_NEWFLOWER;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}

void	DBLOG_FishFoodItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_FISHFOOD;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}

void	DBLOG_MultiRiteItem(T_FAMILYID _FID, T_FITEMID _ItemID, T_ITEMSUBTYPEID _TypeID, uint32 _Number, ucstring _Name, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USEITEM;
	SubType = DBLSUB_USEITEM_MULTIRITE;
	PID = _FID;
	UID = UserID;
	ItemID = _ItemID;
	ItemTypeID = _TypeID;
	VI1 = _Number;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = L"[" + _RoomName + L"]" + _Name;
	DOING_LOG();
}
*/
//void	DBLOG_RoomPart(T_FAMILYID _FID, T_ROOMPARTID _PartID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_BUYUSEITEM;
//	SubType = DBLSUB_BUYUSEITEM_CHANGE;
//	PID = _FID;
//	UID = UserID;
//	ItemID = _PartID;
//	ItemTypeID = _TypeID;
//	VI2 = _UMoney;
//	VI3 = _GMoney;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

//void	DBLOG_Sacrifice(T_FAMILYID _FID, T_COMMON_ID _SacID, uint32 _SacNumber, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_BUYUSEITEM;
//	SubType = DBLSUB_BUYUSEITEM_SAC;
//	PID = _FID;
//	UID = UserID;
//	ItemTypeID = _TypeID;
//	VI1 = _SacNumber;
//	VI2 = _UMoney;
//	VI3 = _GMoney;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

//void	DBLOG_SetMusic(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_BUYUSEITEM;
//	SubType = DBLSUB_BUYUSEITEM_MUSIC;
//	PID = _FID;
//	UID = UserID;
//	ItemTypeID = _TypeID;
//	VI1 = 1;
//	VI2 = _UMoney;
//	VI3 = _GMoney;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

//void	DBLOG_AddPet(T_FAMILYID _FID, T_PETID _PetID, uint32 _PetNumber, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
//{
//	DECLARE_LOG_VARIABLES();
//	MainType = DBLOG_MAINTYPE_BUYUSEITEM;
//	SubType = DBLSUB_BUYUSEITEM_NEWPET;
//	PID = _FID;
//	UID = UserID;
//	ItemID = _PetID;
//	ItemTypeID = _TypeID;
//	VI1 = _PetNumber;
//	VI2 = _UMoney;
//	VI3 = _GMoney;
//	VI4 = _RoomID;
//	PName = L"[" + UserName + L"]" + FamilyName;
//	ItemName = L"[" + _RoomName + L"]" + _Name;
//	DOING_LOG();
//}

void	DBLOG_CarePet(T_FAMILYID _FID, T_ITEMSUBTYPEID _TypeID, ucstring _Name, T_PRICE _UMoney, T_PRICE _GMoney, T_ROOMNAME _RoomName, T_ROOMID _RoomID)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_USENOBODYITEM;
	SubType = DBLSUB_USENOBODYITEM_GIVEWATER;
	PID = _FID;
	UID = UserID;
	VI4 = _RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	ItemName = _RoomName;
	DOING_LOG();
}

void	DBLOG_BuySpace(T_FAMILYID _FID, T_COMMON_ID _SpaceID, uint32 _byte, T_PRICE _UMoney, T_PRICE _GMoney, uint32 room_id)
{
	DECLARE_LOG_VARIABLES();
	MainType = DBLOG_MAINTYPE_BUYSPACE;
	PID = _FID;
	UID = UserID;
	ItemTypeID = _SpaceID;
	VI1 = _byte;
	VI2 = _UMoney;
	VI3 = _GMoney;
	VI4 = room_id;
	PName = L"[" + UserName + L"]" + FamilyName;
	DOING_LOG();
}

#define		DECLARE_GMLOG_VARIABLES()											\
	uint32				MainType = NONE_NUMBER;								\
	uint32				PID = NONE_NUMBER;									\
	uint32				UID = NONE_NUMBER;									\
	uint32				PID2 = NONE_NUMBER;									\
	uint32				UID2 = NONE_NUMBER;									\
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER;	\
	ucstring			VU1 = NONE_NVARCHAR;								\
	ucstring			UName = NONE_NVARCHAR;								\
	ucstring			ObjectName = NONE_NVARCHAR;							\
	\
	uint32				UserID = NONE_NUMBER;								\
	ucstring			FamilyName = NONE_NVARCHAR;							\
	ucstring			UserName = NONE_NVARCHAR;							\
	FAMILY*				pFamily = GetFamilyData(_GMID);						\
	if (pFamily != NULL)													\
{																		\
	FamilyName = pFamily->m_Name; UserID = pFamily->m_UserID;	UserName = pFamily->m_UserName;			\
}

#define		DOING_GMLOG()												\
	CMessage			mesLog(M_CS_GMLOG);								\
	mesLog.serial(MainType, PID, UID, PID2, UID2);						\
	mesLog.serial(VI1, VI2, VI3, VU1, UName, ObjectName);				\
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );	\

void	GMLOG_Show(T_FAMILYID _GMID, bool _bShow)
{
	DECLARE_GMLOG_VARIABLES();
	
	MainType = GMLOG_MAINTYPE_SHOW;
	PID = _GMID;
	UID	= UserID;
	VI1 = _bShow;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}

void	GMLOG_PullFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_GMLOG_VARIABLES();

	FAMILY*	pFamily2 = GetFamilyData(_FID);
	uint32		UserID2;
	ucstring	UserName2, FamilyName2;
	if (pFamily2 != NULL)
	{
		UserID2 = pFamily2->m_UserID;
		UserName2 = pFamily2->m_UserName;
		FamilyName2 = pFamily2->m_Name;
	}

	MainType = GMLOG_MAINTYPE_PULLFAMILY;
	PID = _GMID;
	UID	= UserID;
	PID2 = _FID;
	UID2 = UserID2;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;
	ObjectName = L"[" + UserName2 + L"]" + FamilyName2;

	DOING_GMLOG();

}

void	GMLOG_OutFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_GMLOG_VARIABLES();

	FAMILY*	pFamily2 = GetFamilyData(_FID);
	uint32		UserID2;
	ucstring	UserName2, FamilyName2;
	if (pFamily2 != NULL)
	{
		UserID2 = pFamily2->m_UserID;
		UserName2 = pFamily2->m_UserName;
		FamilyName2 = pFamily2->m_Name;
	}

	MainType = GMLOG_MAINTYPE_OUTFAMILY;
	PID = _GMID;
	UID	= UserID;
	PID2 = _FID;
	UID2 = UserID2;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;
	ObjectName = L"[" + UserName2 + L"]" + FamilyName2;

	DOING_GMLOG();

}

void	GMLOG_SetLevel(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_F_LEVEL _Level)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_SETLEVEL;
	PID = _GMID;
	UID	= UserID;
	PID2 = _FID2;
	UID2 = _UID2;
	VI1 = _Level;
	UName	= L"[" + UserName + L"]" + FamilyName;
	ObjectName = L"[" + _UName2 + L"]" + _FName2;

	DOING_GMLOG();
}

void	GMLOG_GiveItem(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_ITEMSUBTYPEID _ItemID, uint32 _ItemNumber, ucstring _ItemName)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_GIVEITEM;
	PID = _GMID;
	UID	= UserID;
	PID2 = _FID2;
	UID2 = _UID2;
	VI1 = _ItemID;
	VI2 = _ItemNumber;
	VU1 = _ItemName;
	UName	= L"[" + UserName + L"]" + FamilyName;
	ObjectName = L"[" + _UName2 + L"]" + _FName2;

	DOING_GMLOG();
}

void	GMLOG_GiveMoney(T_FAMILYID _GMID, T_FAMILYID _FID2, uint32 _UID2, ucstring _FName2, ucstring _UName2, T_PRICE _UMoney, T_PRICE _GMoney)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_GIVEMONEY;
	PID = _GMID;
	UID	= UserID;
	PID2 = _FID2;
	UID2 = _UID2;
	VI1 = _UMoney;
	VI2 = _GMoney;
	UName	= L"[" + UserName + L"]" + FamilyName;
	ObjectName = L"[" + _UName2 + L"]" + _FName2;

	DOING_GMLOG();
}

void	GMLOG_SetExpRoom(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_SETEXPROOM;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}

void	GMLOG_UnsetExpRoom(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_UNSETEXPROOM;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}


void	GMLOG_PropExpRoomLevel(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_LEVEL _oldLevel, T_F_LEVEL _Level)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_EXPROOM_LEVEL;
	VI1 = _oldLevel;
	VI2 = _Level;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}

void	GMLOG_PropExpRoomExp(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_EXP _oldExp, T_F_EXP _Exp)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_EXPROOM_EXP;
	VI1 = _oldExp;
	VI2 = _Exp;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}

void	GMLOG_PropExpRoomVisit(T_FAMILYID _GMID, T_ROOMID _RoomID, T_ROOMNAME _RoomName, T_F_EXP _oldVisit, T_F_EXP _Visit)
{
	DECLARE_GMLOG_VARIABLES();

	MainType = GMLOG_MAINTYPE_EXPROOM_VISITS;
	VI1 = _oldVisit;
	VI2 = _Visit;
	VI3 = _RoomID;
	VU1 = _RoomName;
	UName	= L"[" + UserName + L"]" + FamilyName;

	DOING_GMLOG();
}


