#include "misc/types_sip.h"
#include "misc/singleton.h"

#include "net/module_manager.h"
#include "net/module_builder_parts.h"

#include "welcome_service_itf.h"
#include "../../common/err.h"
#include "../Common/DBData.h"

T_ERR lsChooseShard (const SIPNET::CLoginCookie &cookie,
							uint8 &userPriv,
							WS::TUserRole userRole,
							uint32 instanceId,
							uint32 charSlot);


bool disconnectClient(uint32 userId);

namespace WS
{
	// welcome service module
	class CWelcomeServiceMod : 
		public SIPNET::CEmptyModuleCommBehav<SIPNET::CEmptyModuleServiceBehav<SIPNET::CEmptySocketBehav<SIPNET::CModuleBase> > >,
		public WS::CWelcomeServiceSkel,
		public SIPBASE::CManualSingleton<CWelcomeServiceMod>
	{
		/// the ring session manager module (if any)
		SIPNET::TModuleProxyPtr		_RingSessionManager;
		/// the login service module (if any)
		SIPNET::TModuleProxyPtr		_LoginService;

		void onModuleUp(SIPNET::IModuleProxy *proxy);
		void onModuleDown(SIPNET::IModuleProxy *proxy);


		////// CWelcomeServiceSkel implementation 

		// ask the welcome service to welcome a user
		virtual void welcomeUser(SIPNET::IModuleProxy *sender, uint32 charId, const SIPNET::CLoginCookie &cookie, uint8 &priviledge, ucstring &exPriviledge, WS::TUserRole mode, uint32 instanceId);
//		virtual void welcomeUser(SIPNET::IModuleProxy *sender, uint32 charId, ucstring &userName, const CLoginCookie &cookie, uint8 priviledge, ucstring &exPriviledge, WS::TUserRole mode, uint32 instanceId);

		// ask the welcome service to disconnect a user
		virtual void disconnectUser(SIPNET::IModuleProxy *sender, uint32 userId)
		{
			disconnectClient(userId);
		}

	public:
		CWelcomeServiceMod()
		{
			CWelcomeServiceSkel::init(this);
		}

		void reportWSOpenState(bool shardOpen)
		{
			if (_RingSessionManager == NULL) // skip if the RSM is offline
				return;

			CWelcomeServiceClientProxy wscp(_RingSessionManager);
			wscp.reportWSOpenState(this, shardOpen);
		}

		// forward response from the front end for a player slot to play in
		// to the client of this welcome service (usually the Ring Session Manager)
		void frontendResponse(SIPNET::IModuleProxy *waiterModule, uint32 userId, T_ERR &errNo, const SIPNET::CLoginCookie &cookie, const std::string &fsAddr)
		{
			CWelcomeServiceClientProxy wscp(waiterModule);
			wscp.welcomeUserResult(this, userId, errNo == ERR_NOERR, fsAddr, errNo);
		}

		// send the current number of players on this shard to the Ring Session Manager
		void updateConnectedPlayerCount(uint32 nbOnlinePlayers, uint32 nbPendingPlayers)
		{
			if (_RingSessionManager == NULL) // skip if the RSM is offline
				return;

			CWelcomeServiceClientProxy wscp(_RingSessionManager);
			wscp.updateConnectedPlayerCount(this, nbOnlinePlayers, nbPendingPlayers);
		}

		// inform the LS that a pending client is lost
		void pendingUserLost(const SIPNET::CLoginCookie &cookie);
	};

	struct TPendingFEResponseInfo
	{
		CWelcomeServiceMod			*WSMod;
		SIPNET::TModuleProxyPtr		WaiterModule;
		uint32						UserId;
	};
	typedef std::map<SIPNET::CLoginCookie, TPendingFEResponseInfo>	TPendingFeReponses;
	// the list of cookie string that are pending an
	TPendingFeReponses	PendingFeResponse;

} // namespace WS

void	cb_M_CS_CHANGE_SHARD_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_CS_ROOMINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_MS_CURRENT_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_MS_CHECK_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_MS_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_MS_CURRENT_CHECKIN(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_ENTER__SHARD(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_MOVE_USER_DATA_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_SEND_USER_DATA(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_CHANGE_NAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_CHANGE_USERNAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_REQ_ROOMINFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_SC_ROOMINFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_CS_DECEASED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_NT_REQ_DECEASED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_ToSpecialService(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_ToUID(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_CS_REQ_FAVORROOM_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_REQ_FAVORROOM_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid);

void	cb_M_NT_AUTOPLAY_REGISTER(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
