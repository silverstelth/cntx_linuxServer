
#include <misc/types_sip.h>
#include "misc/thread.h"
#include <misc/md5.h>
#include <misc/SipHttpClient.h>
#include "misc/db_interface.h"
#include <net/service.h>

#include <assert.h> 

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/err.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/Lang.h"
// #include <rapidjson/document.h>

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

DBCONNECTION	DB;

static  const   string  s_sHSPortConfig    = "HSPort";
static  const   uint16  s_wDefaultHSPort   = 15000;

// Max number of downloading at a time (Default value is 100)
CVariable<uint32> MaxDownloadNum("hisManager","MaxDownloadNum", "Downloadable max number", 100);

#define		SM_CLIENTPORT_CONFIGNAME	"SMClientPort"		// Config name for stream client's port
#define		SM_DEFAULTCLIENTPORT		9000				// Default port for stream client
CVariable<uint16>	SMSStreamPort("SMS", SM_CLIENTPORT_CONFIGNAME, "TCP Port of media streaming for Clients", SM_DEFAULTCLIENTPORT, 0, true);

void sendDataList(T_FAMILYID FID, TServiceId _tsid);
void eraseFromUploadDownloadDeleteList(T_FAMILYID _fid);

static	tchar	s_szQuery[1024];
static	wchar_t	s_szDescription[MAX_DATA_DESCRIPTION_VIDEO];
static  wchar_t md5_buf[33];

#pragma pack(push, 1)
struct _PhotoAlbumInfo
{
	uint32		AlbumID;
	uint32		LogoPhotoID;
	ucchar		AlbumName[MAX_LEN_ALBUM_NAME + 1];

#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return true; }
	static uint32 getSize()
	{
		return sizeof(uint32) * 2 + sizeof(char16_t) * (MAX_LEN_ALBUM_NAME + 1);
	}

	void serialize(uint8 *buf)
	{
		uint32 pos = 0;
		memcpy(&AlbumID, buf + pos, sizeof(uint32));
		pos += sizeof(uint32);

		memcpy(&LogoPhotoID, buf + pos, sizeof(uint32));
		pos += sizeof(uint32);
		
		char16_t character;
		for (int i = 0; i < MAX_LEN_ALBUM_NAME + 1; i++)
		{
			memcpy(&character, buf + pos, sizeof(char16_t));
			pos += sizeof(char16_t);

			AlbumName[i] = (ucchar)character;
		}
	}
#endif
};
struct _VideoAlbumInfo
{
	uint32		AlbumID;
	ucchar		AlbumName[MAX_LEN_ALBUM_NAME + 1];

#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return true; }
	static uint32 getSize()
	{
		return sizeof(uint32) + sizeof(char16_t) * (MAX_LEN_ALBUM_NAME + 1);
	}

	void serialize(uint8 *buf)
	{
		uint32 pos = 0;
		memcpy(&AlbumID, buf + pos, sizeof(uint32));
		pos += sizeof(uint32);
		
		char16_t character;
		for (int i = 0; i < MAX_LEN_ALBUM_NAME + 1; i++)
		{
			memcpy(&character, buf + pos, sizeof(char16_t));
			pos += sizeof(char16_t);

			AlbumName[i] = (ucchar)character;
		}
	}
#endif
};
#pragma pack(pop)

typedef SDATA_LIST<_PhotoAlbumInfo, MAX_ALBUM_COUNT + 1>	PhotoAlbumList;
typedef SDATA_LIST<_VideoAlbumInfo, MAX_ALBUM_COUNT + 2>	VideoAlbumList;

static	uint8	s_GroupBuffer[MAX_ALBUM_BUF_SIZE];

void sendErrorResult(const MsgNameType &_name, uint32 _fid, uint32 _flag, TServiceId _tsid)
{
	CMessage msgout(_name);
	msgout.serial(_fid, _flag);
	CUnifiedNetwork::getInstance()->send( _tsid, msgout);
}

uint32 getMinutesSince1970(std::string& date)
{
	MSSQLTIME	t;
	if ( ! t.SetTime(date) )
		return 0;
	uint32	result = (uint32)(t.timevalue / 60);
	return result;
}

bool IsCommonRoom(T_ROOMID RoomID)
{
	if (RoomID >= MIN_RELIGIONROOMID && RoomID < MAX_RELIGIONROOMID)
		return true;
	return false;
}

uint8	ShardMainCode[25] = {0};

// Send Shard Code from WS to Lobby [[b:ShardMainCode]]
void cb_M_W_SHARDCODE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	int count = 0;
	while (true)
	{
		_msgin.serial(ShardMainCode[count]);
		if (ShardMainCode[count] == 0)
			break;
		count ++;
	}

	//sipdebug("Get Main count=%d, ShardCode[0] = %d", count, ShardMainCode[0]); byy krs
}

bool IsThisShardRoom(uint32 RoomID)
{
	if (RoomID < 0x01000000)
		return true;

	uint8	ShardCode = (uint8)(RoomID >> 24);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

bool IsThisShardFID(uint32 FID)
{
	uint8	ShardCode = (uint8)((FID >> 24) + 1);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

typedef std::map<uint32, bool> GMMAP_T;
GMMAP_T gm_map;
bool IsGM(uint32 FID) {
	return gm_map.find(FID) != gm_map.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// History data
struct SHISDATA
{
	SHISDATA(uint8 _type, uint32 _pos)
		:m_byHisType(_type), m_uHisPos(_pos)	{}
	uint8				m_byHisType;				// History data's type
	uint32				m_uHisPos;					// History data's position
};

// Property of Downloading
struct SDOWNLOADING
{
	SDOWNLOADING(SHISDATA _his)
		:m_HisData(_his)
	{
		m_tmStartTime = CTime::getLocalTime();
	}
	SHISDATA			m_HisData;					// Downloading data
	TTime				m_tmStartTime;				// Start time
};
typedef	map<T_FAMILYID, SDOWNLOADING>	MAP_DOWNLOADING;

// Property of Waiting for download
struct SDOWNLOADWAIT
{
	SDOWNLOADWAIT(T_FAMILYID _FID)
		:m_FID(_FID), m_uWaitCount(0)
	{
		m_tmLastReqTime = CTime::getLocalTime();
	}
	T_FAMILYID			m_FID;					
	uint32				m_uWaitCount;
	TTime				m_tmLastReqTime;
};
typedef	list<SDOWNLOADWAIT>	LST_DOWNLOADWAIT;

// Property of posted key
struct SPOSTKEYDATA
{
	SPOSTKEYDATA(uint32 _uKey)
		: m_uKey(_uKey)
	{
		m_tmPostTime = CTime::getLocalTime();
	}
	uint32				m_uKey;				// Key value
	TTime				m_tmPostTime;		// post time
};
typedef	map<T_FAMILYID, SPOSTKEYDATA>	MAP_POSTKEYDATA;

map<uint32, uint32>	g_mapCurManagerList;

// Information of family entered in room
struct SINROOMFAMILYS 
{
	SINROOMFAMILYS(T_ROOMID _RoomID, T_ROOMROLE _RoleInRoom, TTime _InTime, uint8 _IsMobile)
		: m_RoomID(_RoomID), m_RoleInRoom(_RoleInRoom), m_tmInTime(_InTime), m_bDataManaging(false), m_bIsMobile(_IsMobile) {}
	
	T_ROOMID			m_RoomID;				// Room id
	//uint32				m_uRoomKey;				// Key value for Confirming
	T_ROOMROLE			m_RoleInRoom;			// Role in room
	TTime				m_tmInTime;				// Entered time
	list<SHISDATA>		m_lstDownloadedHis;		// Data downloaded already
	uint8				m_bIsMobile;			// by krs

	bool				m_bDataManaging;

	bool	IsDownloaded(const SHISDATA& _his)
	{
		list<SHISDATA>::iterator it;
		for(it = m_lstDownloadedHis.begin(); it != m_lstDownloadedHis.end(); it++)
		{
			if ((*it).m_byHisType == _his.m_byHisType && (*it).m_uHisPos == _his.m_uHisPos)
				return true;
		}
		return false;
	}

	void	AddDownloadedHis(const SHISDATA & _his)
	{
		SHISDATA	newHis(_his.m_byHisType, _his.m_uHisPos);
		m_lstDownloadedHis.push_back(newHis);
	}
};
typedef	map<T_FAMILYID, SINROOMFAMILYS>	MAP_INROOMFAMILYS;

// ----------------------------------------------------------------------------
static MAP_INROOMFAMILYS	s_mapInRoomFamilys;

// ----------------------------------------------------------------------------
#define	IFNOTFIND_INROOMFAMILYS(it, FID)										\
	MAP_INROOMFAMILYS::iterator it = s_mapInRoomFamilys.find(FID);				\
	if (it == s_mapInRoomFamilys.end())											\

// Delete family entered in room
static void DeleteInRoomFamilys(T_FAMILYID _FID)
{
	s_mapInRoomFamilys.erase(_FID);
}

bool IsHisManager(T_FAMILYID FID, uint32 RoomID)
{
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
	}
	else
	{
		if (itFamily->second.m_RoomID == RoomID)
		{
			if (itFamily->second.m_RoleInRoom == ROOM_MANAGER)
				return true;
			else
				return false;
		}
	}

	MAKEQUERY_IsRoomManager(s_szQuery, FID, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if(!nPrepareRet)
	{
		sipwarning("DB_EXE_PREPARESTMT failed in MAKEQUERY_IsRoomManager.");
		return false;
	}

	SQLLEN len1 = 0;	
	sint32	ret = 0;

	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

	if(!nBindResult)
	{
		sipwarning("DB_EXE_BINDPARAM_OUT failed in MAKEQUERY_IsRoomManager.");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if(!nQueryResult)
	{
		sipwarning("DB_EXE_PARAMQUERY failed in MAKEQUERY_IsRoomManager.");
		return false;
	}

	if(ret == 0)
		return true;

	return false;
}

void HisManageStop(uint32 FID)
{
	uint32		RoomID;

	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{		
		return;
	}

	itFamily->second.m_bDataManaging = false;

	// Get RoomId
	RoomID = itFamily->second.m_RoomID;

	g_mapCurManagerList.erase(RoomID);

	//sipdebug("HisManageStop FID=%d", FID); byy krs
}

void RemoveFamily(T_FAMILYID _FID)
{
	HisManageStop(_FID);

	DeleteInRoomFamilys(_FID);
	eraseFromUploadDownloadDeleteList(_FID);	
}

static bool SavePhotoAlbum(uint32 RoomID, PhotoAlbumList *pAlbumList)
{
	uint32	GroupBufferSize = sizeof(s_GroupBuffer);
	if(!pAlbumList->toBuffer(s_GroupBuffer, &GroupBufferSize))
	{
		sipwarning("pAlbumList->toBuffer failed.");
		return false;
	}

	MAKEQUERY_SavePhotoAlbumList(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_insert: prepare statement failed!!");
		return false;
	}

	SQLLEN	num1 = GroupBufferSize, num2 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_ALBUM_BUF_SIZE, 0, s_GroupBuffer, 0, &num1);
	DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num2);
	if ( ! nBindResult )
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_insert: bind parameter failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if ( ! nQueryResult || ret == -1)
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_insert: execute failed!");
		return false;
	}

	return true;
}

static bool ReadPhotoAlbum(uint32 RoomID, PhotoAlbumList *pAlbumList, bool bAddDefault = false)
{
	MAKEQUERY_LoadPhotoAlbumList(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_List: prepare statement failed!!");
		return false;
	}

	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);
	if ( ! nBindResult )
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_List: bind parameter failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if ( ! nQueryResult || ret == -1)
	{
		sipwarning("Shrd_PhotoAlbumInfo_Page_List: execute failed!");
		return false;
	}

	SQLLEN			lGroupBufSize = sizeof(s_GroupBuffer);

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if ( ! IS_DB_ROW_VALID(row) )
	{
		if(bAddDefault)
		{
			pAlbumList->DataCount = 1;
			pAlbumList->Datas[0].AlbumID = 1;
			wcscpy(pAlbumList->Datas[0].AlbumName, GetLangText(ID_PHOTO_DEFAULTGROUP_NAME).c_str()); // L"\x9ED8\x8BA4\x76F8\x518C"); // L"默认相册"); // 
			pAlbumList->Datas[0].LogoPhotoID = -1;

			if(!SavePhotoAlbum(RoomID, pAlbumList))
			{
				sipwarning("SavePhotoAlbum(default) failed.");
				return false;
			}
		}
		else
		{
			sipwarning("AlbumGroup Empty!!!");
			return false;
		}
	}
	else
	{
		row.GetData(1, s_GroupBuffer, static_cast<ULONG>(lGroupBufSize), &lGroupBufSize, SQL_C_BINARY);
		if(!pAlbumList->fromBuffer(s_GroupBuffer, (uint32)lGroupBufSize))
		{
			sipwarning("AlbumList.fromBuffer failed.");
			return false;
		}
	}

	return true;
}

static bool SaveVideoAlbum(uint32 RoomID, VideoAlbumList *pAlbumList)
{
	uint32	GroupBufferSize = sizeof(s_GroupBuffer);
	if(!pAlbumList->toBuffer(s_GroupBuffer, &GroupBufferSize))
	{
		sipwarning("pAlbumList->toBuffer failed.");
		return false;
	}

	MAKEQUERY_SaveVideoAlbumList(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_insert: prepare statement failed!!");
		return false;
	}

	SQLLEN	num1 = GroupBufferSize, num2 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_ALBUM_BUF_SIZE, 0, s_GroupBuffer, 0, &num1);
	DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num2);
	if ( ! nBindResult )
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_insert: bind parameter failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if ( ! nQueryResult || ret == -1)
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_insert: execute failed!");
		return false;
	}

	return true;
}

