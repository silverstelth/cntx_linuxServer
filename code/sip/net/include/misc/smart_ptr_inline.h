/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SMART_PTR_INLINE_H__
#define __SMART_PTR_INLINE_H__

namespace	SIPBASE
{



// ***************************************************************************
#ifdef SIP_OS_WINDOWS
#define	SMART_INLINE __forceinline
#else
#define	SMART_INLINE inline 
#endif


// ***************************************************************************
inline CRefCount::~CRefCount()
{
	// This is the destruction of the objet.
#ifdef SIP_DEBUG
	sipassert(crefs==0);
#endif

	// If a CRefPtr still points on me...
	if(!pinforef->IsNullPtrInfo)
	{
		// inform them of my destruction.
		pinforef->Ptr= NULL;
	}
}


// ***************************************************************************
// ***************************************************************************
// CSmartPtr.
// ***************************************************************************
// ***************************************************************************


// ***************************************************************************
template<class T>
inline CSmartPtr<T>::~CSmartPtr(void) 
{ 
	SMART_TRACE("dtor()");

    if(Ptr)
	{
#ifdef SIP_DEBUG
		sipassert(Ptr->crefs>=0);
#endif
		if (--(Ptr->crefs) == 0)
			delete Ptr;
		Ptr=NULL;
	}
}
template<class T>    
SMART_INLINE CSmartPtr<T>& CSmartPtr<T>::operator=(T* p)
{
	SMART_TRACE("ope=(T*)Start");

	// Implicit manage auto-assignation.
    if(p)
		p->crefs++;
    if(Ptr)
	{
		if (--(Ptr->crefs) == 0)
			delete Ptr;
	}
	Ptr = p;

	SMART_TRACE("ope=(T*)End");

	return *this;
}
template<class T>    
SMART_INLINE CSmartPtr<T>& CSmartPtr<T>::operator=(const CSmartPtr &p)
{
	return operator=(p.Ptr);
}
template<class T>    
SMART_INLINE bool CSmartPtr<T>::operator<(const CSmartPtr &p) const
{
	return Ptr<p.Ptr;
}



// ***************************************************************************
// ***************************************************************************
// CRefPtr.
// ***************************************************************************
// ***************************************************************************


// ***************************************************************************
template<class T>    
SMART_INLINE void	CRefPtr<T>::unRef() const
{
	pinfo->RefCount--;
	if(pinfo->RefCount==0)
	{
		// In CRefPtr, Never delete the object.

		// We may be in the case that this==NullPtrInfo, and our NullPtrInfo has done a total round. Test it.
		if(pinfo->IsNullPtrInfo)
		{
			// This should not happens, but I'm not sure :) ...
			// Reset the NullPtrInfo to a middle round.
			pinfo->RefCount= 0x7FFFFFFF;
		}
		else
		{
			// If the CRefPtr still point to a valid object.
			if(pinfo->Ptr)
			{
				// Inform the Object that no more CRefPtr points on it.
				pinfo->Ptr->pinforef= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
			}
			// Then delete the pinfo.
			delete pinfo;
		}

	}
}


// ***************************************************************************
// Cons - dest.
template <class T> inline CRefPtr<T>::CRefPtr() 
{ 
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;

	REF_TRACE("Smart()");
}
template <class T> inline CRefPtr<T>::CRefPtr(T *v)
{
	Ptr= v;
    if(v)
	{
		// If no CRefPtr handles v, create a pinfo ref...
		if(v->pinforef->IsNullPtrInfo)
			v->pinforef=new CRefCount::CPtrInfo(v);
		pinfo=v->pinforef;
		// v is now used by this.
		pinfo->RefCount++;

#ifdef SIP_DEBUG
		sipassert(v == const_cast<T*>(static_cast<T const*>(pinfo->Ptr)));
#endif
	}
	else
		pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);

	REF_TRACE("Smart(T*)");
}
template <class T> inline CRefPtr<T>::CRefPtr(const CRefPtr &copy)
{
	pinfo=copy.pinfo;
	pinfo->RefCount++;
	Ptr= const_cast<T*>(static_cast<T const*>(pinfo->Ptr));

	REF_TRACE("SmartCopy()");
}
template <class T> inline CRefPtr<T>::~CRefPtr(void)
{
	REF_TRACE("~Smart()");

	unRef();
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;
}

