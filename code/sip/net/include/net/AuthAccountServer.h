#pragma once

class CAuthAccountServer
{
	struct TACCOUNTINFO {
	};

public:
	static	void	connect();
	static	void	firstAuth_HELLO();
	static	void	clientAuth_AUTH();
	static	void	onClientReply_HELLO();

public:
	CAuthAccountServer(void);
	virtual ~CAuthAccountServer(void);
};
