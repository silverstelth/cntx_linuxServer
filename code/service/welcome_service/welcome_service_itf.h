
/////////////////////////////////////////////////////////////////
// WARNING : this is a generated file, don't change it !
/////////////////////////////////////////////////////////////////

#ifndef WELCOME_SERVICE_ITF
#define WELCOME_SERVICE_ITF
#include "misc/types_sip.h"
#ifdef SIP_COMP_VC8
  #include <memory>
#endif
#include "misc/hierarchical_timer.h"
#include "misc/string_conversion.h"
#include "net/message.h"
#include "net/module.h"
#include "net/module_builder_parts.h"
#include "net/module_message.h"
#include "net/module_gateway.h"

#include "net/login_cookie.h"
#include "../../common/err.h"
#include "../Common/DBData.h"
	
namespace WS
{
	


	struct TUserRole
	{
		enum TValues
		{
			ur_player,
			ur_editor,
			ur_animator,
			/// the highest valid value in the enum
			last_enum_item = ur_animator,
			/// a value equal to the last enum item +1
			end_of_enum,

			invalid_val,
			
			/// Number of enumerated values
			nb_enum_items = 3
		};
		
		/// Index table to convert enum value to linear index table
		static std::map<TValues, uint32>	_IndexTable;
		
		// commented by ksr, for warning C4172
		//static const SIPBASE::CStringConversion<TValues> &getConversionTable() 
		static const SIPBASE::CStringConversion<TValues> getConversionTable()
		{
			SIP_BEGIN_STRING_CONVERSION_TABLE(TValues)
				SIP_STRING_CONVERSION_TABLE_ENTRY(ur_player)
				SIP_STRING_CONVERSION_TABLE_ENTRY(ur_editor)
				SIP_STRING_CONVERSION_TABLE_ENTRY(ur_animator)
				SIP_STRING_CONVERSION_TABLE_ENTRY(invalid_val)
			SIP_END_STRING_CONVERSION_TABLE(TValues, conversionTable, invalid_val)
			
			return conversionTable;
		}

		TValues	_Value;

	public:
		TUserRole()
			: _Value(invalid_val)
		{
		
			static bool init(false);
			if (!init)
			{
				// fill the index table
				_IndexTable.insert(std::make_pair(ur_player, 0));
				_IndexTable.insert(std::make_pair(ur_editor, 1));
				_IndexTable.insert(std::make_pair(ur_animator, 2));
			
				init = true;
			}
		
		}
		TUserRole(TValues value)
			: _Value(value)
		{
		}

		TUserRole(const std::string &str)
		{
			_Value = getConversionTable().fromString(str);
		}

		void serial(SIPBASE::IStream &s)
		{
			s.serialEnum(_Value);
		}

		bool operator == (const TUserRole &other) const
		{
			return _Value == other._Value;
		}
		bool operator != (const TUserRole &other) const
		{
			return ! (_Value == other._Value);
		}
		bool operator < (const TUserRole &other) const
		{
			return _Value < other._Value;
		}

		bool operator <= (const TUserRole &other) const
		{
			return _Value <= other._Value;
		}

		bool operator > (const TUserRole &other) const
		{
			return !(_Value <= other._Value);
		}
		bool operator >= (const TUserRole &other) const
		{
			return !(_Value < other._Value);
		}

		const std::string &toString() const
		{
			return getConversionTable().toString(_Value);
		}
		static const std::string &toString(TValues value)
		{
			return getConversionTable().toString(value);
		}

		TValues getValue() const
		{
			return _Value;
		}

