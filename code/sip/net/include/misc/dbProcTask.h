/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DBPROCTASK_H__
#define __DBPROCTASK_H__

#include "misc/bit_mem_stream.h"
#include "misc/thread.h"
#include "misc/buf_fifo.h"

namespace SIPBASE
{

//template<class T>
class CQueryRequest;
class CDBCaller;
class CDBConnector;

class CDBProcTask : public SIPBASE::IRunnable
{
public:
	bool	init(CDBCaller *caller);
	virtual void	run();
	void	close();
	uint32	process();

	CDBProcTask();

protected:

public:
	void	requireExit() { _ExitRequired = true; }
	bool	exitRequired() const { return _ExitRequired; }
	CDBCaller	*DBCaller() { return _Caller; };
	void	ReConnectDB();
	HDBC	GetDBHandle();
private:
	volatile bool	_ExitRequired;
	CDBCaller		*_Caller;
	CDBConnector	*_DBExecutor;
	SIPBASE::TTime	_LastDBPingTime;
};
} // SIPBASE
#endif // end of guard __DBPROCTASK_H__
