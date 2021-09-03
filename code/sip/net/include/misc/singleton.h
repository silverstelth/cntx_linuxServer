/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include "misc/common.h"
#include "misc/debug.h"
#include "misc/thread.h"
#include "misc/app_context.h"


namespace SIPBASE 
{
		
	/**
	 * Example:
	 * \code
		struct CFooSingleton : public CSingleton<CFooSingleton>
		{
			void foo() { sipinfo("foo!"); }
		};	

		// call the foo function:
		CFooSingleton::getInstance().foo();

	 * \endcode
	 * \date 2004
	 */

	template<class T>
	class CSingleton
	{
	public:
		virtual ~CSingleton() {}

		/// returns a reference and not a pointer to be sure that the user
		/// doesn't have to test the return value and can directly access the class
		static T &getInstance()
		{
			if(!Instance)
			{
				Instance = new T;
				sipassert(Instance);
			}
			return *Instance;
		}

		static void releaseInstance()
		{
			if(Instance)
			{
				delete Instance;
				Instance = NULL;
			}
		}

	protected:
		/// no public ctor to be sure that the user can't create an instance
		CSingleton()
		{
		}

		static T *Instance;
	};

	template <class T>
	T* CSingleton<T>::Instance = 0;



	/** A variant of the singleton, not fully compliant with the standard design pattern
	 *	It is more appropriate for object built from a factory but that must
	 *	be instanciate only once.
	 *	The singleton paradigm allow easy access to the unique instance but
	 *	I removed the automatic instanciation of getInstance().
	 *
	 *	Consequently, the getInstance return a pointer that can be NULL
	 *	if the singleton has not been build yet.
	 *
	 * Example:
	 * \code
		struct CFooSingleton : public CManualSingleton<CFooSingleton>
		{
			void foo() { sipinfo("foo!"); }
		};	

		// create an instance by any mean
		CFooSingleton	mySingleton

		// call the foo function:
		CFooSingleton::getInstance()->foo();

  		// create another instance is forbiden
		CFooSingleton	otherInstance;		// ASSERT !


	 * \endcode
	 * \date 2005
	 */

	template <class T>
	class CManualSingleton
	{
		static T *&_instance()
		{
			static T *instance = NULL;

			return instance;
		}

	protected:


		CManualSingleton()
		{
			sipassert(_instance() == NULL);
			_instance() = static_cast<T*>(this);
		}

		~CManualSingleton()
		{
			sipassert(_instance() == this);
			_instance() = NULL;
		}

	public:

		static bool isInitialized()
		{
			return _instance() != NULL;
		}
		
		static T* getInstance()
		{
			sipassert(_instance() != NULL);

			return _instance();
		}
	};


} // SIPBASE

#endif // __SINGLETON_H__

/* End of singleton.h */