static bool ReadVideoAlbum(uint32 RoomID, VideoAlbumList *pAlbumList, bool bAddDefault = false)
{
	MAKEQUERY_LoadVideoAlbumList(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_List: prepare statement failed!!");
		return false;
	}

	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);
	if ( ! nBindResult )
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_List: bind parameter failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if ( ! nQueryResult || ret == -1)
	{
		sipwarning("Shrd_VideoAlbumInfo_Page_List: execute failed!");
		return false;
	}

	SQLLEN			lGroupBufSize = sizeof(s_GroupBuffer);

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if ( ! IS_DB_ROW_VALID(row) )
	{
		if(bAddDefault)
		{
			pAlbumList->DataCount = 2;
			pAlbumList->Datas[0].AlbumID = 1;
			wcscpy(pAlbumList->Datas[0].AlbumName, GetLangText(ID_VIDEO_PLAYGROUP_NAME).c_str()); // L"\x5929\x56ED\x89C6\x9891"); // L"天园视频"); // 
			pAlbumList->Datas[1].AlbumID = 2;
			wcscpy(pAlbumList->Datas[1].AlbumName, GetLangText(ID_VIDEO_DEFAULTGROUP_NAME).c_str()); // L"\x89C6\x9891"); // L"视频"); // 

			if(!SaveVideoAlbum(RoomID, pAlbumList))
			{
				sipwarning("SaveVideoAlbum(default) failed.");
				return false;
			}
		}
		else
		{
			sipwarning("AlbumGroup Empty!!!");
			return false;
		}
	}
	else
	{
		row.GetData(1, s_GroupBuffer, static_cast<ULONG>(lGroupBufSize), &lGroupBufSize, SQL_C_BINARY);
		if(!pAlbumList->fromBuffer(s_GroupBuffer, (uint32)lGroupBufSize))
		{
			sipwarning("AlbumList.fromBuffer failed.");
			return false;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////
// [11/9/2009 NamJuSong]
struct UploadConnect {
	T_FAMILYID	familyid;
	T_ROOMID	roomid;
	T_DATATYPE	datatype;
	uint8		DeadID;
	uint32		filesize;	
	ucstring	datacomment;
	T_ALBUMID	albumid;
	TServiceId	tsid;
	uint32		OldDataID;
	TTime		req_time;
	TTime		start_time;

	UploadConnect()	{}
	UploadConnect(T_FAMILYID _fid, T_ROOMID _rid, T_DATATYPE _dtype, uint8 _DeadID, uint32 _fsize, ucstring _comment, T_ALBUMID _albumId, TServiceId _tsid, uint32 _OldDataID) :
		familyid(_fid), roomid(_rid), datatype(_dtype), DeadID(_DeadID), filesize(_fsize), datacomment(_comment), albumid(_albumId), tsid(_tsid), OldDataID(_OldDataID)
	{
		req_time = CTime::getLocalTime();
		start_time = 0;
	}
};

struct DownloadConnect {
	T_FAMILYID	familyid;
	T_ROOMID	roomid;
	T_DATATYPE	datatype;
	uint32		fileid;
	TTime		req_time;
	TTime		start_time;

	DownloadConnect() {}
	DownloadConnect(T_FAMILYID _fid, T_ROOMID _rid, T_DATATYPE _dtype, uint32 _fileid) :
		familyid(_fid), roomid(_rid), datatype(_dtype), fileid(_fileid)
	{
		req_time = CTime::getLocalTime();
		start_time = 0;
	}
};

struct StreamConnect {
	T_FAMILYID	familyid;
	T_ROOMID	roomid;
	T_DATATYPE	datatype;
	uint32		fileid;
	TTime		req_time;
	TTime		start_time;

	StreamConnect() {}
	StreamConnect(T_FAMILYID _fid, T_ROOMID _rid, T_DATATYPE _dtype, uint32 _fileid) :
		familyid(_fid), roomid(_rid), datatype(_dtype), fileid(_fileid)
	{
		req_time = CTime::getLocalTime();
		start_time = 0;
	}
};

struct DeleteConnect {
	T_FAMILYID	familyid;
	T_ROOMID	roomid;
	T_DATATYPE	datatype;
	uint32		fileid;
	TServiceId	sid;
	TTime		req_time;

	DeleteConnect() {}
	DeleteConnect(T_FAMILYID _fid, T_ROOMID _rid, T_DATATYPE _dtype, uint32 _fileid, TServiceId _sid) :
		familyid(_fid), roomid(_rid), datatype(_dtype), fileid(_fileid), sid(_sid)
	{
		req_time = CTime::getLocalTime();
	}
};

struct PhotoInfo{
	ucstring    comment;
	uint32      crc32;
	uint32      fileSize;
	uint32      nCreateTime;
	uint32		masterFID;
	uint8		checked;
	TTime		generate_time;
	uint8		webid;	

	PhotoInfo(ucstring _comment, uint32 _crc32, uint32 _fileSize,	uint32 _nCreateTime, uint32 _masterFID, uint8 _checked, uint8 _webid):
		comment(_comment), crc32(_crc32), fileSize(_fileSize), nCreateTime(_nCreateTime), masterFID(_masterFID), checked(_checked),
		webid(_webid)
		{
			generate_time = CTime::getLocalTime();
		}

	PhotoInfo() {}
};

struct VideoInfo{
	ucstring    comment;	
	uint32      crc32;
	uint32      fileSize;
	uint32      nCreateTime;	
	uint32		index;
	uint32		masterFID;
	uint8		checked;
	TTime		generate_time;
	
	VideoInfo(ucstring _comment, uint32 _crc32, uint32 _fileSize, uint32 _nCreateTime, uint32 _index, uint32 _masterFID, uint8 _checked):
		comment(_comment), crc32(_crc32), fileSize(_fileSize), nCreateTime(_nCreateTime), index(_index), masterFID(_masterFID), checked(_checked)		
		{
			generate_time = CTime::getLocalTime();
		}

	VideoInfo() {}
};

typedef	std::map<T_AUTHKEY, UploadConnect>		TUploadConnects;
typedef	std::map<T_AUTHKEY, DownloadConnect>	TDownloadConnects;
typedef	std::map<T_AUTHKEY, DeleteConnect>		TDeleteConnects;
typedef	std::map<T_AUTHKEY, StreamConnect>		TStreamConnects;
typedef std::map<uint64, PhotoInfo> TPhotoInfos;
typedef std::map<uint64, VideoInfo> TVideoInfos;
inline uint64 MakeUINT64(uint32 a, uint32 b)
{
	return ((uint64)a << 32) | (uint64)b;
}

class OnlineDataServer : public SIPBASE::CRefCount
{
public:
	T_DATASERVERID			m_ServerId;
	bool					m_bPortrait, m_bPhoto, m_bVideo;

	SIPNET::CInetAddress	m_Addr;

	TUploadConnects			m_UploadConnections;
	TDownloadConnects		m_DownloadConnections;
	TDeleteConnects			m_DeleteConnections;
	TStreamConnects			m_StreamConnections;

	int		getConnectCount()
	{
		int count = static_cast<int>(m_UploadConnections.size() + m_DownloadConnections.size() + m_DeleteConnections.size() + m_StreamConnections.size());
		return count;
	}

	OnlineDataServer() {}
	OnlineDataServer(T_DATASERVERID _id, std::string _url, uint16 _port, bool _portrait, bool _photo, bool _video) :
		m_ServerId(_id), m_bPortrait(_portrait), m_bPhoto(_photo), m_bVideo(_video)
	{
		m_Addr.setByName(_url);
		m_Addr.setPort(_port);
	}

	~OnlineDataServer()
	{
	}
};

typedef CSmartPtr<OnlineDataServer>					TOnlineDataServer;
typedef	std::map<T_DATASERVERID, TOnlineDataServer> TOnlineDataServers;
typedef	std::map<std::string, T_DATASERVERID>		TDataServerAddrs;

TOnlineDataServers		s_OnlineDataServers;
TDataServerAddrs		s_DataServerAddrs;

TPhotoInfos  s_photoInfoInAlbum;
TVideoInfos  s_videoInfoInAlbum;

static	CCallbackServer *	s_pDataServer = NULL;

#define	RECONNECT_INUERVAL		300		//second

void eraseFromUploadDownloadDeleteList(T_FAMILYID _fid)
{
	TOnlineDataServers::iterator itserver;
	for ( itserver = s_OnlineDataServers.begin(); itserver != s_OnlineDataServers.end(); itserver ++ )
	{
		TOnlineDataServer s = itserver->second;
		{
			TUploadConnects::iterator it = s->m_UploadConnections.begin();
			while (it != s->m_UploadConnections.end())
			{
				if (it->second.familyid == _fid)
				{
					TUploadConnects::iterator itt = it++;
					s->m_UploadConnections.erase(itt);
					continue;
				}
				it++;
			}
		}
	}
}

TTime LastRefreshCheckTime = 0;				// Last refresh time
const TTime	RefreshCycle = 10000;			// Refreshing cycle
void CheckUploadDownloadDeleteList()
{
	TTime	curTime = CTime::getLocalTime();

	if (curTime - LastRefreshCheckTime < RefreshCycle)
		return;
	LastRefreshCheckTime = curTime;

	TOnlineDataServers::iterator itserver;
	for ( itserver = s_OnlineDataServers.begin(); itserver != s_OnlineDataServers.end(); itserver ++ )
	{
		TOnlineDataServer	s = itserver->second;

		{
			TUploadConnects::iterator it = s->m_UploadConnections.begin();
			while (it != s->m_UploadConnections.end())
			{
				if (( (it->second.req_time > 0) && (curTime - it->second.req_time > HIS_UPAUTHKEY_WAIT_TIME) )	// 30m
					|| ( (it->second.start_time > 0) && (curTime - it->second.start_time > HIS_UPAUTHKEY_WAIT_TIME) ))	// 30m
				{
					//sipdebug("UPLOAD AuthKey Deleted. auth_key=%d", it->first); byy krs
					TUploadConnects::iterator itt = it++;
					s->m_UploadConnections.erase(itt);
					continue;
				}
				it++;
			}
		}
		{
			TDownloadConnects::iterator it = s->m_DownloadConnections.begin();
			while (it != s->m_DownloadConnections.end())
			{
				if (((it->second.req_time > 0) && (curTime - it->second.req_time > HIS_AUTHKEY_WAIT_TIME))	// 30s
					|| ( (it->second.start_time > 0) && (curTime - it->second.start_time > HIS_AUTHKEY_WAIT_TIME) ))	// 30s
				{
					//sipdebug("DOWNLOAD AuthKey Deleted. auth_key=%d", it->first); byy krs
					TDownloadConnects::iterator itt = it++;
					s->m_DownloadConnections.erase(itt);
					continue;
				}
				it++;
			}
		}
		{
			TDeleteConnects::iterator it = s->m_DeleteConnections.begin();
			while (it != s->m_DeleteConnections.end())
			{
				if ((it->second.req_time > 0) && (curTime - it->second.req_time > HIS_AUTHKEY_WAIT_TIME))	// 30s
				{
					//sipdebug("DELETE AuthKey Deleted. auth_key=%d", it->first); byy krs
					TDeleteConnects::iterator itt = it++;
					s->m_DeleteConnections.erase(itt);
					continue;
				}
				it++;
			}
		}
	}
}

static T_DATASERVERID getDataServerId(TTcpSockId from, CCallbackNetBase& clientcb)
{
	const CInetAddress &	addr = clientcb.hostAddress(from);
	std::string	ipaddr = addr.ipAddress();
	TDataServerAddrs::iterator it = s_DataServerAddrs.find(ipaddr);
	if ( it == s_DataServerAddrs.end() )
	{
		sipwarning("Received from no registered data server %s", ipaddr.c_str());
		return INVALID_DATASERVERID;
	}
	return it->second;
}

static bool sendPhotoFrameList(T_FAMILYID FID, TServiceId _tsid)
{
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker
		sipwarning("There is no room info of family(%d)", FID);
		return false;
	}

	// Get RoomId
	uint32	RoomID = itFamily->second.m_RoomID;

	// get the photo frame Info for one room
	MAKEQUERY_LoadPhotoFrame(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_Room_PhotoFrame_List: prepare statement failed!!");
		return false;
	}	

	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);
	if(!nBindResult)
	{
		sipwarning("Shrd_Room_PhotoFrame_List: bind parameter failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if(!nQueryResult || ret == -1)
	{
		sipwarning("Shrd_Room_PhotoFrame_List: execute failed!");
		return false;
	}

	CMessage msgout_frm(M_SC_FRAMELIST);
	msgout_frm.serial(FID, RoomID);	

	int	count = 0;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if(!IS_DB_ROW_VALID(row))
			break;

		COLUMN_DIGIT(row,	0,	uint32,	frameid);
		COLUMN_DIGIT(row,	1, 	uint32, fileid);
		COLUMN_DIGIT(row,   2,  uint32, filesize);
		COLUMN_DIGIT(row,	3,	uint32, crc32);
		COLUMN_WSTR_BUF(row, 4,	comment, s_szDescription, MAX_DATA_DESCRIPTION_VIDEO);
		COLUMN_DATETIME(row, 5, creationtime);
		COLUMN_DIGIT(row,	6,	uint16,	OffsetX);
		COLUMN_DIGIT(row,	7,	uint16,	OffsetY);
		COLUMN_DIGIT(row,	8,	uint32,	Scale);
		COLUMN_DIGIT(row,	9,	uint8, checked);
		COLUMN_DIGIT(row,   10,  uint32, masterFID);
		COLUMN_DIGIT(row,	11,	uint8, webid);		

		uint32 nCreateTime = getMinutesSince1970(creationtime);

		PhotoInfo temp(comment, crc32, filesize, nCreateTime, masterFID, checked, webid);
		s_photoInfoInAlbum[MakeUINT64(RoomID, fileid)] = temp;

		if((checked == HISDATA_CHECKFLAG_CHECKED) 
			|| (itFamily->second.m_RoleInRoom == ROOM_MASTER) 
			|| (itFamily->second.m_RoleInRoom == ROOM_MANAGER) 
			|| IsCommonRoom(RoomID))
		{
			msgout_frm.serial(frameid, fileid, OffsetX, OffsetY, Scale);
			count ++;
		}
	} while ( true );

	CUnifiedNetwork::getInstance()->send(_tsid, msgout_frm);

	return true;
}

static bool sendFigureFrameList(T_FAMILYID FID, TServiceId _tsid)
{
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker
		sipwarning("There is no room info of family(%d)", FID);
		return false;
	}

	// Get RoomId
	uint32	RoomID = itFamily->second.m_RoomID;

	// get the photo frame Info for one room
	MAKEQUERY_LoadFigureFrame(s_szQuery, RoomID);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_Room_GetFigureFrame: prepare statement failed!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if(!nQueryResult)
	{
		sipwarning("Shrd_Room_GetFigureFrame: execute failed!");
		return false;
	}

	CMessage msgout_frm(M_SC_FIGUREFRAME_LIST);
	msgout_frm.serial(FID);

	int	count = 0;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if(!IS_DB_ROW_VALID(row))
			break;

		COLUMN_DIGIT(row,	0,	uint32,	roomid);
		COLUMN_DIGIT(row,	1, 	uint32, frameid);
		COLUMN_DIGIT(row,   2,  uint8, figuretype);
		COLUMN_DIGIT(row,	3,	uint32, photoid1);
		COLUMN_DIGIT(row,   4,  uint8, checked1);
		COLUMN_DIGIT(row,	5,	uint32, photoid2);
		COLUMN_DIGIT(row,   6,  uint8, checked2);

		if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
		{
			if(checked1 != HISDATA_CHECKFLAG_CHECKED)
				photoid1 = 0;
			if(checked2 != HISDATA_CHECKFLAG_CHECKED)
				photoid2 = 0;
		}
		msgout_frm.serial(frameid, figuretype, photoid1, checked1, photoid2, checked2);
		count ++;
	} while ( true );

	CUnifiedNetwork::getInstance()->send(_tsid, msgout_frm);

	return true;
}

