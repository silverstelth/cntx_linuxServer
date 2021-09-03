/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_WINDOWS
#include "misc/HttpDown.h"
#include "misc/path.h"

#	pragma comment(lib, "Wininet.lib")
#define MAX_READ_PERTIMES	4 * 1024

#define MIN_WRITE_BUFFER_TIMES	8

namespace SIPBASE
{

bool CHttpDown::OpenHttp(const ucstring& ucServerName, const INTERNET_PORT nPort, const ucstring& ucUsername, const ucstring& ucPassword, bool bPassive)
{
	m_internet = InternetOpenW(L"HTTPFILE", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if ( !m_internet )
		return false;

	DWORD dwFlags = 0;
	if ( bPassive )
		dwFlags = INTERNET_FLAG_PASSIVE;

	m_http = InternetConnectW(m_internet, ucServerName.c_str(), nPort, ucUsername.c_str(), ucPassword.c_str(), INTERNET_SERVICE_HTTP, dwFlags, 0);

	if ( !m_http )
		return false;

	m_ucPrefix = L"http://" + ucServerName + L"/";

	return true;
}

void CHttpDown::Close()
{
	if ( m_httpFile )
	{
		InternetCloseHandle(m_httpFile);
		m_httpFile = NULL;
	}
	if ( m_http )
	{
		InternetCloseHandle(m_http);
		m_http = NULL;
	}
	if ( m_internet )
	{
		InternetCloseHandle(m_internet);
		m_internet = NULL;
	}
	if ( m_pBuffer )
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
}

bool CHttpDown::Download(const ucstring& ucRemoteFileName, const ucstring& ucLocalFileName, void * pParam, fndefDownloadNotify fnNotify)
{
	if ( m_httpFile )
		return false;

	/////////////////////////////////////////////
	ucstring absURI = m_ucPrefix + ucRemoteFileName;
	m_httpFile = HttpOpenRequestW(m_http, L"GET", ucRemoteFileName.c_str(), NULL, absURI.c_str(), NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);

	m_unLocalFileSeekPos = 0;
	if ( SIPBASE::CFileW::isExists(ucLocalFileName) )
	{
		CFileW::deleteFile(ucLocalFileName);
//		m_unLocalFileSeekPos = SIPBASE::CFileW::getFileSize(ucLocalFileName);
	}

	ucstring ucHeader = L"Accept: */*\r\nRange: bytes=" + SIPBASE::toStringW(m_unLocalFileSeekPos) + L"-";

	HttpAddRequestHeadersW(m_httpFile, ucHeader.c_str(), -1L, HTTP_ADDREQ_FLAG_ADD);

	if ( HttpSendRequestW(m_httpFile, NULL, -1L, NULL, 0) )
	{
		DWORD dwStatusCode;
		DWORD dwLength = sizeof(DWORD);
		if ( HttpQueryInfoW(m_httpFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwLength, NULL) &&
			 ( dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT ) )
		{
			m_hOutFile = CreateFileW( ucLocalFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( m_hOutFile != INVALID_HANDLE_VALUE )
			{
				SetFilePointer(m_hOutFile, 0, NULL, FILE_END);

				m_fnNotify = fnNotify;
				m_pPrivateParam = pParam;

				Download();

				InternetCloseHandle(m_httpFile);
				m_httpFile = NULL;

				return true;
			}
		}
// 		else if ( dwStatusCode == 416 ) //Requested range not satisfiable
// 		{
// 			InternetCloseHandle(m_httpFile);
// 			m_httpFile = NULL;
// 
// 			return true;
// 		}
	}	

	InternetCloseHandle(m_httpFile);
	m_httpFile = NULL;
	return false;
}

bool CHttpDown::Download()
{
	if ( m_httpFile == NULL )
		return false;

	DWORD	totalSize = 0, savedSize = 0, sizeRead = 0;
	DWORD	dwLength = sizeof(DWORD);
	if ( !HttpQueryInfoW(m_httpFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &totalSize, &dwLength, NULL) )
		return false;

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

			if ( !InternetReadFile(m_httpFile, (uint8 *)m_pBuffer + iWriteBufCount * MAX_READ_PERTIMES, readSize, &sizeRead ) )
			{
				InternetCloseHandle(m_httpFile);
				m_httpFile = NULL;
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
				float f = (float)( savedSize + m_unLocalFileSeekPos ) / (float)( totalSize + m_unLocalFileSeekPos );
				m_fnNotify(f, m_pPrivateParam, 0, NULL);
			}
		}
		if ( iWriteBufCount )
			WriteFile(m_hOutFile, m_pBuffer, (iWriteBufCount-1)*MAX_READ_PERTIMES+sizeRead, &sizeRead, NULL);

		CloseHandle(m_hOutFile);
		m_hOutFile = NULL;
	}

	return true;
}

}

#endif // SIP_OS_WINODWS
