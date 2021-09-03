
#ifndef TIANGUO_MACRO_H
#define TIANGUO_MACRO_H

// DB조작실패
#define	COMMON_SUCCESS	0
#define DB_ERROR		100

#define SYSTEMUSERIS_START	1
#define SYSTEMUSERIS_END	7000
#define SYSTEMROOM_START	1
#define SYSTEMROOM_END		1000

// Check-in state
enum CheckInState
{
	CIS_CANT_CHECK	= -2,	// 통신파케트에서는 0xfe
	CIS_MAX_DAYS	= 7
};

// 가족명중복검사결과
enum
{
	FAMILYNAMECHECK_OK = 0,
	FAMILYNAMECHECK_ISTHERE,
};

// 고인등록결과
enum
{
	DBR_INVALID_FULL_DEAD =1,				// 고인을 더 모실수 없다.
};

// Family search mode
enum	_FAMILY_SEARCH_MODE
{
	FSM_NONE		= -1,
	FSM_FAMILYID	= 0,
	FSM_FAMILYNAME	= 1
};

// 방탐색기발
enum	_FROOMFIND{
	FIND_NONE	= -1,
	FIND_ALL	= 0,
	FIND_MASTER	= 1,
	FIND_DEAD,
	FIND_CHARACTER,
	FIND_ROOMID,
	FIND_ROOMNAME
};

// 방탐색방법
enum	_FROOMFINDMETHOD{
	FIND_CORRECT,
	FIND_FUZZY
};

enum	_ROOMSTATE{
	ROOMSTATE_NORMAL = 0,	// 보통
	ROOMSTATE_CLOSED,		// 방동결
};

enum	_TALKMODE {
	TALK_VIDEO = 1,
	TALK_AUDIO
};

// 지진추모방종류
enum	_MROOMKIND{
	MROOMKIND_MASS,
	MROOMKIND_PERSON
};

// 방형상종류
enum	_SCENEKIND
{
	SCENE_GANEN_FREE = 6,
	SCENE_GANEN_NOFREE = 7,
	SCENE_RUYI = 21,
	SCENE_WANSHOU = 22,
	SCENE_JISHANG = 23,
	SCENE_FUGUI = 24
};
// Contact
enum	_CONTACTTYPE {
	CONTACTTYPE_NONE = 0,
	CONTACTTYPE_START = 1,
	CONTACTTYPE_FRIEND = CONTACTTYPE_START,		// 친구
	CONTACTTYPE_RELATIVE,						// 친인
	CONTACTTYPE_USER = 4,						// User정의그룹
	CONTACTTYPE_STRANGER,						// 초면의 련계인
	CONTACTTYPE_BLOCK = 10,						// 차단객
	CONTACTTYPE_END = CONTACTTYPE_BLOCK	
};

enum	_CONTACTSTATE {
	CONTACTSTATE_OFFLINE,
	CONTACTSTATE_ONLINE,
	CONTACTSTATE_DIFFERENT,
	CONTACTSTATE_WAITREPLY = CONTACTSTATE_OFFLINE,
	CONTACTSTATE_REFUSE = CONTACTSTATE_WAITREPLY,
};

enum _ROOMMANAGER_ANS
{
	ROOMMANAGER_ANS_YES = 1,
	ROOMMANAGER_ANS_NO,
	ROOMMANAGER_ANS_DELETE
};

enum _ROOMMANAGER_DEL_REASON
{
	ROOMMANAGER_DEL_REASON_OWNERDEL = 1,
	ROOMMANAGER_DEL_REASON_REMOVEFRIENDLIST,
	ROOMMANAGER_DEL_REASON_ROOMMASTERCHANGED
};

// Chit
// 주의: 이 자료는 DB에 기록되여있기때문에 수정불가능하다!!!
enum	_CHITTYPE{
	CHITTYPE_NONE		= 0,

	CHITTYPE_NEWMAIL	= 1,		// 행사초청쪽지
	CHITTYPE_CONTACT	= 2,		// 련계인추가통지( p1:ModelID, p2:<>, p3:<RequestFamilyName>, p4<> )
	CHITTYPE_CHROOMMASTER = 3,		// 방양도( p1:방id, p2:방typeid, p3:방이름 )
	CHITTYPE_CHROOMMASTER_OK = 4,	// 방양도접수( p1:방id, p2:방typeid, p3:방이름 )
	CHITTYPE_CHROOMMASTER_NO = 5,	// 방양도거절( p1:방id, p2:방typeid, p3:방이름 )
	CHITTYPE_ROOM_DELETED = 6,		// 무료체험방이 3달동안 사용되지 않아 자동삭제됨. ( p1:방id, p3:방이름 )
	CHITTYPE_AUTOPLAY3_FAIL = 7,	// 예약의식 진행 실패! ( p1:방id, p2:Reason(0:성공, 1:방열기실패,2:방시간초과), p3:방이름 )
	CHITTYPE_INVITEROOM = 8,
	CHITTYPE_TIP = 9,
	CHITTYPE_HELP = 10,
	CHITTYPE_FESTIVAL = 11,
	CHITTYPE_GIFTBOX = 12,
	CHITTYPE_WAIT = 13,
	CHITTYPE_UPLOADVIDEO = 14,
	CHITTYPE_MANAGER_ADD = 15,		// 방관리자설정 (p1:RoomID, p3:RoomName)
	CHITTYPE_MANAGER_ANS = 16,		// 방관리자설정응답 (p1:RoomID, p2:Answer(1-OK,2-Refuse,3-Delete), p3:RoomName)
	CHITTYPE_MANAGER_DEL = 17,		// 방관리자삭제 (p1:RoomID, p2:Reason(1-OwnerDel,2-RemoveFriendList,3-RoomMasterChanged), p3:RoomName)
	CHITTYPE_MULTIRITE_REQ = 18,	// 단체행사 요청 - 클라이언트 사용
	CHITTYPE_GIVEROOM_RESULT = 19,	// 방양도를 하였을 때 대방의 응답결과를 통지한다. (p1:RoomID, p2:Answer(0:No,1:Yes,2:Timeout) | (SceneID << 16), p3:RoomName)
	//클라이언트에서 자체목적을 위해 사용
	CHITTYPE_QUERY_GMANS		= 20,	// GM응답표시.
	CHITTYPE_ONE_WORD			= 21,	// for everyday-one-word.
	CHITTYPE_NOTICE_IMG			= 22,	// for notice image
	CHITTYPE_BLESS_CARD			= 50,	// 축복카드 (p1:0(카드목록), 1(얻기), 2(보내기), 3(받기), p2:카드id)
	CHITTYPE_LARGEACT_PREPARE	= 60,	// 대형행사 준비상태표시
	CHITTYPE_LARGEACT_GET_ITEM	= 61,	// 대형행사후 얻은 아이템
	CHITTYPE_LARGEACT_VIPREQ	= 62,	// 대형행사 귀빈행사신청
	CHITTYPE_LARGEACT_ENDTIP    = 63,   // 대형행사후 체계통보표시
	CHITTYPE_MULTIACT_CANCEL	= 64,	// 단체행사 취소통보표시
	CHITTYPE_ONLY_FOR_CLIENT	= 100,

