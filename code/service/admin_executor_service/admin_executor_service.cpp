/** \file admin_executor_service.cpp
 * Admin Executor Service (AES)
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#	define SIPNS_CONFIG ""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#	define SIPNS_LOGS ""
#endif // SIPNS_LOGS

#include "misc/types_sip.h"
#include "misc/i18n.h"
#include "misc/system_info.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <direct.h>
#	include "shlwapi.h"
#	include "tchar.h"
#	include "misc/win_thread.h"
#	undef max
#	undef min
#else
#	include <unistd.h>
#	include <errno.h>
#	include "misc/p_thread.h"
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
#include "misc/HttpDown.h"
#include "net/service.h"
#include "net/unified_network.h"
#include "net/varpath.h"
#include "net/email.h"
#include "net/admin.h"

#include "../Common/DBData.h"
#include "../Common/Common.h"
#include "log_report.h"
#include "../../common/Packet.h"
#include "../../common/commonForAdmin.h"

//
// Namespaces
//
 
using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CLogReport MainLogReport;

//
// Structures
//

struct CRequest
{
	CRequest(uint32 id, uint16 sid) : Id(id), NbWaiting(0), NbReceived(0), SId(sid)
	{
		sipdebug ("REQUEST: ++ NbWaiting %d NbReceived %d", NbWaiting, NbReceived);
		Time = CTime::getSecondsSince1970 ();
	}

	uint32			Id;
	uint			NbWaiting;
	uint32			NbReceived;
	uint16			SId;
	uint32			Time;	// when the request was ask

	TAdminViewResult Answers;
};

struct CService
{
	CService() { reset(); }

	string			AliasName;		/// alias of the service used in the AES and AS to find him (unique per AES)
									/// If alias name is not empty, it means that the service was registered

	string			ShortName;		/// name of the service in short format ("NS" for example)
	string			LongName;		/// name of the service in long format ("naming_service")
	uint16			ServiceId;		/// service id of the service.
	bool			Ready;			/// true if the service is ready
	bool			Connected;		/// true if the service is connected to the AES
	vector<CSerialCommand>	Commands;
	bool			AutoReconnect;	/// true means that AES must relaunch the service if lost
	uint32			PId;			/// process Id used to kill the application
	bool			Relaunch;		/// if true, it means that the admin want to close and relaunch the service
	uint32			ShardId;

	uint32			LastPing;		/// time in seconds of the last ping sent. If 0, means that the service already pong

	vector<uint32>	WaitingRequestId;		/// contains all request that the server hasn't reply yet

	TREGPORTARY		PortAry;

	string toString() const
	{
		return getServiceUnifiedName();
	}

	void init(const string &shortName, uint16 serviceId)
	{
		reset();

		ShortName = shortName;
		ServiceId = serviceId;
		Connected = true;
	}

	void reset()
	{
		AliasName.clear();
		ShortName.clear();
		LongName.clear();
		ServiceId = 0;
		Ready = false;
		Connected = false;
		Commands.clear();
		AutoReconnect = false;
		PId = 0;
		Relaunch = false;
		LastPing = 0;
		WaitingRequestId.clear();
		PortAry.clear();
	}

	std::string getServiceUnifiedName() const
	{
		sipassert(!ShortName.empty());
		string res;
		if(!AliasName.empty())
		{
			res = AliasName+"/";
		}
		res += ShortName;
		if(ServiceId != 0)
		{
			res += "-";
			res += SIPBASE::toStringA(ServiceId);
		}
		if(res.empty())
			res = "???";
		return res;
	}
};


//
// CVariables (variables that can be set in cfg files, etc)
//

CVariable<uint16>	AESPort("aes", "AESPort", "Listen Port for AES", 59997, 0, true);
CVariable<string> ShardName("aes","ShardName","the shard name to send to the AS in explicit registration","local",0,true);
CVariable<uint32> RequestTimeout("aes","RequestTimeout", "in seconds, time before a request is timeout", 5, 0, true);		// in seconds, timeout before canceling the request
CVariable<uint32> PingTimeout("aes","PingTimeout", "in seconds, time before services have to answer the ping message or will be killed", 900, 0, true);		// in seconds, timeout before killing the service
CVariable<uint32> PingFrequency("aes","PingFrequency", "in seconds, time between each ping message", 60, 0, true);		// in seconds, frequency of the send ping to services
CVariable<bool>   KillServicesOnDisconnect("aes","KillServicesOnDisconnect", "if set, call killProgram on services as they disconnect", false, 0, true);
static	string		s_sVersion = "Unknown";
static	string		s_sUpdateExe = "Update.exe";
CVariable<string> AdminMainServiceName("aes","AdminMainServiceName","the system service name of adminmain","AdminMain",0,true);
CVariable<string> AdminExecutorServiceName("aes","AdminExecutorServiceName","the system service name of adminexecutor","AdminExecutor",0,true);
CVariable<string> AdminMainServiceExePath("aes","AdminMainServiceExePath","the service exe path of adminmain","01_admin_service",0,true);
CVariable<string> AdminMainServiceExeName("aes","AdminMainServiceExeName","the service exe name of adminmain","admin_main_service",0,true);
CVariable<string> AdminExeServiceExeName("aes","AdminExeServiceExeName","the service exe name of adminexecutor","admin_executor_service",0,true);
//
// Global Variables (containers)
//
	
typedef vector<CService> TServices;
TServices Services;

vector<CRequest> Requests;
vector<string> RegisteredServices;
std::map< std::string, bool >	_UniqueServices;
vector<pair<uint32, string> > WaitingToLaunchServices;	// date and alias name
vector<string> AllAdminAlarms;	// contains *all* alarms
vector<string> AllGraphUpdates;	// contains *all* the graph updates


// 써비스들의 root path
string sServiceRootPath;

string	GetServiceRootPath()
{
	return sServiceRootPath;
}
string	GetPathFromAliasName(string _sAliasName)
{
	string	sRelative;
	MAP_MSTSERVICECONFIG::iterator it;
	for ( it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++ )
	{
		if ( it->second.sAliasName == _sAliasName )
		{
			sRelative = it->second.sRelativePath;
			break;
		}
	}

	if (it == map_MSTSERVICECONFIG.end())
	{
		return GetServiceRootPath();
	}

	string	sPath = GetServiceRootPath() + sRelative + "\\";
	return sPath;
}

string	GetExeNameFromAliasName(string _sAliasName)
{
	string	sExe = "";
	MAP_MSTSERVICECONFIG::iterator it;
	for ( it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++ )
	{
		if ( !strcmp( it->second.sAliasName.c_str(), _sAliasName.c_str()) )
		{
			sExe = it->second.sExeName;
			break;
		}
	}

	return sExe;
}

string	GetAdminExePath(string _sExeName)
{
	string	sPath = GetServiceRootPath();
	sPath = sPath + AdminMainServiceExePath.get() + "/";

	return sPath;
}


string	GetPathFromExeName(string _sExeName)
{
	MAP_MSTSERVICECONFIG::iterator it;
	for ( it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++ )
	{
		if ( it->second.sExeName == _sExeName )
			break;
	}

	if (it == map_MSTSERVICECONFIG.end())
	{
		return GetServiceRootPath();
	}

	string	sRelative = it->second.sRelativePath;
	string	sPath = GetServiceRootPath() + sRelative + "/";
	return sPath;
}

void	SetServiceRootPathAndRelativePath()
{
	// Set root path
	if (IService::getInstance()->ConfigFile.exists("ServiceRootPath"))
		sServiceRootPath = IService::getInstance()->ConfigFile.getVar("ServiceRootPath").asString();
	else
		sServiceRootPath = CPath::getExePathA();

	sipinfo("Service root path = %s", sServiceRootPath.c_str());
}

void	LoadAdminConfig()
{
	map_MSTSERVICECONFIG.clear();

	CConfigFile::CVar* varExe	= IService::getInstance()->ConfigFile.getVarPtr ("RegExeInfoList");

	if (varExe == NULL)
	{
		sipwarning("There is no 'RegExeInfoList' config.");
		return;
	}
	uint32	uExeNum		= varExe->size();
	for (uint32 i = 0; i < uExeNum; i= i+5)
	{
		std::string		sAlias		= varExe->asString(i);
		std::string		sShortName	= varExe->asString(i+1);
		std::string		sExe	= varExe->asString(i+2);
		std::string		sPath	= varExe->asString(i+3);
		uint32			nCfgFlag= varExe->asInt(i+4);

		uint32			uIndex = i / 5;
		MSTSERVICECONFIG	info(uIndex, sAlias, sShortName, sExe, sPath, nCfgFlag);
		map_MSTSERVICECONFIG.insert(make_pair(uIndex, info));
	}
}

//
// Global Variables (scalars)
//



//
// Alarms
//

void sendInformations(uint16 sid)
{
	CMessage msgout(M_ADMIN_INFO);
	msgout.serialCont(AllAdminAlarms);
	msgout.serialCont(AllGraphUpdates);
	for (uint j = 0; j < Services.size(); j++)
	{
		if (Services[j].ServiceId == sid && Services[j].Connected)
		{
			CUnifiedNetwork::getInstance()->send(TServiceId(Services[j].ServiceId), msgout);
		}
	}
}


//
// Launch services functions
//

void sendAdminEmail(const char *format, ...)
{
	char *text;
	SIPBASE_CONVERT_VARGS(text, format, 4096);
	
	CMessage msgout(M_ADMIN_ADMIN_EMAIL);
	string shard = ShardName;
	string sT = text;
	string str = "Shard " + shard + " " + sT;
	msgout.serial(str);
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);

	sipinfo("Forwarded email to AS with '%s'", text);
}

static void cbAdminEmail(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	string str;
	msgin.serial(str);
	sendAdminEmail(str.c_str());
}

static void cbGraphUpdate(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	CMessage msgout(M_ADMIN_GRAPH_UPDATE);
	uint32 CurrentTime;
	msgin.serial(CurrentTime);
	msgout.serial(CurrentTime);
	while (msgin.getPos() < (sint32)msgin.length())
	{
		string service, var;
		sint32 val;
		msgin.serial(service, var, val);
		msgout.serial(service, var, val);
	}
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);

	sipinfo("GRAPH: Forwarded graph update to AS");
}

// Packet : M_ADMIN_SHARDID
static void cbShardID(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	CMessage	msgout(M_ADMIN_SHARDID);

	uint32	nSize = 1;								msgout.serial(nSize);
	uint16	serviceID = tsid.get();					msgout.serial(serviceID);
	uint32	shardID;		msgin.serial(shardID);	msgout.serial(shardID);
	Services[serviceID].ShardId = shardID;

	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
}

// decode a service in a form 'alias/shortname-sid'
void decodeUnifiedName(const string &unifiedName, string &alias, string &shortName, uint16 &sid)
{
	size_t pos1 = 0, pos2 = 0;
	pos1 = unifiedName.find("/");
	if (pos1 != string::npos)
	{
		alias = unifiedName.substr(0, pos1);
		pos1++;
	}
	else
	{
		alias = "";
		pos1 = 0;
	}
	pos2 = unifiedName.find("-");
	if (pos2 != string::npos)
	{
		shortName = unifiedName.substr(pos1,pos2-pos1);
		sid = atoi(unifiedName.substr(pos2+1).c_str());
	}
	else
	{
		shortName = unifiedName.substr(pos1);
		sid = 0;
	}

	if (alias.empty())
	{
		alias = shortName;
	}
}


bool getServiceLaunchInfo(const string& unifiedName,string& alias,string& command,string& path,string& arg)
{
	string shortName;
	uint16 sid;
	decodeUnifiedName(unifiedName, alias, shortName, sid);
	try
	{
		path = IService::getInstance()->ConfigFile.getVar(alias).asString(0);
		command = IService::getInstance()->ConfigFile.getVar(alias).asString(1);
	}
	catch(EConfigFile &e)
	{
		sipwarning("Error in service '%s' in config file (%s)", unifiedName.c_str(), e.what());
		return false;
	}

	if (IService::getInstance()->ConfigFile.getVar(alias).size() >= 3)
	{
		arg = IService::getInstance()->ConfigFile.getVar(alias).asString(2);
	}

	path = SIPBASE::CPath::standardizePath(path);
	SIPBASE::CFileA::createDirectoryTree(path);
	return true;
}

std::string getServiceStateFileName(const std::string& serviceAlias,const std::string& serviceExecutionPath)
{
	return SIPBASE::CPath::standardizePath(serviceExecutionPath)+serviceAlias+".state";
}

// the following routine reads the text string contained in the ".state" file for this service
// it's used to provide a 'state' value for services that are registered but are not connected
// to give info on whether they've been launched, whether their launcher is online, etc
std::string getOfflineServiceState(const std::string& serviceAlias,const std::string& serviceExecutionPath)
{
	// open the file for reading
	FILE* f= fopen(getServiceStateFileName(serviceAlias,serviceExecutionPath).c_str(),"rt");
	if (f==NULL) return "Offline";

	// setup a buffer to hold the text read from the file
	uint32 fileSize= SIPBASE::CFileA::getFileSize(f);
	std::string txt;
	txt.resize(fileSize);

	// read the text from the file - note that the number of bytes read may be less than the
	// number of bytes requested because we've opened the file in text mode and not binary mode
	size_t bytesRead= fread(&txt[0],1,fileSize,f);
	txt.resize(bytesRead);
	fclose(f);

	// return the text read from the file
	return txt;
}


const char* LaunchCtrlStart= "LAUNCH";
const char* LaunchCtrlStop= "STOP";

std::string getServiceLaunchCtrlFileName(const std::string& serviceAlias,const std::string& serviceExecutionPath,bool delay)
{
	return SIPBASE::CPath::standardizePath(serviceExecutionPath)+serviceAlias+(delay?"_waiting":"_immediate")+".launch_ctrl";
}

bool writeServiceLaunchCtrl(const std::string& serviceAlias,const std::string& serviceExecutionPath,bool delay,const std::string& txt)
{
	// make sure the path exists
	SIPBASE::CFileA::createDirectoryTree(serviceExecutionPath);

	// open the file for writing
	FILE* f= fopen(getServiceLaunchCtrlFileName(serviceAlias,serviceExecutionPath,delay).c_str(),"wt");
	if (f==NULL) return false;

	// write the text to the file
	fprintf(f,"%s",txt.c_str());
	fclose(f);

	return true;
}

bool writeServiceLaunchCtrl(uint32 serviceId,bool delay,const std::string& txt)
{
	// run trough the services container looking for a match for the service id that we've been given
	for (TServices::iterator it= Services.begin(); it!=Services.end(); ++it)
	{
		if (it->ServiceId==serviceId)
		{
			// we found a match for the service id so try to do something sensible with it...

			// get hold of the different components of the command description...
			string alias, command, path, arg;
			bool ok= getServiceLaunchInfo(it->AliasName,alias,command,path,arg);
			if (!ok) return false;

			// go ahead and write the launch ctrl file...
			return writeServiceLaunchCtrl(alias,path,delay,txt);
		}
	}

	// we failed to find a match for the serviceId that we've been given so complain and return false
	sipwarning("Failed to write launch_ctrl file for unknown service: %u",serviceId);
	return false;
}

// start a service imediatly
bool startService(const string &unifiedName)
{
	// lookup the alias, command to execute, execution path and defined command args for the given service
	string alias, command, path, arg;
	bool ok= getServiceLaunchInfo(unifiedName,alias,command,path,arg);

	// make sure the alias, command, etc were setup ok
	if (!ok) return false;
	sipinfo("Starting the service alias '%s'", alias.c_str());
	
	bool dontLaunchServicesDirectly= IService::getInstance()->ConfigFile.exists("DontLaunchServicesDirectly")? IService::getInstance()->ConfigFile.getVar("DontLaunchServicesDirectly").asBool(): false;
	if (!dontLaunchServicesDirectly)
	{	
		// give the service alias to the service to forward it back when it will connected to the aes.
		arg += " -N";
		arg += alias;

		// set the path for running
		arg += " -A";
		arg += path;
		
		// suppress output to stdout
		#ifdef SIP_OS_WINDOWS
			arg += " >NUL:";
		#else
			arg += " >/dev/null";
		#endif

		// launch the service
		bool res = launchProgram(command, arg);
		
		// if launching ok, leave 1 second to the new launching service before lauching next one
		if (res)
			sipSleep(1000);
		
		return res;
	}
	else
	{
		// there's some other system responsible for launching apps so just set a flag
		// for the system in question to use
		return writeServiceLaunchCtrl(alias,path,false,LaunchCtrlStart);
	}
}


// start service in future
void startService(uint32 delay, const string &unifiedName)
{
	// make sure there really is a delay specified - otherwise just launch the servicde directly
	if (delay == 0)
	{
		startService(unifiedName);
		return;
	}

	// lookup the alias, command to execute, execution path and defined command args for the given service
	string alias, command, path, arg;
	bool ok= getServiceLaunchInfo(unifiedName,alias,command,path,arg);

	// make sure the alias, command, etc were setup ok
	if (!ok) return;
	sipinfo("Setting up restart for service '%s'", alias.c_str());

	// check whether a config file variable has been set to signal that some other process is responsible for launching services
	bool dontLaunchServicesDirectly= IService::getInstance()->ConfigFile.exists("DontLaunchServicesDirectly")? IService::getInstance()->ConfigFile.getVar("DontLaunchServicesDirectly").asBool(): false;
	if (dontLaunchServicesDirectly)
	{
		// there's some other system responsible for launching apps so just set a flag
		// for the system in question to use
		writeServiceLaunchCtrl(alias,path,true,LaunchCtrlStart);
		return;
	}

	// check that the service isn't already in the queue of waiting services
	for(uint i = 0; i < WaitingToLaunchServices.size(); i++)
	{
		if (WaitingToLaunchServices[i].second == alias)
		{
			sipwarning("Service %s already in waiting queue to launch", unifiedName.c_str());
			return;
		}
	}

	// queue up this service for launching
	sipinfo("Relaunching the service %s in %d seconds", unifiedName.c_str(), delay);
	WaitingToLaunchServices.push_back(make_pair(CTime::getSecondsSince1970() + delay, unifiedName));
}

void checkWaitingServices()
{
	uint32 d = CTime::getSecondsSince1970();

	for(uint i = 0; i < WaitingToLaunchServices.size(); )
	{
		if (WaitingToLaunchServices[i].first <= d)
		{
			startService(WaitingToLaunchServices[i].second);
			WaitingToLaunchServices.erase(WaitingToLaunchServices.begin()+i);
		}
		else
		{
			i++;
		}
	}
}

static void checkPingPong()
{
	static uint32 LastPing = 0;		// contains the date of the last ping sent to the services
	uint32 d = CTime::getSecondsSince1970();

	bool allPonged = true;
	bool haveService = false;
	
	for(uint i = 0; i < Services.size(); i++)
	{
		if(Services[i].Ready)
		{
			haveService = true;
			if(Services[i].LastPing != 0)
			{
				allPonged = false;
				if(d > Services[i].LastPing + PingTimeout)
				{
					sipwarning("Service %s-%hu seems dead, no answer for the last %d second, I kill it right now and relaunch it in few time : curTime=%d, lastPing=%d", Services[i].LongName.c_str(), Services[i].ServiceId,(uint32)PingTimeout, (uint32)d, (uint32)Services[i].LastPing);
					Services[i].AutoReconnect = false;
					Services[i].Relaunch = true;
					abortProgram(Services[i].PId);
					Services[i].LastPing = 0;

					//Notify to AS
					CMessage	msgout(M_AES_SERVICE_NOTPONG);
					msgout.serial(Services[i].ShortName);
					msgout.serial(Services[i].ServiceId);
					CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
				}
			}
		}
	}

	if(haveService && allPonged && d > LastPing + PingFrequency)
	{
		LastPing = d;
		for(uint i = 0; i < Services.size(); i++)
		{
			if(Services[i].Ready)
			{
				Services[i].LastPing = d;

				CMessage msgout(M_ADMIN_ADMIN_PING);
				CUnifiedNetwork::getInstance()->send(TServiceId(Services[i].ServiceId), msgout);
			}
		}
	}
}

// check to see if a file exists that should provoke a shutdown of running services
// this is typically used in the deployment process to ensure that all services are
// given a chance to shut down cleanly before servre patches are applied
CVariable<string> ShutdownRequestFileName("aes","ShutdownRequestFileName", "name of the file to use for shutdown requests", "./global.launch_ctrl", 0, true);
static void checkShutdownRequest()
{
	// a little system to prevent us from eating too much CPU on systems that have a cstly 'fileExists()'
	static uint32 count=0;
	if ((++count)<10)	return;
	count=0;

	// if there's no ctrl file to be found then giveup
	if (!SIPBASE::CFileA::fileExists(ShutdownRequestFileName)) return;

	// if a shutdown ctrl file exists then read it's contents (if the file doesn't exist this returns an empty string)
	CSString fileContents;
	fileContents.readFromFile(ShutdownRequestFileName.c_str());

	// see if the file exists
	if (!fileContents.empty())
	{
		SIPBASE::CFileA::deleteFile(ShutdownRequestFileName);
		fileContents = fileContents.strip().splitToOneOfSeparators(_t(" \t\n\r\x1a"));
		// get rid of any unwanted junk surrounding the file contents
		sipinfo(_t("Treating shutdown request from ctrl file %S: %s"), ShutdownRequestFileName.c_str(), (_t("#.State=") + fileContents).c_str());
		SIPBASE::ICommand::execute("getViewAES #.State=" + SIPBASE::toString("%S", fileContents.c_str()), *SIPBASE::InfoLog);
	}
}

static void cbAdminPong(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	for(uint i = 0; i < Services.size(); i++)
	{
		if(Services[i].ServiceId == sid)
		{
			if(Services[i].LastPing == 0)
			{
				sipwarning("Received a pong from service %s-%hu but we didn't expect a pong from it", serviceName.c_str(), sid);
			}
			else
			{
				Services[i].LastPing = 0;
			}
			return;
		}
	}
	sipwarning("Received a pong from service %s-%hu that is not in my service list", serviceName.c_str(), sid);
}


//
// Request functions
//

void addRequestWaitingNb(uint32 rid)
{
	for (uint i = 0 ; i < Requests.size(); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].NbWaiting++;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting+ %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			// if we add a waiting, reset the timer
			Requests[i].Time = CTime::getSecondsSince1970();
			return;
		}
	}
	sipwarning("addRequestWaitingNb: can't find the rid %d", rid);
}

void subRequestWaitingNb(uint32 rid)
{
	for (uint i = 0 ; i < Requests.size(); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].NbWaiting--;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting- %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			return;
		}
	}
	sipwarning("subRequestWaitingNb: can't find the rid %d", rid);
}

void aesAddRequestAnswer(uint32 rid, const TAdminViewResult& answer)
{
	for (uint i = 0 ; i < Requests.size(); i++)
	{
		if (Requests[i].Id == rid)
		{
			for (uint t = 0; t < answer.size(); t++)
			{
				if (!answer[t].VarNames.empty() && answer[t].VarNames[0] == "__log")
				{	sipassert(answer[t].VarNames.size() == 1); }
				else
				{	sipassert(answer[t].VarNames.size() == answer[t].Values.size()); }
				Requests[i].Answers.push_back(SAdminViewRow(answer[t].VarNames, answer[t].Values));
			}
			Requests[i].NbReceived++;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting %d NbReceived+ %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			return;
		}
	}
	// we received an unknown request, forget it
	sipwarning("Receive an answer for unknown request %d", rid);
}


void aesAddRequestAnswer(uint32 rid, TAdminViewVarNames& varNames, const TAdminViewValues& values)
{
	if (!varNames.empty() && varNames[0] == "__log")
	{	sipassert(varNames.size() == 1); }
	else
	{	sipassert(varNames.size() == values.size()); }

	for (uint i = 0 ; i < Requests.size(); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].Answers.push_back(SAdminViewRow(varNames, values));

			Requests[i].NbReceived++;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting %d NbReceived+ %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			
			return;
		}
	}
	// we received an unknown request, forget it
	sipwarning("Receive an answer for unknown request %d", rid);
}

void cleanRequests()
{
	uint32 currentTime = CTime::getSecondsSince1970();

	// just a debug check
	for (uint t = 0 ; t < Requests.size(); t++)
	{
		uint32 NbWaiting = Requests[t].NbWaiting;
		uint32 NbReceived = Requests[t].NbReceived;

		uint32 NbRef = 0;
		for (uint j = 0; j < Services.size(); j++)
		{
			if (Services[j].Connected)
			{
				for (uint k = 0; k < Services[j].WaitingRequestId.size(); k++)
				{
					if(Services[j].WaitingRequestId[k] == Requests[t].Id)
					{
						NbRef++;
					}
				}
			}
		}
		sipinfo("REQUEST: Waiting request %d: NbRef %d NbWaiting %d NbReceived %d", Requests[t].Id, NbRef, NbWaiting, NbReceived);
		
		if (NbRef != NbWaiting - NbReceived)
		{
			sipwarning("REQUEST: **** i %d rid %d -> NbRef(%d) != NbWaiting(%d) - NbReceived(%d) ", t, Requests[t].Id, NbRef, NbWaiting, NbReceived);
		}
	}

	for (uint i = 0 ; i < Requests.size();)
	{
		// timeout
		if (currentTime >= Requests[i].Time+RequestTimeout)
		{
			sipwarning("REQUEST: Request %d timeouted, only %d on %d services have replied", Requests[i].Id, Requests[i].NbReceived, Requests[i].NbWaiting);

			TAdminViewVarNames varNames;
			TAdminViewValues values;
			
			varNames.push_back("service");
			for (uint j = 0; j < Services.size(); j++)
			{
				if (Services[j].Connected)
				{
					for (uint k = 0; k < Services[j].WaitingRequestId.size(); k++)
					{
						if(Services[j].WaitingRequestId[k] == Requests[i].Id)
						{
							// this services didn't answer
							string s;
							if(Services[j].AliasName.empty())
								s = Services[j].ShortName;
							else
								s = Services[j].AliasName;
							s += "-"+toStringA(Services[j].ServiceId);
							s += "((TIMEOUT))"; 
							values.clear();
							values.push_back(s);
							aesAddRequestAnswer(Requests[i].Id, varNames, values);
							break;
						}
					}
				}
			}
			if (Requests[i].NbWaiting != Requests[i].NbReceived)
			{
				sipwarning("REQUEST: **** i %d rid %d -> Requests[i].NbWaiting(%d) != Requests[i].NbReceived(%d)", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
				sipwarning("REQUEST: Need to add dummy answer");
				values.clear();
				values.push_back("UnknownService");
				while (Requests[i].NbWaiting != Requests[i].NbReceived)
					aesAddRequestAnswer(Requests[i].Id, varNames, values);
			}
		}

		if (Requests[i].NbWaiting <= Requests[i].NbReceived)
		{
			// the request is over, send to the php

			CMessage msgout(M_ADMIN_VIEW);
			msgout.serial(Requests[i].Id);

			for (uint j = 0; j < Requests[i].Answers.size(); j++)
			{
				msgout.serialCont(Requests[i].Answers[j].VarNames);
				msgout.serialCont(Requests[i].Answers[j].Values);
			}

			if (Requests[i].SId == 0)
			{
				sipinfo("REQUEST: Receive an answer for the fake request %d with %d answers", Requests[i].Id, Requests[i].Answers.size());
				for (uint j = 0; j < Requests[i].Answers.size(); j++)
				{
					uint k;
					for (k = 0; k < Requests[i].Answers[j].VarNames.size(); k++)
					{
						InfoLog->displayRaw("%-20s ", Requests[i].Answers[j].VarNames[k].c_str());
					}
					InfoLog->displayRawNL("");
					for (k = 0; k < Requests[i].Answers[j].Values.size(); k++)
					{
						InfoLog->displayRaw("%-20s", Requests[i].Answers[j].Values[k].c_str());
					}
					InfoLog->displayRawNL("");
					InfoLog->displayRawNL("----------------------------------------------");
				}	
			}
			else
				CUnifiedNetwork::getInstance()->send(TServiceId(Requests[i].SId), msgout);

			// set to 0 to erase it
			Requests[i].NbWaiting = 0;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting0 %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
		}

		if (Requests[i].NbWaiting == 0)
		{
			Requests.erase(Requests.begin()+i);
		}
		else
		{
			i++;
		}
	}
}

//
// Functions
//

void findServices(const string &name, uint16 sid, vector<TServices::iterator> &services)
{
	string alias, shortName;
//	uint16 sid; //  [3/18/2010 KimSuRyon]
	decodeUnifiedName(name, alias, shortName, sid);

	services.clear();

	// try to find if the name match an alias nameg
	TServices::iterator sit;
	for (sit = Services.begin(); sit != Services.end(); sit++)
	{
		if ((*sit).AliasName == alias)
		{
			services.push_back(sit);
		}
	}
	// if we found some alias, it's enough, return
	if(!services.empty())
		return;
	// not found in short name, try with serviceid
	for (sit = Services.begin(); sit != Services.end(); sit++)
	{
		if (sid != 0 && (*sit).ServiceId== sid) 
		{
			services.push_back(sit);
		}
	}
	// if we found some alias, it's enough, return
	if(!services.empty())
		return;
	// not found in alias, try with short name
	for (sit = Services.begin(); sit != Services.end(); sit++)
	{
		if ((*sit).ShortName == shortName) 
		{
			services.push_back(sit);
		}
	}
}

bool isRegisteredService(const string &name)
{
	string alias, shortName;
	uint16 sid;
	decodeUnifiedName(name, alias, shortName, sid);

	for (uint i = 0; i != RegisteredServices.size(); i++)
		if (RegisteredServices[i] == alias)
			return true;
	return false;
}

TServices::iterator findService(uint32 sid, bool asrt = true)
{
	TServices::iterator sit;
	for (sit = Services.begin(); sit != Services.end(); sit++)
	{
		if ((*sit).ServiceId == sid) break;
	}
	if (asrt)
		sipassert(sit != Services.end());
	return sit;
}

void treatRequestForRunnignService(uint32 rid, TServices::iterator sit, const string& viewStr)
{
	CVarPath subvarpath(viewStr);

	bool send = true;

	// check if the command is not to stop the service
	for (uint k = 0; k < subvarpath.Destination.size(); k++)
	{
		if (subvarpath.Destination[k].first == "State=0")
		{
			// If we stop the service, we don't have to reconnect the service
			sit->AutoReconnect = false;
			writeServiceLaunchCtrl(sit->ServiceId,true,LaunchCtrlStop);
		}
		else if (subvarpath.Destination[k].first == "State=-1")
		{
			sit->AutoReconnect = false;
			killProgram(sit->PId);
			send = false;
			writeServiceLaunchCtrl(sit->ServiceId,true,LaunchCtrlStop);
		}
		else if (subvarpath.Destination[k].first == "State=-2")
		{
			sit->AutoReconnect = false;
			abortProgram(sit->PId);
			send = false;
			writeServiceLaunchCtrl(sit->ServiceId,true,LaunchCtrlStop);
		}
		else if (subvarpath.Destination[k].first == "State=2")
		{
			sit->AutoReconnect = false;
			sit->Relaunch = true;
			writeServiceLaunchCtrl(sit->ServiceId,true,LaunchCtrlStart);
		}
		else if (subvarpath.Destination[k].first == "State=3")
		{
			sit->AutoReconnect = false;
			sit->Relaunch = true;
			killProgram(sit->PId);
			send = false;
			writeServiceLaunchCtrl(sit->ServiceId,true,LaunchCtrlStart);
		}
	}

	if (send)
	{
		// now send the request to the service
		addRequestWaitingNb(rid);
		sit->WaitingRequestId.push_back(rid);
		CMessage msgout(M_ADMIN_GET_VIEW);
		msgout.serial(rid);
		msgout.serial(const_cast<string&>(viewStr));
		sipassert(sit->ServiceId);
		CUnifiedNetwork::getInstance()->send(TServiceId(sit->ServiceId), msgout);
		sipinfo("REQUEST: Sent view '%s' to service '%s'", viewStr.c_str(), sit->toString().c_str());
	}
}

void treatRequestOneself(uint32 rid, const string& viewStr)
{
	addRequestWaitingNb(rid);
	TAdminViewResult answer;
	serviceGetView(rid, viewStr, answer);
	aesAddRequestAnswer(rid, answer);
	sipinfo("REQUEST: Treated view myself directly: '%s'", viewStr.c_str());
}	

void treatRequestForOfflineService(uint32 rid, const string& serviceName, const string& viewStr)
{
	CVarPath subvarpath(viewStr);

	addRequestWaitingNb(rid);

	TAdminViewVarNames varNames;
	TAdminViewValues values;
	
	// add default row
	varNames.push_back("service");
	values.push_back(serviceName);
	
	for (uint k = 0; k < subvarpath.Destination.size(); k++)
	{
		size_t pos = subvarpath.Destination[k].first.find("=");
		if (pos != string::npos)
			varNames.push_back(subvarpath.Destination[k].first.substr(0, pos));
		else
			varNames.push_back(subvarpath.Destination[k].first);

		string val = "???";
		// handle special case of non running service
		if (subvarpath.Destination[k].first == "State")
		{
			// lookup the alias, command to execute, execution path and defined command args for the given service
			string alias, command, path, arg;
			getServiceLaunchInfo(serviceName,alias,command,path,arg);
			val = getOfflineServiceState(alias,path);
		}
		else if (subvarpath.Destination[k].first == "State=1" ||
				 subvarpath.Destination[k].first == "State=2" ||
				 subvarpath.Destination[k].first == "State=3")
		{
			// we want to start the service
			if (startService(serviceName))
				val = "Launching";
			else
				val = "Failed";
		}
		else if (subvarpath.Destination[k].first == "State=0" ||
				 subvarpath.Destination[k].first == "State=-1" ||
				 subvarpath.Destination[k].first == "State=-2")
		{
			// lookup the alias, command to execute, execution path and defined command args for the given service
			string alias, command, path, arg;
			bool ok= getServiceLaunchInfo(serviceName,alias,command,path,arg);
			if (ok) writeServiceLaunchCtrl(alias,path,true,LaunchCtrlStop);
			if (ok) writeServiceLaunchCtrl(alias,path,false,LaunchCtrlStop);
			val= "Stopping";
		}

		values.push_back(val);
	}

	aesAddRequestAnswer(rid, varNames, values);
	sipinfo("REQUEST: Sent and received view '%s' to offline service '%s'", viewStr.c_str(), serviceName.c_str());
}
	
void addRequestForOnlineServices(uint32 rid, const string& viewStr)
{
	// add services that I manage
	for (TServices::iterator sit= Services.begin(); sit!=Services.end(); ++sit)
	{
		if (sit->Connected)
		{
			treatRequestForRunnignService(rid,sit,viewStr);
		}
	}

	// add myself
	treatRequestOneself(rid,viewStr);
}							

void addRequestForAllServices(uint32 rid, const string& viewStr)
{
	uint j;

	// add registered services that are not online
	for (j = 0; j < RegisteredServices.size(); j++)
	{
		vector<TServices::iterator> sits;
		findServices(RegisteredServices[j], 0, sits);

		// check for service that is registered but not online
		if (sits.empty())
		{
			treatRequestForOfflineService(rid,RegisteredServices[j],viewStr);
		}
	}

	// add all running services (and for oneself)
	addRequestForOnlineServices(rid,viewStr);
}							

void addRequestForNamedService(uint32 rid, const string& service, const string& viewStr, uint16 sid)
{
	if (service.find("AES") != string::npos)
	{
		// it's for me, I don't send message to myself so i manage it right now
		treatRequestOneself(rid,viewStr);
	}
	else
	{
		// see if the service is running
		vector<TServices::iterator> sits;
		findServices(service, sid, sits);
		if (sits.empty())
		{
			// the service is not running - so make sure it's registered
			if (!isRegisteredService(service))
			{
				sipwarning("Service %s is not online and not found in registered service list", service.c_str());
			}
			else
			{
				treatRequestForOfflineService(rid,service,viewStr);
			}
		}
		else
		{
			// there is at least one match from a running service for the given service name so treat the matches
			for(uint p = 0; p < sits.size(); p++)
			{
				treatRequestForRunnignService(rid, sits[p], viewStr);
			}
		}
	}
}							

void addRequest(uint32 rid, const string &rawvarpath, uint16 sid)
{
	sipinfo("addRequest from %hu: '%s'", sid, rawvarpath.c_str());

	string str;
	CLog logDisplayVars;
	CMemDisplayer mdDisplayVars;
	logDisplayVars.addDisplayer(&mdDisplayVars);

	CVarPath varpath(rawvarpath);

	// add the request
	Requests.push_back(CRequest(rid, sid));

	// for each destination in the rawvarpath...
	for (uint i = 0; i < varpath.Destination.size(); i++)
	{
		CVarPath vp(varpath.Destination[i].first);
		for (uint t = 0; t < vp.Destination.size(); t++)
		{
			string service = vp.Destination[t].first;
			
			if (service == "*")
			{
				addRequestForOnlineServices(rid,varpath.Destination[i].second);
			}
			else if (service == "#")
			{
				addRequestForAllServices(rid,varpath.Destination[i].second);
			}
			else
			{
				addRequestForNamedService(rid,service,varpath.Destination[i].second, sid);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

static	void	LaunchAdminService(string _ASHostIP)
{
	vector<string> serverIP;
	vector<CInetAddress> laddr = CInetAddress::localAddresses();
	for (uint i = 0; i < laddr.size(); i++)
	{
		serverIP.push_back(laddr[i].ipAddress());
	}
	uint32	uIPNum = static_cast<uint32>(serverIP.size());
	if (uIPNum < 1)
		return;
	bool bASHost = false;

	for (uint32 i = 0; i < uIPNum; i++)
	{
		if (serverIP[i] == _ASHostIP)
		{
			bASHost = true;
			break;
		}
	}

	if (!bASHost && _ASHostIP == "localhost")
		bASHost = true;

	if (!bASHost)
		return;

	string		sAdminMainServiceExeName	= AdminMainServiceExeName;
	ucstring	ucAdminMainServiceExeName	= sAdminMainServiceExeName;
	bool	bIsRunning = IsRunningProcess((tchar *)ucAdminMainServiceExeName.c_str());

	if (bIsRunning)
		return;

	string	sAdminMainServiceName = AdminMainServiceName;
	bool	bIsSystemService = CSystemInfo::IsSystemService(sAdminMainServiceName);

	if (bIsSystemService)
	{
		string	sStartAdminMain = "net start " + sAdminMainServiceName;
		system(sStartAdminMain.c_str());
		return;
	}

	string		strPath = GetAdminExePath(sAdminMainServiceExeName);
	string		sFullName = strPath + sAdminMainServiceExeName;

	launchProgram("", sFullName);
}

static	void	LaunchAllLocalServices()
{
	CConfigFile::CVar *	varExe = IService::getInstance()->ConfigFile.getVarPtr ("LaunchExeList");
	if ( varExe == NULL )
		return;

	uint32	count = varExe->size();
	for ( uint32 i = 0; i < count; i ++ )
	{
		CSStringA	sCommand = varExe->asString(i);
		CSStringA	sExe = sCommand.splitTo(" ");
		MAP_MSTSERVICECONFIG::iterator it;
		for ( it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++ )
		{
			if ( it->second.sExeName == sExe )
				break;
		}

		if ( it == map_MSTSERVICECONFIG.end() )
			continue;
		std::string	sPath		= GetPathFromExeName(sExe);
		std::string	sFullName	= sPath + sCommand;

		launchProgram("", sFullName);
		SIPBASE::sipSleep(1000);

		uint32	failed_count = 0;
#ifdef SIP_OS_WINDOWS
		while ( ! IsRunningProcess((tchar *)CSStringW(sExe).c_str()) && failed_count ++ < 100 )
#else
		while ( ! IsRunningProcess(CSStringW(sExe).c_str()) && failed_count ++ < 100 )
#endif
		{
			IService::getInstance()->update();
			SIPBASE::sipSleep(100);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////// CONNECTION TO THE SERVICES //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

static void cbServiceIdentification(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	if (sid >= Services.size())
	{
		sipwarning("Identification of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	if (!Services[sid].Connected)
	{
		sipwarning("Identification of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	msgin.serial(Services[sid].AliasName, Services[sid].LongName, Services[sid].PId);
	msgin.serialCont(Services[sid].Commands);
	msgin.serialCont(Services[sid].PortAry);
	sipinfo("Received service identification: Sid=%-3i Alias='%s' LongName='%s' ShortName='%s' PId=%u, RegPortAry num=%d",sid ,Services[sid].AliasName.c_str(),Services[sid].LongName.c_str(),serviceName.c_str(),Services[sid].PId, Services[sid].PortAry.size());

	CMessage	msgout(M_AD_SERVICE_IDENTIFY);
	uint32 nSize = 1;	msgout.serial(nSize);
	msgout.serial(sid);
	TREGPORTARY::iterator	it;
	TREGPORTARY		portAry;
	for ( it = Services[sid].PortAry.begin(); it != Services[sid].PortAry.end(); it ++ )
	{
		TRegPortItem	port = *it;
		if ( port._uFilter == FILTER_NONE )
			portAry.push_back(port);
	}
	msgout.serialCont(portAry);
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
	sipinfo("Received Port Ary From Service (sid %d)", sid);	

	// if there's an alias, it means that it s me that launch the services, autoreconnect it
	if (!Services[sid].AliasName.empty())
		Services[sid].AutoReconnect = true;

	sipinfo("*:*:%d is identified to be '%s' '%s' pid:%d", Services[sid].ServiceId, Services[sid].getServiceUnifiedName().c_str(), Services[sid].LongName.c_str(), Services[sid].PId);
}

static void cbServiceReady(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	if (sid >= Services.size())
	{
		sipwarning("Ready of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	if (!Services[sid].Connected)
	{
		sipwarning("Ready of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	sipinfo("*:*:%d is ready '%s'", Services[sid].ServiceId, Services[sid].getServiceUnifiedName().c_str());
	Services[sid].Ready = true;
}

static void getRegisteredServicesFromCfgFile()
{
	// look and see if we have a registered services entry in out cfg file
	CConfigFile::CVar* ptr= IService::getInstance()->ConfigFile.getVarPtr("RegisteredServices");
	if (ptr!=NULL)
	{
		// the variable exists so extract the service list...
		for (uint32 i=0;i<ptr->size();++i)
		{
			RegisteredServices.push_back(ptr->asString(i));
		}
	}
}

static void getUniqueueServicesFromCfgFile()
{
	// UniqueByMachineServices: unique services on a machine
	if ( ! IService::getInstance()->ConfigFile.exists("UniqueByMachineServices") )
		return;
	CConfigFile::CVar& uniqueServicesM = IService::getInstance()->ConfigFile.getVar("UniqueByMachineServices");
	for ( uint i=0; i!=uniqueServicesM.size(); ++i )
	{
		_UniqueServices.insert( std::make_pair( uniqueServicesM.asString(i), true ) );
	}
}


static void cbAESInfo(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	sipinfo("Updating all informations for AES and hosted service");

	//
	// setup the list of all registered services
	//
//	RegisteredServices.clear();
	// start with the service list that we've been sent
//	msgin.serialCont(RegisteredServices);
	// append the registered services listed in our cfg file
	getRegisteredServicesFromCfgFile();

	//
	// receive the list of all alarms and graph update
	//
	msgin.serialCont(AllAdminAlarms);
	msgin.serialCont(AllGraphUpdates);
	
	// set our own alarms and graph update for this service
	setInformations(AllAdminAlarms, AllGraphUpdates);

	// now send alarms and graph update to all services
	for (uint j = 0; j < Services.size(); j++)
	{
		if (Services[j].Connected)
		{
			sendInformations(Services[j].ServiceId);
		}
	}

	vector<TServices::iterator> services;
	TServices::iterator sit;
	for (sit = Services.begin(); sit != Services.end(); sit++)
	{
		if ( (*sit).PortAry.empty() )
			continue;
		TREGPORTARY::iterator	it;
		TREGPORTARY		portAry;
		for ( it = (*sit).PortAry.begin(); it != (*sit).PortAry.end(); it ++ )
		{
			TRegPortItem	port = *it;
			if ( port._uFilter == FILTER_NONE )
				portAry.push_back(port);
		}
		if ( portAry.empty() )
			continue;

		services.push_back(sit);
	}
	// if we found some alias, it's enough, return
	if(services.empty())
		return;

	CMessage	msgout(M_AD_SERVICE_IDENTIFY);
	uint32		nSize = static_cast<uint32>(services.size());
	msgout.serial(nSize);
	for (uint32 i = 0; i < nSize; i ++ )
	{
		sit = services[i];
		msgout.serial(sit->ServiceId);
		TREGPORTARY::iterator	it;
		TREGPORTARY		portAry;
		for ( it = (*sit).PortAry.begin(); it != (*sit).PortAry.end(); it ++ )
		{
			TRegPortItem	port = *it;
			if ( port._uFilter == FILTER_NONE )
				portAry.push_back(port);
		}
		if ( portAry.empty() )
			continue;

		msgout.serialCont(sit->PortAry);
	}
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
	sipinfo("Send Service Port Info (size %d)", nSize);	

	CMessage	msgout1(M_ADMIN_SHARDID);
	nSize = static_cast<uint32>(Services.size());
	int nCount = 0;
	msgout1.serial(nSize);
	for (uint32 i = 0; i < nSize; i ++ )
	{
		CService	&service = Services[i];
		if(!service.Connected)
			continue;
		msgout1.serial(service.ServiceId);
		msgout1.serial(service.ShardId);
		nCount ++;
	}
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout1);
	sipinfo("Send Service Admin Config Info (size=%d, count=%d)", nSize, nCount);
}

CVariable<bool> UseExplicitAESRegistration("aes","UseExplicitAESRegistration","flag to allow AES services to register explicitly if automatic registration fails",false,0,true);
static void cbRejected(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	if (!UseExplicitAESRegistration)
	{
		// receive a message that means that the AS doesn't want me
		string res;
		msgin.serial(res);
		sipwarning("AS rejected our connection: %s", res.c_str());
		IService::getInstance()->exit();
	}
	else
	{
		// we assume that the rejection message from the AS is not important... we'll try an explicit registration

		// lookup the machine's host name and prune off the domain name
		string server =	CSStringA(SIPNET::CInetAddress::localHost().hostName()).splitTo('.');
		// setup the shard name from the config file vaiable
		string shard = ShardName;

		// setup the registration message and dispatch it to the AS
		sipinfo("Sending manual registration: server='%s' shard='%s' (auto-registration was rejected by AS)",server.c_str(),shard.c_str());
		CMessage msgout(M_AES_REGISTER_AES);
		msgout.serial(server);
		msgout.serial(shard);
		CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgout);
	}
}

static uint32 requestId=0;

static void cb_M_AES_DISCONNECT_SERVICE(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	std::string		sServiceName;	uint16	sid;
	msgin.serial(sServiceName, sid);

	std::string		sUnifiedName = SIPBASE::toString("%s-%d", sServiceName.c_str(), sid);
	if (Services[sid].Connected)
	{
		string strCommand = sUnifiedName + ".quit";
		//addRequest(requestId++, strCommand, (uint16)sid);
		addRequest(requestId++, strCommand, 0);
		sipinfo("send quit command to service = %s", strCommand.c_str());
	}
}

static void cbView(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	// receive an view answer from the service
	TServices::iterator sit = findService(sid);
	
	uint32 rid;
	msgin.serial(rid);

	TAdminViewResult answer;
	TAdminViewVarNames varNames;
	TAdminViewValues values;

	while ((uint32)msgin.getPos() < msgin.length())
	{
		varNames.clear();
		values.clear();
		
		// adding default row
		
		uint32 i, nb;
		string var, val;
		
		msgin.serial(nb);
		for (i = 0; i < nb; i++)
		{
			msgin.serial(var);
			varNames.push_back(var);
		}
		
		msgin.serial(nb);
		for (i = 0; i < nb; i++)
		{
			msgin.serial(val);
			values.push_back(val);
		}
		answer.push_back(SAdminViewRow(varNames,values));
	}
	aesAddRequestAnswer(rid, answer);
	
	// remove the waiting request
	for (uint i = 0; i < (*sit).WaitingRequestId.size();)
	{
		if ((*sit).WaitingRequestId[i] == rid)
		{
			(*sit).WaitingRequestId.erase((*sit).WaitingRequestId.begin()+i);
		}
		else
		{
			i++;
		}
	}
}

// Packet : M_AES_AES_GET_VIEW
static void cbGetView(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	uint32 rid;
	string rawvarpath;

	msgin.serial(rid);
	msgin.serial(rawvarpath);

	addRequest(rid, rawvarpath, sid);
}


static void cbAESConnect(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	vector<string> serverIP;
	if (IService::getInstance()->ConfigFile.exists("OwnHost")) // 기대의 IP주소를 명시적으로 지정
	{
		string temp = IService::getInstance()->ConfigFile.getVar("OwnHost").asString();
		serverIP.push_back(temp);
	}
	else
	{
		vector<CInetAddress> laddr = CInetAddress::localAddresses();
		for (uint i = 0; i < laddr.size(); i++)
		{
			serverIP.push_back(laddr[i].ipAddress());
		}
	}
	uint32	uIPNum = static_cast<uint32>(serverIP.size());
	if (uIPNum < 1)
	{
		sipwarning("There is no available ip address");
		return;
	}

	// setup the shard name from the config file vaiable
	string		shard = ShardName;
	ucstring	ucShard;	ucShard.fromUtf8(shard);

	string		sVersion = s_sVersion;

	CMessage msgout(M_AD_AES_CONNECT);
	msgout.serial(ucShard, sVersion, uIPNum);
	for (uint32 i = 0; i < uIPNum; i++)
		msgout.serial(serverIP[i]);

	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);

/*	
	// Update정형을 통보한다
	string		strPath = IService::getInstance()->getConfigDirectory() + "UpdateAction.ini";
	ucstring	ucPath = strPath;
	uint32	nAction = GetPrivateProfileInt(L"UPDATEACTION", L"YES", 0, ucPath.c_str());
	if (nAction == 1)
	{
		tchar	chBuffer[100];	memset(chBuffer, 0, sizeof(chBuffer));
		GetPrivateProfileString(L"UPDATEACTION", L"VERSION", L"", chBuffer, 100, ucPath.c_str());
		ucstring	ucVersion(chBuffer);
		string		strVersion = ucVersion.toString();
		CMessage	msgUpdate("UPDATE_RESULT");
		msgUpdate.serial(strVersion);
		CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgUpdate);

		wchar_t strAction[MAX_PATH];		memset(strAction, 0, sizeof(strAction));
		_stprintf(strAction, L"%d", 0);
		WritePrivateProfileString( L"UPDATEACTION", L"YES", strAction, ucPath.c_str());
		
	}
*/
	// 써비스들의 접속정형을 통보한다
	for (uint16 i = 0; i < Services.size(); i++)
	{
		if (Services[i].Connected)
		{
			CMessage	msgout(M_AES_SERVICE_CONNECT);
			string	strName = Services[i].ShortName;
			msgout.serial(strName);
			msgout.serial(i);
			CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
		}
	}
}

void serviceConnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	uint16 sid = tsid.get();

	// don't add AS
	if (serviceName == AS_S_NAME)
		return;

	if (sid >= Services.size())
	{
		Services.resize(sid+1);
	}

	Services[sid].init(serviceName, sid);

	sendInformations(sid);

	//Notify to AS
	CMessage	msgout(M_AES_SERVICE_CONNECT);
	string	strName = serviceName;
	msgout.serial(strName);
	msgout.serial(sid);
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);

	sipinfo("%s-%hu connected", Services[sid].ShortName.c_str(), Services[sid].ServiceId);
}

void serviceDisconnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	uint16 sid = tsid.get();

	// don't remove AS
	if (serviceName == AS_S_NAME)
		return;

	//Notify to AS
	CMessage	msgout(M_AES_SERVICE_DISCONNECT);
	string	strName = serviceName;
	msgout.serial(strName);
	msgout.serial(sid);
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);

	if (sid >= Services.size())
	{
		sipwarning("Disconnection of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	if (!Services[sid].Connected)
	{
		sipwarning("Disconnection of an unknown service %s-%hu", serviceName.c_str(), sid);
		return;
	}

	// we need to remove pending request

	for(uint i = 0; i < Services[sid].WaitingRequestId.size(); i++)
	{
		subRequestWaitingNb(Services[sid].WaitingRequestId[i]);
	}

	sipinfo("%s-%hu disconnected", Services[sid].ShortName.c_str(), Services[sid].ServiceId);

	if (Services[sid].Relaunch)
	{
		// we have to relaunch it in time because ADMIN asked it
		sint32 delay = IService::getInstance()->ConfigFile.exists("RestartDelay")? IService::getInstance()->ConfigFile.getVar("RestartDelay").asInt(): 5;

		// must restart it so if delay is -1, set it by default to 5 seconds
		if (delay == -1) delay = 5;

		startService(delay, Services[sid].getServiceUnifiedName());
	}
	else if (Services[sid].AutoReconnect)
	{
		// we have to relaunch it
		sint32 delay = IService::getInstance()->ConfigFile.exists("RestartDelay")? IService::getInstance()->ConfigFile.getVar("RestartDelay").asInt(): 5;
		if (delay >= 0)
		{
			startService(delay, Services[sid].getServiceUnifiedName());
		}
		else
			sipinfo("Don't restart the service because RestartDelay is %d", delay);

		sendAdminEmail("Server %s service '%s' : Stopped, auto reconnect in %d seconds", CInetAddress::localHost().hostName().c_str(), Services[sid].getServiceUnifiedName().c_str(), delay);
	}

	// if the appropriate cfg file variable is set then we kill the service brutally on disconnection - this
	// allows one to fix a certain number of problem cases in program shut-down that would otherwise require
	// manual intervention on the machine
	if (KillServicesOnDisconnect)
	{
		killProgram(Services[sid].PId);
	}

	Services[sid].reset();
}

/*static void cbAdminConfiguration(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	map_MSTSERVICECONFIG.clear();
	msgin.serialCont(map_MSTSERVICECONFIG);
}
*/

static void cbAESUpdate(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	ucstring		ucServerName;
	uint16			uPort;
	ucstring		ucUsername;
	ucstring		ucPassword;
	ucstring		ucRemoteFileName;
	msgin.serial(ucServerName, uPort, ucUsername, ucPassword, ucRemoteFileName);

	string		strPath = GetServiceRootPath();
	ucstring	ucLocalFileName = ucstring(strPath) + ucstring(s_sUpdateExe);

#ifdef SIP_OS_WINDOWS
	CHttpDown	HttpDown;

	if ( !HttpDown.OpenHttp(ucServerName, uPort, ucUsername, ucPassword) )
	{
		sipwarning(L"Failed OpenHttp for updating : Server=%s, Port=%d, User=%s, Password=%s", ucServerName.c_str(), uPort, ucUsername.c_str(), ucPassword.c_str());
		return;
	}

	void*				pParam = NULL;
	fndefDownloadNotify	fnNotify = NULL;
	if ( !HttpDown.Download(ucRemoteFileName, ucLocalFileName, pParam, fnNotify) )
	{
		sipwarning(L"Failed download for updating : RemoteFile=%s, LocalFile=%s", ucRemoteFileName.c_str(), ucLocalFileName.c_str());
		HttpDown.Close();
		return;
	}
	HttpDown.Close();
#else
	ucstring temp;
	const char* localFileName = ucLocalFileName.toString().c_str();
	temp = ucUsername + ":" + ucPassword;
	const char* userAndPassword = temp.toString().c_str();

	temp = "http://" + ucServerName + ":" + uPort;
	const char* remoteUrl = temp.toString().c_str();
	execl("curl", "-ou", localFileName, userAndPassword, remoteUrl);
#endif

	// Never change following code order!!!
	string	sAdminMainSN = AdminMainServiceName;
	string	sAdminExecutorSN = AdminExecutorServiceName;
	string	sStopAdminMain = "net stop " + sAdminMainSN;
	string	sStopAdminExecutor = "net stop " + sAdminExecutorSN;
	string	sStartAdminExecutor = "net start " + sAdminExecutorSN;

	system(sStopAdminMain.c_str());
	string		sAdminMainServiceExeName	= AdminMainServiceExeName;
	ucstring	ucAdminMainServiceExeName	= sAdminMainServiceExeName;
	TerminateRunningProcess((tchar*)ucAdminMainServiceExeName.c_str());


	string strCommand = "AES.quit";
	addRequest(requestId++, strCommand, 0);

	string strFull = ucLocalFileName.toString();
	string strLine = strPath;
	strLine = CPath::standardizePath(strLine, false);
	strLine = "\"" + strLine + "\"";

	string strTotal;
	bool	bIsSystemService = CSystemInfo::IsSystemService(sAdminExecutorSN);
	if (bIsSystemService)
	{
		strTotal = '"' + strFull + '"' + " " + strLine + " " + '"' + "system" + '"' + " " + '"' + sStartAdminExecutor + '"';
	}
	else
	{
		string		sAdminExeServiceExeName	= AdminExeServiceExeName;
		string		sPath = GetPathFromExeName(sAdminExeServiceExeName);
		string		sFullName = sPath + sAdminExeServiceExeName;
		sFullName = CPath::standardizePath(sFullName, false);
		strTotal = '"' + strFull + '"' + " " + strLine + " " + '"' + sFullName + '"';
	}
	sipinfo(L"Launch update : %s", ucstring(strTotal).c_str());
	
	launchProgram("", strTotal);

	system(sStopAdminExecutor.c_str());
}


/** Callback Array
 */
static TUnifiedCallbackItem CallbackArray[] =
{
	// messages sent by services (see sip/src/net/admin.cpp)
	{ M_ADMIN_SID, cbServiceIdentification },
	{ M_ADMIN_ADMIN_PONG, cbAdminPong },

	// messages sent by services (see sip/src/net/admin.cpp) - for forwarding to admin service
	{ M_ADMIN_VIEW, cbView },
	{ M_ADMIN_ADMIN_EMAIL, cbAdminEmail },
	{ M_ADMIN_GRAPH_UPDATE, cbGraphUpdate },
	{ M_ADMIN_SHARDID, cbShardID },

	// messages sent by services (see sip/src/net/admin.cpp AND sip/src/net/service.cpp)
	{ M_SR, cbServiceReady },

	// messages sent by admin service (see service/admin_service/admin_service.cpp)
	{ M_AES_AES_INFO, cbAESInfo },
	{ M_AES_REJECTED, cbRejected },
	{ M_AES_DISCONNECT_SERVICE, cb_M_AES_DISCONNECT_SERVICE },
	{ M_AES_AES_GET_VIEW, cbGetView },
	{ M_AD_AES_CONNECT,	cbAESConnect },
	//{ M_AD_ADMIN_CONF,	cbAdminConfiguration },
	{ M_MH_UPDATE,		cbAESUpdate },
};


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////// CONNECTION TO THE AS ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

static void ASConnection(const string &serviceName, TServiceId tsid, void *arg)
{
	uint16 sid = tsid.get();
	sipinfo("Connected to %s-%hu", serviceName.c_str(), sid);

/*
	CMessage	msgout(M_AES_REG_SVCS);
	uint32		size = map_MSTSERVICECONFIG.size();
	msgout.serial(size);
	MAP_MSTSERVICECONFIG::iterator it;
	for (it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++)
	{
		string	sAlias	= it->second.sAliasName;
		string	sExe	= it->second.sExeName;

		msgout.serial(sAlias, sExe);
	}
	CUnifiedNetwork::getInstance()->send(AS_S_NAME, msgout);
*/
}

static void ASDisconnection(const string &serviceName, TServiceId tsid, void *arg)
{
	uint16 sid = tsid.get();
	sipinfo("Disconnected to %s-%hu", serviceName.c_str(), sid);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////// SERVICE IMPLEMENTATION //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

TRegPortItem	PortAry[] =
{
	{"TIANGUO_AESPort",	PORT_TCP, AESPort.get(), FILTER_SUBNET},
};

class CAdminExecutorService : public IService
{
public:
	/// PreInit
	void preInit()
	{
		setRegPortAry(PortAry, sizeof(PortAry)/sizeof(PortAry[0]));
	}
	/// Init the service
	void init()
	{
		// be warn when a new service comes
		CUnifiedNetwork::getInstance()->setServiceUpCallback("*", serviceConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback("*", serviceDisconnection, NULL);

		// add connection to the admin service
		CUnifiedNetwork::getInstance()->setServiceUpCallback(AS_S_NAME, ASConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback(AS_S_NAME, ASDisconnection, NULL);

		// setup the connection address for the admin service
		uint16	uASPort = 59996;
		string	ASHostIP;
		string ASHost = ConfigFile.exists("ASHost")? ConfigFile.getVar("ASHost").asString(): "localhost";

		ASHostIP = ASHost;
		if (ASHost.find(":") == string::npos)
		{
			CConfigFile::CVar *var;
			if ((var = ConfigFile.getVarPtr ("ASPort")) != NULL)
				uASPort = var->asInt ();
			ASHost += ":" + toStringA(uASPort);
		}

		// try to connect to the admin service (if we fail then SIP'll try again every few seconds until it succeeds...)
		try
		{
			CUnifiedNetwork::getInstance()->addService(AS_S_NAME, CInetAddress(ASHost));
		}
		catch ( ESocket& e )
		{
			siperror( "Can't connect to AS: %s", e.what() );
		}

		// setup the default registered service list (from our cfg file)
		getRegisteredServicesFromCfgFile();

//		getUniqueueServicesFromCfgFile();

		// get rid of the ".state" file (if we have one) to make sure that external processes that wait for
		// shards to stop don't wait for us
		std::string stateFileName= getServiceStateFileName(IService::getServiceAliasName(),"./");
		if (SIPBASE::CFileA::fileExists(stateFileName))
		{
			SIPBASE::CFileA::deleteFile(stateFileName);
		}

		SetServiceRootPathAndRelativePath();
		LoadAdminConfig();

		// read version
		string		sPath = GetServiceRootPath();
		string		sVersionFile = sPath + "VERSION";
		ucchar		ucVersion[100];
		memset(ucVersion, 0, sizeof(ucVersion));

		SIPBASE::CIFile	file(sVersionFile, true);
		file.getline (ucVersion, 100);
		ucstring	ucV = ucVersion;
		if (ucVersion[0] == 0xfeff)
		{
			string	sV = ucV.toString();
			s_sVersion = (sV.c_str() + 1);
		}
		else
			s_sVersion = ucV.toString();
		if (s_sVersion == "")
			s_sVersion = "Unknown";

		// init
		ucstring ucLocalFileName = ucstring(sPath) + ucstring(s_sUpdateExe);
		if (CFileW::isExists(ucLocalFileName))
		{
			CFileW::deleteFile(ucLocalFileName);
		}

		LaunchAdminService(ASHostIP);

		if ( IService::getInstance()->haveLongArg("noautolaunch") )
			return;

		LaunchAllLocalServices();
	}

	bool update()
	{
		cleanRequests();
		checkWaitingServices();
		checkPingPong();
		checkShutdownRequest();
		return true;
	}
};

/// Admin executor Service
SIPNET_SERVICE_MAIN(CAdminExecutorService, AES_S_NAME, AES_L_NAME, "AESPort", 59991, true, CallbackArray, SIPNS_CONFIG, SIPNS_LOGS);

SIPBASE_CATEGORISED_COMMAND(AES, getViewAES, "send a view and receive an array as result", "<service shortname.command>")
{
	string cmd;
	for (uint i = 0; i < args.size(); i++)
	{
		if (i != 0) cmd += " ";
		cmd += args[i];
	}
	
	addRequest(requestId++, cmd, 0);

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, clearRequests, "clear all pending requests", "")
{
	if(args.size() != 0) return false;

	// for all request, set the NbWaiting to NbReceived, next cleanRequests() will send answer and clear all request
	for (uint i = 0 ; i < Requests.size(); i++)
	{
		if (Requests[i].NbWaiting <= Requests[i].NbReceived)
		{
			Requests[i].NbWaiting = Requests[i].NbReceived;
			sipdebug("REQUEST: ++ i %d rid %d NbWaiting= %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
		}
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, displayRequests, "display all pending requests", "")
{
	if(args.size() != 0) return false;

	log.displayNL("Display %d pending requests", Requests.size());
	for (uint i = 0 ; i < Requests.size(); i++)
	{
		log.displayNL("id: %d wait: %d recv: %d sid: %hu", Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived, Requests[i].SId);
	}
	log.displayNL("End of display pending requests");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, sendAdminEmail, "Send an email to admin", "<text>")
{
	if(args.size() <= 0)
		return false;
	
	string text;
	for (uint i =0; i < args.size(); i++)
	{
		text += args[i]+" ";
	}
	sendAdminEmail(text.c_str());
	
	return true;
}



#ifdef SIP_OS_UNIX

static inline char *skipToken(const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}

// col: 0        1       2    3    4    5     6          7         8        9       10   11   12   13    14      15
//      receive                                                    sent
//      bytes    packets errs drop fifo frame compressed multicast bytes    packets errs drop fifo colls carrier compressed
uint64 getSystemNetwork(uint col)
{
	if (col > 15)
		return 0;

	int fd = open("/proc/net/dev", O_RDONLY);
	if (fd == -1)
	{
		sipwarning("Can't get OS from /proc/net/dev: %s", strerror(errno));
		return 0;
	}
	else
	{
		char buffer[4096+1];
		int len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		buffer[len] = '\0';

		char *p = strchr(buffer, '\n')+1;
		p = strchr(p, '\n')+1;

		uint64 val = 0;
		while (true)
		{
			p = strchr(p, ':');
			if (p == NULL)
				break;
			p++;
			for (uint i = 0; i < col; i++)
			{
				p = skipToken(p);
			}
			val += strtoul(p, &p, 10);
		}
		return val;
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(AES, string, NetBytesSent, "Amount of bytes sent to all networks cards in bytes")
{
	if (get) *pointer = bytesToHumanReadable(getSystemNetwork(8));
}

SIPBASE_CATEGORISED_DYNVARIABLE(AES, string, NetBytesReceived, "Amount of bytes received to all networks cards in bytes")
{
	if (get) *pointer = bytesToHumanReadable(getSystemNetwork(0));
}

SIPBASE_CATEGORISED_DYNVARIABLE(AES, uint32, NetError, "Number of error on all networks cards")
{
	if (get) *pointer = (uint32) (getSystemNetwork(2) + getSystemNetwork(10));
}

#endif // SIP_OS_UNIX

class CTempThread : public SIPBASE::IRunnable
{
public:
	CTempThread(string sCommand);
	~CTempThread();
	virtual void	run();

	bool	m_bExit;
	string	m_sCommand;
};

CTempThread::CTempThread(string sCommand)
{
	m_bExit = false;
	m_sCommand = sCommand;
}

CTempThread::~CTempThread()
{
}

void	CTempThread::run()
{
	m_bExit = false;
	system(m_sCommand.c_str());
	m_bExit = true;
}
CTempThread		*TempThread = NULL;
IThread			*ThreadProc = NULL;

SIPBASE_CATEGORISED_COMMAND(AES, aesSystem, "Execute a system() call", "<command>")
{
#ifdef SIP_OS_WINDOWS
	if(args.size() <= 0)
		return false;
	
	string cmd;
	for (uint i =0; i < args.size(); i++)
	{
		cmd += args[i]+" ";
	}
	
	string path;
#ifdef SIP_OS_UNIX
	path = "/tmp/";
#endif

	PROCESS_INFORMATION	proc;	memset(&proc, 0, sizeof(proc));
	STARTUPINFOA		start;	memset(&start, 0, sizeof(start));
	SECURITY_ATTRIBUTES	sa;		memset(&sa, 0, sizeof(sa));
	sa.nLength				=   sizeof(sa);     
	sa.lpSecurityDescriptor	=	0;     
	sa.bInheritHandle		=   true;     

	HANDLE				hReadPipe, hWritePipe;
	unsigned   long		lngBytesRead;

	bool	bret   =::CreatePipe(&hReadPipe,   &hWritePipe,&sa,   0);     
	if   (!bret)     
		return   false;
	start.cb			= sizeof(STARTUPINFOA);     
	start.dwFlags		= STARTF_USESTDHANDLES;
	start.hStdOutput	= hWritePipe;     
	start.hStdError		= hWritePipe;     
	bool	ret   =::CreateProcessA(NULL, (LPSTR)cmd.c_str(), &sa, &sa, true, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, 0, 0, &start, &proc);     
	if   (!ret)     
	{     
		return   false;
	}
	::CloseHandle(hWritePipe);
	HANDLE  hProcess = proc.hProcess;

	static	ucchar   ucBuffer[10001];
	bool bRead;
	TTime	tmStartTime = CTime::getLocalTime();
	do     
	{     
		memset(ucBuffer, 0, sizeof(ucBuffer));     
		bRead = ::ReadFile(hReadPipe,   &ucBuffer,   10000,   &lngBytesRead,   0);
		if (bRead)
			log.displayRaw(ucBuffer);

		TTime	tmCur = CTime::getLocalTime();
		if (tmCur - tmStartTime > 5000)
		{
			TerminateProcess(hProcess, 10);
			break;
		}
		sipSleep(10);
	}   while   (bRead);

	::CloseHandle(proc.hProcess);
	::CloseHandle(proc.hThread);
	::CloseHandle(hReadPipe);


/*	
	string fn = path+CFileA::findNewFile("aessys.tmp");
	string fne = path+CFileA::findNewFile("aessyse.tmp");
	CFileA::deleteFile(fn);
	CFileA::deleteFile(fne);
	
	cmd += " >" + fn + " 2>" + fne;
	
	log.displayNL("Executing: '%s' in directory '%s'", cmd.c_str(), CPath::getCurrentPathA().c_str());

	// system(cmd.c_str());

	TempThread = new CTempThread(cmd);
	ThreadProc = IThread::create(TempThread);
	ThreadProc->start();
	TTime tm = CTime::getLocalTime();
	while (!TempThread->m_bExit)
	{
		TTime Curtm = CTime::getLocalTime();
		if (Curtm - tm > 5000)
			break;
	}
	if (!TempThread->m_bExit)
	{
		ThreadProc->terminate();
	}
	ThreadProc->wait();
	delete TempThread;
	delete ThreadProc;

	
	char str[1024];

	FILE *fp = fopen(fn.c_str(), "rt");
	if (fp != NULL)
	{
		while (true)
		{
			char *res = fgets(str, 1023, fp);
			if (res == NULL)
				break;
			log.displayRaw(res);
		}
		
		fclose(fp);
	}
	else
	{
		log.displayNL("No stdout");
	}
	
	fp = fopen(fne.c_str(), "rt");
	if (fp != NULL)
	{
		while (true)
		{
			char *res = fgets(str, 1023, fp);
			if (res == NULL)
				break;
			log.displayRaw(res);
		}
		
		fclose(fp);
	}
	else
	{
		log.displayNL("No stderr");
	}

	CFileA::deleteFile(fn);
	CFileA::deleteFile(fne);
*/
#endif
	return true;
}
SIPBASE_CATEGORISED_COMMAND(AES, enumHardDisk, "Enumerate hard disk information", "")
{
#ifdef SIP_OS_WINDOWS
	DWORD dwDrives = GetLogicalDrives(); 
	for (int nDrv = 0; nDrv < 26; nDrv++)
	{
		if((dwDrives >> nDrv) & 0x00000001)
		{
			char chDrive = (char)(nDrv + 'A');
			char chDriveRoot[4] = "A:\\";
			chDriveRoot[0] += nDrv;
			UINT uType = GetDriveTypeA(chDriveRoot);
			if (uType != DRIVE_FIXED)
				continue;

			ULARGE_INTEGER TotalNumberOfBytes;
			ULARGE_INTEGER DiskFree;

			GetDiskFreeSpaceExA(chDriveRoot, NULL, &TotalNumberOfBytes, &DiskFree);
			
			DWORD64 dwTemp1 = (DWORD64)TotalNumberOfBytes.HighPart * 0xFFFFFFFF;
			DWORD64 dwTotalBytes = dwTemp1 + TotalNumberOfBytes.LowPart;
			DWORD64 dwTemp2 = (DWORD64)DiskFree.HighPart * 0xFFFFFFFF;
			DWORD64 dwFreeBytes = dwTemp2 + DiskFree.LowPart;

			DWORD	dwPercent = (DWORD)((dwFreeBytes * 100) / dwTotalBytes);
			string	strTotalBytes = _toString("%I64u", dwTotalBytes);
			string	strFressBytes = _toString("%I64u", dwFreeBytes);

			string	strTotalView;
			if ( TotalNumberOfBytes.HighPart )
				strTotalView = _toString("%.1f GB", (float)TotalNumberOfBytes.HighPart * 4 + ((float)TotalNumberOfBytes.LowPart / (1024*1024*1024) ));
			else if(TotalNumberOfBytes.LowPart < 1024 )
				strTotalView = "";
			else if(TotalNumberOfBytes.LowPart < 1024*1024)
				strTotalView = _toString("%d KB",TotalNumberOfBytes.LowPart / 1024);
			else if(TotalNumberOfBytes.LowPart < 1024*1024*1024)        
				strTotalView = _toString("%d MB", TotalNumberOfBytes.LowPart / (1024*1024));
			else
				strTotalView = _toString("%.1f GB",(float)TotalNumberOfBytes.LowPart / (1024*1024*1024));

			string	strFreeView;
			if ( DiskFree.HighPart )
				strFreeView = _toString("%.1f GB", (float)DiskFree.HighPart * 4 + ((float)DiskFree.LowPart / (1024*1024*1024) ));
			else if(DiskFree.LowPart < 1024 )
				strFreeView = "";
			else if(DiskFree.LowPart < 1024*1024)
				strFreeView = _toString("%d KB",DiskFree.LowPart / 1024);
			else if(DiskFree.LowPart < 1024*1024*1024)        
				strFreeView = _toString("%d MB", DiskFree.LowPart / (1024*1024));
			else
				strFreeView = _toString("%.1f GB",(float)DiskFree.LowPart / (1024*1024*1024));

/*
			log.displayRaw("-----  Drive %c : Free Space = %sBytes", chDrive, strFressBytes.c_str());
			if (strFreeView != "")
				log.displayRaw("(%s), ", strFreeView.c_str());
			else
				log.displayRaw(", ", strFreeView.c_str());

			log.displayRaw("Total Space = %sBytes", strTotalBytes.c_str());
			if (strTotalView != "")
				log.displayNL("(%s)", strTotalView.c_str());
*/
			log.displayRaw("-----  Drive %c : Free Space = ", chDrive);
			if (strFreeView != "")
				log.displayRaw("%s, ", strFreeView.c_str());
			else
				log.displayRaw(", ", strFreeView.c_str());

			log.displayRaw("Total Space = ");
			if (strTotalView != "")
				log.displayRaw("%s, ", strTotalView.c_str());

			log.displayNL("Available percent = %d(%c)", dwPercent, '%');
		}
	}
#endif
	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, enumLogfile, "Enumerate log files", "")
{
	string		strRootPath = GetServiceRootPath();

	MAP_MSTSERVICECONFIG::iterator it;
	for (it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++)
	{
		string	sRelative = it->second.sRelativePath;
		string	sPath = strRootPath + sRelative + "\\";
		CPath::setCurrentPath(sPath.c_str());

		ucstring	ucPath = sPath;

		std::vector<std::basic_string<ucchar> > filenames;
		CPath::getPathContent (ucPath, false, false, true, filenames);

		uint32	uSize = static_cast<uint32>(filenames.size());

		for (uint32 i = 0; i < uSize; i++)
		{
			if (CFileW::isExists(filenames[i]))
			{
				ucstring ucExetension =  CFile::getExtension (filenames[i]);
				if (ucExetension != L"log")
					continue;

				ucstring ucfile = CFile::getFilename(filenames[i]);
				log.displayNL(L"%s", ucfile.c_str());
			}
		}
	}
// Root path
	CPath::setCurrentPath(strRootPath.c_str());

	ucstring	ucPath = strRootPath;

	std::vector<std::basic_string<ucchar> > filenames;
	CPath::getPathContent (ucPath, false, false, true, filenames);

	uint32	uSize = static_cast<uint32>(filenames.size());

	for (uint32 i = 0; i < uSize; i++)
	{
		if (CFileW::isExists(filenames[i]))
		{
			ucstring ucExetension =  CFile::getExtension (filenames[i]);
			if (ucExetension != L"log")
				continue;

			ucstring ucfile = CFile::getFilename(filenames[i]);
			log.displayNL(L"%s", ucfile.c_str());
		}
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, deleteLogfile, "Delete log files", "[<name>-<startpos>-<endpos>]")
{
	string		strRootPath = GetServiceRootPath();
	std::vector<std::basic_string<ucchar> > lognames;

	MAP_MSTSERVICECONFIG::iterator it;
	for (it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++)
	{
		string	sRelative = it->second.sRelativePath;
		string	sPath = strRootPath + sRelative + "\\";
		CPath::setCurrentPath(sPath.c_str());

		ucstring	ucPath = sPath;

		std::vector<std::basic_string<ucchar> > filenamesfull;
		CPath::getPathContent (ucPath, false, false, true, filenamesfull);

		uint32	uSize = static_cast<uint32>(filenamesfull.size());

		for (uint32 i = 0; i < uSize; i++)
		{
			if (CFileW::isExists(filenamesfull[i]))
			{
				ucstring ucExetension =  CFile::getExtension (filenamesfull[i]);
				if (ucExetension != L"log")
					continue;

				ucstring ucfile = strRootPath + sRelative + "\\" + CFile::getFilename(filenamesfull[i]);
				lognames.push_back(ucfile);
			}
		}
	}
	// Root path
	CPath::setCurrentPath(strRootPath.c_str());

	ucstring	ucPath = strRootPath;

	std::vector<std::basic_string<ucchar> > filenames;
	CPath::getPathContent (ucPath, false, false, true, filenames);

	uint32	uSize = static_cast<uint32>(filenames.size());

	for (uint32 i = 0; i < uSize; i++)
	{
		if (CFileW::isExists(filenames[i]))
		{
			ucstring ucExetension =  CFile::getExtension (filenames[i]);
			if (ucExetension != L"log")
				continue;

			ucstring ucfile = strRootPath + CFile::getFilename(filenames[i]);
			lognames.push_back(ucfile);
		}
	}

	if (args.size() < 1)	// 모든 로그삭제
	{
		uint32	uSize = static_cast<uint32>(lognames.size());
		for (uint32 i = 0; i < uSize; i++)
		{
			if (CFileW::isExists(lognames[i]))
			{
				bool b = CFile::deleteFile(lognames[i]);
				if (b)
					log.displayNL(L"%s", lognames[i].c_str());
			}
		}
		return true;
	}

	if (args.size() == 1)	// 화일이름에 이 문자렬이 속한것 삭제
	{
		ucstring	ucParam = args[0];
		uint32	uSize = static_cast<uint32>(lognames.size());
		for (uint32 i = 0; i < uSize; i++)
		{
			if (!CFileW::isExists(lognames[i]))
				continue;
			ucstring	filefull = lognames[i];
			size_t nPos = filefull.find(ucParam.c_str());
			if (nPos == std::string::npos)
				continue;
			bool b = CFile::deleteFile(filefull);
			if (b)
				log.displayNL(L"%s", filefull.c_str());
		}
		return true;
	}

	if (args.size() == 3)	// 화일이름에 이 문자렬이 속한것, 번호지정 삭제
	{
		ucstring	ucParam = args[0];
		uint32		StartPos = atoi( args[1].c_str() );
		uint32		EndPos = atoi( args[2].c_str() );
		uint32	uSize = static_cast<uint32>(lognames.size());
		for (uint32 i = 0; i < uSize; i++)
		{
			if (!CFileW::isExists(lognames[i]))
				continue;
			ucstring	filefull = lognames[i];
			size_t nPos = filefull.find(ucParam.c_str());
			if (nPos == std::string::npos)
				continue;
			if (filefull.size() <= 7)	// "000.log"
				continue;
			
			nPos = filefull.find(L".log");
			if (nPos == std::string::npos)
				continue;
			if (filefull[nPos-1] >= L'0' && filefull[nPos-1] <= L'9' &&
				filefull[nPos-2] >= L'0' && filefull[nPos-2] <= L'9' &&
				filefull[nPos-3] >= L'0' && filefull[nPos-3] <= L'9')
			{
				ucstring ucDigit = "000";
				ucDigit[0] = filefull[nPos-3];
				ucDigit[1] = filefull[nPos-2];
				ucDigit[2] = filefull[nPos-1];
				string sDigit = ucDigit.toString();
				uint32	uDigit = atoi(sDigit.c_str());
				if (uDigit >= StartPos && uDigit <= EndPos)
				{
					bool b = CFile::deleteFile(filefull);
					if (b)
						log.displayNL(L"%s", filefull.c_str());
				}
			}
		}
		return true;
	}

	return true;
}

CMakeLogTask MakingLogTask;

SIPBASE_CATEGORISED_COMMAND(AES, makeLogReport, "Build a report of logs produced on the machine", "[stop | <logpath>]" )
{
	bool start = args.empty() || (!args.empty() && args[0] != "stop");

	if (start)
	{

		if (!args.empty())
		{
			MakingLogTask.setLogPath(args[0]);
		}
		else
		{
			MakingLogTask.setLogPathToDefault();
		}

		if ( ! MakingLogTask.isRunning() )
		{
			MakingLogTask.start();
			log.displayNL( "Task started" );
		}
		else
		{
			log.displayNL( "The makeLogReport task is already in progress, wait until the end of call 'makeLogReport stop' to terminate it");
		}
	}
	else if ( args[0] == "stop" )
	{
		if ( MakingLogTask.isRunning() )
		{
			sipinfo( "Stopping makeLogReport task..." );
			MakingLogTask.terminateTask();
			log.displayNL( "Task stopped" );
		}
		else
			log.displayNL( "Task is not running" );
	} 

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, displayLogReport, "Display summary of a part of the log report built by makeLogReport",
			    "[<service id> | <-p page number>]" )
{
	if ( MakingLogTask.isRunning() )
	{
		uint currentFile, totalFiles;
		MainLogReport.getProgress( currentFile, totalFiles );
		log.displayNL( "Please wait until the completion of the makeLogReport task, or stop it" );
		log.displayNL( "Currently processing file %u of %u", currentFile, totalFiles );
		return true;
	}
	log.displayNL( MakingLogTask.isComplete() ? "Full stats:" : "Temporary stats" );
	if ( args.empty() )
	{
		// Summary
		MainLogReport.report( &log );
	}
	else if ( (! args[0].empty()) && (args[0] == "-p") )
	{
		// By page
		if ( args.size() < 2 )
		{
			log.displayNL( "Page number missing" );
			return false;
		}
		uint pageNum = atoi( args[1].substr( 1 ).c_str() );
		MainLogReport.reportPage( pageNum, &log );
	}
	else
	{
		// By service
		MainLogReport.reportByService( args[0], &log );
	}

	return true;
}

/*
 * Command to allow AES to create a file with content
 */
SIPBASE_CATEGORISED_COMMAND(AES, createFile, "Create a file and fill it with given content", "<filename> [<content>]" )
{
	// check args
	if (args.size() < 1 || args.size() > 2)
		return false;

	COFile	f;
	if (!f.open(args[0]))
	{
		log.displayNL("Failed to open file '%s' in write mode", args[0].c_str());
		return false;
	}

	// check something to write, in case serialBuffer() doesn't accept empty buffer
	if (args.size() == 2)
	{
		// dirty const cast, but COFile won't erase buffer content
		char*	buffer = const_cast<char*>(args[1].c_str());

		f.serialBuffer((uint8*)buffer, static_cast<uint>(args[1].size()));
	}

	f.close();

	return true;
}

/*
 * Command for testing purposes - allows us to add an entry to the 'registered services' container
 */
SIPBASE_CATEGORISED_COMMAND(AES, addRegisteredService, "Add an entry to the registered services container", "<service alias name>" )
{
	// check args
	if (args.size() != 1 )
		return false;

	RegisteredServices.push_back(args[0]);

	return true;
}

// Manage
// 써비스를 기동시킨다
SIPBASE_CATEGORISED_COMMAND(AES, launchService, "launch service", "<service exe name>" )
{
	string		strOnlyExe = "";
	if (args.size() == 0 )
		return false;

	CSStringA	args0 = args[0];
	args0 = args0.right(4).c_str();
	if ( !strcmp(args0.c_str(), ".exe") )
	{
		strOnlyExe = args[0];
	}
	else // needless
	{
		strOnlyExe = GetExeNameFromAliasName(args[0]);
	}

	string strPath = GetPathFromExeName(strOnlyExe);

	string		strExe;
	if (args.size() == 1 )
	{
		strExe = strOnlyExe;
	}
	else
	{
		strExe = strOnlyExe;
		for ( uint i = 1; i < args.size(); i ++ )
			 strExe = strExe + " " + args[i];
	}

	ucstring	ucExe = strExe;;
	bool	bIsRunning = IsRunningProcess((tchar *)ucExe.c_str());
	if (bIsRunning)
		return true;

	strExe = strPath + strExe;
	if (launchProgram("", strExe))
		sipinfo("Success launch service = %s", strExe.c_str());
	else
		sipinfo("Failed launch service = %s", strExe.c_str());

	
	return true;
}

// 모든 써비스를 기동시킨다.
SIPBASE_CATEGORISED_COMMAND(AES, launchAllService, "launch all service", "" )
{
	LaunchAllLocalServices();
	return true;
}

// 모든 써비스를 종료시킨다
SIPBASE_CATEGORISED_COMMAND(AES, quitAllService, "quit all service", "" )
{
	for (uint i = 0; i < Services.size(); i++)
	{
		if (Services[i].Connected)
		{
			string strCommand = Services[i].ShortName + ".quit";
			addRequest(requestId++, strCommand, 0);
			log.displayNL("send quit command to service = %s", Services[i].ShortName.c_str());
		}
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, quitall, "quit all service including AS and oneself", "" )
{
	for (uint i = 0; i < Services.size(); i++)
	{
		if (Services[i].Connected)
		{
			string strCommand = Services[i].ShortName + ".quit";
			addRequest(requestId++, strCommand, 0);
			log.displayNL("send quit command to service = %s", Services[i].ShortName.c_str());
		}
	}
	// exit AS service
	SIPBASE::ICommand::execute("exitProcess admin_main_service_r.exe", *SIPBASE::InfoLog);
	// exit oneself
	SIPBASE::ICommand::execute("quit", *SIPBASE::InfoLog);

	return true;
}

// 접속한 써비스들을 종료시키고 Updater를 기동시킨다
// 파라메터로 Ftp주소를 받아 Updater에게 넘겨준다
SIPBASE_CATEGORISED_COMMAND(AES, UpdaterAllService, "Update all service", "<FTP address> | <FTP Path> | <FTP User> | <FTP Password>" )
{
	ucstring		ucServerName = "192.168.1.141";
	uint16			uPort = 80;
	ucstring		ucUsername = "";
	ucstring		ucPassword = "";
	ucstring		ucRemoteFileName = "update.exe";

#ifdef SIP_OS_WINDOWS
	CHttpDown	HttpDown;

	if ( !HttpDown.OpenHttp(ucServerName, uPort, ucUsername, ucPassword) )
	{
		return true;
	}

	ucstring			ucLocalFileName = "D://rr.exe";

	void*				pParam = NULL;
	fndefDownloadNotify	fnNotify = NULL;
	if ( !HttpDown.Download(ucRemoteFileName, ucLocalFileName, pParam, fnNotify) )
	{
		HttpDown.Close();
		return true;
	}
	HttpDown.Close();
#else
	ucstring temp, ucLocalFileName = "/rr";
	const char* localFileName = ucLocalFileName.toString().c_str();
	temp = ucUsername + ":" + ucPassword;
	const char* userAndPassword = temp.toString().c_str();

	temp = "http://" + ucServerName + ":" + uPort;
	const char* remoteUrl = temp.toString().c_str();
	execl("curl", "-ou", localFileName, userAndPassword, remoteUrl);
#endif

	return true;
}

SIPBASE_CATEGORISED_COMMAND(AES, enumAllServices, "enumerate all services", "" )
{
	for (uint i = 0; i < Services.size(); i++)
	{
		if (Services[i].ShortName.size() != 0)
		{
			log.displayNL("Sid=%-3i LongName='%s' ShortName='%s' PId=%u",Services[i].ServiceId, Services[i].LongName.c_str(),Services[i].ShortName.c_str(),Services[i].PId);
		}
	}
	return true;
}

// SS와 FS를 Lock한다
SIPBASE_CATEGORISED_COMMAND(AES, lockShard, "lock shard (LockSS & LockFS)", "" )
{
	for (uint i = 0; i < Services.size(); i++)
	{
		if (Services[i].Connected)
		{
			if (strstr(Services[i].ShortName.c_str(), SS_S_NAME) != 0)
			{
				string strCommand = Services[i].ShortName + ".LockSS";
				addRequest(requestId++, strCommand, 0);
				log.displayNL("send LockSS command to service = %s", Services[i].ShortName.c_str());
			}
			else if (strstr(Services[i].ShortName.c_str(), FRONTEND_SHORT_NAME) != 0)
			{
				string strCommand = Services[i].ShortName + ".LockFS";
				addRequest(requestId++, strCommand, 0);
				log.displayNL("send LockFS command to service = %s", Services[i].ShortName.c_str());
			}
		}
	}

	return true;
}

// SS와 FS를 Unlock한다
SIPBASE_CATEGORISED_COMMAND(AES, unlockShard, "unlock shard (UnlockSS & UnlockFS)", "" )
{
	for (uint i = 0; i < Services.size(); i++)
	{
		if (Services[i].Connected)
		{
			if (strstr(Services[i].ShortName.c_str(), SS_S_NAME) != 0)
			{
				string strCommand = Services[i].ShortName + ".UnlockSS";
				addRequest(requestId++, strCommand, 0);
				log.displayNL("send UnlockSS command to service = %s", Services[i].ShortName.c_str());
			}
			else if (strstr(Services[i].ShortName.c_str(), FRONTEND_SHORT_NAME) != 0)
			{
				string strCommand = Services[i].ShortName + ".UnlockFS";
				addRequest(requestId++, strCommand, 0);
				log.displayNL("send UnlockFS command to service = %s", Services[i].ShortName.c_str());
			}
		}
	}

	return true;
}
