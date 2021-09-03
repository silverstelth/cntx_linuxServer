
#include	"stdnet.h"
#include	"net/AuthAccountServer.h"

CAuthAccountServer::CAuthAccountServer(void)
{
}

CAuthAccountServer::~CAuthAccountServer(void)
{
}

// packet : HELLO
void	CAuthAccountServer::onClientReply_HELLO()
{
	/*
	string		key;
	msgin.serial(key);
	uint8		con = 1;
	CMessage	msgout("OK");

	if( !keyPair(key) )
	{
		con = 0;
		msgout.serial(con);
		clientcb.send(msgout, from);
		disconnectUser(clientcb, from);
	}
	msgout.serial(con);
	// Send Public Key
	if ( con && !DontUseSecret )
	{
		_RSA *pTask= (_RSA *)AsymCryptServer->getCryptTask();
		uint32	nKeyLen		= pTask->getKeyLen();
		msgout.serial(nKeyLen);

		g_info		= AsymCryptServer->getKeyInfo();
		msgout.serial(g_info.pubkeySize);
		msgout.serialBuffer((uint8 *) g_info.pubKey, g_info.pubkeySize);
		sipdebug("Send Public Key!");
	}

	clientcb.send(msgout, from);
	*/
}
