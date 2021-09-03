/** \file logic_event.h
 * 
 *
 * $Id: logic_event.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef LOGIC_EVENT_H
#define LOGIC_EVENT_H

#include "misc/types_sip.h"
#include "misc/stream.h"
#include "misc/entity_id.h"
#include "misc/o_xml.h"
#include "misc/i_xml.h"

//#include "game_share/sid.h"


namespace SIPLOGIC
{

class CLogicStateMachine;


/**
 * CLogicEventMessage
 *
 * \date 2001
 */
class CLogicEventMessage
{
public:
	
	/// true if the message has to be sent
	bool ToSend;

	/// true if the message has been sent
	bool Sent;

	/// message destination
	std::string Destination;

	/// message destination id
	SIPBASE::CEntityId DestinationId;

	/// message id
	std::string MessageId;

	/// message arguments
	std::string Arguments;

	/**
	 *	Default constructor
	 */
	CLogicEventMessage()
	{
		ToSend = false;
		Sent = false;
		Destination = "no_destination";
		MessageId = "no_id";
		DestinationId.setType( 0xfe );
		DestinationId.setCreatorId( 0 );
		DestinationId.setDynamicId( 0 );
		Arguments = "no_arguments";
	}

	/**
	 * serial
	 */
	//void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node, const char *subName = "") const;
	void read (xmlNodePtr node, const char *subName = "");
};


/**
 * CLogicEventAction
 *
 * \date 2001
 */
class CLogicEventAction
{
public:
	
	/// true if this action consist in a state change, false if it's a message
	bool IsStateChange;

	/// state name for state change
	std::string StateChange;

	/// event message
	CLogicEventMessage EventMessage;

	/**
	 * Default constructor
	 */
	CLogicEventAction()
	{
		IsStateChange = false;
	}

	/**
	 * This message will be sent as soon as the dest id will be given
	 */
	void enableSendMessage();

	/**
	 * serial
	 */
	//void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};



/**
 * CLogicEvent
 *
 * \date 2001
 */
class CLogicEvent
{
	/// state machine managing this event
	CLogicStateMachine * _LogicStateMachine;

public:
	
	/// condition name
	std::string ConditionName;
	
	/// event action
	CLogicEventAction EventAction;

	/**
	 * Default constructor
	 */
	CLogicEvent()
	{
		_LogicStateMachine = 0;
		ConditionName = "no_condition";
	}

	/**
	 *	Reset this event
	 */
	void reset();

	/**
	 *	Set the logic state machine
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 * Test the condition 
	 *
	 * \return true if condition is fulfiled
	 */
	bool testCondition();

	/**
	 * serial
	 */
	//void serial(class SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};

} // SIPLOGIC

#endif //LOGIC_EVENT