	CHITTYPE_TALK,				// 화상음성챠팅요청
};

enum	_CHITRES{
	CHITRES_YES = 1,				// 예
	CHITRES_NO,						// 아니
};

enum	ENUM_SHARDSTATEDEFINE
{
	SHARDSTATE_GOOD = 4,
	SHARDSTATE_BUSY = 6,
	SHARDSTATE_FULL = 10,
};

// Lobby state
enum	ENUM_LOBBYSTATE
{
	LOBBYSTATE_GOOD = 30,
	LOBBYSTATE_NORMAL = 80,
	LOBBYSTATE_FULL = 100,
	LOBBYSTATE_CLOSE = 200,
};

// 체계행사 종류
enum	_SYSEVENT {
	SYSEVENT_NEWYEAR = 1,
	SYSEVENT_LUNARNEWYEAR,
	SYSEVENT_15THJANUARY,
	SYSEVENT_HANSIK,
	SYSEVENT_DANOH,
	SYSEVENT_CHUSIK
};

// 캐릭터보임특별속성
enum _CHARACTERSHOWTYPE
{
	CHSHOW_GENERAL = 0,		// 일반 보임속성
	CHSHOW_GM,				// GM표시
	CHSHOW_HIDE,			// 캐릭터감추기
};

// 응답
enum	_REPLY{
	REPLY_NO,
	REPLY_YES
};

// 불법강도
enum	{
	UNLAW_REMARK,						// 주목
	UNLAW_WARNING,						// 경고
	UNLAW_RESTRAINT,					// 제재
	UNLAW_DELETE,						// 완전삭제
};

// 기념관공개속성
enum TRoomAllowType	{
	ROOMOPEN_TOALL = 1,					// 모든 캐릭터들에게 공개
	ROOMOPEN_TONEIGHBOR,				// 련계인들에게 공개
	ROOMOPEN_TONONE,					// 공개하지 않는다
	ROOMOPEN_PASSWORD,					// Password check
	ROOMOPEN_FENGTING,					// 방 fengting
};

// 기념관관리속성
enum TRoomAuthority {
	ROOM_NONE = 0,
	ROOM_MASTER,
	ROOM_MANAGER,
	ROOM_CONTACT,
	ROOM_ALL, 
	ROOM_GM = 0x80
};

// 성별구분
enum	{ 
	SEX_MAN = 1,						// 남자
	SEX_WOMAN							// 녀자
};

// 온라인관계
enum	{
	CH_ONLINE,							// 온라인
	CH_OFFLINE,							// 오프라인
};


// 나이구분
//enum	{ AGE_CHILD = 1, AGE_YOUTH, AGE_MIDDLE, AGE_OLD };	// 소년, 청년, 중년, 로년

// 道具类型 Item type
enum	TItemType {
	ITEMTYPE_NONE				= 0,
	ITEMTYPE_COMMON				= 1,
	ITEMTYPE_ORNAMENT			= 2,	// 纪念馆控件, 기념관부품
	ITEMTYPE_PET_TOOL			= 3,	// 饲养道具, 페트사양도구
	ITEMTYPE_PET				= 4,	// 宠物, 페트
	ITEMTYPE_MUSIC				= 5,	// 音乐, 음악
	ITEMTYPE_ACTDELEGATE		= 6,	// 仪式特使道具, 의식특사
	ITEMTYPE_XUYUNQIAN			= 9,	// 许愿签, 소원막대기
	ITEMTYPE_DRESS				= 10,	// 服装, 복장
	ITEMTYPE_SACRIF_JINGXIAN	= 20,	// 敬献品, 일반행사용 물품
	ITEMTYPE_SACRIF_JISIWUPIN	= 21,	// 祭祀物品
	ITEMTYPE_SACRIF_XIANBAO		= 23,	// 献宝物品, 물건태우기용 제물
	ITEMTYPE_SACRIF_HUALAN		= 24,	// 花篮, 꽃바구니
	ITEMTYPE_ACTRESULT			= 25,	// 仪式结果物, 행사후에 나타나는 결과물
	ITEMTYPE_GUANSHANGYU		= 26,	// 观赏鱼, 어항속의 물고기
	ITEMTYPE_AUTORITE			= 28,	// 智能进入道具, 자동진입 물품

	ITEMTYPE_OTHER				= 50,	// 其他道具, 기타
	ITEMTYPE_OTHER_LARGERITE	= 51,	// 大型仪式道具, 대형의식도구

	ITEMTYPE_MAX				= 100
};

// 敬献品类型
enum SACRIF_TYPE {
	SACRFTYPE_XIANG = 1,			// 敬献品-香
	SACRFTYPE_HUA,					// 敬献品-花
	SACRFTYPE_JIU,					// 敬献品-酒
	SACRFTYPE_FRUIT,				// 敬献品-水果
	SACRFTYPE_CAKE,					// 敬献品-糕点
	SACRFTYPE_TAOCAN_BUDDISM,		// 敬献品-套餐-佛教
	SACRFTYPE_TAOCAN_ROOM,			// 敬献品-套餐-房间
	SACRFTYPE_TAOCAN_XIANBAO,		// 敬献品-套餐-献宝
	SACRFTYPE_TAOCAN_BUDSYS,		// 敬献品-套餐-佛教-体系
	SACRFTYPE_WATER,				// 敬献品-圣水, only used on client side by now
	SACRFTYPE_TAOCAN_ROOM_XDG       // 敬献品-套餐-房间_献大供
};

// 宠物类型
enum PET_TYPE {
	PETTYPE_FLOWER		= 1,		// 花草
	PETTYPE_TREE		= 2,		// 树
	PETTYPE_FISH		= 3,		// 鱼
	PETTYPE_FISHGROUP	= 4,		// 鱼群
	PETTYPE_OTHER		= 101		// 其他动物
};

#define FLAG_MOVESTATE_NOWAIT	0x8000
// 방에서 캐릭터의 동작상태
// 角色的移动状态
enum	TMoveState {
	moveStand,		// 정지상태
	moveWalking,	// 일반걷기중
	moveTurning,	// 회전하고 있음.
	moveRunning,	// 미끄러져이동중.
	moveFlying,		// 날기중
	movePut,		// 해당위치에 세우기.
	moveBack,		// 뒤로걷기
	moveStateCount,
};

// 宠物/动物的自由移动类型
enum TAnimalMoveType {
	AM_FIXED	= 1,	// 原地不动
	AM_FREE		= 2,	// 自由移动
	AM_PATH		= 3		// 沿着路径移动
};

// 宠物/动物的服务器控制类型
enum TServerControlType {
	SCT_NO		= 0,	// 由客户端控制
	SCT_YES		= 1,	// 由服务端控制其移动
	SCT_ESCAPE	= 40	// 由服务端控制其移动和逃跑
};

