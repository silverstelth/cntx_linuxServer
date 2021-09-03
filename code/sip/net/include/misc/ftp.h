/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __FTP_H__
#define __FTP_H__

#ifdef SIP_OS_WINDOWS
#	include <Windows.h>
#	include <WinInet.h>
#	pragma comment(lib, "Wininet.lib")
#include "types_sip.h"
#include <TCHAR.H>

namespace SIPBASE
{
	typedef	void	(*fndefDownloadNotify)(float fProgress, void* pParam, DWORD dwNowSize, void* pMemoryPtr);

	class CFtpDown
	{
	public:
	public:
		inline	CFtpDown(void)
			: m_internet(0)
			, m_ftp		(0)
			, m_ftpFile	(0)
			, m_pBuffer	(0)
			, m_hThread	(0)
			, m_hEvent	(0)
			, m_hOutFile(0)
			, m_bClosing(false)
		{
		}

		inline ~CFtpDown(void)	{	Close();	}
		inline bool IsConnected() const		{	return m_ftp != NULL;	}

		void Close();

		bool OpenFtp(LPCTSTR lpszServerName, INTERNET_PORT nServerPort, LPCTSTR lpszUsername, LPCTSTR lpszPassword, bool bPassive = false);
		bool Download(const TCHAR * pRemoteFileName, const TCHAR* pLocalFileName, void * pParam = NULL, fndefDownloadNotify fnNotify = NULL);
		bool Download(const TCHAR * pRemoteFileName, void * pParam = NULL, fndefDownloadNotify fnNotify = NULL);
		bool IsExistingFileInFTPDir(const TCHAR * pRemoteDir, const TCHAR * pRemoteFileName, int * pnSize = NULL, FILETIME * pFileTime = NULL);
		void SetCurrentDirectory(LPCTSTR path);

	protected:
		static DWORD WINAPI threadForFtpDownload(LPVOID lpParam);
		bool Download();

	protected:
		HINTERNET		m_internet;
		HINTERNET		m_ftp;
		HINTERNET		m_ftpFile;
		HANDLE			m_hThread;
		HANDLE			m_hEvent;
		HANDLE			m_hOutFile;
		void *			m_pBuffer;
		void *			m_pPrivateParam;
		bool			m_bClosing;
		fndefDownloadNotify m_fnNotify;

	};
} // SIPBASE
#endif // SIP_OS_WINDOWs
#endif // __FTP_H__

/* End of ftp.h */
