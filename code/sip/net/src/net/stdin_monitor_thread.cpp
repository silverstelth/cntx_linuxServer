/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include <string>

#include "misc/common.h"
#include "misc/thread.h"
#include "misc/command.h"

#include "stdin_monitor_thread.h"


//-----------------------------------------------------------------------------
// namespace: SIPNET
//-----------------------------------------------------------------------------

namespace SIPNET
{

	//-----------------------------------------------------------------------------
	// class CStdinMonitorThread
	//-----------------------------------------------------------------------------

	class CStdinMonitorThread : public SIPBASE::IRunnable
	{
	public:
		// ctor
		CStdinMonitorThread();

		// main routine executed when the thread is started
		void run();

		// interface for adding commands, retrieving commands and verifying whether there are commands waiting
		void pushCommand(std::string nextCommand);
		std::string popCommand();
		bool commandWaiting() const;

	private:
		std::string _NextCommand;
		volatile bool _CommandWaiting;

	};

	//-----------------------------------------------------------------------------
	// methods CStdinMonitorThread
	//-----------------------------------------------------------------------------

	CStdinMonitorThread::CStdinMonitorThread()
	{
		_CommandWaiting= false;
	}

	void CStdinMonitorThread::run ()
	{
		while(!feof(stdin))
		{
			// wait for the main thread to deal with the previous command
			while (commandWaiting())
			{
				SIPBASE::sipSleep(1);
			}

			// get the next command from the command line
			char theCommand[1024] = "";
			fgets(theCommand, sizeofarray(theCommand), stdin);

			// push the command to allow reader thread to deal with it
			pushCommand(theCommand);
		}
	}

	void CStdinMonitorThread::pushCommand(std::string nextCommand)
	{
		// wait for the previous command to be treated
		while (_CommandWaiting)
		{
			SIPBASE::sipSleep(1);
		}

		// copy the next command into the appropriate buffer
		_NextCommand= nextCommand;

		while (!_NextCommand.empty() 
				&& (_NextCommand[_NextCommand.size()-1] == '\n'
					|| _NextCommand[_NextCommand.size()-1] == '\r'))
		{
			_NextCommand = _NextCommand.substr(0, _NextCommand.size()-1);
		}

		// set the _CommandWaiting flag, to allow reader thread to treat the new command
		_CommandWaiting= true;
	}

	std::string CStdinMonitorThread::popCommand()
	{
		// wait for a command to be ligned up (waiting)
		while (!_CommandWaiting)
		{
		}

		// copy out the next command
		std::string result= _NextCommand;

		// clear out the next command variable ready for its next use
		_NextCommand.clear();

		// clear the _CommandWaiting flag, to allow writer thread to add a new command
		_CommandWaiting= false;

		// all done so return the command
		return result;
	}

	bool CStdinMonitorThread::commandWaiting() const
	{
		return _CommandWaiting;
	}


	//-----------------------------------------------------------------------------
	// class CStdinMonitorSingleton
	//-----------------------------------------------------------------------------

	class CStdinMonitorSingleton: public IStdinMonitorSingleton
	{
	public:
		// static for getting hold of the singleton instance
		static CStdinMonitorSingleton* getInstance();

		// methods required by IStdinMonitorSingleton
		void init();
		void update();
		void release();
		virtual ~CStdinMonitorSingleton() {}

	private:
		// this is a singleton so dissallow construction from outside...
		CStdinMonitorSingleton();
		CStdinMonitorSingleton(const CStdinMonitorSingleton&);
		CStdinMonitorSingleton& operator =(const CStdinMonitorSingleton&);

	private:
		// data for the singleton instance
		CStdinMonitorThread* _StdinMonitorThreadInstance;
		SIPBASE::IThread* _StdinMonitorThreadHandle;
	};


	//-----------------------------------------------------------------------------
	// methods IStdinMonitorSingleton
	//-----------------------------------------------------------------------------

	IStdinMonitorSingleton* IStdinMonitorSingleton::getInstance()
	{
		return CStdinMonitorSingleton::getInstance();
	}


	//-----------------------------------------------------------------------------
	// methods CStdinMonitorSingleton
	//-----------------------------------------------------------------------------

	CStdinMonitorSingleton* CStdinMonitorSingleton::getInstance()
	{
		static CStdinMonitorSingleton* instance= NULL;
		if (instance==NULL)
			instance= new CStdinMonitorSingleton;
		return instance;
	}

	void CStdinMonitorSingleton::init()
	{
		_StdinMonitorThreadInstance= new CStdinMonitorThread;
		_StdinMonitorThreadHandle = SIPBASE::IThread::create (_StdinMonitorThreadInstance, 1024*4*4);
		_StdinMonitorThreadHandle->start();
	}

	void CStdinMonitorSingleton::update()
	{
		// if we're not initialised yet then return
		if (_StdinMonitorThreadInstance== NULL)
			return;

		// if there's a command waiting then treat it (not more than one command per visit)
		if (_StdinMonitorThreadInstance->commandWaiting())
		{
			std::string nextCommand= _StdinMonitorThreadInstance->popCommand();
			if (!nextCommand.empty())
			{
				SIPBASE::ICommand::execute(nextCommand,*SIPBASE::InfoLog);
			}
		}
	}

	void CStdinMonitorSingleton::release()
	{
		// if we've never been initialised or we've already been released thent there's nothing more to do...
		if (_StdinMonitorThreadInstance== NULL)
			return;

		// terminate the thread and wait for it to finish
		_StdinMonitorThreadHandle->terminate();
		_StdinMonitorThreadHandle->wait();

		delete _StdinMonitorThreadHandle;
		_StdinMonitorThreadHandle = NULL;
		// destroy the thread object instance and reset the pointer to NULL to mark as 'uninitialised'
		delete _StdinMonitorThreadInstance;
		_StdinMonitorThreadInstance= NULL;
	}

	CStdinMonitorSingleton::CStdinMonitorSingleton()
	{
		_StdinMonitorThreadHandle= NULL;
		_StdinMonitorThreadInstance= NULL;
	}

	CStdinMonitorSingleton::CStdinMonitorSingleton(const CStdinMonitorSingleton&)
	{
		_StdinMonitorThreadHandle= NULL;
		_StdinMonitorThreadInstance= NULL;
	}

	CStdinMonitorSingleton& CStdinMonitorSingleton::operator =(const CStdinMonitorSingleton&)
	{
		return *this;
	}

} // SIPNET
