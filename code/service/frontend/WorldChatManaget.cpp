#include "WorldChatManaget.h"

CWorldChatManager::CWorldChatManager(void)
{
}

CWorldChatManager::~CWorldChatManager(void)
{
}

bool	CWorldChatManager::IsFullChatBuffer()
{
	uint32	uRemainItem = m_ChatBuffer.fifoSize();
	if (uRemainItem >= MAX_WORLDCHATNUM)
		return true;
//	if (m_lstChat.size() >= MAX_WORLDCHATNUM)
//		return true;
	return false;
}

bool	CWorldChatManager::AddChat(T_FAMILYID fid, const ucchar* fname, const ucchar* chat)
{
	if (IsFullChatBuffer())
		return false;

	static	WORLDCHATITEM	newTemp;
	newTemp.Set(fid, fname, chat);
	m_ChatBuffer.push((uint8 *)(&newTemp), sizeof(newTemp));

//	m_lstChat.push_back(WORLDCHATITEM(fid, fname, chat));
	return true;
}

bool	CWorldChatManager::PopChat(WORLDCHATITEM *pResult)
{
/*
	if (m_lstChat.size() == 0)
		return false;

	*pResult = m_lstChat.front();
	m_lstChat.pop_front();
	*/
	if (m_ChatBuffer.empty())
		return false;
	uint8	*tempBuffer = NULL;
	uint32	s;
	m_ChatBuffer.front(tempBuffer, s);
	if (tempBuffer == NULL)
		return false;

	memcpy(pResult, tempBuffer, s);
	m_ChatBuffer.pop();
	return true;
}
