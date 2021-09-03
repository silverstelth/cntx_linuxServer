/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"


#ifdef SIP_OS_WINDOWS

#include "misc/ftp.h"
#include "misc/sstring.h"

#define MAX_READ_PERTIMES	4 * 1024
#define MIN_WRITE_BUFFER_TIMES	8

namespace SIPBASE
{
	bool CFtpDown::OpenFtp(LPCTSTR lpszServerName, INTERNET_PORT nServerPort, LPCTSTR lpszUsername, LPCTSTR lpszPassword, bool bPassive)
	{
		m_internet = InternetOpen(0, 0, 0, 0, 0);
		if ( !m_internet )
			return false;

		DWORD dwFlags = 0;
		if ( bPassive )
			dwFlags = INTERNET_FLAG_PASSIVE;

		m_ftp = InternetConnect(m_internet, lpszServerName, nServerPort, lpszUsername, lpszPassword, INTERNET_SERVICE_FTP, dwFlags, 0);

		if ( !m_ftp )
			return false;

		FtpSetCurrentDirectory(m_ftp, _t("/"));

		m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

		return true;
	}

	void CFtpDown::Close()
	{
		m_bClosing = true;
		if ( m_hEvent )
		{
			SetEvent(m_hEvent);

			DWORD exitcode;
			while ( GetExitCodeThread(m_hThread, &exitcode ) && exitcode == 0 )
				break;

			CloseHandle(m_hEvent);
			m_hEvent = NULL;
		}
		if ( m_internet )
		{
			InternetCloseHandle(m_internet);
			m_internet = NULL;
		}
		if ( m_ftp )
		{
			InternetCloseHandle(m_ftp);
			m_ftp = NULL;
		}
		if ( m_ftpFile )
		{
			InternetCloseHandle(m_ftpFile);
			m_ftpFile = NULL;
		}
		if ( m_pBuffer )
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
		}
	}

	bool CFtpDown::Download(const TCHAR * pRemoteFileName, const TCHAR* pLocalFileName, void * pParam, fndefDownloadNotify fnNotify)
	{
		if (m_ftpFile != NULL)
			return false;

		m_ftpFile = FtpOpenFile(m_ftp, pRemoteFileName, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);

		if ( m_ftpFile == NULL )
		{
			DWORD	dwErr = GetLastError(), dwLen = 500;
			TCHAR	szTextErr[300] = _t("");
			BOOL b = InternetGetLastResponseInfo(&dwErr, szTextErr, &dwLen);
			if ( _tcscmp(szTextErr, _t("Cannot create a file when that file already exist.") ) == 0 )

				if ( !b )
					dwErr = GetLastError();
			return false;
		}

		m_hOutFile = CreateFile(pLocalFileName, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, NULL);

		if ( m_hOutFile == INVALID_HANDLE_VALUE )
		{
			InternetCloseHandle(m_ftpFile);
			m_ftpFile = NULL;
			return false;
		}

		m_fnNotify = fnNotify;
		m_pPrivateParam = pParam;

		Download();

		InternetCloseHandle(m_ftpFile);
		m_ftpFile = NULL;

		return true;
	}

	bool CFtpDown::Download(const TCHAR * pRemoteFileName, void * pParam, fndefDownloadNotify fnNotify)
	{
		if ( m_ftpFile )
			InternetCloseHandle(m_ftpFile);

		m_ftpFile = FtpOpenFile(m_ftp, pRemoteFileName, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);
		if ( m_ftpFile == NULL )
			return false;

		m_fnNotify = fnNotify;
		m_pPrivateParam = pParam;

		if ( m_hThread == NULL )
		{
			m_hThread = CreateThread(NULL, 0, threadForFtpDownload, this, 0, NULL);
			CloseHandle(m_hThread);
		}
		else
		{
			SetEvent(m_hEvent);
		}
		return true;
	}

	DWORD CFtpDown::threadForFtpDownload(LPVOID lpParam)
	{
		CFtpDown * pFtp = (CFtpDown*)lpParam;
_rewait:
		WaitForSingleObject(pFtp->m_hEvent, INFINITE);

		if ( pFtp->m_bClosing )
			return 0;

		pFtp->Download();

		if ( !pFtp->m_bClosing )
			goto _rewait;

		return 0;
	}

	bool CFtpDown::Download()
	{
		if (m_ftpFile == NULL)
			return false;

		DWORD	sizeHigh, savedSize = 0, totalSize, sizeRead;
		totalSize = FtpGetFileSize(m_ftpFile, &sizeHigh);

		int iWriteBufCount = 0;
		if ( m_hOutFile )
		{
			if ( m_pBuffer == NULL )
				m_pBuffer = malloc(MAX_READ_PERTIMES * MIN_WRITE_BUFFER_TIMES);

			memset(m_pBuffer, 0, MAX_READ_PERTIMES * MIN_WRITE_BUFFER_TIMES);

			while ( savedSize < totalSize )
			{
				DWORD readSize = totalSize - savedSize;
				if ( readSize > MAX_READ_PERTIMES )
					readSize = MAX_READ_PERTIMES;

				if ( !InternetReadFile(m_ftpFile, (uint8 *)m_pBuffer + iWriteBufCount * MAX_READ_PERTIMES, readSize, &sizeRead ) )
				{
					InternetCloseHandle(m_ftpFile);
					m_ftpFile = NULL;
					if ( m_hOutFile )
						CloseHandle(m_hOutFile);
					return false;
				}
				iWriteBufCount ++;
				savedSize += sizeRead;

				if ( iWriteBufCount >= MIN_WRITE_BUFFER_TIMES )
				{
					WriteFile(m_hOutFile, m_pBuffer, (iWriteBufCount-1)*MAX_READ_PERTIMES+sizeRead, &sizeRead, NULL);
					iWriteBufCount  = 0;
				}

				if ( m_fnNotify )
				{
					float f = (float)savedSize / (float)totalSize;
					m_fnNotify(f, m_pPrivateParam, 0, NULL);
				}
			}
			if ( iWriteBufCount )
				WriteFile(m_hOutFile, m_pBuffer, (iWriteBufCount-1)*MAX_READ_PERTIMES+sizeRead, &sizeRead, NULL);

			CloseHandle(m_hOutFile);
			m_hOutFile = NULL;
		}
		else
		{
			/*		TCHAR * pLocalMem = (TCHAR *)malloc(totalSize);

			if ( m_pBuffer == NULL )
			m_pBuffer = (TCHAR *)malloc(MAX_READ_PERTIMES);

			while ( savedSize < totalSize )
			{
			DWORD readSize = totalSize - savedSize;
			if ( readSize > MAX_READ_PERTIMES )
			readSize = MAX_READ_PERTIMES;

			if ( !InternetReadFile(m_ftpFile, m_pBuffer, readSize, &sizeRead) )
			{
			InternetCloseHandle(m_ftpFile);
			m_ftpFile = NULL;
			free(pLocalMem);
			return NULL;
			}

			memcpy(pLocalMem + savedSize, m_pBuffer, readSize);
			savedSize += sizeRead;

			if ( m_fnNotify )
			{
			float f = (float)savedSize / (float)totalSize;
			m_fnNotify(f, m_pPrivateParam, readSize, pLocalMem);
			}
			}
			*/	}

			return true;
	}

	bool CFtpDown::IsExistingFileInFTPDir(const TCHAR * pRemoteDir, const TCHAR * pRemoteFileName, int * pnSize, FILETIME * pFileTime)
	{
		if ( pnSize != NULL )
			*pnSize = 0;

		tstring				strFileName;
		strFileName		= pRemoteDir;
		strFileName		+=_t("/*.*");

		WIN32_FIND_DATA		FindFileData;
		HINTERNET			hFindFile = NULL;
		hFindFile		= FtpFindFirstFile(m_ftp, strFileName.c_str(), &FindFileData, INTERNET_FLAG_RELOAD, 0); 
		if (hFindFile == NULL)
		{
			DWORD	dwErr = GetLastError();
			DWORD	dwLen = 500;
			TCHAR	szTextErr[300] = _t("");
			InternetGetLastResponseInfo(&dwErr, szTextErr, &dwLen);
			TCHAR * szDest = _tcsstr(szTextErr, _t("The directory is not empty"));
			return false;
		}

		do
		{
			if ( _tcscmp(FindFileData.cFileName, pRemoteFileName) == 0 )
			{
				if ( pnSize != NULL )
					*pnSize = (int)( FindFileData.nFileSizeLow );
				if ( pFileTime != NULL )
					*pFileTime = FindFileData.ftLastWriteTime;

				InternetCloseHandle(hFindFile);
				hFindFile = NULL;
				return true;
			}
		}
		while(InternetFindNextFile(hFindFile, &FindFileData));

		InternetCloseHandle(hFindFile);
		hFindFile = NULL;
		return false;
	}

	void CFtpDown::SetCurrentDirectory(LPCTSTR path)
	{
		BOOL bR = FtpSetCurrentDirectory(m_ftp, path);
	}
}


#else
// #	error	Sorry! We haven't ready FTP for Unix Verssion Compile!
#endif // SIP_OS_WINODWS

