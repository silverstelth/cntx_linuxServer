/** \file logic_state.h
 * 
 *
 * $Id: logic_state.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef LOGIC_STATE_H
#define LOGIC_STATE_H

#include "misc/types_sip.h"
#include "misc/stream.h"
#include "misc/entity_id.h"
#include "misc/o_xml.h"
#include "misc/i_xml.h"

#include "net/service.h"

//#include "game_share/sid.h"

#include "logic_event.h"

namespace SIPLOGIC
{

class CLogicStateMachine;

/// map destination names to destination sid
typedef std::map<std::string, SIPBASE::CEntityId> TSIdMap;

/**
 * CLogicState
 *
 * \date 2001
 */
class CLogicState
{
public:

	/// state name
	std::string _StateName;

	/// entry messages
	std::vector<CLogicEventMessage> _EntryMessages;

	/// exit messages
	std::vector<CLogicEventMessage> _ExitMessages;

	/// logic
	std::vector<CLogicEvent> _Events;

	/// state machine containing this state
	CLogicStateMachine * _LogicStateMachine;

	/// messages to send by the service
	std::multimap<SIPBASE::CEntityId, SIPNET::CMessage> _MessagesToSend;
	
public:

	/**
	 *	Default constructor
	 */
	CLogicState();
	
	/**
	 * Set the state machine which contains this state
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 * set the state name
	 *
	 * \param name is the new state's name
	 */
	void setName( std::string name ) { _StateName = name; }

	/**
	 * get the state name
	 *
	 * \return the state's name
	 */
	std::string getName() { return _StateName; }

	/**
	 * Add an event
	 *
	 * \param event is the event to add
	 */
	void addEvent( CLogicEvent event );

	/**
	 * Associate message destination name with sid
	 *
	 * \param sIdMap is the map associating destination name with a destination id
	 */
	void addSIdMap( const TSIdMap& sIdMap );

	/**
	 * Test the conditions of this state
	 */
	void processLogic();

	/**
	 *	Get the messages to send
	 *
	  * \param msgs is the map associating all the message to send with their destination id
	 */
	void getMessagesToSend( std::multimap<SIPBASE::CEntityId, SIPNET::CMessage>& msgs );

	/**
	 * send the entry messages
	 */
	void enterState();

	/**
	 * send the exit messages
	 */
	void exitState();

	/**
	 *	Try to send the entry messages
	 */
	void trySendEntryMessages();

	/**
	 *	Try to send the event messages
	 */
	void trySendEventMessages();

	/**
	 * Fill a map associating all the referenced var in the state with the id of service managing them
	 * (debug purpose)
	 */
	void fillVarMap( std::multimap<SIPBASE::CEntityId,std::string >& stateMachineVariables );

	/**
	 * serial
	 */
	//void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};


} // SIPLOGIC

#endif //LOGIC_STATE



