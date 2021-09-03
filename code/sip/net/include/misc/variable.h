/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "types_sip.h"
#include "command.h"
#include "value_smoother.h"

namespace SIPBASE {

/** WARNING:
 *   This is POSIX C/C++ linker behavior: object files
 *   that are not referenced from outside are discarded. The
 *   file in which you run your constructor is thus simply
 *   thrown away by the linker, which explains why the constructor
 *   is not run.
 */


/**
 * Add a variable that can be modify in realtime. The variable must be global. If you must acces the variable with
 * function, use SIPBASE_DYNVARIABLE
 *
 * Example:
 * \code
	// I want to look and change the variable 'foobar' in realtime, so, first i create it:
	uint8 foobar;
	// and then, I add it
	SIPBASE_VARIABLE(uint8, FooBar, "this is a dummy variable");
 * \endcode
 *
 * Please use the same casing than for the variable (first letter of each word in upper case)
 * ie: MyVariable, NetSpeedLoop, Time
 *
 * \date 2001
 */
#define SIPBASE_VARIABLE(__type,__var,__help) SIPBASE_CATEGORISED_VARIABLE(variables,__type,__var,__help)
#define SIPBASE_CATEGORISED_VARIABLE(__category,__type,__var,__help) \
SIPBASE::CVariablePtr<__type> __var##Instance(#__category,#__var, __help " (" #__type ")", &__var)



/**
 * Add a variable that can be modify in realtime. The code profide the way to access to the variable in the read
 * and write access (depending of the \c get boolean value)
 *
 * Example:
 * \code
	// a function to read the variable
	uint8 getVar() { return ...; }

	// a function to write the variable
	void setVar(uint8 val) { ...=val; }

	// I want to look and change the variable in realtime:
	SIPBASE_DYNVARIABLE(uint8, FooBar, "this is a dummy variable")
	{
		// read or write the variable
		if (get)
			*pointer = getVar();
		else
			setVar(*pointer);
	}
 * \endcode
 *
 * Please use the same casing than for the variable (first letter of each word in upper case)
 * ie: MyVariable, NetSpeedLoop, Time
 *
 * \date 2001
 */
#define SIPBASE_DYNVARIABLE(__type,__name,__help) SIPBASE_CATEGORISED_DYNVARIABLE(variables,__type,__name,__help)
#define SIPBASE_CATEGORISED_DYNVARIABLE(__category,__type,__name,__help) \
class __name##Class : public SIPBASE::IVariable \
{ \
public: \
	__name##Class () : IVariable(#__category, #__name, __help) { } \
	 \
	virtual void fromString(const std::string &val, bool human=false) \
	{ \
		/*std::stringstream ss (val);*/ \
		__type p; \
		/*ss >> p;*/ \
		SIPBASE::fromString(val, p) ; \
		ptr (&p, false, human); \
	} \
	 \
	virtual std::string toString(bool human) const \
	{ \
		__type p; \
		ptr (&p, true, human); \
		/*std::stringstream ss;*/ \
		/*ss << p;*/ \
		/*return ss.str();*/ \
		return SIPBASE::toStringA(p); \
	} \
	\
	void ptr(__type *pointer, bool get, bool human) const; \
}; \
__name##Class __name##Instance; \
void __name##Class::ptr(__type *pointer, bool get, bool human) const

/** Helper to declare a variable as friend of a class.
 *	Useful when you want to declare variable that need to access private data to act on or display internal state of the class
 */

#define SIPBASE_DYNVARIABLE_FRIEND(__name) SIPBASE_CATEGORISED_DYNVARIABLE_FRIEND(variables, __name)
#define SIPBASE_CATEGORISED_DYNVARIABLE_FRIEND(__category, __name) friend class __name##Class

//
//
//
//

class IVariable : public ICommand
{
	friend class CCommandRegistry;
public:

	IVariable(const char *categoryName, const char *commandName, const char *commandHelp, const char *commandArgs = "[<value>]", bool useConfigFile = false, void (*cc)(IVariable &var)=NULL) :
		ICommand(categoryName,commandName, commandHelp, commandArgs), _UseConfigFile(useConfigFile), ChangeCallback(cc)
	{
		Type = Variable;
	}
		  
	virtual void fromString(const std::string &val, bool human=false) = 0;
	
