/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "types_sip.h"
#include "debug.h"
#include <map>

namespace SIPBASE
{

/** Interface class for object registered in the factory.
 *	This is for factory internal use, you should not need to use this class directly.
 */
template <class BaseClass>
class IFactoryRegister
{
public:
	/// This method is called to create an instance of the factored object.
	virtual BaseClass *createObject(const typename BaseClass::TCtorParam &ctorParam) = 0;
	virtual ~IFactoryRegister(){};
};


/** Factory implementation class.
 *	the class take 2 template argument :
 *		* BaseClass : the common base class for all factored object of one factory.
 *		* KeyType : the type of the key that identify the factorable object (string by default).
 *
 *	The factory conforms to singleton design pattern.
 *	
 *	BaseClass must provide a typedef for TCTorParam corresponding to the parameter
 *	required by the constructor of factored object.
 */


template <class BaseClass, class KeyType = std::string>
class CFactory
{
	typedef std::map<KeyType, IFactoryRegister<BaseClass>*> TRegisterCont;

public:
	static	void	free_CFactory(void * _cl) { CFactory *Ins = (CFactory *)_cl;	delete Ins; }

	/// Get the singleton instance reference.
	static CFactory &instance()
	{
		// Singleton instance pointer.
		static CFactory	*instance = NULL;
		if (!instance)
		{
			instance = new CFactory();
			AutoDestructorSingletone_setSingletonPointer(typeid(CFactory).name(), instance, free_CFactory);
		}
		return *instance;
	}

	/** Register a factorable object in the factory.
	 *	The method receive the key for this factorable object and
	 *	a pointer on the interface of a factory register object.
	 */
	void registerClass(const KeyType &key, IFactoryRegister<BaseClass> *factoryRegister)
	{
		sipassert(_FactoryRegisters.find(key) == _FactoryRegisters.end());
		_FactoryRegisters.insert(std::make_pair(key, factoryRegister));
	}

	/** Create a new instance of a factorable object.
	 *	\param key the identifier of the object to create.
	 *	\param ctorParam the parameter for the constructor.
	 *	\return	a pointer on the newly created object.
	 */
	BaseClass *createObject(const KeyType &key, const typename BaseClass::TCtorParam &ctorParam)
	{
		typename TRegisterCont::iterator it (_FactoryRegisters.find(key));
		if (it == _FactoryRegisters.end())
			return NULL;
		else
			return it->second->createObject(ctorParam);
	}

	// Add some introspection
	void fillFactoryList(std::vector<KeyType> &classList)
	{
		typename TRegisterCont::iterator first(_FactoryRegisters.begin()), last(_FactoryRegisters.end());
		for (; first != last; ++first)
		{
			classList.push_back(first->first);
		}
	}
protected:
	/// Singleton instance pointer.
//	static CFactory	*_Instance;

	/// Container of all registered factorable object.
	TRegisterCont	_FactoryRegisters;
};

/** This class is responsible for creating the factorable object and to register
 *	them in the factory instance.
 *	You must declare an instance of this class for each factorable object.
 **/
template <class FactoryClass, class BaseClass, class FactoredClass, class KeyType>
class CFactoryRegister : public IFactoryRegister<BaseClass>
{
public:
	/** Constructor.
	 *	Register the factorable object in the factory.
	 *	/param key The identifier for this factorable object.
	 */
	CFactoryRegister(const KeyType &key)
	{
		FactoryClass::instance().registerClass(key, this);
	}