// Send Video Group List [ [d:AlbumId][u:Name] ]
static bool sendVideoGrpList(T_FAMILYID FID, T_ROOMID RoomID, TServiceId _tsid)
{
	VideoAlbumList	AlbumList;
	if(!ReadVideoAlbum(RoomID, &AlbumList, true))
	{
		sipwarning("ReadVideoAlbum failed!!");
		return false;
	}

	CMessage msgout(M_SC_VIDEOALBUMLIST);
	msgout.serial(FID);
	for(uint32 i = 0; i < AlbumList.DataCount; i ++)
	{
		ucstring	AlbumName = AlbumList.Datas[i].AlbumName;
		msgout.serial(AlbumList.Datas[i].AlbumID, AlbumName);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	return true;
}

// Send Photo Album List [ [d:AlbumId][u:Name][d:LogoDataId] ]
static bool sendPhotoAlbumList(T_FAMILYID FID, T_ROOMID RoomID, TServiceId _tsid)
{
	PhotoAlbumList	AlbumList;
	if (!ReadPhotoAlbum(RoomID, &AlbumList, true))
	{
		sipwarning("ReadPhotoAlbum failed!!");
		return false;
	}

	CMessage msgout(M_SC_PHOTOALBUMLIST);
	msgout.serial(FID);
	for (uint32 i = 0; i < AlbumList.DataCount; i ++)
	{
		ucstring	AlbumName = AlbumList.Datas[i].AlbumName;
		msgout.serial(AlbumList.Datas[i].AlbumID, AlbumName, AlbumList.Datas[i].LogoPhotoID);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	return true;
}

void sendDataList(T_FAMILYID FID, TServiceId _tsid)
{
	// Try to find family information from family ID	
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	if(!sendPhotoFrameList(FID, _tsid))
		return;
	if(!sendFigureFrameList(FID, _tsid))
		return;
}

T_AUTHKEY MakeAuthKey(uint16 _upload_key)
{
	uint32	upload_key = _upload_key;
	uint32	rand_v = Irand(0, 0x10000);
	uint32	rand_v1 = (rand_v & 0xFF00) << 16;
	uint32	rand_v2 = rand_v & 0xFF;
	return rand_v1 | (upload_key << 8) | rand_v2;
}

typedef	map<T_FAMILYID, std::string> family_fileauth_t;
static family_fileauth_t s_mapFamilyFileAuth;
const std::string GenToken(uint32 family_id, uint32 room_id)
{
	static uint32 last_t = 0;
	uint32 t = CTime().getSecondsSince1970();
	if (last_t >= t) {
		t = last_t + 1;
	}
	uint64 id = (uint64)t << 32 | family_id;
	char buf[12] = {0};
	memcpy(buf, &id, sizeof(id));
	memcpy(&buf[8], &room_id, sizeof(room_id));
	SIPBASE::CHashKeyMD5 hs = SIPBASE::getMD5((const uint8 *)buf, sizeof(buf));
	return hs.toString();
}
std::string GetToken(uint32 family_id)
{
	family_fileauth_t::iterator ite = s_mapFamilyFileAuth.find(family_id);
	if (ite != s_mapFamilyFileAuth.end())
	{
		return ite->second;
	}
	return "";
}

// ----------------------------------------------------------------------------
// Packet: M_NT_ADDTOHIS
void cb_EnterRoom(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);
	T_ROOMROLE			RoleInRoom;			_msgin.serial(RoleInRoom);
	uint16				fesid;				_msgin.serial(fesid);
	uint8				bIsMobile;			_msgin.serial(bIsMobile);

	TTime InTime = CTime::getLocalTime();
	RemoveFamily(FID);
	s_mapInRoomFamilys.insert( make_pair(FID, SINROOMFAMILYS(RoomID, RoleInRoom, InTime, bIsMobile)) );

	TServiceId	fes_tid(fesid);
	sendDataList(FID, fes_tid);

	
	std::string token = GenToken(FID, RoomID);
	s_mapFamilyFileAuth[FID] = token;
	
	//sipinfo("New family : FID=%d, RoomID=%d, RoomRole=%d", FID, RoomID, RoleInRoom); byy krs
}

void cb_EnterLobby(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	
}

void cb_ChangeRole(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);
	T_ROOMROLE			RoleInRoom;			_msgin.serial(RoleInRoom);

	MAP_INROOMFAMILYS::iterator	it = s_mapInRoomFamilys.find(FID);
	if((it != s_mapInRoomFamilys.end()) && (it->second.m_RoomID == RoomID))
	{
		it->second.m_RoleInRoom = RoleInRoom;
	}
}

void cb_LeaveRoom(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID			FID;		_msgin.serial(FID);
	T_ROOMID			RoomID;		_msgin.serial(RoomID);
	
	RemoveFamily(FID);
	family_fileauth_t::iterator ite = s_mapFamilyFileAuth.find(FID);
	if (ite != s_mapFamilyFileAuth.end())
	{		
		s_mapFamilyFileAuth.erase(ite);
	}

	//sipinfo("Remove family : FID=%d", FID); byy krs
}

//////////////////////////////////////////////////////////////////////////

