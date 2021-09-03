/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SPEAKER_LISTENER_H__
#define __SPEAKER_LISTENER_H__

#include "types_sip.h"

namespace SIPBASE
{
	class ISpeaker;

	class IListener
	{
	public:
		virtual void speakerIsDead	(ISpeaker *speaker) =0;
	};

	class ISpeaker
	{
	public:
		virtual void registerListener	(IListener *listener) =0;
		virtual void unregisterListener	(IListener *listener) =0;
	};

	template <class Listener>
	class CSpeaker : public ISpeaker
	{
	public:
		typedef std::set<IListener*>	TListeners;

	private:
		TListeners		_Listeners;

	public:

		~CSpeaker()
		{
			while (!_Listeners.empty())
			{
				IListener *listener = *_Listeners.begin();
				listener->speakerIsDead(this);
				_Listeners.erase(_Listeners.begin());
			}
		}

		void registerListener	(IListener *listener)
		{
			_Listeners.insert(listener);
		}
		void unregisterListener	(IListener *listener)
		{
			_Listeners.erase(listener);
		}

		const TListeners &getListeners()
		{
			return _Listeners;
		}

	};


	/** A macro to facilitate method invocation on listeners */
#define SIPBASE_BROADCAST_TO_LISTENER(listenerClass, methodAndParam)	\
	CSpeaker<listenerClass>::TListeners::const_iterator first(CSpeaker<listenerClass>::getListeners().begin()), last(CSpeaker<listenerClass>::getListeners().end()); \
	for (; first != last; ++first)				\
	{											\
		listenerClass *listener = static_cast<listenerClass*>(*first);		\
												\
		listener->methodAndParam;				\
	}											\



	template <class Speaker>
	class CListener : public IListener
	{
		ISpeaker	*_Speaker;

		void speakerIsDead(ISpeaker *speaker)
		{
			sipassert(speaker == _Speaker);
			_Speaker = NULL;
		}

	public:

		CListener()
			: _Speaker(NULL)
		{
		}

		~CListener()
		{
			if (_Speaker != NULL)
				_Speaker->unregisterListener(this);
		}

		void registerListener(ISpeaker *speaker)
		{
			sipassert(_Speaker == NULL);
			_Speaker = speaker;
			_Speaker->registerListener(this);
		}

		void unregisterListener(ISpeaker *speaker)
		{
			sipassert(_Speaker != NULL);
			_Speaker->unregisterListener(this);
			_Speaker = NULL;
		}

		ISpeaker *getSpeaker()
		{
			return _Speaker;
		}

	};


} // SIPBASE

#endif // __SPEAKER_LISTENER_H__
