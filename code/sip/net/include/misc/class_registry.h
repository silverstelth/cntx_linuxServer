/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CLASS_REGISTRY_H__
#define __CLASS_REGISTRY_H__

#include	"types_sip.h"
#include	"common.h"
#include	<typeinfo>
#include	<string>
#include	<set>
#ifdef SIP_OS_UNIX
#	include <unordered_map>
#else
#	include <hash_map>
#endif


namespace	SIPBASE
{


// ======================================================================================================
/**
 * Class Registry Exception.
 * \date 2000
 */
struct ERegistry : public Exception
{
	ERegistry() : Exception( "Registry error" ) {};

	ERegistry( const std::string& str ) : Exception( str ) {};
};

struct ERegisteredClass : public ERegistry
{
	ERegisteredClass() : ERegistry( "Class already registered" ) {};
};

struct EUnregisteredClass : public ERegistry
{
	EUnregisteredClass() : ERegistry( "Class not registered" ) {}
	EUnregisteredClass(const std::string &className) : ERegistry( std::string("Class not registered : ") + className ) {}
};


// ======================================================================================================
/**
 * An Object Streamable interface.
 * \date 2000
 */
class IClassable
{
public:
	virtual std::string		getClassName() =0;
	virtual ~IClassable() {}
};


// ======================================================================================================
/**
 * The Class registry where we can instanciate IClassable objects from their names.
 * \date 2000
 */
class CClassRegistry
{
public:
	/// Inits the ClassRegistry (especially RegistredClasses)
	static void			init();

	/// release memory
	static void			release();
	
	///	Register your class for future Instanciation.
	static	void		registerClass(const std::string &className, IClassable* (*creator)(), const std::string &typeidCheck) throw(ERegistry);

	/// Create an object from his class name.
	static	IClassable	*create(const std::string &className) throw(ERegistry);

	/// check if the object has been correctly registered. Must be used for debug only, and Must compile with RTTI.
	static	bool		checkObject(IClassable* obj);


private:
	struct	CClassNode
	{		
		std::string			TypeIdCheck;
		IClassable*	(*Creator)();		
	};
	typedef HashMap<std::string, CClassNode> TClassMap;
	static	TClassMap	*RegistredClasses;
};


/// Usefull Macros.
#define	SIPBASE_DECLARE_CLASS(_class_)					\
	virtual std::string	getClassName() {return #_class_;}		\
	static	SIPBASE::IClassable	*creator() {return new _class_;}
#define	SIPBASE_REGISTER_CLASS(_class_) SIPBASE::CClassRegistry::registerClass(#_class_, _class_::creator, typeid(_class_).name());



}	// namespace SIPBASE.


#endif // SIP_STREAM_H

/* End of stream.h */
