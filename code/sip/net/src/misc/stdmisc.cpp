/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"


// leave not static else this workaround don't work
void	dummyToAvoidStupidCompilerWarning_std_misc_cpp()
{
}

FILE* sfWfopen(const wchar_t * fileName, const wchar_t * mode)
{
	FILE* fp;
#ifdef SIP_OS_WINDOWS
	fp = _wfopen (fileName, mode);
#else
	char scFName[1024];
	sprintf(scFName, "%S", fileName);
	char scMode[16];
	sprintf(scMode, "%S", mode);
	fp = fopen (scFName, scMode);
#endif
	return fp;
}

int sfWrename(const wchar_t * oldName, const wchar_t * newName) 
{
#ifdef SIP_OS_WINDOWS
	return _wrename(oldName, newName);
#else
	char scOldName[1024];
	sprintf(scOldName, "%S", oldName);
	char scNewName[1024];
	sprintf(scNewName, "%S", newName);
	return rename (scOldName, scNewName);
#endif
}

