/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __VALUE_SMOOTHER_H__
#define __VALUE_SMOOTHER_H__

#include "types_sip.h"

namespace SIPBASE {

// ***************************************************************************
/**
 * A smoother of values.
 * \date 2001
 */
template <class T>
class CValueSmootherTemplate
{
public:

	/// Constructor
	explicit CValueSmootherTemplate(uint n=16)
	{
		init(n);
	}

	/// reset the ValueSmoother, and set the number of frame to smooth.
	void		init(uint n)
	{
		// reset all the array to 0.
		_LastFrames.clear();

		if (n > 0)
			_LastFrames.resize(n, 0);
		
		_CurFrame = 0;
		_NumFrame = 0;
		_FrameSum = 0;
	}
	
	/// reset only the ValueSmoother
	void		reset()
	{
		std::fill(_LastFrames.begin(), _LastFrames.end(), T(0));
		
		_CurFrame = 0;
		_NumFrame = 0;
		_FrameSum = 0;
	}

	/// add a new value to be smoothed.
	void		addValue(T dt)
	{
		if (_LastFrames.empty())
			return;

		// update the frame sum. NB: see init(), at start, array is full of 0. so it works even for un-inited values.
		_FrameSum-= _LastFrames[_CurFrame];
		_FrameSum+= dt;
		
		// backup this value in the array.
		_LastFrames[_CurFrame]= dt;
		
		// next frame.
		_CurFrame++;
//		_CurFrame%=_LastFrames.size();
		if (_CurFrame >= _LastFrames.size())
			_CurFrame -= _LastFrames.size();
		
		// update the number of frames added.
		_NumFrame++;
		_NumFrame= std::min(_NumFrame, (uint)_LastFrames.size());
	}
	
	/// get the smoothed value.
	T		getSmoothValue() const
	{
		if(_NumFrame>0)
			return T(_FrameSum) / T(_NumFrame);
		else
			return T(0);
	}

	uint getNumFrame() const
	{
		return _NumFrame;
	}

	const std::vector<T> &getLastFrames() const
	{
		return _LastFrames;
	}

private:
	std::vector<T>			_LastFrames;
	uint					_CurFrame;
	uint					_NumFrame;
	T						_FrameSum;
};

// ***************************************************************************
/**
 * A smoother replacement for boolean.
 * \date 2003
 */
template <>
class CValueSmootherTemplate<bool>
{
public:

	/// Constructor
	explicit CValueSmootherTemplate(uint n=1)
	{
		init(n);
	}

	/// reset the ValueSmoother, and set the number of frame to smooth.
	void		init(uint n)
	{
		// reset all the array to 0.
		_LastFrames.clear();

		if (n>0)
			_LastFrames.resize(n, false);

		_NumFrame = 0;
	}
	
	/// reset only the ValueSmoother
	void		reset()
	{
		std::fill(_LastFrames.begin(), _LastFrames.end(), false);
 		_NumFrame = 0;
	}

	/// add a new value to be smoothed.
	void		addValue(bool dt)
	{
		if(_NumFrame>0)
			_LastFrames[0] = dt;
	}
	
	/// get the smoothed value.
	bool		getSmoothValue() const
	{
		if(_NumFrame>0)
			return _LastFrames[0];
		else
			return false;
	}

	uint getNumFrame() const
	{
		return _NumFrame;
	}

	const std::vector<bool> &getLastFrames() const
	{
		return _LastFrames;
	}

private:
	std::vector<bool>       _LastFrames;
	uint            		_NumFrame;
};

class CValueSmoother : public CValueSmootherTemplate<float>
{
public:
	/// Constructor
	explicit CValueSmoother(uint n=16) : CValueSmootherTemplate<float>(n)
	{
	}
};

} // SIPBASE


#endif // __VALUE_SMOOTHER_H__

/* End of value_smoother.h */