void cb_M_CS_HISMANAGE_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		HisManageType;
	_msgin.serial(FID, HisManageType);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	uint32		RoomID;

	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	if ((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try manage hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

	if (itFamily->second.m_bDataManaging)
	{
		sipdebug("You are current managing data. FID=%d", FID);
		return;
	}

	// Get RoomId
	RoomID = itFamily->second.m_RoomID;

	uint32	CurrentManagingFID = 0;
	map<uint32, uint32>::iterator it = g_mapCurManagerList.find(RoomID);
	if (it != g_mapCurManagerList.end())
	{
		CurrentManagingFID = it->second;
	}
	else
	{
		g_mapCurManagerList[RoomID] = FID;
		itFamily->second.m_bDataManaging = true;
	}

	CMessage	msgOut(M_SC_HISMANAGE_START);
	msgOut.serial(FID, CurrentManagingFID, HisManageType);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

	//sipdebug("CurrentManagingFID=%d, FID=%d", CurrentManagingFID, FID); byy krs
}

void cb_M_CS_HISMANAGE_END(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	HisManageStop(FID);
}

void cb_M_CS_REQ_UPLOAD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_DATATYPE	data_type;
	uint8		DeadID;
	T_ALBUMID   albumId;
	uint32		data_size;	
	ucstring	comment;

	_msgin.serial(FID, data_type, DeadID, albumId, data_size, comment);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	//sipdebug("fid=%d, data_type=%d, data_size=%d, comment=%s", FID, data_type, data_size, comment.c_str()); byy krs

	uint32	nResultCode = ERR_NOERR;
	T_AUTHKEY auth_key = 0;
	TOnlineDataServer	webserver;

	uint32		data_size_k = (data_size + 1023) / 1024;

	if (data_size == 0)// || ! isValidDataType(data_type) )
	{
		sipwarning("Invalid size or type: data_type=%d, data_size=%d", data_type, data_size);
		return;
	}

	T_ROOMID RoomID;

	while (false)
	{
		if (data_type == dataRecommendRoomFigure)
		{
			RoomID = ROOM_ID_VIRTUAL;
		}
		else if (data_type == dataPublicroomFigure)
		{
			IFNOTFIND_INROOMFAMILYS(itFamily, FID)
			{
				// Hacker
				sipwarning("There is no room info of family(%d)", FID);
				//			sendErrorResult(M_SC_REQ_UPLOAD, FID, E_DBERR, _tsid);
				return;
			}
			RoomID = itFamily->second.m_RoomID; // ROOM_ID_PUBLICROOM;
		}
		else if (data_type == dataRoomTitlePhoto)
		{
			RoomID = albumId;
		}
		else if (data_type == dataFamilyFigure)
		{
			RoomID = ROOM_ID_FAMILYFIGURE;
		}
		else
		{
			IFNOTFIND_INROOMFAMILYS(itFamily, FID)
			{
				// Hacker
				sipwarning("There is no room info of family(%d)", FID);
				return;
			}

			// Try to find family information from family ID	
			if ((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
			{
				// Hacker!!!
				sipwarning("Visitor try update hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
				return;
			}

			if ((data_type != dataFigure) && (data_type != dataAncestorFigure_start) && (data_type != dataFrameFigure))
			{
				if (!itFamily->second.m_bDataManaging)
				{
					sipwarning("You are not in managing! FID=%d", FID);
					return;
				}
			}

			// Get RoomId
			RoomID = itFamily->second.m_RoomID;
			if (!IsCommonRoom(RoomID))
			{
				// check space capacity
				MAKEQUERY_CheckSpace(s_szQuery, RoomID, data_size_k);
				DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
				if (!nPrepareRet)
				{
					nResultCode = E_DBERR;
					break;
				}
				SQLLEN	num1 = 0;
				int	ret = 0;
				DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);
				if (!nBindResult)
				{
					nResultCode = E_DBERR;
					break;
				}
				DB_EXE_PARAMQUERY (Stmt, s_szQuery);
				if (!nQueryResult)
				{
					nResultCode = E_DBERR;
					break;
				}
				if (ret != 0)
				{
					if (ret == 1029)
					{
						nResultCode = E_PHOTO_HISSPACE_FULL;
						break;
					}
					else
					{
						nResultCode = ret;
						break;
					}
					sipwarning("check space failed: data_type=%d, data_size=%d, ret=%d", data_type, data_size, ret);
					return;
				}
			}
		}

		uint32	old_photoid = 0;
		{
			T_DATASERVERID	web_id = INVALID_DATASERVERID;
			uint32	con_min = 1000000;
			TOnlineDataServers::iterator it;
			for (it = s_OnlineDataServers.begin(); it != s_OnlineDataServers.end(); it ++)
			{
				TOnlineDataServer	ds = it->second;

				if (data_type == dataFigure && ! ds->m_bPortrait)	continue;
				if (data_type == dataPhoto && ! ds->m_bPhoto)	continue;
				if ((data_type == dataAncestorFigure_start) && ! ds->m_bPhoto)	continue;
				if (data_type == dataMedia && ! ds->m_bVideo)	continue;
				if (data_type == dataRecommendRoomFigure && ! ds->m_bPhoto)	continue;	// 凯痢眠玫拳惑磊丰绰 Photo率俊 焊包茄促.
				if (data_type == dataPublicroomFigure && ! ds->m_bPhoto)	continue;	// 傍傍玫盔狼 眉氰规拳惑磊丰绰 Photo率俊 焊包茄促.
				if (data_type == dataRoomTitlePhoto && ! ds->m_bPhoto)	continue;		// 眉氰规鸥捞撇荤柳磊丰绰 Photo率俊 焊包茄促.
				if (data_type == dataFamilyFigure && ! ds->m_bPhoto)	continue;		// 荤侩磊屈惑荤柳磊丰绰 Photo率俊 焊包茄促.
				if (data_type == dataFrameFigure && ! ds->m_bPhoto)	continue;		// FigureFrame荤柳磊丰绰 Photo率俊 焊包茄促.

				uint32 con_count = ds->getConnectCount();
				if (con_count < con_min)
				{
					con_min = con_count;
					web_id = it->first;
				}
			}
			if (web_id == INVALID_DATASERVERID)
			{
				nResultCode = E_PHOTO_NO_SERVER;
				break;
			}

			if (con_min > MaxDownloadNum)
			{
				nResultCode = E_PHOTO_SERVER_BUSY;
				break;
			}

			webserver = s_OnlineDataServers[web_id];
			static uint16 upload_key = 1;
			auth_key = MakeAuthKey(upload_key++);
			webserver->m_UploadConnections[auth_key] = UploadConnect(FID, RoomID, data_type, DeadID, data_size, comment, albumId, _tsid, old_photoid);
		}
	}

// _finished:
	CMessage msgout(M_SC_REQ_UPLOAD);

	std::string strWebIP = webserver->m_Addr.asWebIPString();
	msgout.serial(FID, nResultCode, data_type, DeadID, albumId);
	if (nResultCode == ERR_NOERR)
	{
		msgout.serial(strWebIP);
		msgout.serial(auth_key);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgout);

	//sipdebug("Request Photo Upload: FID=%d, nResultCode=%d, auth_key=%d, data_size=%d", FID, nResultCode, auth_key, data_size); byy krs
}

// [b:Flag (1:Thumbnail, 2:Main)][d:RoomID][b:DataType][d:DataID]
void cb_M_CS_REQ_DOWNLOAD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		flag;		_msgin.serial(flag);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	T_DATATYPE	datatype;	_msgin.serial(datatype);
	uint32		fileid;		_msgin.serial(fileid);

	if (RoomID == 0)
	{
		// Try to find family information from family ID	
		IFNOTFIND_INROOMFAMILYS(itFamily, FID)
		{
			// Hacker
			sipwarning("There is no room info of family(FID=%d)", FID);
//			sendErrorResult(M_SC_REQ_DOWNLOAD, FID, E_DBERR, _tsid);
			return;
		}

		// Get RoomId
		RoomID = (*itFamily).second.m_RoomID;
	}
	//sipdebug("RoomID=%d, FID=%d, datatype=%d, dataid=%d", RoomID, FID, datatype, fileid); byy krs

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	uint32		err = ERR_NOERR;
	T_AUTHKEY	auth_key = 0;
	std::string	url_photo = "";
	std::string	url_stream = "";
	uint32		stream_port = 0;
	uint8		WebID = 0;

	try
	{
		uint64	datakey = MakeUINT64(RoomID, fileid);
		TPhotoInfos::iterator itData = s_photoInfoInAlbum.find(datakey);
		if (itData != s_photoInfoInAlbum.end() && (CTime::getLocalTime() - itData->second.generate_time <= PHOTOINFO_REFRESH_TIME))
		{
			WebID = itData->second.webid;
		}
		else
		{
			if (datatype != dataMedia)
			{
				MAKEQUERY_GetPhotoInfo(s_szQuery, RoomID, fileid);
			}
			else
			{
				MAKEQUERY_GetVideoInfo(s_szQuery, RoomID, fileid);
			}
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if (!nPrepareRet)
			{
				err = E_DBERR;
				throw "prepare statement failed!!";		
			}
			SQLLEN	num1 = 0;
			int	ret = 0;
			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if (!nQueryResult)
			{
				err = E_DBERR;
				throw "execute failed!!";
			}

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
			{
				err = E_DBERR;
				throw "fetch failed!!";
			}

			COLUMN_DIGIT(row,	0,	uint8,	webid);

			WebID = webid;

			if (datatype != dataMedia)
			{
				COLUMN_DATETIME(row, 4, creationtime);
				COLUMN_DIGIT(row,	5,	uint8,	checked);
				uint32 nCreateTime = getMinutesSince1970(creationtime);

				if(itData == s_photoInfoInAlbum.end())
				{
					COLUMN_WSTR_BUF(row, 1,	comment, s_szDescription, MAX_DATA_DESCRIPTION_VIDEO);
					COLUMN_DIGIT(row,	2,	uint32,	crc32);
					COLUMN_DIGIT(row,	3,	uint32,	filesize);
					COLUMN_DIGIT(row,	6,	uint32,	masterFID);					

					s_photoInfoInAlbum.insert( make_pair(datakey, PhotoInfo(comment, crc32, filesize, nCreateTime, masterFID, checked, webid)) );
				}
				else
				{
					itData->second.checked = checked;
					itData->second.generate_time = CTime::getLocalTime();
				}
			}
		}

		// [d:Result Code][s:ServerUrl][d:AuthKey][d:StreamPort]
		TOnlineDataServers::iterator it = s_OnlineDataServers.find(WebID);
		if (it == s_OnlineDataServers.end())
		{
			sipwarning("No data server %d", WebID);
			err = E_PHOTO_NO_SERVER;
			throw "No data server";
		}

		TOnlineDataServer &	ds = it->second;

		uint32	cur_con_count = ds->getConnectCount();
		if (cur_con_count > MaxDownloadNum)
		{
			sipwarning("Connect count is full: cur=%d, max=%d, up=%d, down=%d, FID=%d, RoomID=%d, datatype=%d, fileid=%d", cur_con_count, MaxDownloadNum.get(),
				ds->m_UploadConnections.size(), ds->m_DownloadConnections.size(), FID, RoomID, datatype, fileid);
			err = E_PHOTO_SERVER_BUSY;
			throw "Connect count is full";
		}

		static	uint16	download_key = 1;
		auth_key = MakeAuthKey(download_key ++);

		ds->m_DownloadConnections[auth_key] = DownloadConnect(FID, RoomID, datatype, fileid);

		url_photo = ds->m_Addr.asWebIPString();

		stream_port = static_cast<uint32>(SMSStreamPort.get());
		url_stream = ds->m_Addr.ipAddress() + ":" + 9000; //SIPBASE::toStringA(stream_port); by krs

		//sipdebug("RoomID=%d, FID=%d, DataType=%d, FileID=%d, auth_key=%d, url=%s, url_stream=%s", RoomID, FID, datatype, fileid, auth_key, url_photo.c_str(), url_stream.c_str()); byy krs
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}

	CMessage	msgout(M_SC_REQ_DOWNLOAD);
	msgout.serial(FID, err, flag);

	{
		msgout.serial(datatype, fileid, url_photo, auth_key, url_stream);
	}
	

	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
}

// [b:DataType][d:DataID][d:RoomID] : RoomID绰 眉氰规鸥捞撇荤柳牢 版快父 蜡瓤窍促. (DataType=10)
void cb_M_CS_REQ_DELETE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	datatype;	_msgin.serial(datatype);
	uint32		fileid;		_msgin.serial(fileid);
	T_ROOMID RoomID;

	if (datatype == dataRecommendRoomFigure)
	{
		RoomID = ROOM_ID_VIRTUAL;
	}
	else if (datatype == dataPublicroomFigure)
	{
		IFNOTFIND_INROOMFAMILYS(itFamily, FID)
		{
			sipwarning("There is no room info of family(%d)", FID);
			return;
		}
		RoomID = itFamily->second.m_RoomID; // ROOM_ID_PUBLICROOM;
	}
	else if (datatype == dataRoomTitlePhoto)
	{
		_msgin.serial(RoomID);
	}
	else if (datatype == dataFamilyFigure)
	{
		RoomID = ROOM_ID_FAMILYFIGURE;
	}
	else
	{	
		IFNOTFIND_INROOMFAMILYS(itFamily, FID)
		{
			sipwarning("There is no room info of family(%d)", FID);
			return;
		}
		if ((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
		{
			// Hacker!!!
			sipwarning("Visitor try delete hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
			return;
		}
		if ((datatype != dataFigure) && (datatype != dataAncestorFigure_start) && (datatype != dataFrameFigure))
		{
			if (!itFamily->second.m_bDataManaging)
			{
				sipwarning("You are not in managing! FID=%d", FID);
				return;
			}
		}
		RoomID = (*itFamily).second.m_RoomID;
	}

	uint8 bIsMobile = 0; //by krs
	MAP_INROOMFAMILYS::iterator	itd = s_mapInRoomFamilys.find(FID);
	if ((itd != s_mapInRoomFamilys.end()) && (itd->second.m_RoomID == RoomID))
	{
		bIsMobile = itd->second.m_bIsMobile;
	}	

	uint32 err = ERR_NOERR;
	CMessage msgout(M_SC_REQ_DELETE);	
	{
		int webid = 1;
		TOnlineDataServers::iterator it = s_OnlineDataServers.find(webid);
		if (it == s_OnlineDataServers.end())
		{
			sipwarning("No data server %d", webid);
			err = E_PHOTO_NO_SERVER;			
			if (bIsMobile == 2) // by krs
				msgout.serial(FID, err, datatype, RoomID, fileid);
			else
				msgout.serial(FID, err, datatype, fileid);
			CUnifiedNetwork::getInstance()->send( _tsid, msgout);
			return;
		}
				
		TOnlineDataServer &	ds = it->second;
		
		for (TDownloadConnects::iterator itDownload = ds->m_DownloadConnections.begin(); itDownload != ds->m_DownloadConnections.end(); itDownload++)
		{
			if (itDownload->second.datatype == datatype && itDownload->second.roomid == RoomID && itDownload->second.fileid == fileid && itDownload->second.req_time == 0)
			{
				sipinfo("Can't delete because file is downloading...");
				err = E_FILE_DOWNLOADING;
				CMessage msgOut(M_SC_REQ_DELETE);
				msgOut.serial(FID, err, datatype, fileid);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
		}

		for (TDownloadConnects::iterator itDownload = ds->m_DownloadConnections.begin(); itDownload != ds->m_DownloadConnections.end(); )
		{
			if (itDownload->second.datatype == datatype && itDownload->second.roomid == RoomID && itDownload->second.fileid == fileid)
				itDownload = ds->m_DownloadConnections.erase(itDownload);
			else
				itDownload++;
		}
			
		static	uint16	delete_key = 1;
		T_AUTHKEY	auth_key = MakeAuthKey(delete_key++);
		uint32	cur_con_count = ds->getConnectCount();
		if (cur_con_count > MaxDownloadNum)
		{
			sipwarning("Connect count is full: cur=%d, max=%d, up=%d, down=%d", cur_con_count, MaxDownloadNum.get(),
				ds->m_UploadConnections.size(), ds->m_DownloadConnections.size());
			err = E_PHOTO_SERVER_BUSY;
			CMessage msgOut(M_SC_REQ_DELETE);
			msgOut.serial(FID, err, datatype, fileid);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}
		ds->m_DeleteConnections[auth_key] = DeleteConnect(FID, RoomID, datatype, fileid, _tsid);
		std::string	url = ds->m_Addr.asWebIPString();			
		msgout.serial(FID, err, datatype, fileid, url, auth_key);
	}
	
	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
}

// [b:Flag (1:Thumbnail, 2:Main)][d:RoomID][b:DataType][d:DataID]
void cb_M_CS_PLAY_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		flag;		_msgin.serial(flag);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	T_DATATYPE	datatype;	_msgin.serial(datatype);
	uint32		fileid;		_msgin.serial(fileid);
	
	uint32		err = ERR_NOERR;
	uint8		WebID = 0;
	std::string	url_photo = "";
	std::string	url_stream = "";
	T_AUTHKEY auth_key;	
	static	uint16	play_key = 1;
	auth_key = MakeAuthKey(play_key++);

	CMessage msgout(M_SC_REQ_DOWNLOAD);

	MAKEQUERY_GetVideoInfo(s_szQuery, RoomID, fileid);

	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if (!nPrepareRet)
	{
		err = E_DBERR;
		sipwarning("prepare statement failed!!");
		return;
	}
	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if (!nQueryResult)
	{
		err = E_DBERR;
		sipwarning("execute failed!!");
		return;
	}

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (!IS_DB_ROW_VALID(row))
	{
		std::string strToken = GetToken(FID);

		err = E_DBERR;
		sipwarning("fetch failed!!");
		msgout.serial(FID, err, flag, datatype, RoomID, fileid);
		msgout.serial(url_photo, auth_key, url_stream, strToken);
		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
		return;
	}

	COLUMN_DIGIT(row, 0, uint8, webid);

	WebID = webid;

	TOnlineDataServers::iterator it = s_OnlineDataServers.find(WebID);
	if (it == s_OnlineDataServers.end())
	{
		sipwarning("No data server %d", WebID);
		err = E_PHOTO_NO_SERVER;
		return;
	}

	TOnlineDataServer &	ds = it->second;

	uint32	cur_con_count = ds->getConnectCount();
	if (cur_con_count > MaxDownloadNum)
	{
		sipwarning("Connect count is full: cur=%d, max=%d, up=%d, down=%d, stream=%d, FID=%d, RoomID=%d, datatype=%d, fileid=%d", cur_con_count, MaxDownloadNum.get(),
			ds->m_UploadConnections.size(), ds->m_DownloadConnections.size(), ds->m_StreamConnections.size(), FID, RoomID, datatype, fileid);
		err = E_PHOTO_SERVER_BUSY;
		return;
	}
		
	ds->m_StreamConnections[auth_key] = StreamConnect(FID, RoomID, datatype, fileid);

	
}

// [b:Flag (1:Thumbnail, 2:Main)][d:RoomID][b:DataType][d:DataID]
void cb_M_CS_PLAY_STOP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		flag;		_msgin.serial(flag);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	T_DATATYPE	datatype;	_msgin.serial(datatype);
	uint32		fileid;		_msgin.serial(fileid);
	
	uint32		err = ERR_NOERR;
	uint8		WebID = 0;

	MAKEQUERY_GetVideoInfo(s_szQuery, RoomID, fileid);

	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if (!nPrepareRet)
	{
		err = E_DBERR;
		sipwarning("prepare statement failed!!");
		return;
	}
	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if (!nQueryResult)
	{
		err = E_DBERR;
		sipwarning("execute failed!!");
		return;
	}

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (!IS_DB_ROW_VALID(row))
	{
		err = E_DBERR;
		sipwarning("fetch failed!!");
		return;
	}

	COLUMN_DIGIT(row, 0, uint8, webid);

	WebID = webid;

	TOnlineDataServers::iterator it = s_OnlineDataServers.find(WebID);
	if (it == s_OnlineDataServers.end())
	{
		sipwarning("No data server %d", WebID);
		err = E_PHOTO_NO_SERVER;		
		return;
	}

	TOnlineDataServer s = it->second;	

	TStreamConnects::iterator its = s->m_StreamConnections.begin();
	while (its != s->m_StreamConnections.end())
	{
		if ((its->second.roomid == RoomID) && (its->second.datatype == datatype) && (its->second.fileid == fileid))	
		{
			sipdebug("Stream connection Deleted. auth_key=%d", its->first);
			TStreamConnects::iterator itt = its++;
			s->m_StreamConnections.erase(itt);
			return;
		}
		its++;
	}	
	
}

// Get Photo&Video list of a Album [b:DataType][d:RoomID][d:AlbumId]
void cb_M_CS_ALBUMINFO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);
	T_COMMON_ID	albumid;	_msgin.serial(albumid);	

	uint8		checkflag = HISDATA_CHECKFLAG_NONE;
	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	roomid = itFamily->second.m_RoomID;
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && 
		(itFamily->second.m_RoleInRoom != ROOM_MANAGER) &&
		(!IsCommonRoom(roomid)))
		checkflag = HISDATA_CHECKFLAG_CHECKED;

	uint32	index = 0;
	uint32	total_time = 0;
	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// search the photos' info of one album
			MAKEQUERY_LoadPhotoList(s_szQuery, roomid, albumid, checkflag);
			DB_EXE_QUERY(DB, Stmt, s_szQuery);
			if ( ! nQueryResult )
			{
				sendErrorResult(M_SC_ALBUMINFO, FID, E_DBERR, _tsid);
				return;
			}

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_ALBUMINFO);
			msgout.serial(FID, uResult, data_type, albumid);

			do
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if ( ! IS_DB_ROW_VALID(row) )
					break;

				COLUMN_DIGIT(row,	0, 	uint32, fileid);
				COLUMN_WSTR_BUF(row, 2,	comment, s_szDescription, MAX_DATA_DESCRIPTION_VIDEO);
				COLUMN_DIGIT(row,	3,	uint8, checked);
				COLUMN_DIGIT(row,	4,	uint32, crc32);
				COLUMN_DIGIT(row,   5,  uint32, filesize);
				COLUMN_DATETIME(row, 6, creationtime);
				COLUMN_DIGIT(row,   7,  uint32, masterFID);
				COLUMN_DIGIT(row,	8,	uint8, webid);
				
				uint32 nCreateTime = getMinutesSince1970(creationtime);

				PhotoInfo temp(comment, crc32, filesize, nCreateTime, masterFID, checked, webid);
				s_photoInfoInAlbum[MakeUINT64(roomid, fileid)] = temp;
				msgout.serial(fileid, index);
			} while ( true );
				
			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			//sipinfo("[M_SC_ALBUMINFO] : user:%u searches the photos of album:%u", FID,  albumid);	 byy krs	
		}
		else if ( data_type == dataMedia ) 
		{
			// search the videos' info of one group
			MAKEQUERY_LoadVideoList(s_szQuery, roomid, albumid, checkflag);
			DB_EXE_QUERY(DB, Stmt, s_szQuery);
			if ( ! nQueryResult )
			{
				sendErrorResult(M_SC_ALBUMINFO, FID, E_DBERR, _tsid);
				return;
			}

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_ALBUMINFO);
			msgout.serial(FID, uResult, data_type, albumid);

			do
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if ( ! IS_DB_ROW_VALID(row) )
					break;

				COLUMN_DIGIT(row,	0, 	uint32, fileid);
				COLUMN_WSTR_BUF(row, 2,	comment, s_szDescription, MAX_DATA_DESCRIPTION_VIDEO);
				COLUMN_DIGIT(row,	3,	uint8, checked);
				COLUMN_DIGIT(row,	4,	uint32, crc32);
				COLUMN_DIGIT(row,   5,  uint32, filesize);
				COLUMN_DATETIME(row, 6, creationtime);
				COLUMN_DIGIT(row,	7, 	uint32, index);
				COLUMN_DIGIT(row,	8, 	uint32, masterFID);
				COLUMN_DIGIT(row, 9, uint32, total_time);
				
				uint32 nCreateTime = getMinutesSince1970(creationtime);

				VideoInfo temp(comment, crc32, filesize, nCreateTime, index, masterFID, checked);
				s_videoInfoInAlbum[MakeUINT64(roomid, fileid)] = temp;
				msgout.serial(fileid, index);
			} while ( true );

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			//sipinfo("[M_SC_ALBUMINFO] : user:%u searches the videos of album:%u", FID,  albumid);	byy krs	
		}	
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
		sendErrorResult(M_SC_ALBUMINFO, FID, E_DBERR, _tsid);
	}
}

