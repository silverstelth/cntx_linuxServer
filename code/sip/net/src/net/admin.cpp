/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include <ctime>

#ifdef SIP_OS_WINDOWS
#   include <windows.h>
#   include <Psapi.h>
#	include <aclapi.h>
#	include <shlwapi.h>
#	include "misc/ftp.h"
#	include "misc/win_thread.h"
#	pragma comment(lib, "Psapi.lib")
#if defined (_DEBUG)
#define TRACE(str)	OutputDebugString(str)
#else
#define TRACE(str)
#endif

#endif

#include "net/service.h"
#include "net/admin.h"
#include "net/varpath.h"

//
// Namspaces
//

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

namespace SIPNET {

//
// Structures
//

struct CRequest
{
	CRequest (uint32 id, TServiceId sid) : Id(id), NbWaiting(0), NbReceived(0), SId(sid)
	{
		sipdebug ("ADMIN: ++ NbWaiting %d NbReceived %d", NbWaiting, NbReceived);
		Time = CTime::getSecondsSince1970 ();
	}
	
	uint32			Id;
	uint			NbWaiting;
	uint32			NbReceived;
	TServiceId			SId;
	uint32			Time;	// when the request was ask
	
	TAdminViewResult Answers;
};
	

//
// Variables
//

TRemoteClientCallback RemoteClientCallback = 0;

vector<CAlarm> Alarms;

vector<CGraphUpdate> GraphUpdates;

// check alarms every 5 seconds
const uint32 AlarmCheckDelay = 5;

vector<CRequest> Requests;

uint32 RequestTimeout = 4;	// in second

CVariable<uint32> DontUseGraphUpdate("admin", "DontUseGraphUpdate", "In Admin system, Server don't use this GraphUpdate function", 0, 0, true);		// in seconds, timeout before canceling the request

//
// Callbacks
//

static void cbInfo (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	sipinfo ("ADMIN: Updating admin informations");

	vector<string> alarms;
	msgin.serialCont (alarms);
	vector<string> graphupdate;
	msgin.serialCont (graphupdate);
	
	setInformations (alarms, graphupdate);
}	

static void cbServGetView (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32 rid;
	string rawvarpath;

	msgin.serial (rid);
	msgin.serial (rawvarpath);

	Requests.push_back (CRequest(rid, sid));

	TAdminViewResult answer;
	// just send the view in async mode, don't retrieve the answer
	serviceGetView (rid, rawvarpath, answer, true);
	sipassert (answer.empty());
}

static void cbExecCommand (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	// create a displayer to gather the output of the command
#ifdef SIP_USE_UNICODE
	class CStringDisplayer: public IDisplayerW
	{
	public:
		void serial(SIPBASE::IStream &stream)
		{
			stream.serial(_Data);
		}

	protected:
		virtual void doDisplay( const CLogW::TDisplayInfo& args, const ucchar *message)
		{
			_Data += message;
		}

		ucstring _Data;
	};
#else
	class CStringDisplayer: public IDisplayerA
	{
	public:
		void serial(SIPBASE::IStream &stream)
		{
			stream.serial(_Data);
		}

	protected:
		virtual void doDisplay( const CLogA::TDisplayInfo& args, const char *message)
		{
			_Data += message;
		}

		std::string _Data;
	};
#endif

	CStringDisplayer stringDisplayer;
	IService::getInstance()->CommandLog.addDisplayer(&stringDisplayer);

	// retreive the command from the input message and execute it
	string command;
	msgin.serial (command);
	sipinfo ("ADMIN: Executing command from network : '%s'", command.c_str());
	ICommand::execute (command, IService::getInstance()->CommandLog);

	// unhook our displayer as it's work is now done
	IService::getInstance()->CommandLog.removeDisplayer(&stringDisplayer);

	// send a reply message to the originating service
	CMessage msgout(M_ADMIN_EXEC_COMMAND_RESULT);
	stringDisplayer.serial(msgout);
	CUnifiedNetwork::getInstance()->send(sid, msgout);
}


// AES wants to know if i'm not dead, I have to answer faster as possible or i'll be killed
static void cbAdminPing (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	// Send back a pong to say to the AES that I'm alive
	CMessage msgout(M_ADMIN_ADMIN_PONG);
	CUnifiedNetwork::getInstance()->send(sid, msgout);
}

static void cbStopService (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	sipinfo ("ADMIN: Receive a stop from service %s-%hu, need to quit", serviceName.c_str(), sid.get());
	IService::getInstance()->exit (0xFFFF);
}


void cbAESConnection (const string &serviceName, TServiceId sid, void *arg)
{
	// established a connection to the AES, identify myself

	//
	// Sends the identification message with the name of the service and all commands available on this service
	//

	sipinfo("ADMIN: Identifying self as: AliasName='%s' LongName='%s' PId=%u",
		IService::getInstance()->_AliasName.c_str(),
		IService::getInstance()->_LongName.c_str(), 
		getpid ());
	CMessage msgout (M_ADMIN_SID);
	uint32 pid = getpid ();
	msgout.serial (IService::getInstance()->_AliasName, IService::getInstance()->_LongName, pid);
	ICommand::serialCommands (msgout);
	for ( size_t i = 0; i < IService::getInstance()->getRegPortAry().size(); i ++ )
	{
		TRegPortItem item = IService::getInstance()->getRegPortAry().at(i);
		sipinfo("ADMIN: PortAry(%d) : Alias %s, type : %d, FilterType:%d, PortNo:%d", i, item._sPortAlias.c_str(), item._typePort, item._uFilter, item._uPort, item._uPort);
	}

	TREGPORTARY PortAry = IService::getInstance()->getRegPortAry();
	msgout.serialCont(PortAry);
	CUnifiedNetwork::getInstance()->send("AES", msgout);

	if (IService::getInstance()->_Initialized)
	{
		CMessage msgout2 (M_SR);
		CUnifiedNetwork::getInstance()->send("AES", msgout2);
	}
}


static void cbAESDisconnection (const std::string &serviceName, TServiceId sid, void *arg)
{
	sipinfo("Lost connection to the %s-%hu", serviceName.c_str(), sid.get());
}


static TUnifiedCallbackItem CallbackArray[] =
{
	{ M_ADMIN_INFO,			cbInfo },
	{ M_ADMIN_GET_VIEW,		cbServGetView },
	{ M_ADMIN_STOPS,		cbStopService },
	{ M_ADMIN_EXEC_COMMAND,	cbExecCommand },
	{ M_ADMIN_ADMIN_PING,	cbAdminPing },
};


//
// Functions
//

void setRemoteClientCallback (TRemoteClientCallback cb)
{
	RemoteClientCallback = cb;
}


//
// Request functions
//

static void addRequestWaitingNb (uint32 rid)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].NbWaiting++;
			sipdebug ("ADMIN: ++ i %d rid %d NbWaiting+ %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			// if we add a waiting, reset the timer
			Requests[i].Time = CTime::getSecondsSince1970 ();
			return;
		}
	}
	sipwarning ("ADMIN: addRequestWaitingNb: can't find the rid %d", rid);
}

