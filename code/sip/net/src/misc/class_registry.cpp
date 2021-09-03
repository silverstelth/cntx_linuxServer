/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/class_registry.h"
#include <typeinfo>

using namespace std;

namespace SIPBASE
{


// ======================================================================================================
CClassRegistry::TClassMap		*CClassRegistry::RegistredClasses = NULL;


// ======================================================================================================
void		CClassRegistry::init()
{
	if (RegistredClasses == NULL)
		RegistredClasses = new TClassMap;
}

// ======================================================================================================
void		CClassRegistry::release()
{
	if( RegistredClasses )
		delete RegistredClasses;
	RegistredClasses = NULL;
}

// ======================================================================================================
IClassable	*CClassRegistry::create(const string &className)  throw(ERegistry)
{
	init();

	TClassMap::iterator	it;	
	
	it=RegistredClasses->find(className);

	if(it==RegistredClasses->end())
		return NULL;
	else
	{
		IClassable	*ptr;
		ptr=it->second.Creator();
		#ifdef SIP_DEBUG
			sipassert(CClassRegistry::checkObject(ptr));
		#endif
		return ptr;
	}

}

// ======================================================================================================
void		CClassRegistry::registerClass(const string &className, IClassable* (*creator)(), const string &typeidCheck)  throw(ERegistry)
{
	init();

	CClassNode	node;	
	node.Creator=creator;
	node.TypeIdCheck= typeidCheck;
	std::pair<TClassMap::iterator, bool> result;
	result = RegistredClasses->insert(TClassMap::value_type(className, node));
	if(!result.second)
	{
		sipstop;
		throw ERegisteredClass();
	}	
}

// ======================================================================================================
bool		CClassRegistry::checkObject(IClassable* obj)
{
	init();

	TClassMap::iterator	it;	
	it=RegistredClasses->find(obj->getClassName());
	if(it==RegistredClasses->end())
		return false;	

	if( it->second.TypeIdCheck != string(typeid(*obj).name()) )
		return false;

	return true;
}



}






















