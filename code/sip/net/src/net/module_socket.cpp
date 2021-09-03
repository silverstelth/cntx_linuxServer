/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/module_socket.h"
#include "net/module.h"
#include "net/module_manager.h"
#include "net/module_gateway.h"
#include "net/module_common.h"

using namespace std;
using namespace SIPBASE;

namespace SIPNET
{

	CModuleSocket::CModuleSocket()
		: _SocketRegistered(false)
	{
	}

	CModuleSocket::~CModuleSocket()
	{
		unregisterSocket();
	}


	void CModuleSocket::registerSocket()
	{
		if (!_SocketRegistered)
		{
			_SocketRegistered = true;
			IModuleManager::getInstance().registerModuleSocket(this);
		}
	}

	void CModuleSocket::unregisterSocket()
	{
		if (_SocketRegistered)
		{
			IModuleManager::getInstance().unregisterModuleSocket(this);
			_SocketRegistered = false;
		}
	}

	void CModuleSocket::_onModulePlugged(const TModulePtr &pluggedModule)
	{
		TPluggedModules::TBToAMap::const_iterator it(_PluggedModules.getBToAMap().find(pluggedModule));
		if (it != _PluggedModules.getBToAMap().end())
		{
			throw IModule::EModuleAlreadyPluggedHere();
		}

		_PluggedModules.add(pluggedModule->getModuleId(), pluggedModule);

		// callback socket implementation
		onModulePlugged(pluggedModule);
	}

	void CModuleSocket::_onModuleUnplugged(const TModulePtr &pluggedModule)
	{
		TPluggedModules::TBToAMap::const_iterator it(_PluggedModules.getBToAMap().find(pluggedModule));
		if (it == _PluggedModules.getBToAMap().end())
		{
			throw EModuleNotPluggedHere();
		}

		// callback socket implementation
		onModuleUnplugged(pluggedModule);

		_PluggedModules.removeWithB(pluggedModule);
	}

	void CModuleSocket::sendModuleMessage(IModule *senderModule, TModuleId destModuleProxyId, const SIPNET::CMessage &message )
			throw (EModuleNotPluggedHere)
	{
		TPluggedModules::TBToAMap::const_iterator it(_PluggedModules.getBToAMap().find(senderModule));
		if (it == _PluggedModules.getBToAMap().end())
		{
			throw EModuleNotPluggedHere();
		}

		// forward to socket implementation
		_sendModuleMessage(senderModule, destModuleProxyId, message);

	}

	void CModuleSocket::broadcastModuleMessage(IModule *senderModule, const SIPNET::CMessage &message)
			throw (EModuleNotPluggedHere)
	{
		TPluggedModules::TBToAMap::const_iterator it(_PluggedModules.getBToAMap().find(senderModule));
		if (it == _PluggedModules.getBToAMap().end())
		{
			throw EModuleNotPluggedHere();
		}

		// forward to socket implementation
		_broadcastModuleMessage(senderModule, message);
	}

} // namespace SIPNET
