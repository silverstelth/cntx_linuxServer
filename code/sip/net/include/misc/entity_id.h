/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __ENTITY_ID_H__
#define __ENTITY_ID_H__

#include "types_sip.h"
#include "debug.h"
#include "common.h"
#include "stream.h"

namespace SIPBASE {

/**
 * Entity identifier
 * \date 2001
 */
struct CEntityId
{
	// pseudo constante
	enum
	{
		DYNAMIC_ID_SIZE = 8,
		CREATOR_ID_SIZE = 8,
		TYPE_SIZE = 8,
		ID_SIZE = 40,
		UNKNOWN_TYPE = (1 << TYPE_SIZE)-1
	};

protected:

	// ---------------------------------------------------------------------------------
	// instantiated data

	union 
	{
		struct
		{
		/// Id of the service where the entity is (variable routing info).
		uint64	DynamicId   :  DYNAMIC_ID_SIZE;
		/// Id of the service who created the entity (persistent).
		uint64	CreatorId   :  CREATOR_ID_SIZE;
		/// Type of the entity (persistent).
		uint64	Type :			TYPE_SIZE;
		/// Local entity number (persistent).
		uint64	Id :			ID_SIZE;
		} DetailedId;

		uint64 FullId;
	};

	// ---------------------------------------------------------------------------------
	// static data

	/// Counter for generation of unique entity ids
	static SIPBASE::CEntityId	_NextEntityId;

	///The local num service id of the local machin.
	static uint8				_ServerId;

public:

	// ---------------------------------------------------------------------------------
	// static data

	///The maximume of number that we could generate without generate an overtaking exception.
	static const uint64			MaxEntityId;

	/// Unknow CEntityId is similar as an NULL pointer.
	static const CEntityId		Unknown;


	// ---------------------------------------------------------------------------------
	// generation of new unique entity ids

	/// Set the service id for the generator
	static void					setServiceId( uint8 sid )
	{
		_NextEntityId.setDynamicId( sid );
		_NextEntityId.setCreatorId( sid ); 
		_ServerId = sid;
	}

	/// Generator of entity ids
	static CEntityId			getNewEntityId( uint8 type )
	{
		sipassert(_NextEntityId != Unknown ); // type may be Unknown, so isUnknownId() would return true
		SIPBASE::CEntityId id = _NextEntityId++;
		id.setType( type );
		return id;
	}

	// ---------------------------------------------------------------------------------
	// constructors

	///\name Constructor
	//@{

	CEntityId ()
	{
		FullId = 0;
		DetailedId.Type = UNKNOWN_TYPE;

		/*
		DynamicId = 0;
		CreatorId = 0;
		Type = 127;
		Id = 0;
		*/
	}

	CEntityId (uint8 type, uint64 id, uint8 creator, uint8 dynamic)
	{
		DetailedId.DynamicId = dynamic;
		DetailedId.CreatorId = creator;
		DetailedId.Type = type;
		DetailedId.Id = id;
	}

	CEntityId (uint8 type, uint64 id)
	{
		DetailedId.Type = type;
		DetailedId.Id = id;
		DetailedId.CreatorId = _ServerId;
		DetailedId.DynamicId = _ServerId;
	}

	explicit CEntityId (uint64 p)
	{	
		FullId = p;
		/*
		DynamicId = (p & 0xff);
		p >>= 8;
		CreatorId = (p & 0xff);
		p >>= 8;
		Type = (p & 0xff);
		p >>= 8;
		Id = (p);			
		*/
	}

	CEntityId (const CEntityId &a)
	{
		FullId = a.FullId;
		/*
		DynamicId = a.DynamicId;
		CreatorId = a.CreatorId;			
		Type = a.Type;
		Id = a.Id;
		*/
	}

	///fill from read stream.
	CEntityId (SIPBASE::IStream &is)
	{
		is.serial(FullId);
		/*
		uint64 p;
		is.serial(p);

		DynamicId = (p & 0xff);
		p >>= 8;
		CreatorId = (p & 0xff);
		p >>= 8;
		Type = (p & 0xff);
		p >>= 8;
		Id = p;
		*/
	}

	explicit CEntityId (const std::string &str)
	{
		fromString(str.c_str());
	}
		
	explicit CEntityId (const char *str)
	{
		CEntityId ();
		fromString(str);
	}
	//@}	
	

