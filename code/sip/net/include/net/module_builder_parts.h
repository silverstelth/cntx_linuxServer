/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MODULE_BUILDER_PARTS_H__
#define __MODULE_BUILDER_PARTS_H__

#include "misc/types_sip.h"
#include "misc/mutable_container.h"
#include "module.h"

namespace SIPNET
{

	template <class T>
	class CEmptyModuleServiceBehav : public T
	{
	public:
		virtual void				onServiceUp(const std::string &serviceName, SIPNET::TServiceId serviceId)
		{}
		virtual void				onServiceDown(const std::string &serviceName, SIPNET::TServiceId serviceId)
		{}
		virtual void				onModuleUpdate()
		{}
		virtual void				onApplicationExit()
		{}
	};

	template <class T>
	class CEmptyModuleCommBehav : public T
	{
	public:
		std::string					buildModuleManifest() const
		{
			return "";
		}
		virtual void				onModuleUp(IModuleProxy *moduleProxy)
		{}
		virtual void				onModuleDown(IModuleProxy *moduleProxy)
		{}
		virtual void				onModuleSecurityChange(IModuleProxy *moduleProxy)
		{}
		virtual bool				onProcessModuleMessage(IModuleProxy *senderModuleProxy, const CMessage &message)
		{	return false; }
	};

	template <class T>
	class CEmptySocketBehav : public T
	{
	public:
		virtual void	onModuleSocketEvent(IModuleSocket *moduleSocket, IModule::TModuleSocketEvent eventType)
		{
		}
	};


	/** Interceptor forwarder
	 *	The trick is that if you build a module interceptor class 
	 *	and then you want to inherit this class in a module definition, then
	 *	the virtual callbacks are received by the module instead of by your 
	 *	interceptor (because the base module is also an interceptor and
	 *	it eventualy overides the calls).
	 *	The workaround consist of having the interceptor implemented in
	 *	an inner class with method forwarded to you class with a different 
	 *	interface.
	 */
	template <class ParentClass>
	class CInterceptorForwarder : public IModuleInterceptable
	{
		ParentClass		*_Parent;
	public:
		CInterceptorForwarder()
			:	_Parent(NULL)
		{}

		void init(ParentClass *parent, IModule *module)
		{
			sipassert(parent != NULL);
			sipassert(module != NULL);

			_Parent = parent;
			registerInterceptor(module);
		}

		ParentClass *getParent()
		{
			return _Parent;
		}

		virtual std::string			buildModuleManifest() const
		{
			return _Parent->fwdBuildModuleManifest();
		}
		virtual void				onModuleUp(IModuleProxy *moduleProxy)
		{
			_Parent->fwdOnModuleUp(moduleProxy);
		}
		virtual void				onModuleDown(IModuleProxy *moduleProxy)
		{
			_Parent->fwdOnModuleDown(moduleProxy);
		}
		virtual bool				onProcessModuleMessage(IModuleProxy *senderModuleProxy, const CMessage &message)
		{
			return _Parent->fwdOnProcessModuleMessage(senderModuleProxy, message);
		}
		virtual void				onModuleSecurityChange(IModuleProxy *moduleProxy)
		{
			_Parent->fwdOnModuleSecurityChange(moduleProxy);
		}
	};

	/** Callback class used by the CModuleTracker class below and to be 
	 *	implemented if you want callback when tracked module are up/down.
	 */
	class IModuleTrackerCb
	{
	public:
		virtual void onTrackedModuleUp(IModuleProxy *moduleProxy) =0;
		virtual void onTrackedModuleDown(IModuleProxy *moduleProxy) =0;
		virtual ~IModuleTrackerCb();
	};

	/** A module interceptor that keep of a set of known module that match a given
	 *	predicate.
	 *	A typical usage is to declare an instance of this class (specialized with
	 *	the appropriate predicate) as member of your module.
	 *	You can also have more than one tracker with different predicate
	 *	if you are interested in different modules.
	 *	
	 *	NB : don't forget to init() each tracker in order to let it register
	 *	in you module interceptor list (typically, call "_MyTracker.init(this, this)" in
	 *	your module's constructor).
	 */
	template <class ModulePredicate>
	class CModuleTracker
	{
	public:
		typedef SIPBASE::TMutableContainer<std::set<TModuleProxyPtr> >	TTrackedModules;

	private:
		/// this is the list of module that match the predicate
		TTrackedModules		_TrackedModules;

		/// An instance of the predicate class
		ModulePredicate		_ModulePred;

		/// The callback interface implementation (if any)
		IModuleTrackerCb	*_TrackerCallback;

	public:
		CModuleTracker(const ModulePredicate &pred)
			:	_ModulePred(pred),
				_TrackerCallback(NULL)
		{
		}
		/** Init : set the owner module (to register the interceptor) and the 
		 *	optional callback interface.
		 */
		void init(SIPNET::IModule *module, IModuleTrackerCb *trackerCallback = NULL)
		{
			_Interceptor.init(this, module);
			_TrackerCallback = trackerCallback;
		}

		/** Return the set of tracked module.
		 *	The set is wrapped into a mutable container that allow
		 *	to call non const begin() and end() from a const container.
		 */	
		const TTrackedModules &getTrackedModules() const
		{
			return _TrackedModules;
		}

	private:

		// unused interceptors 
		std::string			fwdBuildModuleManifest() const	{ return std::string(); }
		void				fwdOnModuleSecurityChange(SIPNET::IModuleProxy *moduleProxy) {};
		bool				fwdOnProcessModuleMessage(SIPNET::IModuleProxy *sender, const SIPNET::CMessage &message)	{return false;}

		// check module up
		void				fwdOnModuleUp(SIPNET::IModuleProxy *moduleProxy)  
		{
			if (_ModulePred(moduleProxy))
			{
				if (_TrackedModules.insert(moduleProxy).second && _TrackerCallback != NULL)
					// warn the callback
					_TrackerCallback->onTrackedModuleUp(moduleProxy);
			}
		};

		// check module down
		void				fwdOnModuleDown(SIPNET::IModuleProxy *moduleProxy) 
		{
			if (_TrackedModules.find(moduleProxy) != _TrackedModules.end())
			{
				_TrackedModules.erase(moduleProxy);
				if (_TrackerCallback != NULL)
					_TrackerCallback->onTrackedModuleDown(moduleProxy);
			}
		};
		
		typedef SIPNET::CInterceptorForwarder < CModuleTracker>	TInterceptor;
		// declare one interceptor member of the skeleton
		TInterceptor	_Interceptor;
		// declare the interceptor forwarder as friend of this class
		friend 		class SIPNET::CInterceptorForwarder < CModuleTracker>;
	};

	/** A canonical module predicate that test a module
	 *	for a specified module class name.
	 */
	struct TModuleClassPred
	{
	private:
		std::string _ClassName;

	public:;
		TModuleClassPred(const std::string &className)
			:	_ClassName(className)
		{
		}

		bool operator () (IModuleProxy *moduleProxy) const
		{
			return moduleProxy->getModuleClassName() == _ClassName;
		}
	};

} // namespace SIPNET

#endif // __MODULE_BUILDER_PARTS_H__