	virtual std::string toString(bool human=false) const = 0;

	virtual bool execute(const std::string &rawCommandString, const std::vector<std::string> &args, SIPBASE::CLog &log, bool quiet, bool human)
	{
		if (args.size() > 1)
			return false;
		
		if (args.size() == 1)
		{
			// set the value
			fromString (args[0], human);
		}
		
		// display the value
		if (quiet)
		{
			log.displayNL(toString(human).c_str());
		}
		else
		{
			log.displayNL("Variable %s = %s", _CommandName.c_str(), toString(human).c_str());
		}
		return true;
	}
	
	static void init (CConfigFile &configFile);

private:

	bool _UseConfigFile;

protected:
	
	// TODO: replace by interface (see IVariableChangedCallback)
	void (*ChangeCallback)(IVariable &var);
	
};




template <class T>
class CVariablePtr : public IVariable
{
public:

	CVariablePtr (const char *categoryName, const char *commandName, const char *commandHelp, T *valueptr, bool useConfigFile = false, void (*cc)(IVariable &var)=NULL) :
		IVariable (categoryName, commandName, commandHelp, "[<value>]", useConfigFile, cc), _ValuePtr(valueptr)
	{
	}

	virtual void fromString (const std::string &val, bool human=false)
	{
		//std::stringstream ss (val);
		//ss >> *_ValuePtr;
		SIPBASE::fromString(val, *_ValuePtr);
		if (ChangeCallback) ChangeCallback (*this);
	}

	virtual std::string toString (bool human) const
	{
		//std::stringstream ss;
		//ss << *_ValuePtr;
		//return ss.str();
		return SIPBASE::toStringA(*_ValuePtr);
	}

private:

	T *_ValuePtr;
};


template <class T>
class CVariable : public IVariable
	{
public:

	CVariable (	const char *categoryName, 
				const char *commandName, 
				const char *commandHelp, 
				const T &defaultValue, 
				uint nbMeanValue = 0, 
				bool useConfigFile = false, 
				void (*cc)(IVariable &var)=NULL,
				bool executeCallbackForDefaultValue=false ) :
		IVariable (categoryName, commandName, commandHelp, "[<value>|stat|mean|min|max]", useConfigFile, cc), _Mean(nbMeanValue), _First(true)
	{
		set (defaultValue, executeCallbackForDefaultValue);
	}

	virtual void fromString (const std::string &val, bool human=false)
	{
		T v;
		SIPBASE::fromString(val, v);
//		std::stringstream ss (val);
//		ss >> v;
		set (v);
	}
	
	virtual std::string toString (bool human) const
	{
		return SIPBASE::toStringA(_Value);
//		std::stringstream ss;
//		ss << _Value;
//		return ss.str();
	}
	
	CVariable<T> &operator= (const T &val)
	{
		set (val);
		return *this;
	}

	operator T () const
	{
		return get ();
	}

	void set (const T &val, bool executeCallback = true)
	{
		_Value = val;
		_Mean.addValue (_Value);
		if (_First)
		{
			_First = false;
			_Min = _Value;
			_Max = _Value;
		}
		else
		{
			if (_Value > _Max) _Max = _Value;
			if (_Value < _Min) _Min = _Value;
		}
		if (ChangeCallback && executeCallback) ChangeCallback (*this);
	}
	
	const T &get () const
	{
		return _Value;
	}

	std::string getStat () const
	{
//		std::stringstream s;
//		s << _CommandName << "=" << _Value;
//		s << " Min=" << _Min;
//		s << " Max=" << _Max;
		std::string str;
		str = _CommandName + "=" + SIPBASE::toStringA(_Value) + " Min=" + SIPBASE::toStringA(_Min);
		if (_Mean.getNumFrame()>0)
		{
			const std::vector<T>& v = _Mean.getLastFrames();
			T theMin = *std::min_element(v.begin(), v.end());
			str += " RecentMin=" + SIPBASE::toStringA(theMin);
		}
		str += " Max=" + SIPBASE::toStringA(_Max);
		if (_Mean.getNumFrame()>0)
		{
			const std::vector<T>& v = _Mean.getLastFrames();
			T theMax = *std::max_element(v.begin(), v.end());
			str += " RecentMax=" + SIPBASE::toStringA(theMax);
		}
		if (_Mean.getNumFrame()>0)
		{
			str += " RecentMean=" + SIPBASE::toStringA(_Mean.getSmoothValue()) + " RecentValues=";
//			s << " Mean=" << _Mean.getSmoothValue();
//			s << " LastValues=";
			for (uint i = 0; i < _Mean.getNumFrame(); i++)
			{
				str += SIPBASE::toStringA(_Mean.getLastFrames()[i]);
				//s << _Mean.getLastFrames()[i];
				if (i < _Mean.getNumFrame()-1)
					str += ","; //s << ",";
			}
		}
		return str;
	}
	