// 사적자료종류 - 수값을 직접 쓰는 부분이 있기때문에 앞으로 수정불가능하다.
enum TDataType
{
	dataFigure					= 1,		// 초상사진		肖像 ==========used in server===========
	dataPhoto					= 2,		// 사적사진		图片 ==========used in server===========
	dataMedia					= 3,		// 사적동영상	影音 ==========used in server===========

	dataMonument				= 4,		// 비문			碑文
	dataWish					= 5,		// 소원			许愿
//	dataDecedentName			= 6,		// 고인이름		升天者姓名。已删除了此类型，因为升天者姓名的表现是较复杂且很多，只能是单独处理。
	dataPhotoThumbnail			= 7,
	dataRecommendRoomFigure		= 8,		// 열점추천방기능에서 리용되는 사진, 방ID가 ROOM_ID_VIRTUAL인 방에만 있다. ==========used in server===========
	dataVirtue					= 9,		// 불교구역에서 공덕방 功德榜
	dataRoomTitlePhoto			= 10,		// 방의 타이틀그림, 체험방에만 있다. ==========used in server===========
	dataVisitorList				= 11,		// The list of the visitors.
	dataAncestorText			= 12,		// Ancestor text
	dataFamilyFigure			= 13,		// Family Figure Photo ==========used in server===========
	dataFrameFigure				= 14,		// Room's FigureFrame Photo (max count is defined as 'MAX_FRAMEFIGURE_COUNT') ==========used in server===========
	dataFamilyFaceModel			= 15,		
	dataPublicroomFigure		= 25,		// PublicRoom's FigureFrame Photo (>= dataFrameFigure+MAX_FRAMEFIGURE_COUNT) ==========used in server===========
	dataCustomFace				= 26,		// Custom face model(zip file of .mesh + .dds)
	dataAncestorFigure_start	= 101,		// start value of the Ancestor's figure ( max count is defined as 'MAX_ANCESTOR_DECEASED_COUNT' ) ==========used in server===========
};

enum TDownloadFileState
{
	DFILE_PROBLEM_XMLDATA,
	DFILE_NO_EXIST,
	DFILE_NO_COMPLETE,
	DFILE_COMPLETE,
};

// 망소속구분
enum TNetworkType
{
	NETWORK_UNKNOWN = 0,
	NETWORK_WANGTONG,
	NETWORK_DIANXIN
};

// 기념관에서 선로의 상태
enum TRoomChannelStatus
{
	ROOMCHANNEL_SMOOTH = 1,
	ROOMCHANNEL_COMPLEX,
	ROOMCHANNEL_FULL
};

// 문자렬길이 제한
// common
#define		MAX_LEN_DATE					32

// Shard DB Data
#define		MAX_LEN_FAMILY_NAME				8					// 가족명길이 최대값(unicode글자수)
#define		MAX_LEN_CHARACTER_NAME			8					// 캐릭터이름길이 최대값(unicode글자수)
#define		MAX_LEN_CHARACTER_ID			4					// the length of the charactor ID
#define		MAX_LEN_FAMILYUSER_NAME			8					// 가족창건자명길이 최대값(unicode글자수)
#define		MAX_LEN_FAMILYCOMMENT			160					// 가족창건자설명길이 최대값(unicode글자수)

// 친구
#define		MAX_LEN_CONTACT_GRPNAME			6					// 련계인모임명의 최대길이(unicode)
#define		MAX_CONTGRPNUM					20					// 친구그룹 최대개수, 흑명단그룹까지 최대+1개이다.
#define		MAX_CONTACT_COUNT				50					// 한그룹당 친구 최대수
#define		MAX_CONTGRP_BUF_SIZE			600					// 친구그룹자료 Buf 최대길이

// Dead Data
#define		MAX_LEN_DEAD_NAME				8					// 고인이름길이 최대값(unicode글자수)
#define		MAX_LEN_DEAD_SURNAME			2					// 고인이름 성길이 최대값(unicode글자수)
#define		MAX_LEN_DEAD_GIVENNAME			3					// 고인이름 이름길이 최대값(unicode글자수)
#define		MAX_LEN_DOMICILE				12					// birth place
#define		MAX_LEN_NATION					12					// 민족명 최대길이
#define		MAX_LEN_DEAD_BFHISTORY			1000				// 고인해설문길이 최대값(unicode글자수)
#define		MAX_LEN_DEAD_HIS_TITLE			25					// 고인리력 제목 최대값(unicode글자수)
#define		MIN_LEN_DEAD_HIS_TITLE			10					// the min length of decedent story title
#define		MAX_NUM_DEAD_HIS_STORY	        30					// the max number of decedent story

#define		MAX_LEN_DEAD_HIS_CONTENT		1000				// 고인리력 내용 최대값(unicode글자수)
#define		MAX_LEN_MROOM_NAME				16
#define		MAX_LEN_MROOM_RELATION			16
#define		MAX_LEN_MROOM_ORIGIN			16
#define		MAX_LEN_MROOM_BFHISTORY			256
#define		MAX_LEN_MROOM_MEMONOTE			256
#define		MAX(a,b)	((a>b)	? a : b)
#define		MAX_LEN_ROOM_FINDSTRING			10					// MAX(MAX_LEN_DEAD_NAME, MAX(MAX_LEN_FAMILY_NAME, MAX(MAX_LEN_CHARACTER_NAME, MAX_LEN_ROOM_NAME)))
#define		MAX_LEN_ROOM_NAME				8					// 기념관명칭길이 최대값(unicode글자수)
#define		MAX_LEN_ROOM_ID					8					// the length of the room ID
#define		MAX_LEN_ROOM_COMMENT			256					// 기념관설명문길이 최대값(unicode글자수)
//#define		MAX_ROOM_MEMORIAL_COUNT			10					// 한 사람이 하루에 한방에 쓸수 있는 방문록 최대개수
#define		MAX_LEN_ROOM_MEMORIAL			150					// 기념관방문록길이 최대값(unicode글자수)
#define		MAX_LEN_ROOM_TOMB				256					// 기념관비석길이 최대값(unicode글자수)
#define		MAX_LEN_RUYIBEI					150					// ruyibei길이 최대값(unicode글자수)

#define		MAX_LEN_CHAT					100					// 채팅문자렬의 최대길이
#define		COMMON_CHAT_DISTANCE			200					// 일반챠팅 보임거리
#define		ANIMAL_ESCAPE_DISTANCE			150					// 사람이 접근할때 동물이 도망치는 접근거리

#define		MAX_LEN_WISH_TARGET				5					// 소원대상이름 최대길이(유니코드)
#define		MAX_LEN_WISH					50					// 소원최대길이

#define		MAX_LEN_ROOM_PRAYTEXT			600					// the length of the pray text

#define		MAX_CHARACNUM_FOR_ONESHARD		5					// 한 계정이 한 지역싸이트에 창조할수 있는 최대 캐릭수
#define		MAX_INVEN_NUM					108					// 인벤최대개수
#define		MAX_INVENBUF_SIZE				(MAX_INVEN_NUM+1)*20
#define		MAX_NUM_PER_INVEN_CELL			10
#define     MAX_RESUME_NUM                  160

