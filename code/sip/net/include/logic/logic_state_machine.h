/** \file logic_state_machine.h
 * 
 *
 * $Id: logic_state_machine.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef LOGIC_STATE_MACHINE_H
#define LOGIC_STATE_MACHINE_H

#include "logic_state.h"
#include "logic_variable.h"
#include "logic_condition.h"

#include "misc/o_xml.h"
#include "misc/i_xml.h"

#include "net/service.h"

#include <vector>
#include <map>
#include <string>


namespace SIPLOGIC
{

void xmlCheckNodeName (xmlNodePtr &node, const char *nodeName);
std::string getXMLProp (xmlNodePtr node, const char *propName);

/**
 * CLogicStateMachine
 *
 * \date 2001
 */
class CLogicStateMachine
{
	/// variables
	std::map<std::string, CLogicVariable> _Variables;

	/// counters
	std::map<std::string, CLogicCounter> _Counters;

	/// conditions used in this state machine
	std::map<std::string, CLogicCondition> _Conditions;

	/// states
	std::map<std::string, CLogicState> _States;

	/// name of the current state
	std::string _CurrentState;

	/// name of this sate machine
	std::string _Name;
	
public:
	
	const std::map<std::string, CLogicVariable> &getVariables () { return _Variables; }
	const std::map<std::string, CLogicCounter> &getCounters () { return _Counters; }
	const std::map<std::string, CLogicCondition> &getConditions () { return _Conditions; }
	const std::map<std::string, CLogicState> &getStates () { return _States; }


	/**
	 *	Default constructor
	 */
	CLogicStateMachine() { _Name = "no_name"; }

	/**
	 * Set the state machine name
	 *
	 * \param name is the name of state machine
	 */
	void setName( const std::string& name ) { _Name = name; }

	/**
	 * Get the state machine name
	 *
	 * \return the name of this state machine
	 */
	std::string getName() const { return _Name; }

	/**
	 *	Set the current state
	 *
	 * \param stateName is the name of the state to give focus to
	 */
	void setCurrentState( std::string stateName );

	/**
	 * call the addSIdMap() method for each sate machines
	 *
	 * \param sIdMap is the map associating destination name with a destination id
	 */
	void addSIdMap( const TSIdMap& sIdMap );

	/**
	 * call the processLogic method for each sate machines
	 */
	void processLogic();

	/**
	 * Get the self-addressed message
	 *
	 * \param msgs is the list used to store the self-addressed messages 
	 */
	void getSelfAddressedMessages( std::list<SIPNET::CMessage>& msgs );

	/**
	 * Add a variable in the state machine
	 *
	 * \param var is the new variable to add in this state machine
	 */
	void addVariable( CLogicVariable var ) { _Variables.insert( std::make_pair(var.getName(),var) ); }

	/**
	 *	Get the variable
	 *
	 * \param varName is the name of the variable to get
	 * \param var is the variable to get
	 * \return true if the variable has been found, false if not
	 */
	bool getVariable( std::string& varName, CLogicVariable& var );

	/**
	 * Add a counter in the state machine
	 *
	 * \param counter is the new counter to add in this state machine
	 */
	void addCounter( CLogicCounter counter ) { _Counters.insert( std::make_pair(counter.getName(),counter) ); }

	/**
	 * Add a condition in the state machine
	 *
	 * \param condition is the new condition to add in this state machine
	 */
	void addCondition( CLogicCondition condition );

	/**
	 *	Get the condition
	 *
	 * \param condName is the name of the condition to get
	 * \param cond is the condition to get
	 * \return true if the condition has been found, false if not
	 */
	bool getCondition( const std::string& condName, CLogicCondition& cond );

	/**
	 *	Get the messages to send
	 *
	 * \param msgs is the list used to store the messages to send
	 */
	void getMessagesToSend( std::multimap<SIPBASE::CEntityId, SIPNET::CMessage>& msgs );

	/** 
	 * Add a state to the state machine
	 *
	 * \param state is the new state to add in this state machine
	 */
	void addState( CLogicState logicState );

	/**
	 *	modify a variable
	 *
	 * \param varName is the name of the variable to modify
	 * \param modifOperator can be one of these operators :"SET"("set"),"ADD"("add"),"SUB"("sub"),"MUL"("mul"),"DIV"("div")
	 * \param value is the value to use along with the modificator
	 */
	void modifyVariable( std::string varName, std::string modifOperator, sint64 value ); 

	/**
	 * serial
	 */
	//void serial( SIPBASE::IStream &f ) throw(SIPBASE::EStream);

	/**
	 *	Display the variables
	 */
	void displayVariables();

	/**
	 *	Display the states
	 */
	void displayStates();

	/**
	 *	Set the verbose mode for a variable
	 *
	 * \param varName is the name of the variable
	 * \param b is true to activate the verbose mode, false else
	 */
	void setVerbose( std::string varName, bool b );

	void write (xmlDocPtr doc) const;
	void read (xmlNodePtr node);
};

} // SIPLOGIC

#endif //LOGIC_SYSTEM