	virtual bool execute (const std::string &rawCommandString, const std::vector<std::string> &args, SIPBASE::CLog &log, bool quiet, bool human)
	{
		if (args.size() > 1)
			return false;

		bool haveVal=false;
		std::string val;

		if (args.size() == 1)
		{
			if (args[0] == "stat")
			{
				// display the stat value
				log.displayNL(getStat().c_str());
				return true;
			}
			else if (args[0] == "mean")
			{
				haveVal = true;
				val = SIPBASE::toStringA(_Mean.getSmoothValue());
			}
			else if (args[0] == "min")
			{
				haveVal = true;
				val = SIPBASE::toStringA(_Min);
			}
			else if (args[0] == "max")
			{
				haveVal = true;
				val = SIPBASE::toStringA(_Max);
			}
			else
			{
				// set the value
				fromString (args[0], human);
			}
		}

		// display the value
		if (!haveVal)
		{
			val = toString(human);
		}

		if (quiet)
		{
			log.displayNL(val.c_str());
		}
		else
		{
			log.displayNL("Variable %s = %s", _CommandName.c_str(), val.c_str());
		}
		return true;
	}
	
private:

	T _Value;
	CValueSmootherTemplate<T> _Mean;
	T _Min, _Max;
	bool _First;
};

template<> class CVariable<std::string> : public IVariable
{
public:
	
	CVariable (const char *categoryName, const char *commandName, const char *commandHelp, const std::string &defaultValue, uint nbMeanValue = 0, bool useConfigFile = false, void (*cc)(IVariable &var)=NULL, bool executeCallbackForDefaultValue=false) :
		IVariable (categoryName, commandName, commandHelp, "[<value>]", useConfigFile, cc)
	{
		set (defaultValue, executeCallbackForDefaultValue);
	}
	  
	virtual void fromString (const std::string &val, bool human=false)
	{
		set (val);
	}

	virtual std::string toString (bool human=false) const
	{
		return _Value;
	}

	CVariable<std::string> &operator= (const std::string &val)
	{
		set (val);
		return *this;
	}

	operator std::string () const
	{
		return get();
	}

	operator const char * () const
	{
		return get().c_str();
	}

	const char *c_str () const
	{
		return get().c_str();
	}

	void set (const std::string &val, bool executeCallback = true)
	{
		_Value = val;
		static bool RecurseSet = false;
		if (ChangeCallback && !RecurseSet && executeCallback)
		{
			RecurseSet = true;
			ChangeCallback(*this);
			RecurseSet = false;
		}
	}

	const std::string &get () const
	{
		return _Value;
	}

	virtual bool execute (const std::string &rawCommandString, const std::vector<std::string> &args, SIPBASE::CLog &log, bool quiet, bool human)
	{
		if (args.size () > 1)
			return false;

		if (args.size () == 1)
		{
			// set the value
			fromString (args[0], human);
		}

		// convert the string from utf-8 to ascii (thrue unicode)
		ucstring temp;
		temp.fromUtf8(toString(human));
		std::string disp = temp.toString();
		// display the value
		if (quiet)
		{
			log.displayNL (disp.c_str());
		}
		else
		{
			log.displayNL ("Variable %s = %s", _CommandName.c_str(), disp.c_str());
		}
		return true;
	}

private:

	std::string _Value;
};


/// This class can provide a callback called when the value of a variable has been changed
class IVariableChangedCallback
{
public:
	virtual void onVariableChanged(SIPBASE::IVariable& var) = 0;
	virtual ~IVariableChangedCallback() {}
};


} // SIPBASE


#endif // __VARIABLE_H__

/* End of variable.h */