// Main DB Data
#define		MAX_LEN_APPNAME					50
#define		MAX_LEN_LASTVERSION				32
#define		MAX_LEN_UPDATE_SRCVER			32
#define		MAX_LEN_UPDATE_DSTVER			32
#define		MAX_LEN_FTPURI_ALIASNAME		16
#define		MAX_LEN_FTPURI_SERVERURI		64
#define		MAX_LEN_FTPURI_ID				64
#define		MAX_LEN_FTPURI_PASSWORD			64

#define		MIN_LEN_USER_ID					1
#define		MAX_LEN_USER_ID					16
#define		MAX_LEN_USER_PASSWORD			32
#define		MAX_LEN_ZONE_NAME				16
#define		MAX_LEN_SHARD_CODELIST			300

#define		MAX_LEN_MONEYSTRING				20
#define		MAX_LEN_GMIP_ADDR				20

#define		MAX_LEN_SHARD_NAME				64
#define		MAX_LEN_SHARD_VERSION			8
#define		MAX_LEN_SHARD_CONTENT			128
#define		MAX_LEN_SHARD_PATCHURI			32
#define		MAX_LEN_SHARD_CODES				300

#define		INVEN_KIND_MAX					1000

#define 	ROOM_LEVEL_MAX					81		// 1 - 81
#define 	FAMILY_LEVEL_MAX				199		// 1 - 199
#define 	FISH_LEVEL_MAX					37		// 1 - 37 : 이 값을 수정하면 ServerInfo.xml의 [养鱼等级信息]쉬트, MiscInfo.xml의 [养鱼等级信息]쉬트도 같이 수정해야 한다.
//#define 	ROOMFISH_LEVEL_MAX				10		// 1 - 10
//#define		FISH_INDEX_MAX					16		// 0 - 15

// Festival
#define		MAX_LEN_FESTIVAL_NAME			25					// unicode count
#define		MAX_FESTIVALNUM					300
#define		TYPE_USERFESTIVAL				200

// Letter
#define		MAX_LEN_LETTER_TITLE			16
#define		MAX_LEN_LETTER_CONTENT			1500
#define		LETTER_PAGE_SIZE				12

// Actpos in Room
#define		MAX_MEMO_DESKS_COUNT			4			// 한 기념관에 방문록을 남기는 책상 최대개수
#define		MAX_ANCESTOR_DECEASED_COUNT		50			// 조상탑에 모시는 고인 최대수
#define		MAX_BLESS_LAMP_COUNT			20			// 한 기념관에 남아있는 길상등 최대개수
#define		MAX_FRAMEFIGURE_COUNT			10			// 한 기념관에 있을수 있는 최대병풍개수
enum ENUM_ACTPOS {
	ACTPOS_NONE = 0xff,

	ACTPOS_YISHI = 1,
	ACTPOS_WRITELETTER,
	ACTPOS_XIANBAO,
	ACTPOS_XINGLI_1,
	ACTPOS_XINGLI_2,
	ACTPOS_WRITEMEMO_START,
	ACTPOS_ANCESTOR_JISI = ACTPOS_WRITEMEMO_START + MAX_MEMO_DESKS_COUNT,
	ACTPOS_ANCESTOR_DECEASED_START,
	ACTPOS_ROOMSTORE = ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT,
	ACTPOS_BLESS_LAMP,
	ACTPOS_MULTILANTERN = ACTPOS_BLESS_LAMP + MAX_BLESS_LAMP_COUNT,
	ACTPOS_AUTO2,		// 천원특사를 위한 가상적인 ActPos이다. 실지 ActPos는 ACTPOS_YISHI이다.
	ACTPOS_TOUXIANG_1,
	ACTPOS_TOUXIANG_2,
	ACTPOS_TOUXIANG_3,
	ACTPOS_TOUXIANG_4,
	ACTPOS_TOUXIANG_5,
	ACTPOS_RUYIBEI,
	COUNT_ACTPOS,
};

// action types in the room
enum TActionType
{
	AT_VISIT,			// just enter the room, do nothing
	AT_SIMPLERITE,		// do simple rite in the room
	AT_BURNING,			// do burning in the room
	AT_MULTIRITE,		// do multi rite in the room
	AT_ANCESTOR_JISI,
	AT_ANCESTOR_MULTIJISI,
	AT_ANCESTOR_DECEASED_XIANG,
	AT_ANCESTOR_DECEASED_HUA = AT_ANCESTOR_DECEASED_XIANG + MAX_ANCESTOR_DECEASED_COUNT,
	AT_MULTILANTERN = AT_ANCESTOR_DECEASED_HUA + MAX_ANCESTOR_DECEASED_COUNT,
	AT_AUTO2,			// 천원특사
	AT_PRAYRITE,
	AT_PRAYMULTIRITE,
	AT_PRAYXIANG,
	AT_XDGRITE,
	AT_XDGMULTIRITE,
	AT_PUBLUCK1,
	AT_PUBLUCK2,
	AT_PUBLUCK3,
	AT_PUBLUCK4,
	AT_PUBLUCK1_MULTIRITE,
	AT_PUBLUCK2_MULTIRITE,
	AT_PUBLUCK3_MULTIRITE,
	AT_PUBLUCK4_MULTIRITE,
	AT_TIANYUANXIANG,
	AT_RUYIBEI,			// Ruyibei행사
	AT_MUSICRITE,		// 음악드리기
	AT_COUNT
};

// FishFlower Action in the room + Letter / BlessLamp / RoomStore
enum FF_TYPE
{
	FF_FISH_FOOD = 1,
	FF_FLOWER_WATER = 2,
	FF_FLOWER_NEW = 3,
	FF_LETTER = 4,
	FF_BLESSLAMP = 5,
	FF_ROOMSTORE = 6,
	FF_GOLDFISH = 7,
	FF_XINGLI = 8,
	FF_LUCK = 9,
};

// Actpos in Religion 0 = RROOM_BUDDHISM
enum ENUM_ACTPOS_R0
{
	ACTPOS_R0_NONE = 0xff,

