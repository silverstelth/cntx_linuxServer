#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#define SIPNS_CONFIG	""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#define SIPNS_LOGS	""
#endif // SIPNS_LOGS

#pragma warning(disable : 4267)
#include "misc/types_sip.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#include <direct.h>
#undef max
#undef min
#else
#include <unistd.h>
#include <errno.h>
#endif
#include <string>
#include <list>

#include "misc/debug.h"
#include "misc/system_info.h"
#include "misc/config_file.h"
#include "misc/thread.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/common.h"
#include "misc/path.h"
#include "misc/value_smoother.h"
#include "misc/singleton.h"
#include "misc/file.h"
#include "misc/algo.h"

#include "net/service.h"
#include "net/unified_network.h"
#include "net/varpath.h"
#include "net/email.h"
#include "net/admin.h"

#include "RelayTask.h"
#include "RTPParser.h"

using namespace							std;
using namespace							SIPBASE;
using namespace							SIPNET;

typedef std::map<uint32, CInetAddress>	TRelayPeers;
TRelayPeers RelayPeers;

CRelayTask::CRelayTask(CUdpSock* sock)
{
	m_sock = sock;
	m_bExit = false;
	m_nRelayMaxCount = 100;
}

CRelayTask::~CRelayTask()
{
}

void CRelayTask::run()
{
	while (!exitRequired())
	{
		CUdpSock*	sock = m_sock;
		if (sock->dataAvailable())
		{
			static uint8 buffer[10240];
			uint len = 10240;
			CInetAddress addr;
			if (sock->receivedFrom(buffer, len, addr, false))
			{
				procRecvPacket(buffer, len, addr);
			}
		}
	}
}

bool isPermittee(uint32 peerid)
{
	if (peerid == 0)
	{
		return (false);
	}

	return (true);
}

void CRelayTask::procRecvPacket(uint8* msg, int len, CInetAddress& recvaddr)
{
	uint32 src_id = CRTPParser::getSyncSource(msg);
	if (!isPermittee(src_id))
	{
		sipinfo("not permittee: %d", src_id);
		return;
	}
	
	bool bReportAddr = len <= 12;
	if (bReportAddr)
	{
		return;
	}
	{
		TRelayPeers::iterator	it = RelayPeers.find(src_id);
		if (it == RelayPeers.end())
		{
			int peer_count = RelayPeers.size();
			if (peer_count < m_nRelayMaxCount)
			{
				RelayPeers.insert(make_pair(src_id, recvaddr));
				sipinfo("new peer added: %d, %s", src_id, recvaddr.asIPString().c_str());
			}
			else
			{
				sipinfo("excess peer count = %d", peer_count);
			}
		}
		else
		{
			CInetAddress&	alreayaddr = it->second;
			if (alreayaddr == recvaddr)
			{
			}
			else
			{
				it->second = recvaddr;
			}
		}
	}
	
	int nDestCount = CRTPParser::getContribSrcCount(msg);
	if (nDestCount > 1)
	{
		nDestCount = 1;
	}

	for (int i = 0; i < nDestCount; i++)
	{
		uint32 dest = CRTPParser::getContribSource(msg, i);
		TRelayPeers::iterator	it = RelayPeers.find(dest);

		if (it != RelayPeers.end())
		{
			CInetAddress&	addr = it->second;
			m_sock->sendTo(msg, len, addr);

			// sipdebug("src_id=%d, %s dest_id=%d, %s", src_id, recvaddr.asString().c_str(),
			// dest, addr.asString().c_str());
		}
	}
}