static void subRequestWaitingNb (uint32 rid)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].NbWaiting--;
			sipdebug ("ADMIN: ++ i %d rid %d NbWaiting- %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			return;
		}
	}
	sipwarning ("ADMIN: subRequestWaitingNb: can't find the rid %d", rid);
}

void addRequestAnswer (uint32 rid, const TAdminViewVarNames& varNames, const TAdminViewValues& values)
{
	if (!varNames.empty() && varNames[0] == "__log")
	{	sipassert (varNames.size() == 1); }
	else
	{	sipassert (varNames.size() == values.size()); }

	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].Answers.push_back (SAdminViewRow(varNames, values));

			Requests[i].NbReceived++;
			sipdebug ("ADMIN: ++ i %d rid %d NbWaiting %d NbReceived+ %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			
			return;
		}
	}
	// we received an unknown request, forget it
	sipwarning ("ADMIN: Receive an answer for unknown request %d", rid);
}

static bool emptyRequest (uint32 rid)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == rid && Requests[i].NbWaiting != 0)
		{
			return false;
		}
	}
	return true;
}

static void cleanRequest ()
{
	uint32 currentTime = CTime::getSecondsSince1970 ();

	for (uint i = 0 ; i < Requests.size ();)
	{
		// timeout
		if (currentTime >= Requests[i].Time+RequestTimeout)
		{
			sipwarning ("ADMIN: **** i %d rid %d -> Requests[i].NbWaiting (%d) != Requests[i].NbReceived (%d)", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
			Requests[i].NbWaiting = Requests[i].NbReceived;
		}

		if (Requests[i].NbWaiting <= Requests[i].NbReceived)
		{
			// the request is over, send to the php

			CMessage msgout(M_ADMIN_VIEW);
			msgout.serial (Requests[i].Id);

			for (uint j = 0; j < Requests[i].Answers.size (); j++)
			{
				msgout.serialCont (Requests[i].Answers[j].VarNames);
				msgout.serialCont (Requests[i].Answers[j].Values);
			}

			if (Requests[i].SId.get() == 0)
			{
				sipinfo ("ADMIN: Receive an answer for the fake request %d with %d answers", Requests[i].Id, Requests[i].Answers.size ());
				for (uint j = 0; j < Requests[i].Answers.size (); j++)
				{
					uint k;
					for (k = 0; k < Requests[i].Answers[j].VarNames.size(); k++)
					{
						InfoLog->displayRaw ("%-10s", Requests[i].Answers[j].VarNames[k].c_str());
					}
					InfoLog->displayRawNL("");
					for (k = 0; k < Requests[i].Answers[j].Values.size(); k++)
					{
						InfoLog->displayRaw ("%-10s", Requests[i].Answers[j].Values[k].c_str());
					}
					InfoLog->displayRawNL("");
					InfoLog->displayRawNL("-------------------------");
				}	
			}
			else
			{
				sipinfo ("ADMIN: The request is over, send the result to AES");
				CUnifiedNetwork::getInstance ()->send (Requests[i].SId, msgout);
			}

			// set to 0 to erase it
			Requests[i].NbWaiting = 0;
			sipdebug ("ADMIN: ++ i %d rid %d NbWaiting0 %d NbReceived %d", i, Requests[i].Id, Requests[i].NbWaiting, Requests[i].NbReceived);
		}

		if (Requests[i].NbWaiting == 0)
		{
			Requests.erase (Requests.begin ()+i);
		}
		else
		{
			i++;
		}
	}
}

// all remote command start with rc or RC
bool isRemoteCommand(string &str)
{
	if (str.size()<2) return false;
	return tolower(str[0]) == 'r' && tolower(str[1]) == 'c';
}


// this callback is used to create a view for the admin system
void serviceGetView (uint32 rid, const string &rawvarpath, TAdminViewResult &answer, bool async)
{
	string str;
	CLog logDisplayVars;
	CLightMemDisplayer mdDisplayVars;
	logDisplayVars.addDisplayer (&mdDisplayVars);
	mdDisplayVars.setParam (4096);

	CVarPath varpath(rawvarpath);

	if (varpath.empty())
		return;

	// special case for named command handler
	if (CCommandRegistry::getInstance().isNamedCommandHandler(varpath.Destination[0].first))
	{
		varpath.Destination[0].first += "."+varpath.Destination[0].second;
		varpath.Destination[0].second = "";
	}

	if (varpath.isFinal())
	{
		TAdminViewVarNames varNames;
		TAdminViewValues values;

		// add default row
		varNames.push_back ("service");
		values.push_back (IService::getInstance ()->getServiceUnifiedName());
	
		for (uint j = 0; j < varpath.Destination.size (); j++)
		{
			string cmd = varpath.Destination[j].first;

			// replace = with space to execute the command
			string::size_type eqpos = cmd.find("=");
			if (eqpos != string::npos)
			{
				cmd[eqpos] = ' ';
				varNames.push_back(cmd.substr(0, eqpos));
			}
			else
				varNames.push_back(cmd);
			
			mdDisplayVars.clear ();
			ICommand::execute(cmd, logDisplayVars, !ICommand::isCommand(cmd));

#ifdef SIP_USE_UNICODE
			const std::deque<std::basic_string<ucchar> > &strs = mdDisplayVars.lockStrings();
#else
			const std::deque<std::string> &strs = mdDisplayVars.lockStrings();
#endif

			if (ICommand::isCommand(cmd))
			{
				// we want the log of the command
				if (j == 0)
				{
					varNames.clear ();
					varNames.push_back ("__log");
					values.clear ();
				}
				
				values.push_back ("----- Result from "+IService::getInstance()->getServiceUnifiedName()+" of command '"+cmd+"'\n");
				for (uint k = 0; k < strs.size(); k++)
				{
#ifdef SIP_USE_UNICODE
					values.push_back(SIPBASE::toString("%S", strs[k].c_str()));
#else
					values.push_back (strs[k]);
#endif
				}
			}
			else
			{
				if (strs.size() > 0)
				{
#ifdef SIP_USE_UNICODE
					basic_string<ucchar> ucstr = strs[0].substr(0, strs[0].size() - 1);
					str = SIPBASE::toString("%S", ucstr.c_str());
#else
					str = strs[0].substr(0, strs[0].size()-1);
#endif
					// replace all spaces into udnerscore because space is a reserved char
					for (uint i = 0; i < str.size(); i ++) if (str[i] == ' ') str[i] = '_';
				}
				else
				{
					str = "???";
				}
				values.push_back (str);
				sipinfo ("ADMIN: Add to result view '%s' = '%s'", varpath.Destination[j].first.c_str(), str.c_str());
			}
			mdDisplayVars.unlockStrings();
		}

		if (!async)
			answer.push_back (SAdminViewRow(varNames, values));
		else
		{
			addRequestWaitingNb (rid);
			addRequestAnswer (rid, varNames, values);
		}
	}
	else
	{
		// there s an entity in the varpath, manage this case

		TAdminViewVarNames *varNames=0;
		TAdminViewValues *values=0;
		
		// varpath.Destination		contains the entity number
		// subvarpath.Destination	contains the command name
		
		for (uint i = 0; i < varpath.Destination.size (); i++)
		{
			CVarPath subvarpath(varpath.Destination[i].second);
			
			for (uint j = 0; j < subvarpath.Destination.size (); j++)
			{
				// set the variable name
				string cmd = subvarpath.Destination[j].first;

				if (isRemoteCommand(cmd))
				{
					if (async && RemoteClientCallback != 0)
					{
						// ok we have to send the request to another side, just send and wait
						addRequestWaitingNb (rid);
						RemoteClientCallback (rid, cmd, varpath.Destination[i].first);
					}
				}
				else
				{
					// replace = with space to execute the command
					string::size_type eqpos = cmd.find("=");
					if (eqpos != string::npos)
					{
						cmd[eqpos] = ' ';
						// add the entity
						cmd.insert(eqpos, " "+varpath.Destination[i].first);
					}
					else
					{
						// add the entity
						cmd += " "+varpath.Destination[i].first;
					}
					
					mdDisplayVars.clear ();
					ICommand::execute(cmd, logDisplayVars, true);
#ifdef SIP_USE_UNICODE
					const std::deque<std::basic_string<ucchar> > &strs = mdDisplayVars.lockStrings();
					for (uint k = 0; k < strs.size(); k++)
					{
						const string &str = SIPBASE::toString("%S", strs[k].c_str());
#else
					const std::deque<std::string> &strs = mdDisplayVars.lockStrings();
					for (uint k = 0; k < strs.size(); k++)
					{
						const string &str = strs[k];
#endif
						string::size_type pos = str.find(" ");
						if(pos == string::npos)
							continue;
						
						string entity = str.substr(0, pos);
						string value = str.substr(pos+1, str.size()-pos-2);
						for (uint u = 0; u < value.size(); u++) if (value[u] == ' ') value[u] = '_';
						
						// look in the array if we already have something about this entity

						if (!async)
						{
							uint y;
							for (y = 0; y < answer.size(); y++)
							{
								if (answer[y].Values[1] == entity)
								{
									// ok we found it, just push_back new stuff
									varNames = &(answer[y].VarNames);
									values = &(answer[y].Values);
									break;
								}
							}
							if (y == answer.size ())
							{
								answer.push_back (SAdminViewRow());

								varNames = &(answer[answer.size()-1].VarNames);
								values = &(answer[answer.size()-1].Values);
								
								// don't add service if we want an entity
		// todo when we work on entity, we don't need service name and server so we should remove them and collapse all var for the same entity
								varNames->push_back ("service");
								string name = IService::getInstance ()->getServiceUnifiedName();
								values->push_back (name);
								
								// add default row
								varNames->push_back ("entity");
								values->push_back (entity);
							}

							varNames->push_back (cmd.substr(0, cmd.find(" ")));
							values->push_back (value);
						}
						else
						{
							addRequestWaitingNb (rid);

							TAdminViewVarNames varNames;
							TAdminViewValues values;
							varNames.push_back ("service");
							string name = IService::getInstance ()->getServiceUnifiedName();
							values.push_back (name);

							// add default row
							varNames.push_back ("entity");
							values.push_back (entity);

							varNames.push_back (cmd.substr(0, cmd.find(" ")));
							values.push_back (value);
	
							addRequestAnswer (rid, varNames, values);
						}
						sipinfo ("ADMIN: Add to result view for entity '%s', '%s' = '%s'", varpath.Destination[i].first.c_str(), subvarpath.Destination[j].first.c_str(), str.c_str());
					}
					mdDisplayVars.unlockStrings();
				}
			}
		}
	}
}


//
// Alarms functions
//

void sendAdminEmail (char *format, ...)
{
	char *text;
	SIPBASE_CONVERT_VARGS (text, format, 4096);
	
	time_t t = time (&t);

	string str;
	str  = asctime (localtime (&t));
	str += " Server " + IService::getInstance()->getHostName();
	str += " service " + IService::getInstance()->getServiceUnifiedName();
	str += " : ";
	str += text;

	CMessage msgout(M_ADMIN_ADMIN_EMAIL);
	msgout.serial (str);
	if(IService::getInstance ()->getServiceShortName()=="AES")
		CUnifiedNetwork::getInstance ()->send ("AS", msgout);
	else
		CUnifiedNetwork::getInstance ()->send ("AES", msgout);

	sipinfo ("ADMIN: Forwarded email to AS with '%s'", str.c_str());
}

void initAdmin (bool dontUseAES, uint16 aesport)
{
	if (!dontUseAES)
	{
		CUnifiedNetwork::getInstance()->setServiceUpCallback ("AES", cbAESConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback ("AES", cbAESDisconnection, NULL);
		CUnifiedNetwork::getInstance()->addService ("AES", CInetAddress("localhost", aesport));
	}
	CUnifiedNetwork::getInstance()->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(CallbackArray[0]));
}


void updateAdmin()
{
	uint32 CurrentTime = CTime::getSecondsSince1970();


	//
	// check admin requests
	//

	cleanRequest ();


	//
	// Check graph updates
	//

	static uint32 lastGraphUpdateCheck = 0;
	
	if (CurrentTime >= lastGraphUpdateCheck+1)
	{
		if ( DontUseGraphUpdate )
		{
			if ( lastGraphUpdateCheck == 0 )
			{
				lastGraphUpdateCheck = 1;
                sipdebug("ADMIN: ++ DontUseGraphUpdate flag set! Graph update Canceled! lastGraphupdateCheck %u", lastGraphUpdateCheck);
			}
			return;
		}

		string str;
		CLog logDisplayVars;
		CLightMemDisplayer mdDisplayVars;
		logDisplayVars.addDisplayer (&mdDisplayVars);
		
		lastGraphUpdateCheck = CurrentTime;

		CMessage msgout (M_ADMIN_GRAPH_UPDATE);
		bool empty = true;
		for (uint j = 0; j < GraphUpdates.size(); j++)
		{
			if (CurrentTime >= GraphUpdates[j].LastUpdate + GraphUpdates[j].Update)
			{
				// have to send a new update for this var
				ICommand::execute(GraphUpdates[j].Name, logDisplayVars, true, false);
#ifdef SIP_USE_UNICODE
				const std::deque<std::basic_string<ucchar> > &strs = mdDisplayVars.lockStrings();
				sint32 val;
				if (strs.size() != 1)
				{
					sipwarning ("ADMIN: The graph update command execution not return exactly 1 line but %d", strs.size());
					for (uint i = 0; i < strs.size(); i++)
					  sipwarning ("ADMIN: line %d: '%S'", i, strs[i].c_str());
					val = 0;
				}
				else
				{
					// changed by ysg 2009-05-18
//					val = _wtoi(strs[0].c_str());
#ifdef SIP_OS_WINDOWS
					val = _wtoi(strs[0].c_str());
#else
					val = atoi(toString("%S", strs[0].c_str()).c_str());
#endif
				}
#else
				const std::deque<std::string> &strs = mdDisplayVars.lockStrings();
				sint32 val;
				if (strs.size() != 1)
				{
					sipwarning ("ADMIN: The graph update command execution not return exactly 1 line but %d", strs.size());
					for (uint i = 0; i < strs.size(); i++)
					  sipwarning ("ADMIN: line %d: '%s'", i, strs[i].c_str());
					val = 0;
				}
				else
				{
					val = atoi(strs[0].c_str());
				}
#endif
				mdDisplayVars.unlockStrings ();
				mdDisplayVars.clear ();
				
				string name = IService::getInstance()->getServiceAliasName();
				if (name.empty())
					name = IService::getInstance()->getServiceShortName();

				if(empty)
					msgout.serial (CurrentTime);

				msgout.serial (name);
				msgout.serial (GraphUpdates[j].Name);
				msgout.serial (val);

				empty = false;

				GraphUpdates[j].LastUpdate = CurrentTime;
				//sipdebug("GraphUpdate To AdminSystem : Time:%u, name:%s, GraphUpdate_Name:%s, val:%u", CurrentTime, name.c_str(), GraphUpdates[j].Name.c_str(), val);
			}
		}

		if(!empty)
		{
			if(IService::getInstance ()->getServiceShortName()=="AES")
			{
				CUnifiedNetwork::getInstance ()->send ("AS", msgout);
			}
			else
			{
				CUnifiedNetwork::getInstance ()->send ("AES", msgout);
			}
			
		}
	}

	//
	// Check alarms
	//

	static uint32 lastAlarmsCheck = 0;

	if (CurrentTime >= lastAlarmsCheck+AlarmCheckDelay)
	{
		string str;
		CLog logDisplayVars;
		CLightMemDisplayer mdDisplayVars;
		logDisplayVars.addDisplayer (&mdDisplayVars);
		
		lastAlarmsCheck = CTime::getSecondsSince1970();

		for (uint i = 0; i < Alarms.size(); )
		{
			mdDisplayVars.clear ();
			ICommand::execute(Alarms[i].Name, logDisplayVars, true, false);
#ifdef SIP_USE_UNICODE
			const std::deque<std::basic_string<ucchar> > &strs = mdDisplayVars.lockStrings();
			
			if (strs.size() > 0)
			{
				basic_string<ucchar> ucstr = strs[0].substr(0, strs[0].size() - 1);
				str = SIPBASE::toString("%S", ucstr.c_str());
			}
			else
			{
				str = "???";
			}
#else
			const std::deque<std::string> &strs = mdDisplayVars.lockStrings();

			if (strs.size()>0)
			{
				str = strs[0].substr(0,strs[0].size()-1);
			}
			else
			{
				str = "???";
			}
#endif
			mdDisplayVars.unlockStrings();
			
			if (str == "???")
			{
				// variable doesn't exist, remove it from alarms
				sipwarning ("ADMIN: Alarm problem: variable '%s' returns ??? instead of a good value", Alarms[i].Name.c_str());
				Alarms.erase (Alarms.begin()+i);
			}
			else
			{
				// compare the value
				uint32 err = Alarms[i].Limit;
				uint32 val = humanReadableToBytes(str);
				if (Alarms[i].GT && val >= err)
				{
					if (!Alarms[i].Activated)
					{
						sipinfo ("ADMIN: VARIABLE TOO BIG '%s' %u >= %u", Alarms[i].Name.c_str(), val, err);
						Alarms[i].Activated = true;
						sendAdminEmail ((char*)"Alarm: Variable %s is %u that is greater or equal than the limit %u", Alarms[i].Name.c_str(), val, err);
					}
				}
				else if (!Alarms[i].GT && val <= err)
				{
					if (!Alarms[i].Activated)
					{
						sipinfo ("ADMIN: VARIABLE TOO LOW '%s' %u <= %u", Alarms[i].Name.c_str(), val, err);
						Alarms[i].Activated = true;
						sendAdminEmail ((char*)"Alarm: Variable %s is %u that is lower or equal than the limit %u", Alarms[i].Name.c_str(), val, err);
					}
				}
				else
				{
					if (Alarms[i].Activated)
					{
						sipinfo ("ADMIN: variable is ok '%s' %u %s %u", Alarms[i].Name.c_str(), val, (Alarms[i].GT?"<":">"), err);
						Alarms[i].Activated = false;
					}
				}
				
				i++;
			}
		}
	}
}

void setInformations (const vector<string> &alarms, const vector<string> &graphupdate)
{
	uint i;

	// add only commands that I understand
	Alarms.clear ();
	for (i = 0; i < alarms.size(); i+=3)
	{
		CVarPath shardvarpath (alarms[i]);
		if(shardvarpath.Destination.size() == 0 || shardvarpath.Destination[0].second.empty())
			continue;
		CVarPath servervarpath (shardvarpath.Destination[0].second);
		if(servervarpath.Destination.size() == 0 || servervarpath.Destination[0].second.empty())
			continue;
		CVarPath servicevarpath (servervarpath.Destination[0].second);
		if(servicevarpath.Destination.size() == 0 || servicevarpath.Destination[0].second.empty())
			continue;
	
		string name = servicevarpath.Destination[0].second;

		if (IService::getInstance()->getServiceUnifiedName().find(servicevarpath.Destination[0].first) != string::npos && ICommand::exists(name))
		{
			sipinfo ("ADMIN: Adding alarm '%s' limit %d order %s (varpath '%s')", name.c_str(), atoi(alarms[i+1].c_str()), alarms[i+2].c_str(), alarms[i].c_str());
			Alarms.push_back(CAlarm(name, atoi(alarms[i+1].c_str()), alarms[i+2]=="gt"));
		}
		else
		{
			if (IService::getInstance()->getServiceUnifiedName().find(servicevarpath.Destination[0].first) == string::npos)
			{
				sipinfo ("ADMIN: Skipping alarm '%s' limit %d order %s (varpath '%s') (not for my service, i'm '%s')", name.c_str(), atoi(alarms[i+1].c_str()), alarms[i+2].c_str(), alarms[i].c_str(), IService::getInstance()->getServiceUnifiedName().c_str());
			}
			else
			{
				sipinfo ("ADMIN: Skipping alarm '%s' limit %d order %s (varpath '%s') (var not exist)", name.c_str(), atoi(alarms[i+1].c_str()), alarms[i+2].c_str(), alarms[i].c_str());
			}
		}
	}

	// do the same with graph update
	GraphUpdates.clear ();
	for (i = 0; i < graphupdate.size(); i+=2)
	{
		CVarPath servicevarpath (graphupdate[i]);
		if(servicevarpath.Destination.size() == 0 || servicevarpath.Destination[0].second.empty())
			continue;
		
		string VarName		= servicevarpath.Destination[0].second;
		string ServiceName	= servicevarpath.Destination[0].first;
		string	shortName	= IService::getInstance()->getServiceShortName();
		CSStringA	name	= ServiceName;
		if ( !strcmp(name.right(1).c_str(), "*") )	// SS*
			name = name.rightCrop(1);

		if ( ICommand::exists(VarName)
			&& (ServiceName == "*" || shortName == ServiceName || strstr(shortName.c_str(), name.c_str()) != NULL) )
		{
			sipinfo ("ADMIN: Adding graphupdate '%s' update %d (varpath '%s')", VarName.c_str(), atoi(graphupdate[i+1].c_str()), graphupdate[i].c_str());
			GraphUpdates.push_back(CGraphUpdate(VarName, atoi(graphupdate[i+1].c_str())));
		}
		else
		{
			if (shortName != ServiceName)
			{
				sipinfo ("ADMIN: Skipping graphupdate '%s' limit %d (varpath '%s') (not for my service, i'm '%s')", VarName.c_str(), atoi(graphupdate[i+1].c_str()), graphupdate[i].c_str(), IService::getInstance()->getServiceUnifiedName().c_str());
			}
			else
			{
				sipinfo ("ADMIN: Skipping graphupdate '%s' limit %d (varpath '%s') (var not exist)", VarName.c_str(), atoi(graphupdate[i+1].c_str()), graphupdate[i].c_str());
			}
		}
	}
}

//
// Commands
//

SIPBASE_CATEGORISED_COMMAND(admin, displayInformations, "displays all admin informations", "")
{
	uint i;

	log.displayNL("There're %d alarms:", Alarms.size());
	for (i = 0; i < Alarms.size(); i++)
	{
		log.displayNL(" %d %s %d %s %s", i, Alarms[i].Name.c_str(), Alarms[i].Limit, (Alarms[i].GT?"gt":"lt"), (Alarms[i].Activated?"on":"off"));
	}
	log.displayNL("There're %d graphupdate:", GraphUpdates.size());
	for (i = 0; i < GraphUpdates.size(); i++)
	{
		log.displayNL(" %d %s %d %d", i, GraphUpdates[i].Name.c_str(), GraphUpdates[i].Update, GraphUpdates[i].LastUpdate);
	}
	return true;
}

SIPBASE_CATEGORISED_COMMAND(admin, getView, "send a view and receive an array as result", "<varpath>")
{
	if(args.size() != 1) return false;
	
	TAdminViewResult answer;
	serviceGetView (0, args[0], answer);
	
	log.displayNL("have %d answer", answer.size());
	for (uint i = 0; i < answer.size(); i++)
	{
		log.displayNL("  have %d value", answer[i].VarNames.size());
		
		sipassert (answer[i].VarNames.size() == answer[i].Values.size());
		
		for (uint j = 0; j < answer[i].VarNames.size(); j++)
		{
			log.displayNL("    %s -> %s", answer[i].VarNames[j].c_str(), answer[i].Values[j].c_str());
		}
	}
	
	return true;
}

// changed by ysg 2009-05-18
void	fDownloadNotify(float fProgress, void* pParam, unsigned long dwNowSize, void* pMemoryPtr)
//void	fDownloadNotify(float fProgress, void* pParam, DWORD dwNowSize, void* pMemoryPtr)
{
	// sipinfo	: use notify
}

SIPBASE_CATEGORISED_COMMAND(admin, downloadFileFromFTP, "Download a file from ftp server", "<Address>|<Filepath>|<Username>|<Password>")
{
/*
	string sAddress = "202.111.175.62";
	string sFilepath = "/test/libeay32.dll";
	string sUsername = "bjyj-ftp";
	string sPassword = "cientx19690526";
*/
#ifdef SIP_OS_WINDOWS
	if(args.size() < 4)
		return false;
	string sAddress = args[0];
	string sFilepath = args[1];
	string sUsername = args[2];
	string sPassword = args[3];
	uint16 uPort = 21;


	string sFile;

	CFtpDown ftp;
	bool bFTPOpen;
#ifdef _UNICODE
	bFTPOpen = ftp.OpenFtp(ucstring(sAddress).c_str(), uPort, ucstring(sUsername).c_str(), ucstring(sPassword).c_str());
#else
	bFTPOpen = ftp.OpenFtp(sAddress.c_str(), uPort,	sUsername.c_str(), sPassword.c_str());
#endif

	TCHAR	drive[_MAX_DRIVE];
	TCHAR	dir[_MAX_DIR];
	TCHAR	fname[_MAX_FNAME];
	TCHAR	ext[_MAX_EXT];
	TCHAR	full[MAX_PATH];

#ifdef _UNICODE
	_tsplitpath(ucstring(sFilepath).c_str(), drive, dir, fname, ext);
	ftp.SetCurrentDirectory(ucstring(dir).c_str());
	swprintf(full, _T("%s%s"), fname, ext);
	sFile = ucstring(full).toString();
#else
	_tsplitpath(sFilepath.c_str(), drive, dir, fname, ext);
	ftp.SetCurrentDirectory(string(dir).c_str());
	sprintf(full, _T("%s%s"), fname, ext);
	sFile = full;
#endif
	

	uint32	errNo;
	char	reason[256];
	uint32	nMax = 256;
	BOOL	ret;

	if ( !bFTPOpen )
	{
		ret		= InternetGetLastResponseInfoA(&errNo, reason, &nMax);
		log.displayNL("Failed Connect FTPServer: Adress %s, User %s, Password %s\n reason : %s", 
			sAddress.c_str(),
			sUsername.c_str(),
			sPassword.c_str(),
			reason
			);
		return FALSE;
	}
	string sDest = IService::getInstance()->getConfigDirectory().c_str() + sFile;
#ifdef _UNICODE
	bool bRet	= ftp.Download(ucstring(sFile).c_str(), ucstring(sDest).c_str(), 0, fDownloadNotify);
#else
	bool bRet	= ftp.Download(sFile.c_str(), sDest.c_str(), 0, fDownloadNotify);
#endif
	ftp.Close();

	if (bRet)
		log.displayNL("Success download file.");
	else
		log.displayNL("Failed download file.");
#else //SIP_OS_WINDOWS
		log.displayNL("This function not prepared on no-windows system.");
#endif
	return true;
}

SIPBASE_CATEGORISED_COMMAND(admin, restartOS, "Restart OS", "")
{
#ifdef SIP_OS_WINDOWS
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
		&tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 

	if (GetLastError() != ERROR_SUCCESS) 
		return false; 

	// Shut down the system and force all applications to close. 

	if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0)) 
		return false;
#else
	sipassert(false);
	return false;
#endif
	return true;
}