// [b:DataType][d:RoomID][d:DataId]
void cb_M_CS_REQDATA_PHOTO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);
	uint8		DeadID;		_msgin.serial(DeadID);
	uint32		RoomID;		_msgin.serial(RoomID);
	uint32		fileId;     _msgin.serial(fileId);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	CMessage msgout(M_SC_REQDATA_PHOTO);
	msgout.serial(FID);
	uint32	uResult = ERR_NOERR;
	uint64	datakey = MakeUINT64(RoomID, fileId);

	if (data_type == dataFigure || data_type == dataPhoto || data_type == dataRecommendRoomFigure || data_type == dataPublicroomFigure
		|| (data_type == dataAncestorFigure_start) || (data_type == dataRoomTitlePhoto) || (data_type == dataFamilyFigure) || (data_type == dataFrameFigure))
	{
		TTime	curTime = CTime::getLocalTime();
		TPhotoInfos::iterator itData = s_photoInfoInAlbum.find(datakey);

		BOOL	bRefresh = false;

		if ( itData == s_photoInfoInAlbum.end() )
		{
			if (data_type == dataPhoto)
			{
				uResult = E_NO_EXIST;
				msgout.serial(uResult, data_type, DeadID, RoomID, fileId);

				CUnifiedNetwork::getInstance()->send(_tsid, msgout);
				sipwarning("there is no info of the FileId:%u", fileId);
				return;
			}
			bRefresh = true;
		}
		else
		{
			if(curTime - itData->second.generate_time > PHOTOINFO_REFRESH_TIME)
				bRefresh = true;
		}

		if (bRefresh) //by krs
		{
			MAKEQUERY_GetPhotoInfo(s_szQuery, RoomID, fileId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if (!nPrepareRet)
			{
				sipwarning(L"%s: prepare statement failed!!", s_szQuery);
				uResult = E_DBERR;
				msgout.serial(uResult, data_type, DeadID, RoomID, fileId);
				CUnifiedNetwork::getInstance()->send( _tsid, msgout);
				return;
			}
			SQLLEN	num1 = 0;
			int	ret = 0;
			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if (!nQueryResult || ret != 0)
			{
				sipwarning(L"%s: execute failed!", s_szQuery);
				uResult = E_DBERR;
				msgout.serial(uResult, data_type, DeadID, RoomID, fileId);
				CUnifiedNetwork::getInstance()->send(_tsid, msgout);
				return;
			}

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( ! IS_DB_ROW_VALID(row) )
			{
				sipwarning(L"%s: fetch failed! FID=%d, data_type=%d, DeadID=%d", s_szQuery, FID, data_type, DeadID);
				uResult = E_DBERR;
				msgout.serial(uResult, data_type, DeadID, RoomID, fileId);
				CUnifiedNetwork::getInstance()->send(_tsid, msgout);
				return;
			}

			COLUMN_DIGIT(row,	0,	uint8,	webid);
			COLUMN_WSTR_BUF(row, 1,	comment, s_szDescription, MAX_DATA_DESCRIPTION_VIDEO);
			COLUMN_DIGIT(row,	2,	uint32,	crc32);
			COLUMN_DIGIT(row,	3,	uint32,	filesize);
			COLUMN_DATETIME(row, 4, creationtime);
			COLUMN_DIGIT(row,	5,	uint8,	checked);
			COLUMN_DIGIT(row,	6,	uint32,	masterFID);

			uint32 nCreateTime = getMinutesSince1970(creationtime);
			
			if(itData == s_photoInfoInAlbum.end())
			{
				s_photoInfoInAlbum.insert( make_pair(datakey, PhotoInfo(comment, crc32, filesize, nCreateTime, masterFID, checked, webid)) );

				itData = s_photoInfoInAlbum.find(datakey);
			}
			else
			{
				itData->second.checked = checked;
				itData->second.generate_time = curTime;
			}
		}
		
		if ( itData != s_photoInfoInAlbum.end() )
		{
			if((data_type != dataFamilyFigure) 
				&& (data_type != dataRecommendRoomFigure) && (data_type != dataPublicroomFigure) && (!(RoomID >= MIN_RELIGIONROOMID && RoomID < MAX_RELIGIONROOMID))
				&& (itData->second.checked != HISDATA_CHECKFLAG_CHECKED) && (FID != itData->second.masterFID) && !IsHisManager(FID, RoomID))
			{
				uResult = E_FILE_NOCHECKED;
				msgout.serial(uResult, data_type, DeadID, fileId);

				CUnifiedNetwork::getInstance()->send(_tsid, msgout);
				//sipinfo("This file is not checked. fileid=%u", fileId); byy krs
			}
			else
			{				
				msgout.serial(uResult);
				msgout.serial(itData->second.checked);
				msgout.serial(data_type, DeadID);
				msgout.serial(fileId);
				msgout.serial(itData->second.fileSize);
				msgout.serial(itData->second.crc32);
				msgout.serial(itData->second.nCreateTime);
				msgout.serial(itData->second.comment);					

				CUnifiedNetwork::getInstance()->send(_tsid, msgout);
				//sipinfo("[M_SC_REQDATA_PHOTO] : FID=%u send the fileId:%u's info checked:%u", FID, fileId, itData->second.checked); byy krs
			}
		}
		else
		{
			uResult = E_NO_EXIST;
			msgout.serial(uResult, data_type, DeadID, fileId);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			//sipwarning("there is no info of the fileId:%u, FID=%d", fileId, FID); byy krs
		}		
	}
	else
	{
		sipwarning("Invalid datatype: datatype=%d, FID=%d", data_type, FID);
	}
}

// [d:RoomID][d:DataId]
void cb_M_CS_REQDATA_VIDEO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		RoomID;		_msgin.serial(RoomID);
	uint32		FileId;     _msgin.serial(FileId);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	CMessage msgout(M_SC_REQDATA_VIDEO);
	msgout.serial(FID);
	uint32	uResult = ERR_NOERR;
	uint64	datakey = MakeUINT64(RoomID, FileId);

	TVideoInfos::iterator itVideo = s_videoInfoInAlbum.find(datakey);
	if( itVideo != s_videoInfoInAlbum.end() )
	{
		TTime	curTime = CTime::getLocalTime();
		if(curTime - itVideo->second.generate_time > 1 * 60 * 60 * 1000) // 1矫埃俊 茄锅 盎脚茄促.
		{
			MAKEQUERY_GetVideoInfo(s_szQuery, RoomID, FileId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( nPrepareRet )
			{
				SQLLEN	num1 = 0;
				int	ret = 0;
				DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);

				DB_EXE_PARAMQUERY(Stmt, s_szQuery);
				if ( nQueryResult && ret == 0 )
				{
					DB_QUERY_RESULT_FETCH(Stmt, row);
					if ( IS_DB_ROW_VALID(row) )
					{
						COLUMN_DIGIT(row,	5,	uint8,	checked);

						itVideo->second.checked = checked;

						itVideo->second.generate_time = curTime;
					}
				}
			}
		}

		if((itVideo->second.checked != HISDATA_CHECKFLAG_CHECKED) && (FID != itVideo->second.masterFID) && !IsHisManager(FID, RoomID))
		{
			uResult = E_FILE_NOCHECKED;
			msgout.serial(uResult, FileId);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			//sipinfo("This file is not checked. fileid=%u", FileId); byy krs
		}
		else
		{			
			msgout.serial(uResult);
			msgout.serial(itVideo->second.checked);
			msgout.serial(FileId);
			msgout.serial(itVideo->second.fileSize);
			msgout.serial(itVideo->second.crc32);
			msgout.serial(itVideo->second.nCreateTime);
			msgout.serial(itVideo->second.comment);
			msgout.serial(itVideo->second.index);						

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			//sipinfo("[M_SC_REQDATA_VIDEO] : user:%u send the FileId:%u's info", FID, FileId); byy krs
		}
	}
	else
	{
		uResult = E_NO_EXIST;
		msgout.serial(uResult, FileId);

		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
		//sipwarning("there is no info of the FileId:%u", FileId); byy krs
	}	
}

// Modify Data's Desc [b:DataType][d:DataId][u:Description]
void cb_M_CS_RENDATA(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);
	uint32	    FileId;     _msgin.serial(FileId);
	ucstring	comment;	_msgin.serial(comment);
	//sipdebug("fid=%d, data_type=%d, data_id=%d", FID, data_type, FileId); byy krs

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try rendata hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// modify the photo's description
			MAKEQUERY_ChangePhotoDesc(s_szQuery, roomid, FileId, comment);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_PhotoInfo_NameAndDesc_Update2: prepare statement failed!!";		

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_PhotoInfo_NameAndDesc_Update2: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_PhotoInfo_NameAndDesc_Update2: execute failed!";	

			sipinfo("user:%u modify photoId:%u description:%u", FID, FileId, comment.c_str());
		}
		else if ( data_type == dataMedia ) 
		{
			// modify the video's description
			MAKEQUERY_ChangeVideoDesc(s_szQuery, roomid, FileId, comment);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_VideoInfo_NameAndDesc_Update2: prepare statement failed!!";		

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_VideoInfo_NameAndDesc_Update2: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_VideoInfo_NameAndDesc_Update2: execute failed!";	

			sipinfo("user:%u modify videoId:%u description:%u", FID, FileId, comment.c_str());
		}
	
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}

}

// Change Data's AlbumID [b:DataType][d:DataId][d:NewAlbumId]
void cb_M_CS_MOVDATA(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);	
	T_DATAID    fileId;     _msgin.serial(fileId);
	T_ALBUMID   newAlbumId; _msgin.serial(newAlbumId);	

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try movedata hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// move a photo from a album to another album
			MAKEQUERY_ChangePhotoAlbumID(s_szQuery, roomid, fileId, newAlbumId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_PhotoInfo_PhotoAlbumID_update2: prepare statement failed!!";		

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_PhotoInfo_PhotoAlbumID_update2: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_PhotoInfo_PhotoAlbumID_update2: execute failed!";	

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_MOVDATA);
			msgout.serial(FID, uResult, data_type, fileId, newAlbumId);
			
			CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
			sipinfo("[M_SC_MOVDATA] : user:%u move photo:%u to album:%u", FID, fileId, newAlbumId);
		}
		else if ( data_type == dataMedia ) 
		{
			// move a video from a group to another group
			MAKEQUERY_ChangeVideoAlbumID(s_szQuery, roomid, fileId, newAlbumId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_VideoInfo_PhotoAlbumID_update2: prepare statement failed!!";		

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_VideoInfo_PhotoAlbumID_update2: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_VideoInfo_PhotoAlbumID_update2: execute failed!";	

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_MOVDATA);
			msgout.serial(FID, uResult, data_type, fileId, newAlbumId);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			sipinfo("[M_SC_MOVDATA] : user:%u move video:%u to album:%u", FID, fileId, newAlbumId);
		}	
	
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
		sendErrorResult(M_SC_MOVDATA, FID, E_DBERR, _tsid);
	}

}

// Change video's index [d:Count][ [d:VideoId][d:NewIndex] ]
void cb_M_CS_CHANGEVIDEOINDEX(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;	 _msgin.serial(FID);
//	uint32		Roomid;  _msgin.serial(Roomid);  by krs
	uint32		count;	 _msgin.serial(count);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	if(_msgin.length() - _msgin.getHeaderSize() != count * 8 + sizeof(FID) + sizeof(count))
	{
		sipwarning("Invalid count or length");
		return;
	}

	MAKEQUERY_ChangeVideoIndex(s_szQuery);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_VideoInfo_Update_IndexID DB_EXE_PREPARESTMT failed!!");
		return;
	}

	uint32 FileID, NewIndex;
	for(uint32 i = 0; i < count; i ++)
	{
		_msgin.serial(FileID);
		_msgin.serial(NewIndex);

		SQLLEN len1 = sizeof(uint32);
		SQLLEN len2 = sizeof(uint32);

		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &FileID, 0, &len1);
		DB_EXE_BINDPARAM_IN(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &NewIndex, 0, &len2);

		if ( ! nBindResult )
		{
			sipwarning("Shrd_VideoInfo_Update_IndexID DB_EXE_BINDPARAM_IN failed!!");
			return;
		}

		DB_EXE_PARAMQUERY(Stmt, s_szQuery);
		if ( ! nQueryResult )
		{
			sipwarning("Shrd_VideoInfo_Update_IndexID DB_EXE_PARAMQUERY failed!!");
			return;
		}
	}
}