	// ---------------------------------------------------------------------------------
	// accessors

	/// Get the full id
	uint64 getRawId() const
	{
		return FullId;
		/*
		return (uint64)*this;
		*/
	}

	/// Get the local entity number
	uint64 getShortId() const
	{
		return DetailedId.Id;
	}

	/// Set the local entity number
	void setShortId( uint64 shortId )
	{
		DetailedId.Id = shortId;
	}

	/// Get the variable routing info
	uint8 getDynamicId() const
	{
		return DetailedId.DynamicId;
	}

	/// Set the variable routing info
	void setDynamicId( uint8 dynId )
	{
		DetailedId.DynamicId = dynId;
	}

	/// Get the persistent creator id
	uint8 getCreatorId() const
	{
		return DetailedId.CreatorId;
	}

	/// Set the persistent creator id
	void setCreatorId( uint8 creatorId )
	{
		DetailedId.CreatorId = creatorId;
	}

	/// Get the entity type
	uint8 getType() const
	{
		return (uint8)DetailedId.Type;
	}

	/// Set the entity type
	void setType( uint8 type )
	{
		DetailedId.Type = type;
	}

	/// Get the persistent part of the entity id (the dynamic part in the returned id is 0)
	uint64 getUniqueId() const
	{
		CEntityId id;
		id.FullId = FullId;
		id.DetailedId.DynamicId = 0;
		return id.FullId;
	}

	/// Test if the entity id is Unknown
	bool isUnknownId() const
	{
		return DetailedId.Type == UNKNOWN_TYPE;
	}


	// ---------------------------------------------------------------------------------
	// operators

	///\name comparison of two CEntityId.
	//@{
	bool operator == (const CEntityId &a) const
//	virtual bool operator == (const CEntityId &a) const
	{

		CEntityId testId ( FullId ^ a.FullId );
		testId.DetailedId.DynamicId = 0;
		return testId.FullId == 0;

		/*
		return (Id == a.DetailedId.Id && DetailedId.CreatorId == a.DetailedId.CreatorId && DetailedId.Type == a.DetailedId.Type);
		*/
	}
	bool operator != (const CEntityId &a) const
	{
		return !((*this) == a);
	}

	bool operator < (const CEntityId &a) const
//	virtual bool operator < (const CEntityId &a) const
	{
		return getUniqueId() < a.getUniqueId();

		/*
		if (Type < a.Type)
		{
			return true;
		}
		else if (Type == a.Type)
		{
			if (Id < a.Id)
			{
				return true;
			}
			else if (Id == a.Id)
			{
				return (CreatorId < a.CreatorId);
			}
		}		
		return false;
		*/
	}

	bool operator > (const CEntityId &a) const
//	virtual bool operator > (const CEntityId &a) const
	{
		return getUniqueId() > a.getUniqueId();

		/*
		if (Type > a.Type)
		{
			return true;
		}
		else if (Type == a.Type)
		{
			if (Id > a.Id)
			{
				return true;
			}
			else if (Id == a.Id)
			{
				return (CreatorId > a.CreatorId);
			}
		}
		// lesser
		return false;
		*/
	}
	//@}

	const CEntityId &operator ++(int)
	{
		if(DetailedId.Id < MaxEntityId)
		{
			DetailedId.Id ++;
		}
		else
		{
			siperror ("CEntityId looped (max was %" SIP_I64 "d", MaxEntityId);
		}
		return *this;
	}

	const CEntityId &operator = (const CEntityId &a)
	{
		FullId = a.FullId;
		/*
		DynamicId = a.DynamicId;
		CreatorId = a.CreatorId;
		Type = a.Type;
		Id = a.Id;
		*/
		return *this;
	}

	const CEntityId &operator = (uint64 p)
	{			
		FullId = p;
		/*
		DynamicId = (uint64)(p & 0xff);
		p >>= 8;
		CreatorId = (uint64)(p & 0xff);
		p >>= 8;
		Type = (uint64)(p & 0xff);
		p >>= 8;
		Id = (uint64)(p);
		*/
		return *this;
	}


	// ---------------------------------------------------------------------------------
	// other methods...
	uint64 asUint64()const 
	{
		return FullId;
	}

/*	operator uint64 () const
	{
		return FullId;

		uint64 p = Id;
		p <<= 8;
		p |= (uint64)Type;
		p <<= 8;
		p |= (uint64)CreatorId;
		p <<= 8;
		p |= (uint64)DynamicId;

		return p;
	}
*/

