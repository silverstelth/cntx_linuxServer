/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __ARRAY_2D_H__
#define __ARRAY_2D_H__

#include "misc/rect.h"
#include "misc/stream.h"
#include "misc/traits_sip.h"

namespace SIPBASE
{

/** A simple 2D array.
  *
  * Access is done using the () operator
  *
  * Example :
  * 
  * CArray2D<uint> myArray;  
  * myArray.init(10, 10, 0); // fill with zero's
  * myArray(5, 5) = 1;
  *
  * \date 2005
  */
template <class T> class CArray2D
{
public:	
	typedef typename std::vector<T> TArrayContainer;
	typedef typename TArrayContainer::iterator iterator;
	typedef typename TArrayContainer::const_iterator const_iterator;
	CArray2D() : _Width(0), _Height(0) {}
	void init(uint width, uint height);
	void init(uint width, uint height, const T &defaultValue);
	bool empty() const { return _Array.empty(); }
	void clear();
	typename iterator begin() { return _Array.begin(); }
	typename iterator end() { return _Array.end(); }
	typename const_iterator begin() const { return _Array.begin(); }
	typename const_iterator end() const { return _Array.end(); }

	bool isIn(sint x, sint y) const { return x >= 0 && y >= 0 && x < (sint) _Width && y < (sint) _Height; }
	
	// access element by column/row
	T &operator()(uint x, uint y)
	{
		#ifdef SIP_DEBUG
			sipassert(x < _Width);
			sipassert(y < _Height);
		#endif		
		return _Array[x + y * _Width];
	}
	// access element by column/row (const version)
	const T &operator()(uint x, uint y) const
	{
		#ifdef SIP_DEBUG
			sipassert(x < _Width);
			sipassert(y < _Height);
		#endif		
		return _Array[x + y * _Width];		
	}
	// Return width of array
	uint getWidth() const { return _Width; }
	// Return height of array
	uint getHeight() const { return _Height; }
	/** Move array content of the given offset. No wrapping is applied
	  * Example : move(1, 0) will move the array of one column to the left. The latest column is lost. The first column remains unchanged
	  */
	void move(sint offsetX, sint offsetY);
	// Move a part of the array. Values are clamped as necessary
	void moveSubArray(sint dstX, sint dstY, sint srcX, sint srcY, sint width, sint height);
	// Blit content from another array
	void blit(const CArray2D<T> &src, sint srcX, sint srcY, sint srcWidth, sint srcHeight, sint dstX, sint dstY);
	// get an iterator to the start of a row
	iterator beginRow(uint row)
	{
		sipassert(row < _Height);
		return _Array.begin() + row * _Width;
	}
	const_iterator beginRow(uint row) const
	{
		sipassert(row < _Height);
		return _Array.begin() + row * _Width;
	}
	iterator endRow(uint row)
	{
		sipassert(row < _Height);
		return _Array.begin() + (row + 1) * _Width;
	}
	const_iterator endRow(uint row) const
	{
		sipassert(row < _Height);
		return _Array.begin() + (row + 1) * _Width;
	}	
	// get an iterator at the given position 
	iterator getIteratorAt(uint x, uint y)
	{
		#ifdef SIP_DEBUG
			sipassert(x < _Width);
			sipassert(y < _Height);
		#endif
		return _Array.begin() + x + (y * _Width);
	}
	// Get a const iterator at the given position
	const_iterator getIteratorAt(uint x, uint y) const
	{
		#ifdef SIP_DEBUG
			sipassert(x < _Width);
			sipassert(y < _Height);
		#endif
		return _Array.begin() + x + (y * _Width);
	}
	/** See which part of array should be updated after its content has been displaced by the given offset (by a call to move for example).
	  * Example: getUpdateRects(0, 1, result) will result the first row as a result
	  */
	void getUpdateRects(sint moveOffsetX, sint moveOffsetY, std::vector<SIPBASE::CRect> &rectsToUpdate);
	// See which parts of array will be discarded if the array is displaced by the given offset
	void getDiscardRects(sint moveOffsetX, sint moveOffsetY, std::vector<SIPBASE::CRect> &discardedRects);
	//
	void serial(SIPBASE::IStream &f) throw(SIPBASE::EStream);
private:
	TArrayContainer _Array;	
	uint _Width;	
	uint _Height;
private:
	inline void checkRect(const SIPBASE::CRect &r) const
	{
		sipassert(r.X >= 0 && r.X < (sint32) _Width);
		sipassert(r.Y >= 0 && r.Y < (sint32) _Height);
		sipassert(r.X + r.Width >= 0 && r.X + (sint32)  r.Width <= (sint32) _Width);
		sipassert(r.Y + r.Height >= 0 && r.Y + (sint32) r.Height <= (sint32) _Height);
	}
};

//*********************************************************************************
template <class T>
void CArray2D<T>::clear()
{
	_Array.clear();
	_Width = _Height = 0;
}

//*********************************************************************************
template <class T>
void CArray2D<T>::serial(SIPBASE::IStream &f) throw(SIPBASE::EStream)
{	
	f.serialCont(_Array);
	uint32 width = _Width;
	uint32 height = _Height;
	f.serial(width);
	f.serial(height);
	_Width = width;
	_Height = height;
}

//*********************************************************************************
template <class T>
void CArray2D<T>::getUpdateRects(sint moveOffsetX, sint moveOffsetY, std::vector<SIPBASE::CRect> &rectsToUpdate)
{
	rectsToUpdate.clear();	
	if (moveOffsetX < 0) // moved right ?
	{
		// the width to update
		uint width = std::min((uint) moveOffsetX, _Width);	
		// the the grid moved top or bottom, exclude this part
		sint height = _Height - abs(moveOffsetY);
		if (height > 0)
		{				
			// complete column on the right
			rectsToUpdate.push_back(SIPBASE::CRect((sint32) (_Width - width), (sint32) (std::max(- moveOffsetY, 0)), (uint32) width, (uint32) height));
			#ifdef SIP_DEBUG
				checkRect(rectsToUpdate.back());
			#endif
		}		
	}
	else if (moveOffsetX > 0) // moved left ?
	{
		// the width to update
		uint width = std::min((uint) (- moveOffsetX), _Width);
		// the the grid moved top or bottom.
		sint height = _Height - abs(moveOffsetY);
		if (height > 0)
		{			
			// complete column on the right
			rectsToUpdate.push_back(SIPBASE::CRect(0, (sint32) std::max(- moveOffsetY, 0), (uint32) width, (uint32) height));
			#ifdef SIP_DEBUG
				checkRect(rectsToUpdate.back());
			#endif
		}
	}
	// update top or bottom part
	if (moveOffsetY < 0)
	{
		sint height = std::min((uint) moveOffsetY, _Height);		
		rectsToUpdate.push_back(SIPBASE::CRect(0, _Height - height, _Width, height));
		#ifdef SIP_DEBUG
			checkRect(rectsToUpdate.back());
		#endif
	}
	else
	if (moveOffsetY > 0)
	{
		sint height = std::min((uint) (- moveOffsetY), _Height);		
		rectsToUpdate.push_back(SIPBASE::CRect(0, 0, _Width, height));
		#ifdef SIP_DEBUG
			checkRect(rectsToUpdate.back());
		#endif
	}	
}

//*********************************************************************************
template <class T>
void CArray2D<T>::getDiscardRects(sint moveOffsetX, sint moveOffsetY,std::vector<SIPBASE::CRect> &discardedRects)
{
	getUpdateRects(- moveOffsetX, - moveOffsetY, discardedRects);
}

//*********************************************************************************
template <class T>
void CArray2D<T>::moveSubArray(sint dstX, sint dstY, sint srcX, sint srcY, sint width, sint height)
{	
	if (srcX >= (sint) getWidth()) return;
	if (srcY >= (sint) getHeight()) return;
	if (dstX >= (sint) getWidth()) return;
	if (dstY >= (sint) getHeight()) return;
	if (srcX < 0)
	{
		width += srcX;
		if (width <= 0) return;
		srcX = 0;
	}
	if (srcY < 0)
	{
		height += srcY;
		if (height <= 0) return;
		srcY = 0;
	}
	if (srcX + width > (sint) getWidth())
	{
		width = getWidth() - srcX;
	}
	if (srcY + height > (sint) getHeight())
	{
		height = getHeight() - srcY;
	}
	if (dstX < 0)
	{
		width += dstX;		
		if (width < 0) return;
		srcX -= dstX;
		dstX = 0;
	}
	if (dstY < 0)
	{
		height += dstY;
		if (height < 0) return;
		srcY -= dstY;
		dstY = 0;
	}
	if (dstX + width > (sint) getWidth())
	{
		width =  getWidth() - dstX;		
	}
	if (dstY + height > (sint) getHeight())
	{
		height =  getHeight() - dstY;
	}
	#ifdef SIP_DEBUG
		sipassert(width > 0);
		sipassert(height > 0);
		sipassert(srcX >= 0 && srcX < (sint) getWidth());
		sipassert(srcY >= 0 && srcY < (sint) getHeight());
	#endif
	if (dstY < srcY)
	{
		const_iterator src = getIteratorAt(srcX, srcY);
		iterator dst = getIteratorAt(dstX, dstY);
		do 
		{
			if (CTraits<T>::SupportRawCopy)
			{
				// type support fast copy
				::memcpy(&(*dst), &(*src), sizeof(T) * width);
			}
			else
			{
				std::copy(src, src + width, dst);
			}
			src += _Width;
			dst += _Width;
		} 
		while(--height);
	}
	else if (dstY > srcY)
	{
		// copy from top to bottom
		const_iterator src = getIteratorAt(srcX, srcY + height - 1);
		iterator dst = getIteratorAt(dstX, dstY + height - 1);
		do 
		{
			if (CTraits<T>::SupportRawCopy)
			{
				// type support fast copy
				::memcpy(&(*dst), &(*src), sizeof(T) * width);
			}
			else
			{			
				std::copy(src, src + width, dst);
			}
			src -= _Width;
			dst -= _Width;
		} 
		while(--height);
	}
	else
	{
		const_iterator src = getIteratorAt(srcX, srcY);
		iterator dst = getIteratorAt(dstX, dstY);		
		if (dstX < srcX)
		{
			do
			{
				if (CTraits<T>::SupportRawCopy)
				{
					// type support fast copy
					::memmove(&(*dst), &(*src), sizeof(T) * width);
				}
				else
				{				
					std::reverse_copy(src, src + width, dst);
				}
				src += _Width;
				dst += _Width;
			} 
			while(--height);	
		}
		else
		{
			do
			{
				if (CTraits<T>::SupportRawCopy)
				{
					// type support fast copy
					::memcpy(&(*dst), &(*src), sizeof(T) * width);
				}
				else
				{
					std::copy(src, src + width, dst);
				}				
				src += _Width;
				dst += _Width;
			} 
			while(--height);
		}
	}
}

//*********************************************************************************
template <class T>
void CArray2D<T>::blit(const CArray2D<T> &src, sint srcX, sint srcY, sint srcWidth, sint srcHeight, sint dstX, sint dstY)
{
	if (&src == this)
	{
		moveSubArray(dstX, dstY, srcX, srcY, srcWidth, srcHeight);
		return;
	}
	// clip x
	if (srcX < 0)
	{
		srcWidth += srcX;
		if (srcWidth <= 0) return;
		dstX -= srcX;
		srcX = 0;
	}
	if (srcX + srcWidth > (sint) src.getWidth())
	{
		srcWidth = src.getWidth() - srcX;
		if (srcWidth <= 0) return;
	}
	if (dstX < 0)
	{
		srcWidth += dstX;
		if (srcWidth <= 0) return;
		srcX -= dstX;
		dstX = 0;
	}
	if (dstX + srcWidth > (sint) getWidth())
	{
		srcWidth = getWidth() - dstX;
		if (srcWidth <= 0) return;
	}
	// clip y
	if (srcY < 0)
	{
		srcHeight += srcY;
		if (srcHeight <= 0) return;
		dstY -= srcY;
		srcY = 0;
	}
	if (srcY + srcHeight > (sint) src.getHeight())
	{
		srcHeight = src.getHeight() - srcY;
		if (srcHeight <= 0) return;
	}
	if (dstY < 0)
	{
		srcHeight += dstY;
		if (srcHeight <= 0) return;
		srcY -= dstY;
		dstY = 0;
	}
	if (dstY + srcHeight > (sint) getHeight())
	{
		srcHeight = getHeight() - dstY;
		if (srcHeight <= 0) return;
	}	
	const T *srcBase = (const T *) &src._Array[0];
	const T *srcPtr = &(srcBase[srcX + srcY * src.getWidth()]);
	const T *srcEndPtr = srcPtr + srcHeight * src.getWidth();
	T *destBase = (T *) &_Array[0];
	T *destPtr = 	&(destBase[dstX + dstY * getWidth()]);
	while (srcPtr != srcEndPtr)
	{
		if (CTraits<T>::SupportRawCopy)
		{
			memcpy(destPtr, srcPtr, sizeof(T) * srcWidth);
		}
		else
		{
			std::copy(srcPtr, srcPtr + srcWidth, destPtr);
		}
		srcPtr += src.getWidth();
		destPtr += getWidth();
	}
	
}



//*********************************************************************************
template <class T>
void CArray2D<T>::move(sint offsetX, sint offsetY)
{
	moveSubArray(offsetX, offsetY, 0, 0, _Width, _Height);
}			

//*********************************************************************************
template <class T>
void CArray2D<T>::init(uint width, uint height)
{	
	_Array.resize(width * height);	
	_Width = width;	
	_Height = height;
}

//*********************************************************************************
template <class T>
void CArray2D<T>::init(uint width,uint height, const T &defaultValue)
{	
	_Array.resize(width * height, defaultValue);	
	_Width = width;	
	_Height = height;	
}


} // SIPBASE



#endif