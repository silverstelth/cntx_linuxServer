/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/reader_writer.h"


namespace SIPBASE {

CReaderWriter::CReaderWriter()
{
	_ReadersLevel = 0;
}

CReaderWriter::~CReaderWriter()
{
	// here some checks to avoid a reader/writer still working while we flush the mutexes...
}



} // SIPBASE
