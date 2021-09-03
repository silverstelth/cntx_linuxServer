/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STDIN_MONITOR_THREAD_H__
#define __STDIN_MONITOR_THREAD_H__

namespace SIPNET
{
	//-----------------------------------------------------------------------------
	// class IStdinMonitorSingleton
	//-----------------------------------------------------------------------------

	class IStdinMonitorSingleton
	{
	public:
		// static for getting hold of the singleton instance
		static IStdinMonitorSingleton* getInstance();

		// methods required by IStdinMonitorSingleton
		virtual void init()=0;
		virtual void update()=0;
		virtual void release()=0;

		virtual ~IStdinMonitorSingleton() {}
	};

} // SIPBASE

#endif