// ***************************************************************************
// Operators=.
template <class T> CRefPtr<T> &CRefPtr<T>::operator=(T *v) 
{
	REF_TRACE("ope=(T*)Start");


	Ptr= v;
	if(v)
	{
		// If no CRefPtr handles v, create a pinfo ref...
		if(v->pinfo->IsNullPtrInfo)
			v->pinfo=new CRefCount::CPtrInfo(v);
		// The auto equality test is implicitly done by upcounting first "v", then downcounting "this".
		v->pinfo->RefCount++;
		unRef();
		pinfo= v->pinfo;

#ifdef SIP_DEBUG
		sipassert(v == const_cast<T*>(static_cast<T const*>(pinfo->Ptr)));
#endif
	}
	else
	{
		unRef();
		pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	}


	REF_TRACE("ope=(T*)End");

	return *this;
}
template <class T> CRefPtr<T> &CRefPtr<T>::operator=(const CRefPtr &copy) 
{
	REF_TRACE("ope=(Smart)Start");

	// The auto equality test is implicitly done by upcounting first "copy", then downcounting "this".
	copy.pinfo->RefCount++;
	unRef();
	pinfo=copy.pinfo;
	// Must Refresh the ptr.
	Ptr= const_cast<T*>(static_cast<T const*>(pinfo->Ptr));

	REF_TRACE("ope=(Smart)End");
	return *this;
}


// ***************************************************************************
// Operations.
template <class T> void	CRefPtr<T>::kill()
{
	REF_TRACE("SmartKill");

	T	*ptr= const_cast<T*>(static_cast<T const*>(pinfo->Ptr));

	// First, release the refptr.
	unRef();
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;

	// Then delete the pointer.
	if(ptr)
		delete ptr;
}


// ***************************************************************************
// Cast.
template <class T> inline CRefPtr<T>::operator T*()	const 
{
	REF_TRACE("SmartCast T*()");

	// Refresh Ptr.
	// NB: It is preferable (faster) here to just copy than testing if NULL and set NULL if necessary.
	// (static_cast is like a simple copy but for multiple inheritance) 
	Ptr= const_cast<T*>(static_cast<T const*>(pinfo->Ptr));
	return Ptr;
}


// ***************************************************************************
// Operators.
template <class T> inline T& CRefPtr<T>::operator*(void)  const
{ 
	REF_TRACE("Smart *()");
	return *Ptr; 
}
template <class T> inline T* CRefPtr<T>::operator->(void) const
{ 
	REF_TRACE("Smart ->()");
	return Ptr;  
}


// ***************************************************************************
// ***************************************************************************
// CVirtualRefPtr.
// ***************************************************************************
// ***************************************************************************


// ***************************************************************************
template<class T>    
SMART_INLINE void	CVirtualRefPtr<T>::unRef() const
{
	pinfo->RefCount--;
	if(pinfo->RefCount==0)
	{
		// In CVirtualRefPtr, Never delete the object.

		// We may be in the case that this==NullPtrInfo, and our NullPtrInfo has done a total round. Test it.
		if(pinfo->IsNullPtrInfo)
		{
			// This should not happens, but I'm not sure :) ...
			// Reset the NullPtrInfo to a middle round.
			pinfo->RefCount= 0x7FFFFFFF;
		}
		else
		{
			// If the CVirtualRefPtr still point to a valid object.
			if(pinfo->Ptr)
			{
				// Inform the Object that no more CVirtualRefPtr points on it.
				// pinfo->Ptr->pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
			}
			// Then delete the pinfo.
			delete pinfo;
		}

	}
}