		// return true if the actual value of the enum is valid, otherwise false
		bool isValid()
		{
			if (_Value == invalid_val)
				return false;

			// not invalid, check other enum value
			return getConversionTable().isValid(_Value);
		}

		
		uint32 asIndex()
		{
			std::map<TValues, uint32>::iterator it(_IndexTable.find(_Value));
			sipassert(it != _IndexTable.end());
			return it->second;
		}
		
	};
	
	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CWelcomeServiceSkel
	{
	public:
		/// the interceptor type
		typedef SIPNET::CInterceptorForwarder < CWelcomeServiceSkel>	TInterceptor;
	protected:
		CWelcomeServiceSkel()
		{
			// do early run time check for message table
			getMessageHandlers();
		}
		virtual ~CWelcomeServiceSkel()
		{
		}

		void init(SIPNET::IModule *module)
		{
			_Interceptor.init(this, module);
		}

		// unused interceptors 
		std::string			fwdBuildModuleManifest() const	{ return std::string(); }
		void				fwdOnModuleUp(SIPNET::IModuleProxy *moduleProxy)  {};
		void				fwdOnModuleDown(SIPNET::IModuleProxy *moduleProxy) {};
		void				fwdOnModuleSecurityChange(SIPNET::IModuleProxy *moduleProxy) {};
	
		// process module message interceptor
		bool fwdOnProcessModuleMessage(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message){return true;}
	private:

		typedef void (CWelcomeServiceSkel::*TMessageHandler)(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message);
		typedef std::map<std::string, TMessageHandler>	TMessageHandlerMap;

		const TMessageHandlerMap &getMessageHandlers() const
		{
			static TMessageHandlerMap map;
			return map;
		}
		
		void welcomeUser_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		void disconnectUser_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		// declare one interceptor member of the skeleton
		TInterceptor	_Interceptor;

		// declare the interceptor forwarder as friend of this class
		friend 		class SIPNET::CInterceptorForwarder < CWelcomeServiceSkel>;
	public:
		/////////////////////////////////////////////////////////////////
		// WARNING : this is a generated file, don't change it !
		/////////////////////////////////////////////////////////////////

		// ask the welcome service to welcome a character
		virtual void welcomeUser(SIPNET::IModuleProxy *sender, uint32 charId, const SIPNET::CLoginCookie &cookie, uint8 &priviledge, ucstring &exPriviledge, WS::TUserRole mode, uint32 instanceId) =0;
		// ask the welcome service to disconnect a user
		virtual void disconnectUser(SIPNET::IModuleProxy *sender, uint32 userId) =0;


	};

	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CWelcomeServiceProxy
	{
		/// Smart pointer on the module proxy
		SIPNET::TModuleProxyPtr	_ModuleProxy;

		// Pointer on the local module that implement the interface (if the proxy is for a local module)
		SIPNET::TModulePtr					_LocalModule;
		// Direct pointer on the server implementation interface for collocated module
		CWelcomeServiceSkel	*_LocalModuleSkel;


	public:
		CWelcomeServiceProxy(SIPNET::IModuleProxy *proxy)
		{
			sipassert(proxy->getModuleClassName() == "WelcomeService");
			_ModuleProxy = proxy;

			// initialize collocated servant interface
			if (proxy->getModuleDistance() == 0)
			{
				_LocalModule = proxy->getLocalModule();
				sipassert(_LocalModule != NULL);
				CWelcomeServiceSkel::TInterceptor *interceptor = NULL;
				interceptor = static_cast < SIPNET::CModuleBase* >(_LocalModule.getPtr())->getInterceptor(interceptor);
				sipassert(interceptor != NULL);

				_LocalModuleSkel = interceptor->getParent();
				sipassert(_LocalModuleSkel != NULL);
			}
			else
				_LocalModuleSkel = 0;

		}
		virtual ~CWelcomeServiceProxy()
		{
		}

		SIPNET::IModuleProxy *getModuleProxy()
		{
			return _ModuleProxy;
		}

		// ask the welcome service to welcome a character
		void welcomeUser(SIPNET::IModule *sender, uint32 charId, ucstring &userName, const SIPNET::CLoginCookie &cookie, uint8 &priviledge, ucstring &exPriviledge, WS::TUserRole mode, uint32 instanceId);
		// ask the welcome service to disconnect a user
		void disconnectUser(SIPNET::IModule *sender, uint32 userId);

		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_welcomeUser(SIPNET::CMessage &__message, uint32 charId, const std::string &userName, const SIPNET::CLoginCookie &cookie, const std::string &priviledge, const std::string &exPriviledge, WS::TUserRole mode, uint32 instanceId);
	
		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_disconnectUser(SIPNET::CMessage &__message, uint32 userId);
	



	};

	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CLoginServiceSkel
	{
	public:
		/// the interceptor type
		typedef SIPNET::CInterceptorForwarder < CLoginServiceSkel>	TInterceptor;
	protected:
		CLoginServiceSkel()
		{
			// do early run time check for message table
			getMessageHandlers();
		}
		virtual ~CLoginServiceSkel()
		{
		}

		void init(SIPNET::IModule *module)
		{
			_Interceptor.init(this, module);
		}

		// unused interceptors 
		std::string			fwdBuildModuleManifest() const	{ return std::string(); }
		void				fwdOnModuleUp(SIPNET::IModuleProxy *moduleProxy)  {};
		void				fwdOnModuleDown(SIPNET::IModuleProxy *moduleProxy) {};
		void				fwdOnModuleSecurityChange(SIPNET::IModuleProxy *moduleProxy) {};
	
		// process module message interceptor
		bool fwdOnProcessModuleMessage(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message);
	private:

		typedef void (CLoginServiceSkel::*TMessageHandler)(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message);
		typedef std::map<std::string, TMessageHandler>	TMessageHandlerMap;

		const TMessageHandlerMap &getMessageHandlers() const;
		
		void pendingUserLost_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		// declare one interceptor member of the skeleton
		TInterceptor	_Interceptor;

		// declare the interceptor forwarder as friend of this class
		friend 		class SIPNET::CInterceptorForwarder < CLoginServiceSkel>;
	public:
		/////////////////////////////////////////////////////////////////
		// WARNING : this is a generated file, don't change it !
		/////////////////////////////////////////////////////////////////

		// An awaited user did not connect before the allowed timeout expire
		virtual void pendingUserLost(SIPNET::IModuleProxy *sender, const SIPNET::CLoginCookie &cookie) =0;


	};

	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CLoginServiceProxy
	{
		/// Smart pointer on the module proxy
		SIPNET::TModuleProxyPtr	_ModuleProxy;

		// Pointer on the local module that implement the interface (if the proxy is for a local module)
		SIPNET::TModulePtr					_LocalModule;
		// Direct pointer on the server implementation interface for collocated module
		CLoginServiceSkel	*_LocalModuleSkel;


	public:
		CLoginServiceProxy(SIPNET::IModuleProxy *proxy)
		{

			_ModuleProxy = proxy;

			// initialize collocated servant interface
			if (proxy->getModuleDistance() == 0)
			{
				_LocalModule = proxy->getLocalModule();
				sipassert(_LocalModule != NULL);
				CLoginServiceSkel::TInterceptor *interceptor = NULL;
				interceptor = static_cast < SIPNET::CModuleBase* >(_LocalModule.getPtr())->getInterceptor(interceptor);
				sipassert(interceptor != NULL);

				_LocalModuleSkel = interceptor->getParent();
				sipassert(_LocalModuleSkel != NULL);
			}
			else
				_LocalModuleSkel = 0;

		}
		virtual ~CLoginServiceProxy()
		{
		}

		SIPNET::IModuleProxy *getModuleProxy()
		{
			return _ModuleProxy;
		}

		// An awaited user did not connect before the allowed timeout expire
		virtual void pendingUserLost(SIPNET::IModule *sender, const SIPNET::CLoginCookie &cookie){}

		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_pendingUserLost(SIPNET::CMessage &__message, const SIPNET::CLoginCookie &cookie);
	



	};

	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CWelcomeServiceClientSkel
	{
	public:
		/// the interceptor type
		typedef SIPNET::CInterceptorForwarder < CWelcomeServiceClientSkel>	TInterceptor;
	protected:
		CWelcomeServiceClientSkel()
		{
			// do early run time check for message table
			getMessageHandlers();
		}
		virtual ~CWelcomeServiceClientSkel()
		{
		}

		void init(SIPNET::IModule *module)
		{
			_Interceptor.init(this, module);
		}

		// unused interceptors 
		std::string			fwdBuildModuleManifest() const	{ return std::string(); }
		void				fwdOnModuleUp(SIPNET::IModuleProxy *moduleProxy)  {};
		void				fwdOnModuleDown(SIPNET::IModuleProxy *moduleProxy) {};
		void				fwdOnModuleSecurityChange(SIPNET::IModuleProxy *moduleProxy) {};
	
		// process module message interceptor
		bool fwdOnProcessModuleMessage(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message);
	private:

		typedef void (CWelcomeServiceClientSkel::*TMessageHandler)(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message);
		typedef std::map<std::string, TMessageHandler>	TMessageHandlerMap;

		const TMessageHandlerMap &getMessageHandlers() const;

		
		void registerWS_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		void reportWSOpenState_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		void welcomeUserResult_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		void updateConnectedPlayerCount_skel(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &__message);

		// declare one interceptor member of the skeleton
		TInterceptor	_Interceptor;

		// declare the interceptor forwarder as friend of this class
		friend 		class SIPNET::CInterceptorForwarder < CWelcomeServiceClientSkel>;
	public:
		/////////////////////////////////////////////////////////////////
		// WARNING : this is a generated file, don't change it !
		/////////////////////////////////////////////////////////////////

		// Register the welcome service in the ring session manager
		// The provided sessionId will be non-zero only for a shard with a fixed sessionId
		virtual void registerWS(SIPNET::IModuleProxy *sender, uint32 shardId, uint32 fixedSessionId, bool isOnline) =0;
		// WS report it's current open state
		virtual void reportWSOpenState(SIPNET::IModuleProxy *sender, bool isOnline) =0;
		// return for welcome user
		virtual void welcomeUserResult(SIPNET::IModuleProxy *sender, uint32 userId, bool ok, const std::string &shardAddr, const std::string &errorMsg) =0;
		// transmits the current player counts
		virtual void updateConnectedPlayerCount(SIPNET::IModuleProxy *sender, uint32 nbOnlinePlayers, uint32 nbPendingPlayers) =0;


	};

