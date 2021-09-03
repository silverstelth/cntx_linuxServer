/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/callback_client.h"
#include "net/callback_server.h"
#include "net/naming_client.h"
#include "net/message.h"

#include "net/unitime.h"

using namespace SIPBASE;
using namespace std;

namespace SIPNET
{

TTime _CUniTime::_SyncUniTime = 0;
TTime _CUniTime::_SyncLocalTime = 0;
bool _CUniTime::_Simulate = false;

bool _CUniTime::Sync = false;


void _CUniTime::setUniTime (SIPBASE::TTime uTime, SIPBASE::TTime lTime)
{
	sipstop;
/*	if (Sync)
	{
		TTime lt = getLocalTime ();
		TTime delta = uTime - lTime + _SyncLocalTime - _SyncUniTime;

		sipinfo ("_CUniTime::setUniTime(%"SIP_I64"d, %"SIP_I64"d): Resyncing delta %"SIP_I64"dms",uTime,lTime,delta);
	}
	else
	{
		sipinfo ("_CUniTime::setUniTime(%"SIP_I64"d, %"SIP_I64"d)",uTime,lTime);
		Sync = true;
	}
	_SyncUniTime = uTime;
	_SyncLocalTime = lTime;
*/}

void _CUniTime::setUniTime (SIPBASE::TTime uTime)
{
	sipstop;
//	setUniTime (uTime, getLocalTime ());
}



TTime _CUniTime::getUniTime ()
{
	sipstop;
	return 0;
/*	if (!Sync)
	{
		siperror ("called getUniTime before calling syncUniTimeFromServer");
	}
	return getLocalTime () - (_SyncLocalTime - _SyncUniTime);
*/
}


const char *_CUniTime::getStringUniTime ()
{
	sipstop;
	return getStringUniTime(_CUniTime::getUniTime());
}


const char *_CUniTime::getStringUniTime (TTime ut)
{
	sipstop;
	static char str[512];

	uint32 ms = (uint32) (ut % 1000); // time in ms 1000ms dans 1s
	ut /= 1000;

	uint32 s = (uint32) (ut % 60); // time in seconds 60s dans 1mn
	ut /= 60;

	uint32 m = (uint32) (ut % 60); // time in minutes 60m dans 1h
	ut /= 60;

	uint32 h = (uint32) (ut % 9); // time in hours 9h dans 1j
	ut /= 9;

	uint32 day = (uint32) (ut % (8*4)); // time in days 8day dans 1month
	ut /= 8;

	uint32 week = (uint32) (ut % 4); // time in weeks 4week dans 1month
	ut /= 4;

	uint32 month = (uint32) (ut % 12); // time in months 12month dans 1year
	ut /= 12;

	uint  year =  (uint32) ut;	// time in years

	smprintf (str, 512, "%02d/%02d/%04d (week %d) %02d:%02d:%02d.%03d", day+1, month+1, year+1, week+1, h, m, s, ms);
	return str;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// SYNCHRONISATION BETWEEN TIME SERVICE AND OTHER SERVICES ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

static bool GetUniversalTime;
static uint32 GetUniversalTimeSecondsSince1970;
static TTime GetUniversalTimeUniTime;


static void cbGetUniversalTime (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase)
{
	sipstop;
	// get the association between a date and unitime
	msgin.serial (GetUniversalTimeSecondsSince1970);
	msgin.serial (GetUniversalTimeUniTime);
	GetUniversalTime = true;
}

/***************************************************************/
/******************* THE FOLLOWING CODE IS COMMENTED OUT *******/
/***************************************************************
static TCallbackItem UniTimeCallbackArray[] =
{
	{ "GUT", cbGetUniversalTime }
};
***************************************************************/

void _CUniTime::syncUniTimeFromService (CCallbackNetBase::TRecordingState rec, const CInetAddress *addr)
{
	sipstop;
/***************************************************************/
/******************* THE FOLLOWING CODE IS COMMENTED OUT *******/
/***************************************************************
	TTime deltaAdjust, lt;
	uint32 firstsecond, nextsecond;
	TTime before, after, delta;

	// create a message with type in the full text format
	CMessage msgout ("AUT");
	CCallbackClient server( rec, "TS.nmr" );
	server.addCallbackArray (UniTimeCallbackArray, sizeof (UniTimeCallbackArray) / sizeof (UniTimeCallbackArray[0]));

	if (addr == NULL)
	{
		CNamingClient::lookupAndConnect ("TS", server);
	}
	else
	{
		server.connect (*addr);
	}

	if (!server.connected()) goto error;

	server.send (msgout);

	// before time
	before = CTime::getLocalTime ();

	// receive the answer
	GetUniversalTime = false;
	while (!GetUniversalTime)
	{
		if (!server.connected()) goto error;
			
		server.update ();

		sipSleep( 0 );
	}

	// after, before and delta is not used. It's only for information purpose.
	after = CTime::getLocalTime ();
	delta = after - before;

	sipinfo ("_CUniTime::syncUniTimeFromService(): ping:%"SIP_I64"dms, time:%ds, unitime:%"SIP_I64"dms", delta, GetUniversalTimeSecondsSince1970, GetUniversalTimeUniTime);

// <-- from here to the "-->" comment, the block must be executed in less than one second or an infinite loop occurs

	// get the second
	firstsecond = CTime::getSecondsSince1970 ();
	nextsecond = firstsecond+1;
	
	// wait the next start of the second (take 100% of CPU to be more accurate)
	while (nextsecond != CTime::getSecondsSince1970 ())
		sipassert (CTime::getSecondsSince1970 () <= nextsecond);

// -->

	// get the local time of the beginning of the next second
	lt = CTime::getLocalTime ();

	if ( ! _Simulate )
	{
		if (abs((sint32)((TTime)nextsecond - (TTime)GetUniversalTimeSecondsSince1970)) > 10)
		{
			siperror ("the time delta (between me and the Time Service) is too big (more than 10s), servers aren't NTP synchronized");
			goto error;
		}
		
		// compute the delta between the other side and our side number of second since 1970
		deltaAdjust = ((TTime) nextsecond - (TTime) GetUniversalTimeSecondsSince1970) * 1000;

		// adjust the unitime to the current localtime
		GetUniversalTimeUniTime += deltaAdjust;

		sipinfo ("_CUniTime::syncUniTimeFromService(): rtime:%ds, runitime:%"SIP_I64"ds, rlocaltime:%"SIP_I64"d, deltaAjust:%"SIP_I64"dms", nextsecond, GetUniversalTimeUniTime, lt, deltaAdjust);
	}
	else
	{
		sipinfo ("_CUniTime::syncUniTimeFromService(): runitime:%"SIP_I64"ds, rlocaltime:%"SIP_I64"d", GetUniversalTimeUniTime, lt);
	}

	_CUniTime::setUniTime (GetUniversalTimeUniTime, lt);

	server.disconnect ();
	return;

error:
	siperror ("Time Service is not found, lost or can't synchronize universal time");
***************************************************************/

}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// SYNCHRONISATION BETWEEN CLIENT AND SHARD ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// Server part

static void cbServerAskUniversalTime (CMessage& msgin, TTcpSockId from, CCallbackNetBase &netbase)
{
	sipstop;
	TTime ut = _CUniTime::getUniTime ();

	// afficher l adresse de celui qui demande
	sipinfo("UT: Send the universal time %" SIP_I64 "d to '%s'", ut, netbase.hostAddress(from).asString().c_str());
	
	CMessage msgout (M_UNITIME_GUT);
	msgout.serial (ut);
	netbase.send (msgout, from);
}

TCallbackItem ServerTimeServiceCallbackArray[] =
{
	{ M_UNITIME_AUT, cbServerAskUniversalTime },
};

void _CUniTime::installServer (CCallbackServer *server)
{
	sipstop;
	static bool alreadyAddedCallback = false;
	sipassert (server != NULL);
	sipassert (!alreadyAddedCallback);

	server->addCallbackArray (ServerTimeServiceCallbackArray, sizeof (ServerTimeServiceCallbackArray) / sizeof (ServerTimeServiceCallbackArray[0]));
	alreadyAddedCallback = true;
}

// Client part

static bool GetClientUniversalTime;
static TTime GetClientUniversalTimeUniTime;

static void cbClientGetUniversalTime (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase)
{
	sipstop;
	// get the association between a date and unitime
	msgin.serial (GetClientUniversalTimeUniTime);
	GetClientUniversalTime = true;
}

/***************************************************************/
/******************* THE FOLLOWING CODE IS COMMENTED OUT *******/
/***************************************************************
static TCallbackItem ClientUniTimeCallbackArray[] =
{
	{ "GUT", cbClientGetUniversalTime }
};
***************************************************************/


void _CUniTime::syncUniTimeFromServer (CCallbackClient *client)
{
	sipstop;
/***************************************************************/
/******************* THE FOLLOWING CODE IS COMMENTED OUT *******/
/***************************************************************

	static bool alreadyAddedCallback = false;
	sipassert (client != NULL);

	if (!alreadyAddedCallback)
	{
		client->addCallbackArray (ClientUniTimeCallbackArray, sizeof (ClientUniTimeCallbackArray) / sizeof (ClientUniTimeCallbackArray[0]));
		alreadyAddedCallback = true;
	}

	sint attempt = 0;
	TTime bestdelta = 60000;	// 1 minute

	if (!client->connected ()) goto error;

	while (attempt < 10)
	{
		CMessage msgout ("AUT");

		if (!client->connected()) goto error;

		// send the message
		client->send (msgout);

		// before time
		TTime before = CTime::getLocalTime ();

		// receive the answer
		GetClientUniversalTime = false;
		while (!GetClientUniversalTime)
		{
			if (!client->connected()) goto error;
				
			client->update ();
		}

		TTime after = CTime::getLocalTime (), delta = after - before;

		if (delta < 10 || delta < bestdelta)
		{
			bestdelta = delta;

			_CUniTime::setUniTime (GetClientUniversalTimeUniTime, (before+after)/2);

			if (delta < 10) break;
		}
		attempt++;
	}
	client->disconnect ();
	sipinfo ("Universal time is %"SIP_I64"dms with a mean error of %"SIP_I64"dms", _CUniTime::getUniTime(), bestdelta/2);
	return;
error:
	sipwarning ("there's no connection or lost or can't synchronize universal time");
***************************************************************/
}


//
// Commands
//
/*
SIPBASE_CATEGORISED_COMMAND(sip, time, "displays the universal time", "")
{
	if(args.size() != 0) return false;

	if ( _CUniTime::Sync )
	{
		log.displayNL ("CTime::getLocalTime(): %"SIP_I64"dms, _CUniTime::getUniTime(): %"SIP_I64"dms", CTime::getLocalTime (), _CUniTime::getUniTime ());
		log.displayNL ("_CUniTime::getStringUniTime(): '%s'", _CUniTime::getStringUniTime());
	}
	else
	{
		log.displayNL ("CTime::getLocalTime(): %"SIP_I64"dms <Universal time not sync>", CTime::getLocalTime ());
	}

	return true;
}
*/

} // SIPNET
