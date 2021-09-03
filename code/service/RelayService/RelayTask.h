#pragma once

#include "misc/thread.h"
#include "net/inet_address.h"
#include "net/udp_sock.h"

using namespace	SIPNET;
class CRelayService;

class CRelayTask : public SIPBASE::IRunnable
{
	CUdpSock*		m_sock;
	volatile bool	m_bExit;
	int				m_nRelayMaxCount;

public:
	virtual void run();
	void	procRecvPacket(uint8* msg, int len, CInetAddress& recvaddr);

	/// Tells the task to exit
	void	requireExit() { m_bExit = true; }

	/// Returns true if the requireExit() has been called
	bool	exitRequired() const { return m_bExit; }

	void	setRelayMaxCount(int count)	{ m_nRelayMaxCount = count; }

public:
	CRelayTask(CUdpSock* sock);
	~CRelayTask();
};