SIPBASE_CATEGORISED_COMMAND(admin, enumProcess, "Enumerate all process.", "")
{
#ifdef SIP_OS_WINDOWS
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, 
		&tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 

	CloseHandle( hToken );

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return false;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for ( i = 0; i < cProcesses; i++ )
	{
		DWORD processID = aProcesses[i];
		TCHAR szProcessName[MAX_PATH] = _T("unknown");
		TCHAR chName[1000] = _T("unknown");

		// Get a handle to the process.

		HANDLE hProcess = adv_open_process( processID, PROCESS_ALL_ACCESS);//PROCESS_QUERY_INFORMATION | PROCESS_VM_READ );
		//		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			ExtractProcessOwner(hProcess, chName);

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
				&cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, 
					sizeof(szProcessName) );
			}
			else
			{ 
				continue;
			}
		}
		else
		{
			continue;
		}

		// Print the process name and identifier.

		log.displayNL( L"%s (Process ID : %u, User Name : %s)", szProcessName, processID, chName );

		CloseHandle( hProcess );
	}
#else
	sipassert(false);
	return false;
#endif
	return true;
}

SIPBASE_CATEGORISED_COMMAND(admin, exitProcessPID, "Exit process of PID.", "Process ID")
{
#ifdef SIP_OS_WINDOWS
	if(args.size() < 1)	
		return false;
	string v	= args[0];
	uint32 val = atoui(v.c_str());
	
	BOOL		bTerminate = kill_process(val);
	if (bTerminate)
		log.displayNL( L"Success terminating (Process ID: %u)", val);
	else
		log.displayNL( L"Failed terminating (Process ID: %u)", val);
#else
	sipassert(false);
	return false;
#endif
	return true;
}
SIPBASE_CATEGORISED_COMMAND(admin, exitProcess, "Exit process of specified name.", "Process name(admin.exe)")
{
#ifdef SIP_OS_WINDOWS
	if(args.size() < 1)	return false;
	string		strName = args[0];
	ucstring	ucName = ucstring::makeFromUtf8(strName);
	BOOL		bTerminate = TerminateRunningProcess((TCHAR *)ucName.c_str());
	if (bTerminate)
		log.displayNL( L"Success terminating %s ", ucName.c_str());
	else
		log.displayNL( L"Failed terminating %s ", ucName.c_str());

/*
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, 
		&tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 

	CloseHandle( hToken );

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return false;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.
	bool bFind = false;
	for ( i = 0; i < cProcesses; i++ )
	{
		DWORD processID = aProcesses[i];
		TCHAR szProcessName[MAX_PATH] = _T("unknown");

		// Get a handle to the process.

		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, processID );

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
				&cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, 
					sizeof(szProcessName) );
			}
			else 
				continue;
		}
		else
			continue;

		// Print the process name and identifier.
		ucstring ucProcess = ucstring(szProcessName);
		if ( ucName == ucProcess)
		{
			bFind = true;
			UINT uExitcode = 0;
			BOOL bS = kill_process(processID);
			if (bS)
				log.displayNL( L"Success terminating %s (Process ID: %u)", szProcessName, processID );
			else
				log.displayNL( L"Failed terminating %s (Process ID: %u)", szProcessName, processID );
		}
		CloseHandle( hProcess );
	}
	if (!bFind)
		log.displayNL( L"There is no process %s", ucName.c_str() );
*/
#else
	sipassert(false);
	return false;
#endif
	return true;
}

} // SIPNET
