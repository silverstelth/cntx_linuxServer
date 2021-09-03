/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MODULE_SOCKET_H__
#define __MODULE_SOCKET_H__

#include "net/message.h"
#include "module_common.h"

namespace SIPNET
{
	class IModuleSocket
	{
	public:
		virtual ~IModuleSocket() {}
		/** Register the socket in the module manager socket registry
		 */
		virtual void registerSocket() =0;
		/** Unregister the socket in the module manager socket registry
		 */
		virtual void unregisterSocket() =0;

		/** Ask derived class to obtain the socket name.
		 */
		virtual const std::string &getSocketName() =0;

		/** A plugged module send a message to another module.
		 *	If the destination module is not accessible through this socket,
		 *	an exception is thrown.
		 */
		virtual void sendModuleMessage(IModule *senderModule, TModuleId destModuleProxyId, const SIPNET::CMessage &message ) 
			throw (EModuleNotPluggedHere)
			=0;
		/** A plugged module send a message to all the module reachable
		 *	with this socket.
		 */
		virtual void broadcastModuleMessage(IModule *senderModule, const SIPNET::CMessage &message)
			throw (EModuleNotPluggedHere)
			=0;

		/** Fill the resultList with the list of module that are
		 *	reachable with this socket.
		 *	Note that the result vector is not cleared before filling.
		 */
		virtual void getModuleList(std::vector<IModuleProxy*> &resultList) =0;

		//@name Callback for socket implementation
		//@{
		/// Called just after a module is plugged in the socket.
		virtual void onModulePlugged(IModule *pluggedModule) =0;
		/// Called just before a module is unplugged from the socket.
		virtual void onModuleUnplugged(IModule *unpluggedModule) =0;
		//@}
	};

//	const TModuleSocketPtr	NullModuleSocket;


	/** A base class for socket.
	 *	It provide plugged module management to
	 *	implementors.
	 */
	class CModuleSocket : public IModuleSocket
	{
	protected:
		typedef SIPBASE::CTwinMap<TModuleId, TModulePtr>	TPluggedModules;
		/// The list of plugged modules
		TPluggedModules			_PluggedModules;

		bool					_SocketRegistered;

		friend class CModuleBase;

		CModuleSocket();
		~CModuleSocket();

		virtual void registerSocket();
		virtual void unregisterSocket();


		virtual void _onModulePlugged(const TModulePtr &pluggedModule);
		virtual void _onModuleUnplugged(const TModulePtr &pluggedModule);

		virtual void _sendModuleMessage(IModule *senderModule, TModuleId destModuleProxyId, const SIPNET::CMessage &message )
			throw (EModuleNotPluggedHere, SIPNET::EModuleNotReachable)
			=0;

		virtual void _broadcastModuleMessage(IModule *senderModule, const SIPNET::CMessage &message)
			throw (EModuleNotPluggedHere)
			=0;
	
		virtual void sendModuleMessage(IModule *senderModule, TModuleId destModuleProxyId, const SIPNET::CMessage &message ) 
			throw (EModuleNotPluggedHere);
		/** A plugged module send a message to all the module reachable
		 *	with this socket.
		 */
		virtual void broadcastModuleMessage(IModule *senderModule, const SIPNET::CMessage &message)
			throw (EModuleNotPluggedHere);

	};


} // namespace SIPNET

#endif // __MODULE_SOCKET_H__