	// ---------------------------------------------------------------------------------
	// loading, saving, serialising...

	/// Save the Id into an output stream.
	void save(SIPBASE::IStream &os)
//	virtual void save(SIPBASE::IStream &os)
	{
		os.serial(FullId);
		/*
		uint64 p = Id;
		p <<= 8;
		p |= (uint64)Type;
		p <<= 8;
		p |= (uint64)CreatorId;
		p <<= 8;
		p |= (uint64)DynamicId;
		os.serial(p);
		*/
	}

	/// Load the number from an input stream.
	void load(SIPBASE::IStream &is)
//	virtual void load(SIPBASE::IStream &is)
	{
		is.serial(FullId);
		/*
		uint64 p;
		is.serial(p);

		DynamicId = (uint64)(p & 0xff);
		p >>= 8;
		CreatorId = (uint64)(p & 0xff);
		p >>= 8;
		Type = (uint64)(p & 0xff);
		p >>= 8;
		Id = (uint64)(p);
		*/
	}


	void serial (SIPBASE::IStream &f) throw (SIPBASE::EStream)
//	virtual void serial (SIPBASE::IStream &f) throw (SIPBASE::EStream)
	{
		if (f.isReading ())
		{
			load (f);
		}
		else
		{				
			save (f);
		}
	}


	// ---------------------------------------------------------------------------------
	// string convertions

	/// return a string in form "(a:b:c:d)" where a,b,c,d are components of entity id.
	std::string toString() const
	{
		std::string id;
		getDebugString (id);
		return "(" + id + ")";
	}

	/// Read from a debug string, use the same format as toString() (id:type:creator:dynamic) in hexadecimal
	void	fromString(const char *str)
//	virtual void	fromString(const char *str)
	{
		uint64		id;
		uint		type;
		uint		creatorId;
		uint		dynamicId;

		if (sscanf(str, "(%" SIP_I64 "x:%x:%x:%x)", &id, &type, &creatorId, &dynamicId) != 4)
		{
			*this = Unknown;
			return;
		}

		DetailedId.Id = id;
		DetailedId.Type = type;
		DetailedId.CreatorId = creatorId;
		DetailedId.DynamicId = dynamicId;
	}
	
	/// Have a debug string.
	void getDebugString(std::string &str) const
//	virtual void getDebugString(std::string &str) const
	{											
		char b[256];
		memset(b,0,255);
		memset(b,'0',19);
		sint n;

		uint64 x = DetailedId.Id;
		char baseTable[] = "0123456789abcdef";
		for(n = 10; n < 20; n ++)
		{
			b[19 - n] = baseTable[(x & 15)];
			x >>= 4;
		}
		b[19 - 9] = ':';

		x = DetailedId.Type;
		for(n = 7; n < 9; n ++)
		{				
			b[19 - n] = baseTable[(x & 15)];
			x >>= 4;
		}
		b[19 - 6] = ':';

		x = DetailedId.CreatorId;
		for(n = 4; n < 6; n ++)
		{				
			b[19 - n] = baseTable[(x & 15)];
			x >>= 4;
		}
		b[19 - 3] = ':';

		x = DetailedId.DynamicId;
		for(n = 1; n < 3; n ++)
		{							
			b[19 - n] = baseTable[(x & 15)];
			x >>= 4;
		}
		str += "0x" + std::string(b);
	}
/*
	/// \name SIPBASE::IStreamable method.
	//@{
	std::string	getClassName ()
//	virtual std::string	getClassName ()
	{
		return std::string ("<CEntityId>");
	}

	//@}
*/

//	friend std::stringstream &operator << (std::stringstream &__os, const CEntityId &__t);
};	

/**
 * a generic hasher for entities
 */
class CEidHash
{
public:
	size_t	operator () ( const SIPBASE::CEntityId & id ) const 
	{ 		
		uint64 hash64 = id.getUniqueId();
		return size_t(hash64) ^ size_t( hash64 >> 32 );
		return (uint32)id.getShortId(); 
	}
};


/*inline std::stringstream &operator << (std::stringstream &__os, const CEntityId &__t)
{
	__os << __t.toString ();
	return __os;
}*/

} // SIPBASE

#endif // __ENTITY_ID_H__

/* End of entity_id.h */
