/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/progress_callback.h"

namespace SIPBASE 
{

float IProgressCallback::getCropedValue (float value) const
{
	sipassert (_CropedValues.size ()>0);
	const CCropedValues &values = _CropedValues.back ();
	return value*(values.Max-values.Min)+values.Min;
}

IProgressCallback::IProgressCallback ()
{
	_CropedValues.push_back (CCropedValues (0, 1));
}

void IProgressCallback::pushCropedValues (float min, float max)
{
	sipassert (_CropedValues.size ()>0);
	//const CCropedValues &values = _CropedValues.back ();
	_CropedValues.push_back (CCropedValues (getCropedValue (min), getCropedValue (max)));
}

void IProgressCallback::popCropedValues ()
{
	sipassert (_CropedValues.size ()>1);
	_CropedValues.pop_back ();
}

void IProgressCallback::progress (float progressValue)
{
}

} // SIPBASE