	ACTPOS_R0_XINGLI_1 = 0,	// 이 값은 현재 사용되지 않는다. 만일 사용된다면 1부터 시작하는것으로 수정해야 한다.
	ACTPOS_R0_TAOCAN_1,			// 사용안함
	ACTPOS_R0_HUA_1,			// 사용안함
	ACTPOS_R0_XIANG_1,			// 사용안함
	ACTPOS_R0_XINGLI_2,			// 사용안함
	ACTPOS_R0_TAOCAN_2,			// 사용안함
	ACTPOS_R0_HUA_2,			// 사용안함
	ACTPOS_R0_XIANG_2,			// 사용안함
	ACTPOS_R0_XINGLI_3,			// 사용안함
	ACTPOS_R0_XIANG_3,			// 사용안함
	ACTPOS_R0_YISHI_1,			// 불교구역 의식위치1
	ACTPOS_R0_YISHI_2,			// 불교구역 의식위치2
	ACTPOS_R0_YISHI_3,			// 불교구역 의식위치3
	ACTPOS_R0_YISHI_4,			// 불교구역 의식위치4
	ACTPOS_R0_YISHI_5,			// 불교구역 의식위치5
	ACTPOS_R0_YISHI_6,			// 불교구역 의식위치6
	ACTPOS_R0_XIANG_INDOOR_1,	// 실내향드리기위치1
	ACTPOS_R0_XIANG_INDOOR_2,	// 실내향드리기위치2
	ACTPOS_R0_XIANG_INDOOR_3,	// 실내향드리기위치3
	ACTPOS_R0_XIANG_INDOOR_4,	// 실내향드리기위치4
	ACTPOS_R0_XIANG_INDOOR_5,	// 실내향드리기위치5
	ACTPOS_R0_XIANG_INDOOR_6,	// 실내향드리기위치6
	ACTPOS_R0_XIANG_INDOOR_7,	// 사용안함
	ACTPOS_R0_XIANG_INDOOR_8,	// 사용안함
	ACTPOS_R0_XIANG_INDOOR_9,	// 사용안함
	ACTPOS_R0_XIANG_INDOOR_10,	// 사용안함
	ACTPOS_R0_XIANG_OUTDOOR_1,	// 야외향드리기위치1
	ACTPOS_R0_XIANG_OUTDOOR_2,	// 야외향드리기위치2
	ACTPOS_R0_XIANG_OUTDOOR_3,	// 야외향드리기위치3
	ACTPOS_R0_XIANG_OUTDOOR_4,	// 야외향드리기위치4
	ACTPOS_R0_XIANG_OUTDOOR_5,	// 야외향드리기위치5
	ACTPOS_R0_XIANG_OUTDOOR_6,	// 야외향드리기위치6
	ACTPOS_R0_XIANG_OUTDOOR_7,	// 야외향드리기위치7
	ACTPOS_R0_XIANG_OUTDOOR_8,	// 사용안함
	ACTPOS_R0_XIANG_OUTDOOR_9,	// 사용안함
	ACTPOS_R0_XIANG_OUTDOOR_10,	// 사용안함
	COUNT_R0_ACTPOS
};

// Actpos in Public Room = RROOM_PUBLIC
enum ENUM_ACTPOS_PUB
{
	ACTPOS_PUB_NONE = 0xff,

	ACTPOS_PUB_YISHI_1 = 1,		// 땅의식
	ACTPOS_PUB_YISHI_2,			// 하늘의식
	ACTPOS_PUB_LUCK,			// 칭선타이의식
	COUNT_PUBLIC_ACTPOS
};

enum ENUM_ACTPOS_KONGZI
{
	ACTPOS_KONGZI_NONE = 0xff,

	ACTPOS_KONGZI_YISHI = ACTPOS_PUB_LUCK,	// = 3, 행사구역, PublicRoom클라스를 같이 사용하기때문에 ACTPOS_PUB_LUCK와 같은 ActPos를 사용한다.
	COUNT_KONGZI_ACTPOS
};

enum ENUM_ACTPOS_HUANGDI
{
	ACTPOS_HUANGDI_NONE = 0xff,

	ACTPOS_HUANGDI_YISHI = ACTPOS_PUB_LUCK,	// = 3, 행사구역, PublicRoom클라스를 같이 사용하기때문에 ACTPOS_PUB_LUCK와 같은 ActPos를 사용한다.
	COUNT_HUANGDI_ACTPOS
};

// Mail
#define		MAX_LEN_MAIL_TITLE				25
#define		MAX_LEN_MAIL_CONTENT			500
#define		MAX_MAIL_NUM					80					// the max mail num of received box/sent box
#define		MAX_SYSMAIL_NUM					10					// the max mail num of received system mail aditional count
#define		MAX_MAIL_ITEMCOUNT				6					// Max item count that can be added in a mail

// [MailType] = 1:Sent mail, 2:Received mail
// [MailKind] = 1:Normal mail, 2:System mail, 3:Rejected mail
// [MailStatus] = 0:New mail, 1:Opened mail, 2:Get item mail
enum TMailType
{
	MT_SENT_MAIL = 1,			// sent mail
	MT_RECEIVED_MAIL			// received mail
};

// mail kind
enum TMailKind
{
	MK_NORMAL = 1,				// normal mail between players
	MK_SYSTEM,					// system mail
	MK_REJECTED,				// rejected mail by user
	MK_SYSTEM_REJECTED,			// rejected mail by system
	MK_SYSTEM_SPEC1,			// special system mail
};

// mail status
enum TMailStatus
{
	MS_NEW_MAIL,				// mail is not read yet
	MS_OPENED_MAIL,				// read the mail, but didn't receive the items in the mail
	MS_RECEIVED_ITEM,			// read the mail, and received the items in the mail

	// below are only for client use
	MS_RECEIVING_ITEM,

	MS_SENT,
	MS_SENDING,
	MS_SEND_FALIED,
	MS_REJECTING,

	MS_NONE
};

// 한방에서 비석최대수
#define		MAX_TOMBNUMINONEROOM			5

#define		MAX_OWNROOMCOUNT				20

// Favorite
#define		MAX_FAVORROOMNUM				50
#define		MAX_FAVORROOMGROUP				11
#define		MAX_FAVORGROUPNAME				6
#define		MAX_FAVORROOMGRP_BUF_SIZE		250					// 그룹자료 Buf 최대길이

//History data
#define		MAX_LEN_ALBUM_NAME				6					// Album이름의 최대길이 (unicode)
#define		MAX_DATA_TITLE					8					// 동화상자료 이름의 최대길이 (unicode)
#define     MAX_DATA_DESCRIPTION			30					// 사진/동화 자료 설명문의 최대길이 (unicode)
#define     MAX_DATA_DESCRIPTION_VIDEO		40					// Album이름 + 설명문 최대길이 (동화상인 경우 설명문에 동화상이름과 설명이 같이 합쳐져 관리된다.) (unicode)
#define		MAX_ALBUM_COUNT					15					// 알범 최대개수
#define		MAX_PHOTO_COUNT					100					// 한 알범에 들어가는 사진 최대개수
#define		MAX_VIDEO_COUNT					50					// 한 알범에 들어가는 동화상 최대개수
#define		MAX_DEFAULT_VIDEO_COUNT			5					// 한 알범에 들어가는 동화상 최대개수
#define		MAX_ALBUM_BUF_SIZE				400					// 알범자료를 위한 DB의 메모리 최대크기

enum HISDATA_STATUS
{
	HISDATA_CHECKFLAG_NONE		= 0,
	HISDATA_CHECKFLAG_REJECTED	= 1,
	HISDATA_CHECKFLAG_CHECKED	= 2
};

// tbl_mstConfig에서 Name의 최대길이수
#define		MAX_LEN_CONFIGNAME				30

// contact group, room group, photo album, video album's default id
#define		DEFAULT_CONTACT_GROUPID			1
#define		ID_CONTACT_BLACKGROUP			((TID)-1)
#define		DEFAULT_ROOM_GROUPID			1
#define		DEFAULT_PHOTO_ALBUMID			1
#define		DEFAULT_VIDEO_ALBUMID			2
#define		SCREENVIDEO_ALBUMID				1
#define		MAX_DEFAULT_SYSTEMGROUP_ID		10
#define		UNASSIGNED_LUCKROOM_GROUP_ID	8		// the default group id for luckroom

