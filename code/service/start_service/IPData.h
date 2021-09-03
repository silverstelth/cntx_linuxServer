#pragma once

#define MAXIPLEN	64
class	CIPAddress
{
public:
	CIPAddress()
	{
		memset(m_strIP, 0, sizeof(m_strIP));
		m_nByNumber = 0;
	}
	CIPAddress(const char* strIP)
	{
		strcpy(m_strIP, strIP);
		m_nByNumber = GetByNumber(m_strIP);
	}
	bool	IsInIPRange(const char* strStartIP, const char* strEndIP)
	{
		unsigned int	uS = GetByNumber(strStartIP);
		unsigned int	uE = GetByNumber(strEndIP);
		if (m_nByNumber >= uS && m_nByNumber <= uE )
			return true;
		if (uS == 0 && m_nByNumber == uE)
			return true;
		if (uE == 0 && m_nByNumber == uS)
			return true;

		return false;
	}
	const char*	GetIPAddress() {return m_strIP; }

	static	unsigned int	GetByNumber(const char* strStartIP)
	{
		char	ip[MAXIPLEN];
		strcpy(ip, strStartIP);
		char seps[]   = ".";
		char *token;

		int	nByNumber = 0;
		int	nDot = 4;
		token = strtok( ip, seps );
		while( token != NULL && nDot > 0)
		{
			int nNum = atoi(token);
			nByNumber += (nNum << ((nDot-1) * 8));
			token = strtok( NULL, seps );
			nDot--;
		}
		return nByNumber;
	}

	static bool	GetIPAddressFromHttpURL(const char *url, char *ipaddress)
	{
		char urlBuffer[32];
		memset(urlBuffer, 0, sizeof(urlBuffer));
		if (strlen(url) > 30)
			strncpy(urlBuffer, url, 30);
		else
			strcpy(urlBuffer, url);

		if (strlen(urlBuffer) <= strlen("http://"))
			return false;

		char* prefixPos = strstr(urlBuffer, "http://");
		if (prefixPos == NULL)
			return false;

		char *ipStart = prefixPos + strlen("http://");

		char seps[]   = ":/\\";
		char *token;

		token = strtok( ipStart, seps );

		strcpy(ipaddress, token);
		return true;
	}
	static bool IsInRange(const char* ipAddress, const char* ipStart, uint32 uNum)
	{
		unsigned int	uV = GetByNumber(ipAddress);
		unsigned int	uS = GetByNumber(ipStart);
		if (uV >= uS && uV < uS + uNum)
			return true;
		return false;
	}
protected:
	char			m_strIP[MAXIPLEN];
	unsigned int	m_nByNumber;
};