	/////////////////////////////////////////////////////////////////
	// WARNING : this is a generated file, don't change it !
	/////////////////////////////////////////////////////////////////
	class CWelcomeServiceClientProxy
	{
		/// Smart pointer on the module proxy
		SIPNET::TModuleProxyPtr	_ModuleProxy;

		// Pointer on the local module that implement the interface (if the proxy is for a local module)
		SIPNET::TModulePtr					_LocalModule;
		// Direct pointer on the server implementation interface for collocated module
		CWelcomeServiceClientSkel	*_LocalModuleSkel;


	public:
		CWelcomeServiceClientProxy(SIPNET::IModuleProxy *proxy)
		{

			_ModuleProxy = proxy;

			// initialize collocated servant interface
			if (proxy->getModuleDistance() == 0)
			{
				_LocalModule = proxy->getLocalModule();
				sipassert(_LocalModule != NULL);
				CWelcomeServiceClientSkel::TInterceptor *interceptor = NULL;
				interceptor = static_cast < SIPNET::CModuleBase* >(_LocalModule.getPtr())->getInterceptor(interceptor);
				sipassert(interceptor != NULL);

				_LocalModuleSkel = interceptor->getParent();
				sipassert(_LocalModuleSkel != NULL);
			}
			else
				_LocalModuleSkel = 0;

		}
		virtual ~CWelcomeServiceClientProxy()
		{
		}