// M_CS_FRAMEDATA_SET
// Set Frame's Photo [d:FrameId][d:PhotoId][b:1 Hang up/0 Hang down][w:OffsetX][w:OffsetY][d:Scale]
void cb_M_CS_FRAMEDATA_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_FRAMEID	frameId;	_msgin.serial(frameId);
	T_DATAID	dataId;		_msgin.serial(dataId);
	uint8		flag;		_msgin.serial(flag);
	uint16		OffsetX;	_msgin.serial(OffsetX);
	uint16		OffsetY;	_msgin.serial(OffsetY);
	uint32		Scale;		_msgin.serial(Scale);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID	roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try movedata hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}

	roomid = itFamily->second.m_RoomID;
	try
	{
		if(flag == 1)
		{
			// set a data for the certain frame(photo/Video)
			MAKEQUERY_SetFrame(s_szQuery, roomid, frameId, dataId, OffsetX, OffsetY, Scale);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_Room_PhotoFrame_insert: prepare statement failed!!";		

			SQLLEN len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_Room_PhotoFrame_insert: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_Room_PhotoFrame_insert: execute failed!";	

			sipinfo("user:%u set photo:%u for photo frame:%u", FID, dataId, frameId);		
		}
		else 
		{
			// cancel a data set for the frame(photo/Video)
			MAKEQUERY_CancelSetFrame(s_szQuery, roomid, frameId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if ( ! nPrepareRet )
				throw "Shrd_Room_PhotoFrame_Delete: prepare statement failed!!";		

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if ( ! nBindResult )
				throw "Shrd_Room_PhotoFrame_Delete: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if ( ! nQueryResult || ret == -1)
				throw "Shrd_Room_PhotoFrame_Delete: execute failed!";	

			sipinfo("user:%u cancel set data:%u for frame:%u ", FID, dataId, frameId);	
		}	

	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// M_CS_FRAMEDATA_GET
// Get Frame's Photo info [d:FrameId]
void cb_M_CS_FRAMEDATA_GET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_FRAMEID	frameId;	_msgin.serial(frameId);

	T_ROOMID	roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	roomid = itFamily->second.m_RoomID;

	// get the photo frame Info for one room
	MAKEQUERY_GetFrame(s_szQuery, roomid, frameId);
	DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
	if ( ! nPrepareRet )
	{
		sipwarning("Shrd_Room_PhotoFrame_get: prepare statement failed!!");
		return;
	}

	DB_EXE_PARAMQUERY(Stmt, s_szQuery);
	if(!nQueryResult)
	{
		sipwarning("Shrd_Room_PhotoFrame_get: execute failed!");
		return;
	}

	CMessage msgOut(M_SC_FRAMEDATA_GET);
	uint32	dataid = 0, Scale = 0, masterFID = 0;
	uint16	OffsetX = 0, OffsetY = 0;
	uint8	checked = 0;

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if(IS_DB_ROW_VALID(row))
	{
		COLUMN_DIGIT(row,	0, 	uint32, _dataid);
		COLUMN_DIGIT(row,	1,	uint16,	_OffsetX);
		COLUMN_DIGIT(row,	2,	uint16,	_OffsetY);
		COLUMN_DIGIT(row,	3,	uint32,	_Scale);
		COLUMN_DIGIT(row,	4,	uint8, _checked);
		COLUMN_DIGIT(row,   5,  uint32, _masterFID);

		if(!((checked == HISDATA_CHECKFLAG_CHECKED) 
			|| (itFamily->second.m_RoleInRoom == ROOM_MASTER) 
			|| (itFamily->second.m_RoleInRoom == ROOM_MANAGER 
			|| (IsCommonRoom(roomid)))))
			_dataid = 0;

		dataid = _dataid;
		OffsetX = _OffsetX;
		OffsetY = _OffsetY;
		Scale = _Scale;
		checked = _checked;
		masterFID = _masterFID;
	}

	msgOut.serial(FID, frameId, dataid, OffsetX, OffsetY, Scale);

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// Request Photo Album List
void cb_M_CS_PHOTOALBUMLIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	roomid = itFamily->second.m_RoomID;

	sendPhotoAlbumList(FID, roomid, _tsid);
}

// Request Video Group List
void cb_M_CS_VIDEOALBUMLIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	roomid = itFamily->second.m_RoomID;

	sendVideoGrpList(FID, roomid, _tsid);
}

// Add new Photo & Video Album [b:DataType][u:Name][d:AlbumId]
void cb_M_CS_ADDALBUM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);	
	ucstring	grpName;	_msgin.serial(grpName);
	uint32		AlbumID;	_msgin.serial(AlbumID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try addalbum hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// Load
			PhotoAlbumList	AlbumList;
			if(!ReadPhotoAlbum(roomid, &AlbumList))
			{
				sipwarning("ReadPhotoAlbum failed!!");
				return;
			}
			if(AlbumList.DataCount >= MAX_ALBUM_COUNT + 1)
			{
				sipwarning("Photo AlbumList.DataCount overflow!! FID=%d, RoomID=%d", FID, roomid);
				return;
			}

			// Check Data
			if(AlbumID < MAX_DEFAULT_SYSTEMGROUP_ID)
			{
				sipwarning("Invalid AlbumID. AlbumID=%d", AlbumID);
				return;
			}
			for(uint32 i = 0; i < AlbumList.DataCount; i ++)
			{
				if(AlbumList.Datas[i].AlbumName == grpName)
				{
					sipwarning("SameName Exist!!");
					return;
				}
				if(AlbumID == AlbumList.Datas[i].AlbumID)
				{
					sipwarning("Invalid AlbumID. AlbumID=%d", AlbumID);
					return;
				}
			}

			// Insert New Album
			AlbumList.Datas[AlbumList.DataCount].AlbumID = AlbumID;
			wcscpy(AlbumList.Datas[AlbumList.DataCount].AlbumName, grpName.c_str());
			AlbumList.Datas[AlbumList.DataCount].LogoPhotoID = -1;
			AlbumList.DataCount ++;

			// Save
			if(!SavePhotoAlbum(roomid, &AlbumList))
			{
				sipwarning("SavePhotoAlbum failed!!");
				return;
			}
		}
		else if ( data_type == dataMedia ) 
		{
			// Load
			VideoAlbumList	AlbumList;
			if(!ReadVideoAlbum(roomid, &AlbumList))
			{
				sipwarning("ReadVideoAlbum failed!!");
				return;
			}
			if(AlbumList.DataCount >= MAX_ALBUM_COUNT + 2)
			{
				sipwarning("Media AlbumList.DataCount overflow!! FID=%d, RoomID=%d", FID, roomid);
				return;
			}

			// Check Data
			if(AlbumID < MAX_DEFAULT_SYSTEMGROUP_ID)
			{
				sipwarning("Invalid AlbumID. AlbumID=%d", AlbumID);
				return;
			}
			for(uint32 i = 0; i < AlbumList.DataCount; i ++)
			{
				if(AlbumList.Datas[i].AlbumName == grpName)
				{
					sipwarning("SameName Exist!!");
					return;
				}
				if(AlbumID == AlbumList.Datas[i].AlbumID)
				{
					sipwarning("Invalid AlbumID. AlbumID=%d", AlbumID);
					return;
				}
			}

			// Insert New Album
			AlbumList.Datas[AlbumList.DataCount].AlbumID = AlbumID;
			wcscpy(AlbumList.Datas[AlbumList.DataCount].AlbumName, grpName.c_str());
			AlbumList.DataCount ++;

			// Save
			if(!SaveVideoAlbum(roomid, &AlbumList))
			{
				sipwarning("SaveVideoAlbum failed!!");
				return;
			}

		}	
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// Delete Photo & Video Album [b:DataType][d:AlbumID]
void cb_M_CS_DELALBUM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);	
	T_DATAID    AlbumID;	_msgin.serial(AlbumID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID	
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try delalbum hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// Load
			PhotoAlbumList	AlbumList;
			if(!ReadPhotoAlbum(roomid, &AlbumList))
				throw("ReadPhotoAlbum failed!!");

			// AlbumID=1 : Default Album
			if(AlbumID < MAX_DEFAULT_SYSTEMGROUP_ID)
			{
				sipwarning("Invalid AlbumID!! AlbumID=%d", AlbumID);
				return;
			}

			int AlbumIndex;
			for(AlbumIndex = 0; AlbumIndex < (int)AlbumList.DataCount; AlbumIndex ++)
			{
				if(AlbumList.Datas[AlbumIndex].AlbumID == AlbumID)
					break;
			}
			if(AlbumIndex >= (int)AlbumList.DataCount)
			{
				sipwarning("Invalid AlbumID!! AlbumID=%d", AlbumID);
				return;
			}

			for(int i = AlbumIndex + 1; i < (int)AlbumList.DataCount; i ++)
			{
				AlbumList.Datas[i - 1] = AlbumList.Datas[i];
			}
			AlbumList.DataCount --;

			// Save
			if(!SavePhotoAlbum(roomid, &AlbumList))
				throw("SavePhotoAlbum failed!!");

			// Change AlbumID of deleted album's group
			{
				MAKEQUERY_ChangePhotoAlbumIDAfterAlbumDeleted(s_szQuery, roomid, AlbumID);
				DB_EXE_QUERY(DB, Stmt, s_szQuery);
				if ( ! nQueryResult )
					throw "Shrd_PhotoAlbumInfo_delete2: execute failed!";	
			}

			sipinfo("user:%u delete album albumId:%d", FID, AlbumIndex);
		}
		else if ( data_type == dataMedia )
		{
			// Load
			VideoAlbumList	AlbumList;
			if(!ReadVideoAlbum(roomid, &AlbumList))
				throw("ReadVideoAlbum failed!!");

			// AlbumID=1,2 : Default Album
			if(AlbumID < MAX_DEFAULT_SYSTEMGROUP_ID)
			{
				sipwarning("Invalid AlbumID!! AlbumID=%d", AlbumID);
				return;
			}

			int AlbumIndex;
			for(AlbumIndex = 0; AlbumIndex < (int)AlbumList.DataCount; AlbumIndex ++)
			{
				if(AlbumList.Datas[AlbumIndex].AlbumID == AlbumID)
					break;
			}
			if(AlbumIndex >= (int)AlbumList.DataCount)
			{
				sipwarning("Invalid AlbumID!! AlbumID=%d", AlbumID);
				return;
			}

			for(int i = AlbumIndex + 1; i < (int)AlbumList.DataCount; i ++)
			{
				AlbumList.Datas[i - 1] = AlbumList.Datas[i];
			}
			AlbumList.DataCount --;

			// Save
			if(!SaveVideoAlbum(roomid, &AlbumList))
				throw("SaveVideoAlbum failed!!");

			// Change AlbumID of deleted album's group
			{
				MAKEQUERY_ChangeVideoAlbumIDAfterAlbumDeleted(s_szQuery, roomid, AlbumID);
				DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
				if ( ! nPrepareRet )
					throw "Shrd_VideoAlbumInfo_delete2: prepare statement failed!!";		

				SQLLEN	len1 = 0;
				sint32	ret = 0;

				DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

				if ( ! nBindResult )
					throw "Shrd_VideoAlbumInfo_delete2: bind failed!";

				DB_EXE_PARAMQUERY(Stmt, s_szQuery);
				if ( ! nQueryResult || ret == -1)
					throw "Shrd_VideoAlbumInfo_delete2: execute failed!";	
			}

			sipinfo("user:%u delete album albumId:%d", FID, AlbumIndex);
		}
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// Rename Photo & Video Album's name [b:DataType][d:AlbumId][u:Name]
void cb_M_CS_RENALBUM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_DATATYPE	data_type;	_msgin.serial(data_type);
	T_DATAID    AlbumID;	_msgin.serial(AlbumID);
	ucstring	newName;	_msgin.serial(newName);	

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try renalbum hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// Load
			PhotoAlbumList	AlbumList;
			if(!ReadPhotoAlbum(roomid, &AlbumList))
				throw("ReadPhotoAlbum failed!!");

			uint32 uResult = 0;
			int		AlbumIndex = -1;
			for(int i = 0; i < (int)AlbumList.DataCount; i ++)
			{
				if(AlbumList.Datas[i].AlbumID == AlbumID)
					AlbumIndex = i;
				else
				{
					if(AlbumList.Datas[i].AlbumName == newName)
					{
						sipwarning("Same AlbumName exist!!!");
						uResult = E_ALREADY_EXIST;
						break;
					}
				}
			}

			// Save
			if(AlbumIndex >= 0)
			{
				wcscpy(AlbumList.Datas[AlbumIndex].AlbumName, newName.c_str());

				if(!SavePhotoAlbum(roomid, &AlbumList))
					throw("SavePhotoAlbum failed!!");
			}

			sipinfo("user:%u rename album name:%s", FID,  newName.c_str());
		}
		else if ( data_type == dataMedia ) 
		{
			// Load
			VideoAlbumList	AlbumList;
			if(!ReadVideoAlbum(roomid, &AlbumList))
				throw("ReadVideoAlbum failed!!");

			uint32 uResult = 0;
			int		AlbumIndex = -1;
			for(int i = 0; i < (int)AlbumList.DataCount; i ++)
			{
				if(AlbumList.Datas[i].AlbumID == AlbumID)
					AlbumIndex = i;
				else
				{
					if(AlbumList.Datas[i].AlbumName == newName)
					{
						sipwarning("Same Name exist!!!");
						uResult = E_ALREADY_EXIST;
						break;
					}
				}
			}

			// Save
			if(AlbumIndex >= 0)
			{
				wcscpy(AlbumList.Datas[AlbumIndex].AlbumName, newName.c_str());

				if(!SaveVideoAlbum(roomid, &AlbumList))
					throw("SaveVideoAlbum failed!!");
			}

			//sipinfo("user:%u rename album name:%s", FID,  newName.c_str()); byy krs
		}
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// Change Album Index [b:DataType][d:AlbumIndex][d:NewAlbumIndex]
void cb_M_CS_CHANGEALBUMINDEX(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_DATATYPE	data_type;		_msgin.serial(data_type);
	uint8		count;			_msgin.serial(count);
	uint32		AlbumIDs[MAX_ALBUM_COUNT + 1];
	if (count >= MAX_ALBUM_COUNT + 1)
	{
		sipwarning("count >= MAX_ALBUM_COUNT + 1 !!! FID=%d, data_type=%d, count=%d", FID, data_type, count);
		return;
	}
	for(uint8 i = 0; i < count; i ++)
		_msgin.serial(AlbumIDs[i]);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_ROOMID    roomid;
	// Try to find family information from family ID
	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker!!!
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}
	if((itFamily->second.m_RoleInRoom != ROOM_MASTER) && (itFamily->second.m_RoleInRoom != ROOM_MANAGER))
	{
		// Hacker!!!
		sipwarning("Visitor try ChangeAlbumIndex hisdata!!! FID=%d, RoomID=%d", FID, itFamily->second.m_RoomID);
		return;
	}