// ***************************************************************************
// Cons - dest.
template <class T> inline CVirtualRefPtr<T>::CVirtualRefPtr() 
{ 
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;

	REF_TRACE("Smart()");
}
template <class T> inline CVirtualRefPtr<T>::CVirtualRefPtr(T *v)
{
	Ptr= v;
    if(v)
	{
		// If no CVirtualRefPtr handles v, create a pinfo ref...
		if(v->pinfo->IsNullPtrInfo)
			v->pinfo=new CRefCount::CPtrInfo(static_cast<CVirtualRefCount const*>(v)); // v MUST derive from CVirtualRefCount
		pinfo=v->pinfo;
		// v is now used by this.
		pinfo->RefCount++;

#ifdef SIP_DEBUG
		sipassert(v == const_cast<T*>(dynamic_cast<T const*>(static_cast<CVirtualRefCount const*>(pinfo->Ptr))));
#endif
	}
	else
		pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);

	REF_TRACE("Smart(T*)");
}
template <class T> inline CVirtualRefPtr<T>::CVirtualRefPtr(const CVirtualRefPtr &copy)
{
	pinfo=copy.pinfo;
	pinfo->RefCount++;
	Ptr= const_cast<T*>(dynamic_cast<T const*>(static_cast<CVirtualRefCount const*>(pinfo->Ptr)));
	sipassert(Ptr != NULL || pinfo->Ptr == NULL);

	REF_TRACE("SmartCopy()");
}
template <class T> inline CVirtualRefPtr<T>::~CVirtualRefPtr(void)
{
	REF_TRACE("~Smart()");

	unRef();
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;
}

// ***************************************************************************
// Operators=.
template <class T> CVirtualRefPtr<T> &CVirtualRefPtr<T>::operator=(T *v) 
{
	REF_TRACE("ope=(T*)Start");


	Ptr= v;
	if(v)
	{
		// If no CVirtualRefPtr handles v, create a pinfo ref...
		if(v->pinfo->IsNullPtrInfo)
			v->pinfo=new CRefCount::CPtrInfo(static_cast<CVirtualRefCount const*>(v)); // v MUST derive from CVirtualRefCount
		// The auto equality test is implicitly done by upcounting first "v", then downcounting "this".
		v->pinfo->RefCount++;
		unRef();
		pinfo= v->pinfo;

#ifdef SIP_DEBUG
		sipassert(v == const_cast<T*>(dynamic_cast<T const*>(static_cast<CVirtualRefCount const*>(pinfo->Ptr))));
#endif
	}
	else
	{
		unRef();
		pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	}


	REF_TRACE("ope=(T*)End");

	return *this;
}
template <class T> CVirtualRefPtr<T> &CVirtualRefPtr<T>::operator=(const CVirtualRefPtr &copy) 
{
	REF_TRACE("ope=(Smart)Start");

	// The auto equality test is implicitly done by upcounting first "copy", then downcounting "this".
	copy.pinfo->RefCount++;
	unRef();
	pinfo=copy.pinfo;
	// Must Refresh the ptr.
	Ptr= const_cast<T*>(dynamic_cast<T const*>(static_cast<CVirtualRefCount const*>(pinfo->Ptr)));
	sipassert(Ptr != NULL || pinfo->Ptr == NULL);

	REF_TRACE("ope=(Smart)End");
	return *this;
}


// ***************************************************************************
// Operations.
template <class T> void	CVirtualRefPtr<T>::kill()
{
	REF_TRACE("SmartKill");

	T	*ptr= const_cast<T*>(dynamic_cast<T const*>(static_cast<CVirtualRefCount const*>(pinfo->Ptr)));
	sipassert(ptr != NULL || pinfo->Ptr == NULL);

	// First, release the refptr.
	unRef();
	pinfo= static_cast<CRefCount::CPtrInfo*>(&CRefCount::NullPtrInfo);
	Ptr= NULL;

	// Then delete the pointer.
	if(ptr)
		delete ptr;
}


// ***************************************************************************
// Cast.
template <class T> inline CVirtualRefPtr<T>::operator T*()	const 
{
	REF_TRACE("SmartCast T*()");

	// Refresh Ptr if necessary.
	// NB: It is preferable (faster) here to test if NULL and set NULL if necessary, than dynamic_casting the ptr.
	if (pinfo->Ptr == NULL)
		Ptr = NULL;
	return Ptr;
}


// ***************************************************************************
// Operators.
template <class T> inline T& CVirtualRefPtr<T>::operator*(void)  const
{ 
	REF_TRACE("Smart *()");
	return *Ptr; 
}
template <class T> inline T* CVirtualRefPtr<T>::operator->(void) const
{ 
	REF_TRACE("Smart ->()");
	return Ptr;  
}

// ***************************************************************************
#undef	SMART_INLINE



} // SIPBASE


#endif // __SMART_PTR_INLINE_H__

