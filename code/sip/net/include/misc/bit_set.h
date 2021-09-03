/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BIT_SET_H__
#define __BIT_SET_H__

#include "types_sip.h"
#include "stream.h"

namespace	SIPBASE
{

// Size in bit of base word.
#define	SIP_BITLEN			(4*8)
#define	SIP_BITLEN_SHIFT		5


// ***************************************************************************
/**
 * A BitSet, to test / set flags quickly.
 * \date 2000
 */
class	CBitSet
{
public:
	/* ***********************************************
	 *	WARNING: This Class/Method must be thread-safe (ctor/dtor/serial): no static access for instance
	 *	It can be loaded/called through CAsyncFileManager for instance
	 * ***********************************************/

	/// \name Object.
	//@{
	CBitSet();
	CBitSet(uint numBits);
	CBitSet(const CBitSet &bs);
	~CBitSet();
	CBitSet	&operator=(const CBitSet &bs);
	//@}

	/// \name Basics.
	//@{
	/// Resize the bit array. All Bits are reseted.
	void	resize (uint numBits);
	/// Resize the bit array. Bits are not reseted. New bits are set with value.
	void	resizeNoReset (uint numBits, bool value=false);
	/// Clear the bitarray so size() return 0.
	void	clear();
	/// Return size of the bit array.
	uint	size() const
	{
		return NumBits;
	}
	/// Set a bit to 0 or 1.
	void	set(sint bitNumber, bool value)
	{
		sipassert(bitNumber>=0 && bitNumber<NumBits);

		uint	mask= bitNumber&(SIP_BITLEN-1);
		mask= 1<<mask;
		if(value)
			Array[bitNumber >> SIP_BITLEN_SHIFT]|= mask ;
		else
			Array[bitNumber >> SIP_BITLEN_SHIFT]&= ~mask;
	}
	/// Get the value of a bit.
	bool	get(sint bitNumber) const
	{
		sipassert(bitNumber>=0 && bitNumber<NumBits);

		uint	mask= bitNumber&(SIP_BITLEN-1);
		mask= 1<<mask;
		return (Array[bitNumber >> SIP_BITLEN_SHIFT] & mask) != 0;
	}
	/// Get the value of a bit.
	bool	operator[](sint bitNumber) const
	{
		return get(bitNumber);
	}
	/// Set a bit to 1.
	void	set(sint bitNumber) {set(bitNumber, true);}
	/// Set a bit to 0.
	void	clear(sint bitNumber) {set(bitNumber, false);}
	/// Set all bits to 1.
	void	setAll();
	/// Set all bits to 0.
	void	clearAll();
	//@}


	/// \name Bit operations.
	//@{
	/// Return The bitarray NOTed.
	CBitSet	operator~() const;
	/**
	 * Return this ANDed with bs.
	 * The result BitSet is of size of \c *this. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	operator&(const CBitSet &bs) const;
	/**
	 * Return this ORed with bs.
	 * The result BitSet is of size of \c *this. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	operator|(const CBitSet &bs) const;
	/**
	 * Return this XORed with bs.
	 * The result BitSet is of size of \c *this. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	operator^(const CBitSet &bs) const;

	/// NOT the BitArray.
	void	flip();
	/**
	 * AND the bitArray with bs.
	 * The bitset size is not changed. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	&operator&=(const CBitSet &bs);
	/**
	 * OR the bitArray with bs.
	 * The bitset size is not changed. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	&operator|=(const CBitSet &bs);
	/**
	 * XOR the bitArray with bs.
	 * The bitset size is not changed. Any missing bits into bs will be considered as 0.
	 */
	CBitSet	&operator^=(const CBitSet &bs);
	//@}


	/// \name Bit comparisons.
	//@{
	/** 
	 * Compare two BitSet not necessarely of same size. The comparison is done on N bits, where N=min(this->size(), bs.size())
	 * \return true if the N common bits of this and bs are the same. false otherwise.
	 */
	bool	compareRestrict(const CBitSet &bs) const;
	/// Compare two BitSet. If not of same size, return false.
	bool	operator==(const CBitSet &bs) const;
	/// operator!=.
	bool	operator!=(const CBitSet &bs) const;
	/// Return true if all bits are set. false if size()==0.
	bool	allSet();
	/// Return true if all bits are cleared. false if size()==0.
	bool	allCleared();
	//@}


	/// Serialize
	void	serial(SIPBASE::IStream &f);

	/// Return the raw vector
	const std::vector<uint32>& getVector() const { return Array; }

	/// Write an uint32 into the bit set (use with caution, no check)
	void	setUint( uint32 srcValue, uint i ) { Array[i] = srcValue; }

	/// Return a string representing the bitfield with 1 and 0 (from left to right)
	std::string	toString() const;

private:
	std::vector<uint32>	Array;
	sint				NumBits;
	uint32				MaskLast;	// Mask for the last uint32.
};


}



#endif // __BIT_SET_H__

/* End of bit_set.h */