///*=====================
	if(!itFamily->second.m_bDataManaging)
	{
		sipwarning("You are not in managing! FID=%d", FID);
		return;
	}
//*/

	roomid = itFamily->second.m_RoomID;

	try
	{
		if ( data_type == dataFigure || data_type == dataPhoto )
		{
			// Load
			PhotoAlbumList	AlbumList;
			if(!ReadPhotoAlbum(roomid, &AlbumList))
				throw("ReadPhotoAlbum failed!!");

			if(count != AlbumList.DataCount)
			{
				// Hacker!!!
				sipwarning("AlbumCount Error! RealCount=%d, InputCount=%d", AlbumList.DataCount, count);
				return;
			}

			// Change Album Index
			_PhotoAlbumInfo	data;
			uint32		i, j;
			for(i = 0; i < AlbumList.DataCount; i ++)
			{
				for(j = i; j < AlbumList.DataCount; j ++)
				{
					if(AlbumList.Datas[j].AlbumID == AlbumIDs[i])
						break;
				}
				if(j >= AlbumList.DataCount)
				{
					// Hacker!!!
					sipwarning("AlbumIDList Error! BadAlbumID=%d", AlbumIDs[i]);
					return;
				}
				if(i != j)
				{
					data = AlbumList.Datas[j];
					AlbumList.Datas[j] = AlbumList.Datas[i];
					AlbumList.Datas[i] = data;
				}
			}

			// Save
			if(!SavePhotoAlbum(roomid, &AlbumList))
				throw("SavePhotoAlbum failed!!");
		}
		else if ( data_type == dataMedia ) 
		{
			// Load
			VideoAlbumList	AlbumList;
			if(!ReadVideoAlbum(roomid, &AlbumList))
				throw("ReadVideoAlbum failed!!");

			if(count != AlbumList.DataCount)
			{
				// Hacker!!!
				sipwarning("AlbumCount Error! RealCount=%d, InputCount=%d", AlbumList.DataCount, count);
				return;
			}

			// Change Album Index
			_VideoAlbumInfo	data;
			uint32		i, j;
			for(i = 0; i < AlbumList.DataCount; i ++)
			{
				for(j = i; j < AlbumList.DataCount; j ++)
				{
					if(AlbumList.Datas[j].AlbumID == AlbumIDs[i])
						break;
				}
				if(j >= AlbumList.DataCount)
				{
					// Hacker!!!
					sipwarning("AlbumIDList Error! BadAlbumID=%d", AlbumIDs[i]);
					return;
				}
				if(i != j)
				{
					data = AlbumList.Datas[j];
					AlbumList.Datas[j] = AlbumList.Datas[i];
					AlbumList.Datas[i] = data;
				}
			}

			// Save
			if(!SaveVideoAlbum(roomid, &AlbumList))
				throw("SaveVideoAlbum failed!!");
		}	
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// [d:AuthKey]
void cb_M_WH_CHECK_AUTH_UPLOAD(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	T_DATASERVERID	webid = getDataServerId(from, clientcb);
	if (webid == INVALID_DATASERVERID)
		return;

	T_AUTHKEY	auth_key;	msgin.serial(auth_key);
	//sipdebug("auth_key=%d", auth_key); byy krs

	TOnlineDataServers::iterator its = s_OnlineDataServers.find(webid);
	if (its == s_OnlineDataServers.end())
		return;

	TOnlineDataServer	ds = its->second;
	TUploadConnects &	uploads = ds->m_UploadConnections;
	TUploadConnects::iterator itu = uploads.find(auth_key);
	if (itu == uploads.end())
	{
		sipwarning("Check Failed. auth_key=%d", auth_key);
		CMessage	msgout(M_HW_CHECK_AUTH_UPLOAD);
		uint32	nResultCode = 1, roomid = 0, DataType = 0, filesize = 0, OldDataID = 0;
		msgout.serial(nResultCode, roomid, filesize, OldDataID);
		clientcb.send(msgout, from);
		return;
	}

	//sipdebug("Check Success. auth_key=%d", auth_key); byy krs
	UploadConnect &	uc = itu->second;
	uc.req_time = 0;
	uc.start_time = CTime::getLocalTime();
	CMessage	msgout(M_HW_CHECK_AUTH_UPLOAD);
	uint32	nResultCode = 0;
	uint32	DataType = (uint32)uc.datatype;
	msgout.serial(nResultCode, uc.roomid, DataType, uc.filesize, uc.OldDataID);
	clientcb.send(msgout, from);

	//sipdebug("Upload file info: RoomID=%d, DataType=%d, filesize=%d, OldDataID=%d", uc.roomid, DataType, uc.filesize, uc.OldDataID); byy krs
}

// [d:Result Code][d:AuthKey][d:DataFileSize][d:FileChecksum]
void cb_M_WH_UPLOAD_DONE(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	T_DATASERVERID	webid = getDataServerId(from, clientcb);
	if (webid == INVALID_DATASERVERID)
		return;

	uint32		nResultCode;	msgin.serial(nResultCode);
	T_AUTHKEY	auth_key;		msgin.serial(auth_key);
	uint32		filesize;		msgin.serial(filesize);
	uint32		md5;			msgin.serial(md5);
	//sipdebug("nResultCode=%d, auth_key=%d", nResultCode, auth_key); byy krs

	TOnlineDataServers::iterator its = s_OnlineDataServers.find(webid);
	if (its == s_OnlineDataServers.end())
		return;
	TOnlineDataServer	ds = its->second;

	TUploadConnects &	uploads = ds->m_UploadConnections;
	TUploadConnects::iterator itu = uploads.find(auth_key);
	if (itu == uploads.end())
		return;
	UploadConnect &uc = itu->second;

	T_DATAID	dataid = 0;
	T_FAMILYID	FID = uc.familyid;
	uint32		nCreateTime = 0;
	uint8		checked = 0;

	if (nResultCode != 0)
	{
		sipwarning("Invalid File Upload! Error=%d, auth_key=%d", nResultCode, auth_key);

		// notify upload success to client
		CMessage	msgout(M_SC_UPLOAD_DONE);
		msgout.serial(FID, nResultCode, auth_key);
		CUnifiedNetwork::getInstance()->send(uc.tsid, msgout);

		ds->m_UploadConnections.erase(auth_key);

		// [d:Result Code][d:AuthKey][b:DataType][d:DataID]
		uint8	DataType = 0;
		uint32	DataID = 0;
		CMessage	msgout_ds(M_HW_UPLOAD_DONE);
		msgout_ds.serial(nResultCode, auth_key, DataType, DataID);
		clientcb.send(msgout_ds, from);

		return;
	}

	try
	{
		if (filesize == 0)
			throw "File size is 0";

		T_ROOMID RoomID;

		RoomID = uc.roomid;
		if (uc.datatype != dataMedia)
		{
			// Save Figure & Photo Info
			{
				uint32 albumid = 0;
				if (uc.datatype == dataPhoto)
					albumid = uc.albumid;
				ucstring&	comment = uc.datacomment;
				MAKEQUERY_InsertPhoto(s_szQuery, webid, RoomID, albumid, comment, md5, filesize, uc.datatype);
				DB_EXE_QUERY(DB, Stmt, s_szQuery);
				if (!nQueryResult)
					throw "Shrd_PhotoInfo_Insert: execute failed!";

				DB_QUERY_RESULT_FETCH(Stmt, row);
				if (!IS_DB_ROW_VALID(row))
					throw "Shrd_PhotoInfo_Insert: fetch failed!";

				COLUMN_DIGIT(row, 0, sint32, ret);
				COLUMN_DATETIME(row, 1, creationtime);

				if (ret != -1)
				{
					dataid = ret;
					nCreateTime = getMinutesSince1970(creationtime);

					PhotoInfo temp(comment, md5, filesize, nCreateTime, FID, checked, webid);
					s_photoInfoInAlbum[MakeUINT64(RoomID, dataid)] = temp;
				}
				else
					throw "Shrd_VideoInfo_Insert: execute failed!";
			}

			if (dataid != 0 && uc.datatype == dataFigure)
			{
				uint32	old_photoid = uc.OldDataID;

				// Delete Old DeadFigure
				if (old_photoid != 0)
				{
					MAKEQUERY_DeletePhoto(s_szQuery, RoomID, old_photoid);
					DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
					if (!nPrepareRet)
						throw "Shrd_PhotoInfo_Delete: prepare statement failed!!";

					SQLLEN	len1 = 0;
					int	ret = 0;

					DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

					if (!nBindResult)
						throw "Shrd_PhotoInfo_Delete: bind failed!";

					DB_EXE_PARAMQUERY(Stmt, s_szQuery);
					if (!nQueryResult || ret == -1)
						throw "Shrd_PhotoInfo_Delete: execute failed!";

					s_photoInfoInAlbum.erase(MakeUINT64(RoomID, old_photoid));
				}
			}

			DBLOG_AddPhoto(FID, dataid, filesize, RoomID);
		}
		else if (uc.datatype == dataMedia)
		{
			ucstring&	comment = uc.datacomment;
			uint32		total_time = 0;
			MAKEQUERY_InsertVideo(s_szQuery, webid, RoomID, uc.albumid, comment, md5, filesize, total_time);
			DB_EXE_QUERY(DB, Stmt, s_szQuery);
			if (!nQueryResult)
				throw "Shrd_VideoInfo_Insert: execute failed!";

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
				throw "Shrd_VideoInfo_Insert: fetch failed!";

			COLUMN_DIGIT(row, 0, sint32, ret);
			COLUMN_DATETIME(row, 1, creationtime);

			if (ret != -1)
			{
				dataid = ret;
				nCreateTime = getMinutesSince1970(creationtime);

				VideoInfo temp(comment, md5, filesize, nCreateTime, 0, FID, checked);
				s_videoInfoInAlbum[MakeUINT64(RoomID, dataid)] = temp;

				DBLOG_AddVideo(FID, dataid, filesize, RoomID);
			}
			else
				throw "Shrd_VideoInfo_Insert: execute failed!";
		}
		//sipdebug("upload success: roomid=%d, dataid = %d", RoomID, dataid); byy krs

		// notify upload success to client
		uint32	nResultCode = ERR_NOERR;
		CMessage	msgout(M_SC_UPLOAD_DONE);
		msgout.serial(FID, nResultCode, auth_key, uc.datatype, uc.DeadID, dataid, filesize, md5, nCreateTime);
		msgout.serial(uc.datacomment);
		CUnifiedNetwork::getInstance()->send(uc.tsid, msgout);

		// [d:Result Code][d:AuthKey][b:DataType][d:DataID]
		CMessage	msgout_ds(M_HW_UPLOAD_DONE);
		msgout_ds.serial(nResultCode, auth_key, uc.datatype, dataid);
		clientcb.send(msgout_ds, from);
	}
	catch (char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
		{
			nResultCode = E_DBERR;
			CMessage	msgOut(M_SC_UPLOAD_DONE);
			msgOut.serial(FID, nResultCode, auth_key);
			CUnifiedNetwork::getInstance()->send(uc.tsid, msgOut);
		}

		{
			CMessage	msgout_ds(M_HW_UPLOAD_DONE);
			uint32	nResultCode = 1;
			msgout_ds.serial(nResultCode);
			clientcb.send(msgout_ds, from);
		}
	}

	uploads.erase(itu);
}

// [d:AuthKey]
void cb_M_WH_CHECK_AUTH_DOWNLOAD(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{	
	T_DATASERVERID	webid = getDataServerId(from, clientcb);	
	if (webid == INVALID_DATASERVERID)
		return;

	T_AUTHKEY	auth_key;	msgin.serial(auth_key);
	//sipdebug("auth_key=%d webid=%d", auth_key, webid); byy krs

	TOnlineDataServers::iterator its = s_OnlineDataServers.find(webid);
	if (its == s_OnlineDataServers.end()) {		
		return;
	}		

	TOnlineDataServer	ds = its->second;
	TDownloadConnects &	cons = ds->m_DownloadConnections;
	TDownloadConnects::iterator itu = cons.find(auth_key);
	if (itu == cons.end())
	{
		sipinfo("Invalid File Download! auth_key=%d", auth_key);
		CMessage	msgout(M_HW_CHECK_AUTH_DOWNLOAD);
		uint32	nResultCode = 1, roomid = 0, dataid = 0;
		uint8	datatype = 0;
		msgout.serial(nResultCode, roomid, datatype, dataid, auth_key);
		clientcb.send(msgout, from);
		return;
	}
	
	// [d:Result Code][d:RoomID][b:DataType][d:DataID][d:AuthKey]
	DownloadConnect &	uc = itu->second;
	uc.req_time = 0;
	uc.start_time = CTime::getLocalTime();
	CMessage	msgout(M_HW_CHECK_AUTH_DOWNLOAD);
	uint32	nResultCode = 0;	
	msgout.serial(nResultCode, uc.roomid, uc.datatype, uc.fileid, auth_key);
	clientcb.send(msgout, from);
}

// [d:AuthKey]
void cb_M_WH_CHECK_AUTH_DELETE(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	T_DATASERVERID	webid = getDataServerId(from, clientcb);
	if (webid == INVALID_DATASERVERID)
		return;

	T_AUTHKEY	auth_key;	msgin.serial(auth_key);
	//sipdebug("auth_key=%d", auth_key); byy krs

	TOnlineDataServers::iterator its = s_OnlineDataServers.find(webid);
	if (its == s_OnlineDataServers.end())
		return;

	TOnlineDataServer	ds = its->second;
	TDeleteConnects &	cons = ds->m_DeleteConnections;
	TDeleteConnects::iterator itu = cons.find(auth_key);
	if (itu == cons.end())
	{
		sipwarning("Invalid File Delete! auth_key=%d", auth_key);
		CMessage	msgout(M_HW_CHECK_AUTH_DELETE);
		uint32	nResultCode = 1;
		msgout.serial(nResultCode);
		clientcb.send(msgout, from);
		return;
	}

	uint8	data_type = itu->second.datatype;
	uint32	dataId = itu->second.fileid;
	uint32	roomid = itu->second.roomid;
	uint32	FID = itu->second.familyid;
	TServiceId	_tsid = itu->second.sid;

	ds->m_DeleteConnections.erase(auth_key);

	// [d:Result Code][d:RoomID][b:DataType][d:DataID]
	CMessage	msgout(M_HW_CHECK_AUTH_DELETE);
	uint32	nResultCode = 0;
	msgout.serial(nResultCode, roomid, data_type, dataId);
	clientcb.send(msgout, from);

	// Delete from DB
	try
	{
		if (data_type != dataMedia)
		{
			// delete a photo from a album
			MAKEQUERY_DeletePhoto(s_szQuery, roomid, dataId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if (!nPrepareRet)
				throw "Shrd_PhotoInfo_Delete: prepare statement failed!!";

			SQLLEN len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if (!nBindResult)
				throw "Shrd_PhotoInfo_Delete: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if (!nQueryResult || ret == -1)
				throw "Shrd_PhotoInfo_Delete: execute failed!";

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_DELETE_DONE);
			msgout.serial(FID);
			msgout.serial(uResult, data_type, dataId);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);

			s_photoInfoInAlbum.erase(MakeUINT64(roomid, dataId));

			DBLOG_DelPhoto(FID, dataId, 0, roomid);

			//sipinfo("[M_SC_DELDATA] : user:%u delete photo:%u ", FID, dataId); byy krs
		}
		else if (data_type == dataMedia)
		{
			// delete a video
			MAKEQUERY_DeleteVideo(s_szQuery, roomid, dataId);
			DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
			if (!nPrepareRet)
				throw "Shrd_VideoInfo_Delete: prepare statement failed!!";

			SQLLEN	len1 = 0;
			int	ret = 0;

			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);

			if (!nBindResult)
				throw "Shrd_VideoInfo_Delete: bind failed!";

			DB_EXE_PARAMQUERY(Stmt, s_szQuery);
			if (!nQueryResult || ret == -1)
				throw "Shrd_VideoInfo_Delete: execute failed!";

			uint32 uResult = ERR_NOERR;
			CMessage msgout(M_SC_DELETE_DONE);
			msgout.serial(FID);
			msgout.serial(uResult, data_type, dataId);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);

			s_videoInfoInAlbum.erase(MakeUINT64(roomid, dataId));

			DBLOG_DelVideo(FID, dataId, 0, roomid);

			//sipinfo("[M_SC_DELDATA] : user:%u delete video:%u ", FID, dataId); byy krs
		}
	}
	catch (char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
	}
}

