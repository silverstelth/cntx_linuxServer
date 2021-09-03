/** \file logic_event.cpp
 * 
 *
 * $Id: logic_event.cpp,v 1.1 2008/06/26 13:41:16 RoMyongGuk Exp $
 */


#include "logic/logic_event.h"
#include "logic/logic_state_machine.h"

#include "net/service.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

namespace SIPLOGIC
{

//----------------------------------- MESSAGE ----------------------------------


//-------------------------------------------------
// serial
//
//-------------------------------------------------
/*void CLogicEventMessage::serial( IStream &f )
{
	f.xmlPush("EVENT_MESSAGE");

	f.serial( Destination );
	f.serial( DestinationId );
	f.serial( MessageId );
	f.serial( Arguments );

	f.xmlPop();

} // serial //*/

void CLogicEventMessage::write (xmlNodePtr node, const char *subName) const
{
	xmlNodePtr elmPtr = xmlNewChild ( node, NULL, (const xmlChar*)string(string(subName)+string("EVENT_MESSAGE")).c_str(), NULL);
	xmlSetProp (elmPtr, (const xmlChar*)"Destination", (const xmlChar*)Destination.c_str());
	xmlSetProp (elmPtr, (const xmlChar*)"DestinationId", (const xmlChar*)toString(DestinationId).c_str());
	xmlSetProp (elmPtr, (const xmlChar*)"MessageId", (const xmlChar*)MessageId.c_str());
	xmlSetProp (elmPtr, (const xmlChar*)"Arguments", (const xmlChar*)Arguments.c_str());
}

void CLogicEventMessage::read (xmlNodePtr node, const char *subName)
{
	xmlCheckNodeName (node, string(string(subName)+string("EVENT_MESSAGE")).c_str());

	Destination = getXMLProp (node, "Destination");
	DestinationId = atoiInt64(getXMLProp (node, "DestinationId").c_str());
	MessageId = getXMLProp (node, "MessageId");
	Arguments = getXMLProp (node, "Arguments");
}





//----------------------------------- ACTION ----------------------------------


//-------------------------------------------------
// enableSendMessage
//
//-------------------------------------------------
void CLogicEventAction::enableSendMessage()
{
	EventMessage.ToSend = true;

} // enableSendMessage //


//-------------------------------------------------
// serial
//
//-------------------------------------------------
/*void CLogicEventAction::serial( IStream &f )
{
	f.xmlPush("EVENT_ACTION");

	f.serial( IsStateChange );
	if( IsStateChange )
	{
		f.serial( StateChange );
	}
	else
	{
		f.serial( EventMessage );
	}

	f.xmlPop();

} // serial //*/

void CLogicEventAction::write (xmlNodePtr node) const
{
	xmlNodePtr elmPtr = xmlNewChild ( node, NULL, (const xmlChar*)"EVENT_ACTION", NULL);
	xmlSetProp (elmPtr, (const xmlChar*)"IsStateChange", (const xmlChar*)toStringA(IsStateChange).c_str());
	if (IsStateChange)
	{
		xmlSetProp (elmPtr, (const xmlChar*)"StateChange", (const xmlChar*)StateChange.c_str());
	}
	else
	{
		EventMessage.write(elmPtr);
	}
}

void CLogicEventAction::read (xmlNodePtr node)
{
	xmlCheckNodeName (node, "EVENT_ACTION");

	IsStateChange = atoi(getXMLProp (node, "IsStateChange").c_str()) == 1;
	if (IsStateChange)
	{
		StateChange = getXMLProp (node, "StateChange");
	}
	else
	{
		EventMessage.read(node);
	}
}








//----------------------------------- EVENT ----------------------------------



//-------------------------------------------------
// reset
//
//-------------------------------------------------
void CLogicEvent::reset()
{ 
	EventAction.EventMessage.Sent = false;
	EventAction.EventMessage.ToSend = false;

} // reset //



//-------------------------------------------------
// setLogicStateMachine
//
//-------------------------------------------------
void CLogicEvent::setLogicStateMachine( CLogicStateMachine * logicStateMachine )
{ 
	if( logicStateMachine == 0 )
	{
		sipwarning("(LOGIC)<CLogicEvent::setLogicStateMachine> The state machine is null");
	}
	else
	{
		// init the logic state machine for this event
		_LogicStateMachine = logicStateMachine;
	}

} // setLogicStateMachine //



//-------------------------------------------------
// testCondition
//
//-------------------------------------------------
bool CLogicEvent::testCondition()
{
	if( _LogicStateMachine )
	{
		if( ConditionName != "no_condition" )
		{
			CLogicCondition cond;
			if( _LogicStateMachine->getCondition( ConditionName, cond ) )
			{
				return cond.testLogic();
			}
			else
			{
				sipwarning("(LOGIC)<CLogicEvent::testCondition> Condition %s not found in the state machine",ConditionName.c_str());	
				return false;
			}
		}
		else
		{
			sipwarning("(LOGIC)<CLogicEvent::testCondition> Condition undefined");	
			return false;
		}
	}
	else
	{
		sipwarning("(LOGIC)<CLogicEvent::testCondition> The state machine managing this event is Null");
	}
	
	return false;	

} // testCondition //


//-------------------------------------------------
// serial
//
//-------------------------------------------------
/*void CLogicEvent::serial( IStream &f )
{
	f.xmlPush("EVENT");
	
	f.serial( ConditionName );
	f.serial( EventAction );
	
	f.xmlPop();

} // serial //*/

void CLogicEvent::write (xmlNodePtr node) const
{
	xmlNodePtr elmPtr = xmlNewChild ( node, NULL, (const xmlChar*)"EVENT", NULL);
	xmlSetProp (elmPtr, (const xmlChar*)"ConditionName", (const xmlChar*)ConditionName.c_str());
	EventAction.write(elmPtr);
}

void CLogicEvent::read (xmlNodePtr node)
{
	xmlCheckNodeName (node, "EVENT");

	ConditionName = getXMLProp (node, "ConditionName");
	EventAction.read(node);
}

} // SIPLOGIC