#define		GROUP_ID_SPECIAL				10000
#define		GROUP_ID_MYROOMS				GROUP_ID_SPECIAL + 1
#define		GROUP_ID_MYMANAGEROOMS			GROUP_ID_SPECIAL + 2

// 홛동을 위한 마크로들
//#define		MAX_ACTIVITYPOS_PER_ROOM		8					// 한 방에서 진행되는 활동의 위치의 최대번호
//#define		MAX_NPCCOUNT_PER_ACTIVITY		64					// 활동에 참가하는 NPC의 최대개수
#define		MAX_LEN_SCOPE_NAME				50					// Scope이름길이 최대값
#define		MAX_LEN_MODEL_NAME				50					// Model이름길이 최대값
#define		MAX_LEN_SCOPEMODEL_NAME			100					// ScopeModel이름길이 최대값 - [ScopeName + 분리기호 + ModelName]
#define		MAX_LEN_NPC_NAME				50					// NPC이름길이 최대값
#define		MAX_LEN_ANIMATION_NAME			50					// Animation이름길이 최대값(unicode)
#define		MAX_ACTCAPTURE_TIME				600000				// 행사차례인데 행사를 시작하지 않는 시간 대기 600초 = 5분
#define		MAX_ITEM_PER_ACTIVITY			16					// 각종 행사시에 소비되는 아이템 최대개수 (한번에)
//#define		MAX_SCOPEMODEL_PER_ACTIVITY		10					// 각종 행사장의 효과모델 최대수, ScopeName과 ModelName이 분리기호(^)로 련결되여있다.
#define		MAX_DOUSER_PER_ACTIVITY			4					// 한 행사장에서 행사위치 최대수
#define		MAX_WAITUSER_PER_ACTIVITY		50					// 행사에 대기자 최대인원수

// MoneyType
enum SHOP_TYPE
{
	MONEYTYPE_UMONEY	= 1,				// UserMoney로 Item을 구입한다.
	MONEYTYPE_POINT		= 2					// Point(GMoney)로 Item을 구입한다.
};

// inventory state
// when consume item, inventory state changes : IVS_RECEIVE->IVS_LOCK->IVS_CONFIRM->IVS_RECEIVE
// when edit inventory, its state changes: IVS_RECEIVE->IVS_CHANGED->IVS_APPLY->IVS_RECEIVE
//                                                                 ->IVS_APPLY_NODRESS->IVS_CHANGED
//                                                                 ->IVS_CHANGED_ONLYDRESS->IVS_APPLY
//                                                                                        ->IVS_CHANGED
enum INVENTORY_STATE {
	IVS_APPLY			= 2,
	IVS_CONFIRM,

	// below are used only in client
	IVS_NO_RECEIVE,
	IVS_RECEIVE,
	IVS_CHANGED,
	IVS_CHANGED_ONLYDRESS,
	IVS_APPLY_NODRESS,
	IVS_LOCK,

	IVS_WAITING_SYNC	= 0x8000,
};

// Item Delete Type
enum ITEM_DEL_TYPE
{
	DELTYPE_TIMEOUT	= 1,				// Timeout에 의해 삭제됨
	DELTYPE_THROW	= 2					// 사용자가 인벤에서 버리기조작을 함
};

//#define		MAX_ROOM_VISIT_HIS_COUNT		100			// 기념관(공공천원, 불교구역 등 모두 포함)에서 기록하는 방문자ID수의 최대값
#define		MAX_LEN_ROOM_VISIT_PASSWORD		12			// 방의 Password 최대길이

#define		MAX_CARD_ID_LEN					8			// 천원카드 ID 길이
//#define		MAX_CARD_PWD_LEN				32			// 천원카드 PWD 길이

#define		MAX_MANAGER_COUNT				10
#define		MANAGER_PERMISSION_USE			1
#define		MANAGER_PERMISSION_GET			2

#define		MAX_BESTSELL_ITEM_COUNT			20

#define		MAX_MULTIACT_FAMILY_COUNT		37

enum _MULTIACTIVITY_FAMILY_STATUS
{
	MULTIACTIVITY_FAMILY_STATUS_NONE = 1,
	MULTIACTIVITY_FAMILY_STATUS_ACCEPT,
	MULTIACTIVITY_FAMILY_STATUS_READY,
	MULTIACTIVITY_FAMILY_STATUS_START
};

#define		MAX_RECOMMEND_ROOM_CNT			5			// 열점추천방 개수(热点推荐 项目数)
#define		MAX_CHAIR_COUNT					200			// 한 방에 있을수 있는 의자 최대개수

#define		TM_SEND_LUCK_REQ_MIN_CYCLE		300			// 300s = 5min : 행운단어 표시간격
#define		MAX_LUCK_LV4_COUNT				10			// 한 써버에서 하루에 표시되는 Lv4 행운단어 최대개수
#define		LUCKY4_INTERVAL_FAMILY			48*3600		// 행운4는 한 사용자가 72시간내에 한번씩 나올수 있다.
#define		LUCKY4_INTERVAL_ROOM			48*3600		// 행운4는 한 방에서 72시간내에 한번씩 나올수 있다.
#define		LUCKY3_COUNT_PER_USER			3			// 행운3는 한 사용자가 하루에 3번 나올수 있다.

#define		MAX_EXPROOMID					6000		// 체험방ID 최소값
#define		MIN_RELIGIONROOMID				10000000	// 종교방ID 최소값
#define		MAX_RELIGIONROOMID				10001000	// 종교방ID 최대값

#define		ROOM_ID_BUDDHISM				10000000	// 불교구역고정 room id ( 佛教区 固定 room id )
#define		ROOM_ID_PUBLICROOM				10000001	// 공공천원고정 room id ( 公共天园 固定 room id )
#define		ROOM_ID_KONGZI					10000002	// 공자구역고정 room id ( 孔子区 固定 room id )
#define		ROOM_ID_HUANGDI					10000003	// 황제구역고정 room id ( 黄帝区 固定 room id )

#define		ROOM_ID_VIRTUAL					9999990		// 가상방ID - 열점추천기능의 사진을 관리하기 위하여 리용된다.
#define		ROOM_ID_FAMILYFIGURE			9999991		// 가상방ID - 사용자의 얼굴형상사진을 관리하기 위하여 사용한다.
#define		ROOM_ID_FAMILYFACEMODEL			9999992		// 가상방ID - 사용자의 얼굴형상모델자료를 관리하기 위하여 사용한다.

#define		RROOM_DEFAULT_CHANNEL_COUNT			100		// 종교방 표준선로개수
#define		EXPROOMROOM_DEFAULT_CHANNEL_COUNT	10		// 체험방 표준선로개수

#define		MAX_VIRTUE_FAMILY_COUNT			50			// 종교구역의 공덕비에 표시되는 사용자 최대수
#define		MAX_LEN_RELIGION_PRAYTEXT		500			// 종교구역에서의 기도문 최대길이

