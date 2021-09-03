/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MUTABLE_CONTAINER_H__
#define __MUTABLE_CONTAINER_H__

namespace SIPBASE
{

	/** Container wrapper that allow read/write access to element stored in 
	 *	a const container.
	 *	In fact, the template only allow calling begin() and end() over
	 *	a const container.
	 *	This prevent the user to change the structure of the container.
	 *	Usage :
	 *		
	 *		class foo
	 *		{
	 *			typedef TMutableContainer<vector<string> > TMyCont;
	 *			TMyCont	_MyCont;
	 *	
	 *		public:
	 *			// return the container with mutable item content but const item list
	 *			const TMyCont getContainer() const { return _MyCont; };
	 *		}
	 *			
	 */
	template <class BaseContainer>
	struct TMutableContainer : public BaseContainer
	{
		typename BaseContainer::iterator begin() const
		{
			return const_cast<BaseContainer*>(static_cast<const BaseContainer*>(this))->begin();
		}

		typename BaseContainer::iterator end() const
		{
			return const_cast<BaseContainer*>(static_cast<const BaseContainer*>(this))->end();
		}
	};

} // namespace SIPBASE

#endif // __MUTABLE_CONTAINER_H__
