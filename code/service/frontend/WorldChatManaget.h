#pragma once

#include "misc/types_sip.h"
#include "misc/ucstring.h"
#include "misc/buf_fifo.h"

#include "../Common/Common.h"
#include "../../common/common.h"
#include "../../common/Macro.h"

#define		MAX_WORLDCHATNUM	500

#define		SAFENCOPY(dest, src)					\
	memset(dest, 0, sizeof(dest));					\
	size_t	nLen##dest = sizeof(dest) / sizeof(ucchar);	\
	size_t nLen##src = wcslen(src);					\
	if (nLen##src >= nLen##dest)					\
	nLen##src = nLen##dest-1;					\
	wcsncpy(dest, src, nLen##src);					\

struct WORLDCHATITEM 
{
	WORLDCHATITEM()
	{
		fid = 0;
		memset(fname, 0, sizeof(fname));
		memset(chat, 0, sizeof(chat));
	}
	WORLDCHATITEM(T_FAMILYID _fid, const ucchar* _fname, const ucchar* _chat)
	{
		Set(_fid, _fname, _chat);
	}
	void Set(T_FAMILYID _fid, const ucchar* _fname, const ucchar* _chat)
	{
		fid = _fid;
		SAFENCOPY(fname, _fname);
		SAFENCOPY(chat, _chat);
	}
	T_FAMILYID		fid;
	ucchar			fname[MAX_LEN_FAMILY_NAME+1];
	ucchar			chat[MAX_LEN_CHAT+1];
};

class CWorldChatManager
{
public:
	CWorldChatManager(void);
	virtual ~CWorldChatManager(void);
	
	bool	AddChat(T_FAMILYID fid, const ucchar* fname, const ucchar* chat);
	bool	PopChat(WORLDCHATITEM *pResult);

protected:
	// std::list<WORLDCHATITEM>	m_lstChat;
	SIPBASE::CBufFIFO			m_ChatBuffer;
	
	bool				IsFullChatBuffer();
};