#define		ROOM_SPECIAL_CHANNEL_START		20000		// 한사람만 들어올수 있는 특수채널시작값
#define		ROOM_MOBILE_CHANNEL_START		10000		// 모바일사용자 전용 채널 시작값, ROOM_SPECIAL_CHANNEL_START보다 작아야 한다. 
//by krs
#define		ROOM_UNITY_CHANNEL_START		5000  // Unity판본을 위한 채널의 시작값

#define		REQUEST_MONEY_INTERVAL			300			// 돈정보요청할수 있는 시간간격 (/초)

#define		MAX_EDITANIMALGROUPNUM			100
#define		MAX_EDITANIMALNUM				100
#define		MAX_EDITANIMALGROUPBASELEN		20

enum _EAMoveType
{
	EA_MOVE = 1,
	EA_ANIMATION = 2
};

enum _ActionResultType
{
	ActionResultType_Xianbao = 0,
	ActionResultType_Yishi = 1,
	ActionResultType_Ancsetor_Jisi = 2,
	ActionResultType_AncestorDeceased_Xiang = 3,
	ActionResultType_AncestorDeceased_Hua = ActionResultType_AncestorDeceased_Xiang + MAX_ANCESTOR_DECEASED_COUNT,
	ActionResultType_Auto2 = ActionResultType_AncestorDeceased_Hua + MAX_ANCESTOR_DECEASED_COUNT,
	ActionResultType_MAX
};
enum _YishiResultType
{
	YishiResultType_None		= 0,
	YishiResultType_Small		= 1,
	YishiResultType_Medium		= 2,
	YishiResultType_Large		= 3,
	YishiResultType_Multi		= 4,
	YishiResultType_XDG			= 6,
	YishiResultType_XDG_Multi	= 15,
	XianbaoResultType_Small		= 5,
	XianbaoResultType_Large		= 7,
	DelegateResultType_Small	= 8,
	DelegateResultType_Medium	= 9,
	DelegateResultType_Large	= 10,

	// only used in client
	LifoResultType_Small		= 11,
	LifoResultType_Medium		= 12,
	LifoResultType_Large		= 13,
	LifoResultType_Multi		= 14
};
#define ACTIONRESULT_YISHI_ITEMCOUNT		5			// 천수의식시 의식후에 남아있는 결과물 최대개수
#define ACTIONRESULT_AUTO2_ITEMCOUNT		10			// 천원특사카드에 의해 남아있는 결과물 최대개수
#define ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT	11			// 조상탑 고인의 향 남아있는 결과물 최대수
#define ACTIONRESULT_DECEASED_HUA_ITEMCOUNT	6			// 조상탑 고인의 꽃 남아있는 결과물 최대수
#define MAX_ACTIONRESULT_ITEM_COUNT			11			// 우의 결과물개수의 최대값
#define ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION 10		// 천수의식시 의식후에 남아있는 결과물 최대개수(불교구역)			

enum SYSTEMNOTICE_TYPE
{
	SN_EVENT = 0,
	SN_SYSTEM,
	SN_GODS,
	SN_LARGEACT
};

enum LUCK_LEVEL
{
	LUCK_1 = 1,
	LUCK_2,
	LUCK_3,
	LUCK_4
};

#define	MAX_LEN_ANCESTOR_TEXT			256

#define AUTOPLAY_FID_START				901				// AutoPlay를 위한 시작FID
#define AUTOPLAY_FID_COUNT				200				// AutoPlay를 위한 FID개수
#define MAX_AUTOPLAY_ROOM_COUNT			100				// AutoPlay아이템 하나로 한번에 처리할수 있는 방의 개수

enum AUTOPLAY_STATUS
{
	AUTOPLAY_STATUS_START = 1,
	AUTOPLAY_STATUS_END,
};

#define INVITE_WAIT_TIME				600000			// 초청요청을 받은 다음 응답할 때까지의 대기시간 - 5분

#define MAX_GOLDFISH_COUNT				10				// 한 어항에서 키울수 있는 금붕어 최대종류수

#define MAX_SITE_ACTIVITY_ITEMCOUNT		24				// 기념일활동시에 최대로 줄수있는 아이템 종류수
#define MAX_SITE_ACTIVITY_TEXT_LEN		150				// 기념일활동시에 입력할수 있는 Text 최대길이

#define MAX_SITE_CHECKIN_ITEMCOUNT		24				// CheckIn활동시에 최대로 줄수있는 아이템 종류수

#define MAX_ROOMSTORE_SAVE_COUNT		12				// 한번에 기념관 보물창고에 보관할수 있는 최대개수
#define MAX_ROOMSTORE_COUNT				108				// 기념관 보물창고 칸 최대개수
#define MAX_LEN_ROOMSTORE_COMMENT		150				// max length of comment
#define ROOMSTORE_HISTORY_ADD			1				// 보물창고에 아이템 넣기
#define ROOMSTORE_HISTORY_USE			2				// 보물창고의 아이템 사용
#define ROOMSTORE_HISTORY_GET			3				// 보물창고의 아이템 꺼내기

#define CRACKER_CREATE_TIME				60000			// 폭죽이 재생되는 시간 (ms)
#define LANTERN_FLY_TIME				60				// 등불이 떠오르는 시간 (s)

#define MAX_DRUM_COUNT					2				// 한방에 있는 Drum 최대개수

#define PRIZE_TYPE_YISHI				1				// 中奖 - 개별행사 - M_SC_PRIZE
#define PRIZE_TYPE_XIANBAO				2				// 中奖 - 물건태우기 - M_SC_PRIZE
#define PRIZE_TYPE_FLOWER				3				// 中奖 - 꽃기르기 - M_SC_PRIZE
#define PRIZE_TYPE_FISH					4				// 中奖 - 물고기기르기 - M_SC_PRIZE
#define PRIZE_TYPE_LUCK					5				// 中奖 - 행운대에서 4레벨 행운 받음 - M_SC_PRIZE
#define PRIZE_TYPE_BUDDHISM_YISHI		6				// 中奖 - 불교구역에서 개별행사를 하고 5레벨 행운 받음 - M_SC_PRIZE
#define PRIZE_TYPE_BUDDHISM_BIGYISHI	7				// 中奖 - 불교구역대형행사
#define PRIZE_TYPE_ANIMAL				8				// 中奖 - 동물교감할 때의 好运道具
#define PRIZE_TYPE_YUWANG				10				// 鱼王 - 물고기기르기에서 물고기왕출현. M_SC_YUWANGPRIZE

#define PRIZE_LEVEL_1					1				// 中奖급수 - 초급
#define PRIZE_LEVEL_2					2				// 中奖급수 - 중급
#define PRIZE_LEVEL_YISHI3				3				// 中奖급수 - 천수의식
#define PRIZE_LEVEL_XIANBAO3			4				// 中奖급수 - 물건태우기
#define PRIZE_LEVEL_BUDDHISM_YISHI		5				// 中奖급수 - 불교구역행사
#define PRIZE_LEVEL_BUDDHISM_BIGYISHI	6				// 中奖급수 - 불교구역대형행사
#define PRIZE_LEVEL_ANIMAL				7				// 中奖급수 - 동물교감할 때의 好运道具

