/** \file logic_condition.h
 * 
 *
 * $Id: logic_condition.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef LOGIC_CONDITION_H
#define LOGIC_CONDITION_H

#include "misc/types_sip.h"
#include "misc/stream.h"
#include "misc/o_xml.h"
#include "misc/i_xml.h"

namespace SIPLOGIC
{

class CLogicStateMachine;
class CLogicConditionNode;

/**
 * CLogicComparisonBlock
 *
 * \date 2001
 */
class CLogicComparisonBlock
{
	/// state machine managing this comparison block
	CLogicStateMachine * _LogicStateMachine;

public:

	/// variable name
	std::string VariableName;

	/// comparison operator
	std::string Operator;
	
	/// comparand
	sint64 Comparand;

	/**
	 * Default constructor
	 */
	CLogicComparisonBlock()
	{
		_LogicStateMachine = 0;
		VariableName = "no_name";
		Operator = ">";
		Comparand = 0;
	}

	/**
	 *	Set the logic state machine
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 *	Test the condition
	 *
	 * \return true if this condition is fulfiled, false else
	 */
	bool testLogic();

	/**
	 * serial
	 */
	//void serial(class SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};





/**
 * CLogicConditionLogicBlock
 *
 * \date 2001
 */
class CLogicConditionLogicBlock
{
	/// state machine managing this logic block
	CLogicStateMachine * _LogicStateMachine;

public:

	/// all condition logic block types
	enum TLogicConditionLogicBlockType
	{
		NOT = 0,
		COMPARISON,
		SUB_CONDITION,
	};
	
	/// type of this condition block
	TLogicConditionLogicBlockType Type;

	/// name of the sub-condition
	std::string SubCondition;
	
	/// comparison block
	CLogicComparisonBlock ComparisonBlock;

	/**
	 * Default constructor
	 */
	CLogicConditionLogicBlock()
	{
		Type = SUB_CONDITION;
		SubCondition = "no_condition";
	}

	/**
	 *	Return info about the type of the block
	 *
	 * \return true if this block is a NOT block
	 */
	bool isNotBlock() const { return (Type == NOT); }
	
	/**
	 *	Set the logic state machine
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 *	Test the condition
	 *
	 * \return true if this condition is fulfiled, false else
	 */
	bool testLogic();

	/**
	 *	Fill a set with all the variables name referenced by this condition
	 *
	 * \param condVars a set to store the variable names
	 */
	void fillVarSet( std::set<std::string>& condVars );

	/**
	 * serial
	 */
	//void serial(class SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};



/**
 * CLogicConditionNode
 *
 * \date 2001
 */
class CLogicConditionNode
{
	/// state machine managing this logic block
	CLogicStateMachine * _LogicStateMachine;

public:
	
	/// all condition node types
	enum TConditionNodeType
	{
		TERMINATOR = 0, 
		LOGIC_NODE
	};

	/// type of this condition node
	TConditionNodeType Type;
	
	// if this node is a logical node :
	/// condition logic node
	CLogicConditionLogicBlock LogicBlock;

	/// condition nodes
	std::vector<CLogicConditionNode *> _Nodes;
	
	/**
	 * Default constructor
	 */
	CLogicConditionNode()
	{
		_LogicStateMachine = 0;
		Type = TERMINATOR;
	}

	/**
	 *	Set the logic state machine
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 * add a node in the subtree
	 *
	 * \param node is the new node to add
	 */
	void addNode( CLogicConditionNode * node );

	/**
	 *	Test the condition
	 *
	 * \return true if this condition is fulfiled, false else
	 */
	bool testLogic();

	/**
	 *	Fill a set with all the variables name referenced by this condition
	 *
	 * \param condVars is a set to store the variable names
	 */
	void fillVarSet( std::set<std::string>& condVars );

	/**
	 * serial
	 */
	//void serial(class SIPBASE::IStream &f) throw(SIPBASE::EStream);

	/**
	 * Destructor
	 */
	~CLogicConditionNode();

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};




/**
 * CLogicCondition
 *
 * \date 2001
 */
class CLogicCondition
{
	/// condition name
	std::string _ConditionName;
		
public:
	
	/// condition tree
	std::vector<CLogicConditionNode> Nodes;
	
	/**
	 *	CLogicCondition
	 */
	CLogicCondition()
	{
		_ConditionName = "no_condition";
	}

	/**
	 *	Set the logic state machine
	 *
	 * \param logicStateMachine is the state machine containing this block
	 */
	void setLogicStateMachine( CLogicStateMachine * logicStateMachine );

	/**
	 *	Set the condition's name
	 *
	 * \param name is the condition's name
	 */
	void setName( std::string name ) { _ConditionName = name; }

	/**
	 *	Get the condition's name
	 *
	 * \return the condition's name
	 */
	std::string getName() const {	return _ConditionName; }

	/**
	 *	Add a condition node
	 *
	 * \param node is the new node to add
	 */
	void addNode( CLogicConditionNode node ) { Nodes.push_back( node ); }

	/**
	 *	Test the condition
	 *
	 * \return true if this condition is fulfiled, false else
	 */
	bool testLogic();

	/**
	 *	Fill a set with all the variables name referenced by this condition
	 *
	 * \param condVars is a set to store the variable names
	 */
	void fillVarSet( std::set<std::string>& condVars );

	/**
	 * serial
	 */
	//void serial(class SIPBASE::IStream &f) throw(SIPBASE::EStream);

	void write (xmlNodePtr node) const;
	void read (xmlNodePtr node);
};

} // SIPLOGIC

#endif //LOGIC_CONDITION



