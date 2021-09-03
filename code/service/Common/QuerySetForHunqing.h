#pragma once

#define MAKEQUERY_GetHQList(strQ, UID)	\
	TCHAR strQ[128] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Act_List(%d)}"), UID);

#define MAKEQUERY_HQInsert(strQ, UID, Name1, Name2, StartTime, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9, ItemIDs, Comment)	\
	TCHAR strQ[4096] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Act_Insert(%d, N'%s', N'%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%s, N'%s')}"), UID, Name1.c_str(), Name2.c_str(), StartTime, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9, ItemIDs, Comment.c_str());

#define MAKEQUERY_HQModify(strQ, ID, Name1, Name2, StartTime, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9, ItemIDs, Comment)	\
	TCHAR strQ[4096] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Act_Modify(%d, N'%s', N'%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%s, N'%s')}"), ID, Name1.c_str(), Name2.c_str(), StartTime, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9, ItemIDs, Comment.c_str());

#define MAKEQUERY_SetHQStatus(strQ, ID, Status)	\
	TCHAR strQ[128] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Act_SetStatus(%d, %d)}"), ID, Status);

#define MAKEQUERY_GetHQInfo(strQ, ID)	\
	TCHAR strQ[128] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Act_GetInfo(%d)}"), ID);

#define MAKEQUERY_DeleteHQPhoto(strQ, PhotoID)	\
	TCHAR strQ[128] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Photo_Delete(%d)}"), PhotoID);

#define MAKEQUERY_SetHQToPhoto(strQ, ActID, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9)	\
	TCHAR strQ[128] = _T("");								\
	_stprintf(strQ, _T("{CALL HQ_Photo_SetHunqing(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)}"), ActID, PhotoID1, PhotoID2, PhotoID3, PhotoID4, PhotoID5, PhotoID6, PhotoID7, PhotoID8, PhotoID9);