#define ROOM_EVENT_MAX_TIME				600				// room event play max time second

#define IsMusicRoomEvent(RoomEventType)		(RoomEventType >223)//12%

//#define SYSTEM_MAIL_FID					900				// 써버프로그람에서 체계메일을 보내는 FID

#define	SHOW_BLESS_LAMP_TIME			7*24*3600		// 길상등 표시시간 (15일)

enum ZAIXIAN_CONTROL_CODE
{
	ZAIXIAN_NEWOK = 0,					// 봉사를 받을수 있다.
	ZAIXIAN_NOGM,						// 봉사성원이 없다
	ZAIXIAN_BUSY,						// 봉사대기하여야 한다.
	ZAIXIAN_EXITGM,						// 해당 봉사성원이 봉사중단
	ZAIXIAN_NOSERVICETIME,				// 봉사시간이 아니다.
	ZAIXIAN_WAITMOMENT,					// 잠시 기다릴것
};

#define MAX_BLESSCARD_ID				4				// 축복카드ID최대값 (1~4)
#define MAX_SHENMINGBLESSCARD			5				// 선밍받았을때 주는 축복카드 개수
#define MAX_BLESSCARD_COUNT				99				// 한명이 소유할수 있는 축복카드 최대개수
#define TGNEWS_TXT_MAX_LEN              200
#define TGNEWS_URL_MAX_LEN              100
#define ZAIXIAN_TXT_MAX_LEN				256

#define	MAX_REVIEW_COUNT				10				// 사용자후기 최대개수
#define MAX_ROOMEVENT_COUNT				2				// 한방에서 하루에 생길수 있는 깜짝이벤트 최대수
#define MAX_FISHFOOD_COUNT				100				// 한벙에서 물고기먹이를 줄수 있는 최대개수

#define	MAX_PUBLICROOM_FRAME_COUNT		100				// 공공구역에서 체험방을 붙이는 벽의 최대수

#define MAX_LARGEACT_COUNT				10				// 써버에서 관리되는 대형행사 최대수
#define MAX_LARGEACT_ITEMCOUNT			20				// 대형행사시에 리용되는 아이템개수 (DB에 반영됨)
#define MAX_LARGEACT_PHOTOCOUNT			7				// 대형행사시에 리용되는 사진개수 (DB에 반영됨)
#define MAX_LARGEACT_USERCOUNT			81				// 대형행사시에 참가하는 최대인원수
#define MAX_LARGEACT_VIPCOUNT			81				// 대형행사시에 참가하는 최대귀빈인원수
#define MAX_LARGEACT_FLAGNAME         4 
#define MAX_LARGEACT_ACTMEAN          20
#define MAX_LARGEACT_NAME             64
#define MAX_LARGEACT_COMMENT          150
#define MAX_LARGEACT_NOTICE           150
#define MAX_LARGEACT_STEP             100

#define		LARGEACTPREPARINGTIMEMINUTE		5			// 대형행사시작전 얼마전에 준비를 통보하는가(분)
#define		LARGEACTDOINGTIMEMINUTE			20			// 대형행사진행시간(분)

#define		MAX_LARGEACTPRIZENUM			10			// 대형행사선물아이템종류 최대수
enum		LARGEACTITEMPRIZE_MODE					// 대형행사참가자들에게 아이템을 주는 방식
{
	LARGEACTITEMPRIZE_WHOLE,						// 모든 아이템을 다준다
	LARGEACTITEMPRIZE_RANDOMONE						// 임의로 한종류를 준다
};

enum		LARGEACTBLESSPRIZE_MODE					// 대형행사참가자들에게 축복카드를 주는 방식
{
	LARGEACTBLESSPRIZE_WHOLE,						// 모든 카드를 다준다
	LARGEACTBLESSPRIZE_RANDOMONE					// 임의로 한종류를 준다
};

#define		MAX_MULTILANTERNJOINUSERS			8		// 집체공명등 최대Join수
#define		MIN_MULTILANERN_POSITION_DIST_SQ	4900	// 집체공명등 올리는 위치사이 최소두제곱거리(y값무시)

#define		BELL_DRUM_AFTEREND_EFFECT_TIME		10.0f	// 북치기/종치기끝난후 효과가 나타나있는 시간[불교구역]

// 불교구역대형행사관련
enum	BUDDISMYISHI_MODE	// 개인대형행사와 체계대형행사 분류값
{ 
	BUDDISMYISHI_PERSONAL=1, 
	BUDDISMYISHI_SYSTEM 
};
enum	SBY_CYCLE_MODE		// 체계대형행사주기속성값	
{
	SBY_EVERYDAY = 1,
	SBY_ONLYONE 
};
enum	BUDDISMYISHI_OPENMODE	// 행사내용공개속성값	
{ 
	BUDDISMYISHI_LOCK = 0, 
	BUDDISMYISHI_OPEN
};

#define MAX_LEN_BUDDISMYISHITARGET 10		// 행사대상문자렬길이 최대값	
#define MAX_LEN_BUDDISMYISHIPROPERTY 10		// 행사류형문자렬길이 최대값	
#define MAX_LEN_BUDDISMYISHICONTENT 600		// 행사기원내용문자렬길이 최대값	
#define MAX_NUM_SBYUSER 108						// 체계대형행사최대참가수	
#define MAX_NUM_PBYUSER 64						// 개인대형행사최대참가수	
#define MAX_NUM_SBYLIST 12						// 체계대형행사목록 최대값	
#define MAX_NUM_BYITEM 12						// 대형행사아이템최대개수	
#define MAX_TIMES_BUDDISMYISHI 3500				// 체계대형행사최대시간(1시간조금 못되는 3500초)
#define MAX_TIMES_BUDDISMPBYYISHI (15*60)		// 개인대형행사최대시간(10분정도)
#define BUDDISMYISHI_READYSTEP 0				// 행사진척준비단계값	
#define BUDDISMYISHI_REALSTARTSTEP 1			// 행사정식시작단계값	
#define BUDDISMYISHI_CANTSTEP 1					// 행사에 참가할수 없는 단계
#define BUDDISMYISHI_FINISH 1000				// 행사완료단계값	
#define BUDDISMYISHI_ALLFINISH 1010				// 행사완전완료단계값	
#define BUDDISMYISHI_READYTIMESECOND (15*60)	// 불교구역체계대형행사준비시간(초)

#define	PC_MOBILE_DISTANCE_RATE	10.0f			// PC와 Mobile의 Map상에서의 거리 비률

#define	PHOTOINFO_REFRESH_TIME	(1 * 60 * 60 * 1000)	// PhotoInfo의 갱신주기 - 1시간

#define HIS_AUTHKEY_WAIT_TIME	30000			// HisService에서 AuthKey대기시간, 이 시간이 지나면 AuthKey를 삭제한다.
#define HIS_UPAUTHKEY_WAIT_TIME	1800000

#endif // TIANGUO_MACRO_H
