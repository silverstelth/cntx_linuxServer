/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __PROGRESS_CALLBACK_H__
#define __PROGRESS_CALLBACK_H__

#include "types_sip.h"

namespace SIPBASE {

/**
 * Progress callback interface
 * \date 2002
 */
class IProgressCallback
{
public:

	IProgressCallback ();
	virtual ~IProgressCallback () {}

	/**
	  * Call back
	  *
	  * progressValue should be 0 ~ 1
	  */
	virtual void progress (float progressValue);

	/**
	  * Push crop values
	  */
	void pushCropedValues (float min, float max);

	/**
	  * Push crop values
	  */
	void popCropedValues ();

	/**
	  * Get croped value
	  */
	float getCropedValue (float value) const;

public:
	
	/// Display string
	std::string		DisplayString;

private:

	class CCropedValues
	{
	public:
		CCropedValues (float min, float max)
		{
			Min = min;
			Max = max;
		}
		float	Min;
		float	Max;
	};

	std::vector<CCropedValues>	_CropedValues;
};


} // SIPBASE


#endif // __PROGRESS_CALLBACK_H__

/* End of progress_callback.h */