		SIPNET::IModuleProxy *getModuleProxy()
		{
			return _ModuleProxy;
		}

		// Register the welcome service in the ring session manager
		// The provided sessionId will be non-zero only for a shard with a fixed sessionId
		virtual void registerWS(SIPNET::IModule *sender, uint32 shardId, uint32 fixedSessionId, bool isOnline){}
		// WS report it's current open state
		virtual void reportWSOpenState(SIPNET::IModule *sender, bool isOnline){}
		// return for welcome user
		virtual void welcomeUserResult(SIPNET::IModule *sender, uint32 userId, bool ok, const std::string &shardAddr, T_ERR errNo){}
		// transmits the current player counts
		virtual void updateConnectedPlayerCount(SIPNET::IModule *sender, uint32 nbOnlinePlayers, uint32 nbPendingPlayers){}

		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_registerWS(SIPNET::CMessage &__message, uint32 shardId, uint32 fixedSessionId, bool isOnline);
	
		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_reportWSOpenState(SIPNET::CMessage &__message, bool isOnline);
	
		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_welcomeUserResult(SIPNET::CMessage &__message, uint32 userId, bool ok, const std::string &shardAddr, const std::string &errorMsg);
	
		// Message serializer. Return the message received in reference for easier integration
		static const SIPNET::CMessage &buildMessageFor_updateConnectedPlayerCount(SIPNET::CMessage &__message, uint32 nbOnlinePlayers, uint32 nbPendingPlayers);
	



	};

}
	
#endif