	/** Create an instance of the factorable class.
	 *	Implements IFactoryRegister::createObject
	 */
	BaseClass *createObject(const typename BaseClass::TCtorParam &ctorParam)
	{
		return new FactoredClass(ctorParam);
	}
};

/** Macro to declare a factory.
 *	Place this macro in an appropriate cpp file to declare a factory implementation.
 *	You just need to specify the base class type and key type.
 */
//#define SIPBASE_IMPLEMENT_FACTORY(baseClass, keyType)	SIPBASE::CFactory<baseClass, keyType>	*SIPBASE::CFactory<baseClass, keyType>::_Instance = NULL

/** Macro to declare a factorable object.
 *	Place this macro in a cpp file after your factorable class declaration.
 */
#define SIPBASE_REGISTER_OBJECT(baseClass, factoredClass, keyType, keyValue)	SIPBASE::CFactoryRegister<SIPBASE::CFactory<baseClass, keyType>, baseClass, factoredClass, keyType>	Register##factoredClass(keyValue)

#define SIPBASE_GET_FACTORY(baseClass, keyType) SIPBASE::CFactory<baseClass, keyType>::instance()


/** Interface class for object registered in the indirect factory.
 *	This is for indirect factory internal use, you should not need to use this class directly.
 */
template <class BaseFactoryClass>
class IFactoryIndirectRegister
{
public:
	virtual ~IFactoryIndirectRegister() {}
	/** Return the factory implementation.*/
	virtual BaseFactoryClass *getFactory() = 0;
};



/** Indirect factory implementation class.
 *	the class take 2 template argument :
 *		* BaseFactoryClass : the common base class for all factory object of one indirect factory.
 *		* KeyType : the type of the key that identify the factorable object (string by default).
 *
 *	The indirect factory conforms to singleton design pattern.
 *	
 *	In indirect factory, the object returned by the factory are not instance of factored object
 *	but instance of 'sub' factory that do the real job.
 *	This can be useful in some case, like adapting existing code into a factory
 *	or having a complex constructor (or more than one constructor).
 */
template <class BaseFactoryClass, class KeyType = std::string>
class CFactoryIndirect
{
	typedef std::map<KeyType, IFactoryIndirectRegister<BaseFactoryClass>*> TRegisterCont;

public:
	static	void	free_CFactory(void * _cl) { CFactoryIndirect *Ins = (CFactoryIndirect *)_cl;	delete Ins; }
	/// Get the singleton instance reference.
	static CFactoryIndirect &instance()
	{
		static CFactoryIndirect	*instance = NULL;
		if (!instance)
		{
			instance = new CFactoryIndirect();
			AutoDestructorSingletone_setSingletonPointer(typeid(CFactoryIndirect).name(), instance, free_CFactory);
		}
		return *instance;
	}

	void registerClass(const KeyType &key, IFactoryIndirectRegister<BaseFactoryClass> *factoryRegister)
	{
		sipassert(_FactoryRegisters.find(key) == _FactoryRegisters.end());
		_FactoryRegisters.insert(std::make_pair(key, factoryRegister));
	}

	BaseFactoryClass *getFactory(const KeyType &key)
	{
		typename TRegisterCont::const_iterator it (_FactoryRegisters.find(key));
		if (it == _FactoryRegisters.end())
			return NULL;
		else
			return it->second->getFactory();
	}
	// Add some introspection
	void fillFactoryList(std::vector<KeyType> &classList)
	{
		typename TRegisterCont::iterator first(_FactoryRegisters.begin()), last(_FactoryRegisters.end());
		for (; first != last; ++first)
		{
			classList.push_back(first->first);
		}
	}
protected:
//	static CFactoryIndirect	*_Instance;

	TRegisterCont	_FactoryRegisters;
};


template <class IndirectFactoryClass, class BaseFactoryClass, class SpecializedFactoryClass, class KeyType>
class CFactoryIndirectRegister : public IFactoryIndirectRegister<BaseFactoryClass>
{
	SpecializedFactoryClass	_FactoryClass;
public:
	CFactoryIndirectRegister(const KeyType &key)
	{
		IndirectFactoryClass::instance().registerClass(key, this);
	}

	BaseFactoryClass *getFactory()
	{
		return &_FactoryClass;
	}
};

//#define SIPBASE_IMPLEMENT_FACTORY_INDIRECT(baseFactoryClass, keyType)	SIPBASE::CFactoryIndirect<baseFactoryClass, keyType>	*SIPBASE::CFactoryIndirect<baseFactoryClass, keyType>::_Instance = NULL

#define SIPBASE_GET_FACTORY_INDIRECT_REGISTRY(baseFactoryClass, keyType)	SIPBASE::CFactoryIndirect<baseFactoryClass, keyType>::getInstance()
#define SIPBASE_REGISTER_OBJECT_INDIRECT(baseFactoryClass, specializedFactoryClass, keyType, keyValue)	SIPBASE::CFactoryIndirectRegister<CFactoryIndirect<baseFactoryClass, keyType>, baseFactoryClass, specializedFactoryClass, keyType>	RegisterIndirect##specializedFactoryClass(keyValue)
#define SIPBASE_DECLARE_OBJECT_INDIRECT(baseFactoryClass, specializedFactoryClass, keyType)	extern SIPBASE::CFactoryIndirectRegister<CFactoryIndirect<baseFactoryClass, keyType>, baseFactoryClass, specializedFactoryClass, keyType>	RegisterIndirect##specializedFactoryClass

#define SIPBASE_GET_FACTORY_INDIRECT(specializedFactoryClass) RegisterIndirect##specializedFactoryClass.getFactory()

} // namespace SIPBASE

#endif // __FACTORY_H__
