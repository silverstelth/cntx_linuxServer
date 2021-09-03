/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TIMEOUT_ASSERTION_THREAD_H__
#define __TIMEOUT_ASSERTION_THREAD_H__

#include "common.h"
#include "thread.h"
/*

  example:
	Encapsulation of message callbacks


  // in init
  CTimeoutAssertionThread *myTAT = new CTimeoutAssertionThread(1000);
  IThread::create(myTAT)->start();

  ...

  // in callback management
  myTAT->activate();
  msg->callback();
  myTAT->disactivate();

  // if the callback call is too slow, an assertion will happen

*/

class CTimeoutAssertionThread: public SIPBASE::IRunnable
{
public:
	enum TControl { ACTIVE, INACTIVE, QUIT };

	CTimeoutAssertionThread(uint32 timeout = 0) : _Control(INACTIVE), _Counter(0), _Timeout(timeout)
	{
	}

	void run()
	{
		uint32 lastCounter;
		while(_Control != QUIT)
		{
			if(_Control != ACTIVE || _Timeout == 0)
			{
				//sipdebug("not active, sleep");
				SIPBASE::sipSleep(1000);
			}
			else
			{
				//sipdebug("active, enter sleep");
				lastCounter = _Counter;

				uint32 cummuledSleep = 0;

				// do sleep until cumulated time reached timeout value
				while (cummuledSleep < _Timeout)
				{
					// sleep 1 s
					SIPBASE::sipSleep(std::min((uint32)(1000), (uint32)(_Timeout-cummuledSleep)));

					cummuledSleep += 1000;

					// check if exit is required
					if (_Control == QUIT)
						return;
				}
				//sipdebug("active, leave sleep, test assert");

				// If this assert occured, it means that a checked part of the code was
				// to slow and then I decided to assert to display the problem.
				sipassert(!(_Control==ACTIVE && _Counter==lastCounter));
			}
		}
	}

	void activate()
	{
		if(_Control == QUIT) return;
		sipassert(_Control == INACTIVE);
		_Counter++;
		_Control = ACTIVE;
		//sipdebug("activate");
	}

	void desactivate()
	{
		if(_Control == QUIT) return;
		sipassert(_Control == ACTIVE);
		_Control = INACTIVE;
		//sipdebug("desactivate");
	}

	void quit()
	{
		sipassert(_Control != QUIT);
		_Control = QUIT;
		//sipdebug("quit");
	}

	void timeout(uint32 to)
	{
		_Timeout = to;
		//sipdebug("change timeout to %d", to);
	}

private:
	volatile TControl	_Control;
	volatile uint32		_Counter;
	volatile uint32		_Timeout;	// in millisecond
};


#endif // __TIMEOUT_ASSERTION_THREAD_H__

/* End of timeout_assertion_thread.h */
