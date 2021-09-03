/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/time_sip.h"
#include "misc/command.h"
#include "misc/buf_fifo.h"

using namespace std;

#define DEBUG_FIFO 0

// if 0, don't stat the time of different function
#define STAT_FIFO 1

namespace SIPBASE {

#ifdef BUFFIFO_TRACK_ALL_BUFFERS
CBufFIFO::TAllBuffers		CBufFIFO::_AllBuffers;
#endif

CBufFIFO::CBufFIFO() : _Buffer(NULL), _BufferSize(0), _Empty(true), _Head(NULL), _Tail(NULL), _Rewinder(NULL)
{
#ifdef BUFFIFO_TRACK_ALL_BUFFERS
	_AllBuffers.insert(this);
#endif

	// reset statistic
	_BiggestBlock = 0;
	_SmallestBlock = 999999999;
	_BiggestBuffer = 0;
	_SmallestBuffer = 999999999;
	_Pushed = 0;
	_Poped = 0;
	_Remained = 0;
	_Fronted = 0;
	_Resized = 0;
	_PushedTime = 0;
	_FrontedTime = 0;
	_ResizedTime = 0;
}

CBufFIFO::~CBufFIFO()
{
#ifdef BUFFIFO_TRACK_ALL_BUFFERS
	_AllBuffers.erase(this);
#endif
	
	if (_Buffer != NULL)
	{
		delete []_Buffer;
#if DEBUG_FIFO
		sipdebug("%p delete", this);
#endif
	}
}

void	 CBufFIFO::push (const uint8 *buffer, uint32 s)
{
	// if the buffer is more than 1 meg, there s surely a problem, no?
//	sipassert( buffer.size() < 1000000 ); // size check in debug mode

#if STAT_FIFO
	TTicks before = CTime::getPerformanceTime();
#endif

#if DEBUG_FIFO
	sipdebug("%p push(%d)", this, s);
#endif

	sipassert(s > 0 && s < pow(2.0, static_cast<double>(sizeof(TFifoSize)*8)));

	// stat code
	if (s > _BiggestBlock) _BiggestBlock = s;
	if (s < _SmallestBlock) _SmallestBlock = s;

	_Pushed++;
	_Remained ++;

	while (!canFit (s + sizeof (TFifoSize)))
	{
		resize(_BufferSize * 2);
	}

	*(TFifoSize *)_Head = s;
	_Head += sizeof(TFifoSize);

	CFastMem::memcpy(_Head, buffer, s);

	_Head += s;

	_Empty = false;

#if STAT_FIFO
	// stat code
	TTicks after = CTime::getPerformanceTime();
	_PushedTime += after - before;
#endif

#if DEBUG_FIFO
	display ();
#endif
}

void CBufFIFO::push(const std::vector<uint8> &buffer1, const std::vector<uint8> &buffer2)
{
#if STAT_FIFO
	TTicks before = CTime::getPerformanceTime();
#endif

	TFifoSize s = buffer1.size() + buffer2.size();

#if DEBUG_FIFO
	sipdebug("%p push2(%d)", this, s);
#endif

	sipassert((buffer1.size() + buffer2.size ()) > 0 && (buffer1.size() + buffer2.size ()) < pow(2.0, static_cast<double>(sizeof(TFifoSize)*8)));

	// avoid too big fifo
	if (this->size() > 10000000)
	{
		throw Exception ("CBufFIFO::push(): stack full (more than 10mb)");
	}


	// stat code
	if (s > _BiggestBlock) _BiggestBlock = s;
	if (s < _SmallestBlock) _SmallestBlock = s;

	_Pushed++;
	_Remained ++;
	// resize while the buffer is enough big to accept the block
	while (!canFit (s + sizeof (TFifoSize)))
	{
		resize(_BufferSize * 2);
	}

	// store the size of the block
	*(TFifoSize *)_Head = s;
	_Head += sizeof(TFifoSize);

	// store the block itself
	CFastMem::memcpy(_Head, &(buffer1[0]), buffer1.size());
	CFastMem::memcpy(_Head + buffer1.size(), &(buffer2[0]), buffer2.size());
	_Head += s;

	_Empty = false;

#if STAT_FIFO
	// stat code
	TTicks after = CTime::getPerformanceTime();
	_PushedTime += after - before;
#endif

#if DEBUG_FIFO
	display ();
#endif
}

void CBufFIFO::pop ()
{
	if (empty ())
	{
		sipwarning("BF: Try to pop an empty fifo!");
		return;
	}

	if (_Rewinder != NULL && _Tail == _Rewinder)
	{
#if DEBUG_FIFO
		sipdebug("%p pop rewind!", this);
#endif

		// need to rewind
		_Tail = _Buffer;
		_Rewinder = NULL;
	}

	TFifoSize s = *(TFifoSize *)_Tail;

#if DEBUG_FIFO
	sipdebug("%p pop(%d)", this, s);
#endif

#ifdef SIP_DEBUG
	// clear the message to be sure user doesn't use it anymore
	memset (_Tail, '-', s + sizeof (TFifoSize));
#endif

	_Tail += s + sizeof (TFifoSize);

	if (_Tail == _Head) _Empty = true;

#if DEBUG_FIFO
	display ();
#endif
	_Remained --;
}

uint8 CBufFIFO::frontLast ()
{
	uint8	*tail = _Tail;
	
	if (empty ())
	{
		sipwarning("BF: Try to get the front of an empty fifo!");
		return 0;
	}

	if (_Rewinder != NULL && tail == _Rewinder)
	{
#if DEBUG_FIFO
		sipdebug("%p front rewind!", this);
#endif

		// need to rewind
		tail = _Buffer;
	}

	TFifoSize s = *(TFifoSize *)tail;

#if DEBUG_FIFO
	sipdebug("%p frontLast() len %d, returns %d ", this, s, *(tail+sizeof(TFifoSize)+s-1));
#endif

	return *(tail+sizeof(TFifoSize)+s-1);
}


void CBufFIFO::front (vector<uint8> &buffer)
{
	uint8 *tmpbuffer;
	uint32 s;

	buffer.clear ();

	front (tmpbuffer, s);
	
	buffer.resize (s);

	CFastMem::memcpy (&(buffer[0]), tmpbuffer, s);

/*	TTicks before = CTime::getPerformanceTime ();

	uint8	*tail = _Tail;
	
	buffer.clear ();

	if (empty ())
	{
		sipwarning("Try to get the front of an empty fifo!");
		return;
	}

	_Fronted++;
	
	if (_Rewinder != NULL && tail == _Rewinder)
	{
#if DEBUG_FIFO
		sipdebug("%p front rewind!", this);
#endif

		// need to rewind
		tail = _Buffer;
	}

	TFifoSize size = *(TFifoSize *)tail;

#if DEBUG_FIFO
	sipdebug("%p front(%d)", this, size);
#endif

	tail += sizeof (TFifoSize);

	buffer.resize (size);

	CFastMem::memcpy (&(buffer[0]), tail, size);

	// stat code
	TTicks after = CTime::getPerformanceTime ();
	_FrontedTime += after - before;

#if DEBUG_FIFO
	display ();
#endif
*/}


void CBufFIFO::front (SIPBASE::CMemStream &buffer)
{
	uint8 *tmpbuffer;
	uint32 s;

	buffer.clear ();

	front (tmpbuffer, s);

	buffer.fill (tmpbuffer, s);
	
	/*
	TTicks before = CTime::getPerformanceTime ();

	uint8	*tail = _Tail;
	
	buffer.clear ();

	if (empty ())
	{
		sipwarning("Try to get the front of an empty fifo!");
		return;
	}

	_Fronted++;
	
	if (_Rewinder != NULL && tail == _Rewinder)
	{
#if DEBUG_FIFO
		sipdebug("%p front rewind!", this);
#endif

		// need to rewind
		tail = _Buffer;
	}

	TFifoSize size = *(TFifoSize *)tail;

#if DEBUG_FIFO
	sipdebug("%p front(%d)", this, size);
#endif

	tail += sizeof (TFifoSize);

	//buffer.resize (size);
	//CFastMem::memcpy (&(buffer[0]), tail, size);

	buffer.fill (tail, size);

	// stat code
	TTicks after = CTime::getPerformanceTime ();
	_FrontedTime += after - before;

#if DEBUG_FIFO
	display ();
#endif*/
}

void CBufFIFO::front (uint8 *&buffer, uint32 &s)
{
#if STAT_FIFO
	TTicks before = CTime::getPerformanceTime ();
#endif

	uint8	*tail = _Tail;

	if (empty ())
	{
		sipwarning("BF: Try to get the front of an empty fifo!");
		return;
	}

	_Fronted++;
	
	if (_Rewinder != NULL && tail == _Rewinder)
	{
#if DEBUG_FIFO
		sipdebug("%p front rewind!", this);
#endif

		// need to rewind
		tail = _Buffer;
	}

	s = *(TFifoSize *)tail;

#if DEBUG_FIFO
	sipdebug("%p front(%d)", this, s);
#endif

	tail += sizeof (TFifoSize);

#if STAT_FIFO
	// stat code
	TTicks after = CTime::getPerformanceTime ();
	_FrontedTime += after - before;
#endif

#if DEBUG_FIFO
	display ();
#endif

	buffer = tail;
}



void CBufFIFO::clear ()
{
	_Tail = _Head = _Buffer;
	_Rewinder = NULL;
	_Empty = true;

	_Remained = 0;
}

uint32 CBufFIFO::size ()
{
	if (empty ())
	{
		return 0;
	}
	else if (_Head == _Tail)
	{
		// buffer is full
		if (_Rewinder == NULL)
			return _BufferSize;
		else
			return _Rewinder - _Buffer;
	}
	else if (_Head > _Tail)
	{
		return _Head - _Tail;
	}
	else if (_Head < _Tail)
	{
		sipassert (_Rewinder != NULL);
		return (_Rewinder - _Tail) + (_Head - _Buffer);
	}
	sipstop;
	return 0;
}

void CBufFIFO::resize (uint32 s)
{
#if STAT_FIFO
	TTicks before = CTime::getPerformanceTime();
#endif

	if (s == 0) s = 100;

#if DEBUG_FIFO
	sipdebug("%p resize(%d)", this, s);
#endif

	if (s > _BiggestBuffer) _BiggestBuffer = s;
	if (s < _SmallestBuffer) _SmallestBuffer = s;

	_Resized++;

	uint32 UsedSize = CBufFIFO::size();

	// creer un nouveau tableau et copie l ancien dans le nouveau.
	if (s < _BufferSize && UsedSize > s)
	{
		// probleme, on a pas assez de place pour caser les datas => on fait pas
		sipwarning("BF: Can't resize the FIFO because there's not enough room in the new wanted buffer (%d bytes needed at least)", UsedSize);
		return;
	}

	uint8 *NewBuffer = new uint8[s];
	if (NewBuffer == NULL)
	{
		siperror("Not enough memory to resize the FIFO to %u bytes", s);
	}
#ifdef SIP_DEBUG
	// clear the message to be sure user doesn't use it anymore
	memset (NewBuffer, '-', s);
#endif

#if DEBUG_FIFO
	sipdebug("%p new %d bytes", this, s);
#endif

	// copy the old buffer to the new one
	// if _Tail == _Head => empty fifo, don't copy anything
	if (!empty())
	{
		if (_Tail < _Head)
		{
			CFastMem::memcpy (NewBuffer, _Tail, UsedSize);
		}
		else if (_Tail >= _Head)
		{
			sipassert (_Rewinder != NULL);

			uint size1 = _Rewinder - _Tail;
			CFastMem::memcpy (NewBuffer, _Tail, size1);
			uint size2 = _Head - _Buffer;
			CFastMem::memcpy (NewBuffer + size1, _Buffer, size2);

			sipassert (size1+size2==UsedSize);
		}
	}

	// resync the circular pointer
	// Warning: don't invert these 2 lines position or it ll not work
	_Tail = NewBuffer;
	_Head = NewBuffer + UsedSize;
	_Rewinder = NULL;

	// delete old buffer if needed
	if (_Buffer != NULL)
	{
		delete []_Buffer;
#if DEBUG_FIFO
		sipdebug ("delete", this);
#endif
	}

	// affect new buffer
	_Buffer = NewBuffer;
	_BufferSize = s;

#if STAT_FIFO
	TTicks after = CTime::getPerformanceTime();
	_ResizedTime += after - before;
#endif

#if DEBUG_FIFO
	display ();
#endif
}

void CBufFIFO::displayStats (CLog *log)
{
	log->displayNL ("%p CurrentQueueSize: %d, TotalQueueSize: %d", this, size(), _BufferSize);
	log->displayNL ("%p InQueue: %d", this, _Pushed - _Poped);

	log->displayNL ("%p BiggestBlock: %d, SmallestBlock: %d", this, _BiggestBlock, _SmallestBlock);
	log->displayNL ("%p BiggestBuffer: %d, SmallestBuffer: %d", this, _BiggestBuffer, _SmallestBuffer);
	log->displayNL ("%p Pushed: %d, PushedTime: total %" SIP_I64 "d ticks, mean %f ticks", this, _Pushed, _PushedTime, (_Pushed>0?(double)(sint64)_PushedTime / (double)_Pushed:0.0));
	log->displayNL ("%p Fronted: %d, FrontedTime: total %" SIP_I64 "d ticks, mean %f ticks", this, _Fronted, _FrontedTime, (_Fronted>0?(double)(sint64)_FrontedTime / (double)_Fronted:0.0));
	log->displayNL ("%p Resized: %d, ResizedTime: total %" SIP_I64 "d ticks, mean %f ticks", this, _Resized, _ResizedTime, (_Resized>0?(double)(sint64)_ResizedTime / (double)_Resized:0.0));
}

void CBufFIFO::display ()
{
	int s = 64;
	int gran = s/30;

	char str[4096];

	smprintf(str, 4096, "%p %p (%5d %5d) %p %p %p ", this, _Buffer, _BufferSize, CBufFIFO::size(), _Rewinder, _Tail, _Head);

	int i;
	for (i = 0; i < (sint32) _BufferSize; i+= gran)
	{
		uint8 *pos = _Buffer + i;
		if (_Tail >= pos && _Tail < pos + gran)
		{
			if (_Head >= pos && _Head < pos + gran)
			{
				if (_Rewinder != NULL && _Rewinder >= pos && _Rewinder < pos + gran)
				{
					strncat (str, "*", 1024);
				}
				else
				{
					strncat (str, "@", 1024);
				}
			}
			else
			{
				strncat (str, "T", 1024);
			}
		}
		else if (_Head >= pos && _Head < pos + gran)
		{
			strncat (str, "H", 1024);
		}
		else if (_Rewinder != NULL && _Rewinder >= pos && _Rewinder < pos + gran)
		{
			strncat (str, "R", 1024);
		}
		else
		{
			if (strlen(str) < 1023)
			{
				uint32 p = strlen(str);
				if (isprint(*pos))
					str[p] = *pos;
				else
					str[p] = '$';

				str[p+1] = '\0';
			}
		}
	}

	for (; i < s; i+= gran)
	{
		strncat (str, " ", 1024);
	}
#ifdef SIP_DEBUG
	strncat (str, "\n", 1024);
#else
	strncat (str, "\r", 1024);
#endif
	DebugLog->display (str);
}

bool CBufFIFO::canFit (uint32 s)
{
	if (_Tail == _Head)
	{
		if (empty())
		{
			// is the buffer large enough?
			if (_BufferSize >= s)
			{
				// reset the pointer
#if DEBUG_FIFO
				sipdebug("%p reset tail and head", this);
#endif
				_Head = _Tail = _Buffer;
				return true;
			}
			else
			{
				// buffer not big enough
#if DEBUG_FIFO
				sipdebug("%p buffer full buffersize<size", this);
#endif
				return false;
			}
		}
		else
		{
			// buffer full
#if DEBUG_FIFO
			sipdebug("%p buffer full h=t", this);
#endif
			return false;
		}
	}
	else if (_Tail < _Head)
	{
		if (_Buffer + _BufferSize - _Head >= (sint32) s)
		{
			// can fit after _Head
#if DEBUG_FIFO
			sipdebug("%p fit after", this);
#endif
			return true;
		}
		else if (_Tail - _Buffer >= (sint32) s)
		{
			// can fit at the beginning
#if DEBUG_FIFO
			sipdebug("%p fit at beginning", this);
#endif
			_Rewinder = _Head;
#if DEBUG_FIFO
			sipdebug("%p set the rewinder", this);
#endif
			_Head = _Buffer;
			return true;
		}
		else
		{
			// can't fit
#if DEBUG_FIFO
			sipdebug("%p no room t<h", this);
#endif
			return false;
		}
	}
	else // the last case is : if (_Tail > _Head)
	{
		if (_Tail - _Head >= (sint32) s)
		{
#if DEBUG_FIFO
			sipdebug("%p fit t>h", this);
#endif
			return true;
		}
		else
		{
#if DEBUG_FIFO
			sipdebug("%p no room t>h", this);
#endif
			return false;
		}
	}
	sipstop;
}

#ifdef BUFFIFO_TRACK_ALL_BUFFERS
SIPBASE_CATEGORISED_COMMAND(sip, dumpAllBuffers, "Dump all the fifo buffer", "no args")
{
	log.displayNL("Dumping %u FIFO buffers :", CBufFIFO::_AllBuffers.size());

	CBufFIFO::TAllBuffers::iterator first(CBufFIFO::_AllBuffers.begin()), last(CBufFIFO::_AllBuffers.end());
	for (; first != last; ++first)
	{
		CBufFIFO *buf = *first;
		
		log.displayNL("Dumping buffer %p:", buf);

		buf->displayStats(&log);
	}

	return true;
}
#endif


} // SIPBASE