// [d:Result Code][d:AuthKey]
void cb_M_WH_DOWNLOAD_DONE(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	T_DATASERVERID	webid = getDataServerId(from, clientcb);
	if (webid == INVALID_DATASERVERID)
		return;

	uint32	nResultCode;	msgin.serial(nResultCode);
	T_AUTHKEY	auth_key;	msgin.serial(auth_key);
	//sipdebug("nResultCode=%d, auth_key=%d", nResultCode, auth_key); byy krs

	TOnlineDataServers::iterator its = s_OnlineDataServers.find(webid);
	if (its == s_OnlineDataServers.end())
		return;
	TOnlineDataServer	ds = its->second;
	ds->m_DownloadConnections.erase(auth_key);
}

TCallbackItem DSCallbackArray[] =
{
	{ M_WH_CHECK_AUTH_UPLOAD,		cb_M_WH_CHECK_AUTH_UPLOAD },
	{ M_WH_UPLOAD_DONE,				cb_M_WH_UPLOAD_DONE },
	{ M_WH_CHECK_AUTH_DOWNLOAD,		cb_M_WH_CHECK_AUTH_DOWNLOAD },
	{ M_WH_CHECK_AUTH_DELETE,		cb_M_WH_CHECK_AUTH_DELETE },
	{ M_WH_DOWNLOAD_DONE,			cb_M_WH_DOWNLOAD_DONE },
};

// Set FigureFrame's Photo [d:FigureFrameId][b:PhotoType][d:PhotoId1][d:PhotoId2]
void cb_M_CS_FIGUREFRAME_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		PhotoType;
	uint32		FigureFrameID, PhotoID1, PhotoID2;

	_msgin.serial(FID, FigureFrameID, PhotoType, PhotoID1, PhotoID2);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		// Hacker
		sipwarning("There is no room info of family(%d)", FID);
		return;
	}

	// Get RoomId
	uint32	RoomID = itFamily->second.m_RoomID;

	{
		MAKEQUERY_SetFigureFrame(s_szQuery, RoomID, FigureFrameID, PhotoType, PhotoID1, PhotoID2);
		DB_EXE_PREPARESTMT(DB, Stmt, s_szQuery);
		if (!nPrepareRet)
		{
			sipwarning("Shrd_Room_SetFigureFrame: prepare statement failed!!");
			return;
		}

		DB_EXE_PARAMQUERY(Stmt, s_szQuery);
		if (!nQueryResult)
		{
			sipwarning("Shrd_Room_SetFigureFrame: execute failed!");
			return;
		}
	}	
}

bool initDataServers(uint16 port)
{
	{
		MAKEQUERY_LoadDataServers(s_szQuery);
		DB_EXE_QUERY(DB, Stmt, s_szQuery);
		if ( ! nQueryResult )
		{
			siperrornoex("execute Shrd_mstDataServers_List failed!");
			return false;
		}

		while ( true )
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( ! IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row,	0,	uint8,	ServerId);
			COLUMN_STR(row,	1,	ServerUrl,	50);
			COLUMN_DIGIT(row,	2,	uint8,	bPortrait);
			COLUMN_DIGIT(row,	3,	uint8,	bPhoto);
			COLUMN_DIGIT(row,	4,	uint8,	bVideo);

			sipassert(ServerId != INVALID_DATASERVERID);

			std::string	addr = ServerUrl;
			int webport = 80;
			size_t pos = addr.find_first_of (':');
			if (pos != string::npos)
			{
				webport = atoi(addr.substr(pos + 1).c_str());
				addr = addr.substr(0, pos);
			}

			OnlineDataServer *	ds = new OnlineDataServer(ServerId, addr, webport, bPortrait==1, bPhoto==1, bVideo==1);
			s_OnlineDataServers[ServerId] = ds;
			s_DataServerAddrs[addr] = ServerId;
		}

		s_pDataServer = new CCallbackServer();
		s_pDataServer->init(port);
		s_pDataServer->addCallbackArray (DSCallbackArray, sizeof(DSCallbackArray)/sizeof(DSCallbackArray[0]));	
	}

	return true;
}

void updateDataServers()
{
	if ( s_pDataServer )
		s_pDataServer->update();
}

void releaseDataServers()
{
	if ( s_pDataServer )
	{
		delete s_pDataServer;
		s_pDataServer = NULL;
	}

	s_OnlineDataServers.clear();
}

void DBLOG_AddPhoto(T_FAMILYID _FID, uint32 PhotoID, uint32 _byte, uint32 RoomID)
{
	DECLARE_LOG_VARIABLES_HIS();
	MainType = DBLOG_MAINTYPE_ADDPHOTO;
	PID = _FID;
	UID = UserID;
	ItemID = PhotoID;
	VI1 = _byte;
	VI4 = RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	DOING_LOG();
}

void DBLOG_DelPhoto(T_FAMILYID _FID, uint32 PhotoID, uint32 _byte, uint32 RoomID)
{
	DECLARE_LOG_VARIABLES_HIS();
	MainType = DBLOG_MAINTYPE_DELPHOTO;
	PID = _FID;
	UID = UserID;
	ItemID = PhotoID;
	VI1 = _byte;
	VI4 = RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	DOING_LOG();
}

void DBLOG_AddVideo(T_FAMILYID _FID, uint32 VideoID, uint32 _byte, uint32 RoomID)
{
	DECLARE_LOG_VARIABLES_HIS();
	MainType = DBLOG_MAINTYPE_ADDVIDEO;
	PID = _FID;
	UID = UserID;
	ItemID = VideoID;
	VI1 = _byte;
	VI4 = RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	DOING_LOG();
}

void DBLOG_DelVideo(T_FAMILYID _FID, uint32 VideoID, uint32 _byte, uint32 RoomID)
{
	DECLARE_LOG_VARIABLES_HIS();
	MainType = DBLOG_MAINTYPE_DELVIDEO;
	PID = _FID;
	UID = UserID;
	ItemID = VideoID;
	VI1 = _byte;
	VI4 = RoomID;
	PName = L"[" + UserName + L"]" + FamilyName;
	DOING_LOG();
}

/****************************************************************************
* CallbackArray for OpenRoom service and Front-end service
****************************************************************************/
TUnifiedCallbackItem CallbackArray[] =
{
	{ M_NT_ADDTOHIS,		cb_EnterRoom },
	{ M_NT_DELFROMHIS,		cb_LeaveRoom },
	{ M_NT_CHANGEROLE,		cb_ChangeRole },
	{ M_NT_ENTERLOBBY,		cb_EnterLobby },

	{ M_CS_HISMANAGE_START,	cb_M_CS_HISMANAGE_START },
	{ M_CS_HISMANAGE_END,	cb_M_CS_HISMANAGE_END },

	{ M_CS_REQ_UPLOAD,		cb_M_CS_REQ_UPLOAD },
	{ M_CS_REQ_DOWNLOAD,	cb_M_CS_REQ_DOWNLOAD },
	{ M_CS_REQ_DELETE,		cb_M_CS_REQ_DELETE },
	{ M_CS_PLAY_START,		cb_M_CS_PLAY_START },
	{ M_CS_PLAY_STOP,		cb_M_CS_PLAY_STOP },

	{ M_CS_PHOTOALBUMLIST,		cb_M_CS_PHOTOALBUMLIST },
	{ M_CS_VIDEOALBUMLIST,		cb_M_CS_VIDEOALBUMLIST },
	{ M_CS_RENDATA,			cb_M_CS_RENDATA  },
	{ M_CS_ALBUMINFO,		cb_M_CS_ALBUMINFO },
	{ M_CS_ADDALBUM,		cb_M_CS_ADDALBUM },
	{ M_CS_DELALBUM,		cb_M_CS_DELALBUM },
	{ M_CS_RENALBUM,		cb_M_CS_RENALBUM },
	{ M_CS_CHANGEALBUMINDEX,cb_M_CS_CHANGEALBUMINDEX },
	{ M_CS_MOVDATA,			cb_M_CS_MOVDATA	},
	{ M_CS_CHANGEVIDEOINDEX,cb_M_CS_CHANGEVIDEOINDEX },
	{ M_CS_FRAMEDATA_SET,	cb_M_CS_FRAMEDATA_SET },
	{ M_CS_FRAMEDATA_GET,	cb_M_CS_FRAMEDATA_GET },
	{ M_CS_REQDATA_PHOTO,	cb_M_CS_REQDATA_PHOTO	},
	{ M_CS_REQDATA_VIDEO,	cb_M_CS_REQDATA_VIDEO	},

	{ M_CS_FIGUREFRAME_SET,	cb_M_CS_FIGUREFRAME_SET },	

	{ M_W_SHARDCODE,			cb_M_W_SHARDCODE },
};

void onWSConnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	// Get ShardCode
	CMessage	msgout(M_W_REQSHARDCODE);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

/****************************************************************************
* CHisManagerService class
****************************************************************************/
class CHisManagerService : public IService
{
public:
	
	void init()
	{
		// Lang data
		{
			if(!LoadLangData(CPath::getExePathW()))
			{
				siperrornoex("LoadLangData failed.");
				return;
			}
		}

		// Connect to Database
		{
			string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Shard_DBName").asString ();
			string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Shard_DBHost").asString ();
			string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Shard_DBLogin").asString ();
			string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Shard_DBPassword").asString ();
			BOOL bSuccess = DB.Connect(	ucstring(strDatabaseLogin).c_str(),
											ucstring(strDatabasePassword).c_str(),
											ucstring(strDatabaseHost).c_str(),
											ucstring(strDatabaseName).c_str());
			if (!bSuccess)
			{
				siperrornoex("Shard Database connection failed to '%s' with login '%s' and database name '%s'",
					strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
				return;
			}
			sipinfo("Shard Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());

		}

		CSock::initNetwork();

		uint16 wPort = s_wDefaultHSPort;
		if (IService::getInstance()->ConfigFile.exists(s_sHSPortConfig))
			wPort = IService::getInstance()->ConfigFile.getVar(s_sHSPortConfig).asInt();

		srand((uint32)CTime::getLocalTime());

		initDataServers(wPort);

		CUnifiedNetwork::getInstance()->setServiceUpCallback(WELCOME_SHORT_NAME, onWSConnection, NULL);
	}

	bool update ()
	{
		CheckUploadDownloadDeleteList();
		updateDataServers();
		return true;
	}

	void release()
	{
		releaseDataServers();
		DB.Disconnect();
		CSock::releaseNetwork();
	}
private:
};

SIPNET_SERVICE_MAIN (CHisManagerService, HISMANAGER_SHORT_NAME, HISMANAGER_LONG_NAME, "", 0, false, CallbackArray, "", "")

/****************************************************************************
* Commands
****************************************************************************/

SIPBASE_CATEGORISED_COMMAND(HIS_S, getRoomIDAndKey, "Get room key of family", "FamilyID")
{
	if(args.size() < 1)	return false;

	string familyId	= args[0];
	T_FAMILYID FID = atoui(familyId.c_str());

	IFNOTFIND_INROOMFAMILYS(itFamily, FID)
	{
		log.displayNL ("There is no room info of family(%d)", FID);
		return true;
	}
	log.displayNL ("RoomID=%d, of family(%d)", itFamily->second.m_RoomID, FID);
	return true;
}
