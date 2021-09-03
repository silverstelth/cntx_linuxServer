/** \file logic_variable.h
 * 
 *
 * $Id: logic_variable.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef LOGIC_VARIABLE_H
#define LOGIC_VARIABLE_H

#include "misc/types_sip.h"
#include "misc/stream.h"
#include "misc/o_xml.h"
#include "misc/i_xml.h"

namespace SIPLOGIC
{

/**
 * CLogicVariable
 *
 * \date 2001
 */
class CLogicVariable
{
protected:

	/// variable value
	sint64 _Value;

	/// variable name
	std::string _Name;

	/// true if verbose mode is active
	bool _Verbose;

public:

	/**
	 * Default constructor
	 */
	CLogicVariable();

	/**
	 * Set the variable name
	 *
	 * \param name is the name of the variable
	 */
	void setName( std::string name ) { _Name = name; }

	/**
	 * Get the variable name
	 *
	 * \return the name of the variable
	 */
	std::string getName() const { return _Name; }

	/**
	 * Set the variable value
	 *
	 * \param value is the new value of the variable
	 */
	void setValue( sint64 value );

	/**
	 * Get the variable value
	 *
	 * \return the variable's value
	 */
	sint64 getValue() const { return _Value; }

	/**
	 *	Set the verbose mode active or inactive
	 *
	 * \param varName is the name of the variable
	 * \param b is true to activate the verbose mode, false else
	 */
	void setVerbose( bool b ) { _Verbose = b; }

	/** 
	 * Apply modifications on a variable
	 *
	 * \param op can be one of these operators :"SET"("set"),"ADD"("add"),"SUB"("sub"),"MUL"("mul"),"DIV"("div")
	 * \param value is the value to use along with the modificator
	 */
	void applyModification( std::string op, sint64 value );
	
	/** 
	 * update the variable
	 */
	virtual void processLogic();
	
	/**
	 * serial
	 */
	//virtual void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);

	virtual void write (xmlNodePtr node) const;
	virtual void read (xmlNodePtr node);
};




/**
 * CLogicCounter
 *
 * \date 2001
 */
class CLogicCounter : public CLogicVariable
{
	uint _TickCount;

public:

	/// counter running mode
	enum TLogicCounterRule 
	{ 
		STOP_AT_LIMIT = 0,
		LOOP,
		SHUTTLE,
		DOWN_UP, // bounce at low end, stop at high end
		UP_DOWN, // bounce at high end, stop at low end
	};

	/// counter running state
	enum TLogicCounterRunningMode
	{ 
		STOPPED = 0,
		RUN,
		REWIND,
		FAST_FORWARD, 
	};


	/// period between inc( measured in game ticks )
	CLogicVariable Period;

	/// time offset to apply with period
	CLogicVariable Phase;

	/// regular increment value( normally 1 or -1 )
	CLogicVariable Step;

	/// lower limit for counter
	CLogicVariable LowLimit;

	/// higher limit for counter
	CLogicVariable HighLimit;


	/// running mode
	CLogicVariable Mode;

	/// running state
	CLogicVariable Control;

	/**
	 * Default constructor
	 */
	CLogicCounter();

	/** 
	 * update the counter
	 */
	void update();

	/**
	 *	check the counter value according to the running mode
	 */
	void manageRunningMode();

	/**
	 * serial
	 */
	//void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);

	virtual void write (xmlNodePtr node) const;
	virtual void read (xmlNodePtr node);
};

} // SIPLOGIC

#endif //LOGIC_VARIABLE



