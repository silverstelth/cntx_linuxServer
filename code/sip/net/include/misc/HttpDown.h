/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __HTTPDOWN_H__
#define __HTTPDOWN_H__

#ifdef SIP_OS_WINDOWS

#include <Windows.h>
#include <WinInet.h>
#include <misc/types_sip.h>
#include <misc/ucstring.h>


namespace SIPBASE
{

typedef	void	(*fndefDownloadNotify)(float fProgress, void* pParam, DWORD dwNowSize, void* pMemoryPtr);

class CHttpDown
{
public:

	inline	CHttpDown(void)
		: m_ucPrefix(L"")
		, m_internet(0)
		, m_http(0)
		, m_httpFile(0)
		, m_pBuffer	(0)
		, m_hOutFile(0)
	{
	}

	inline		~CHttpDown(void)		{	Close();	}
	inline bool IsConnected() const		{	return m_http != NULL;	}

	void		Close();

	bool 		OpenHttp(const ucstring& ucServerName, const INTERNET_PORT nPort, const ucstring& ucUsername, const ucstring& ucPassword, bool bPassive = false);
	bool 		Download(const ucstring& ucRemoteFileName, const ucstring& ucLocalFileName, void * pParam = NULL, fndefDownloadNotify fnNotify = NULL);

protected:

	ucstring				m_ucPrefix;
	uint32					m_unLocalFileSeekPos;

	HINTERNET				m_internet;
	HINTERNET				m_http;
	HINTERNET				m_httpFile;
	HANDLE					m_hOutFile;
	void *					m_pBuffer;
	void *					m_pPrivateParam;
	fndefDownloadNotify		m_fnNotify;

	bool				Download();

};

}

#endif // SIP_OS_WINODWS

#endif // end of guard __HTTPDOWN_H__
