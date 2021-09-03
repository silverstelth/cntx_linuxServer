/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BUF_FIFO_H__
#define __BUF_FIFO_H__

#include "types_sip.h"
#include <vector>
#include "time_sip.h"
#include "mem_stream.h"
#include "command.h"


namespace SIPBASE {

#undef BUFFIFO_TRACK_ALL_BUFFERS


/**
 * This class is a dynamic size FIFO that contains variable size uint8 buffer.
 * It's used in the layer1 network for storing temporary messages.
 * You can resize the internal FIFO buffer if you know the average size
 * of data you'll put in it. It have the same behavior as STL so if the
 * buffer is full the size will be automatically increase by 2.
 *
 * TODO: Add a method getMsgNb() that will return the number of messages in queue.
 * For acceptable performance, it would need to store the current number instead
 * of browsing the blocks.
 *
 * \code
 	CBufFIFO fifo;
	fifo.resize(10000);
	vector<uint8> vec;
	vec.resize(rand()%256);
	memset (&(vec[0]), '-', vec.size());
	// push the vector
	fifo.push(vec);
	// display the fifo
	fifo.display();
	vector<uint8> vec2;
	// get the vector
	fifo.pop(vec2);
 * \endcode
 * \date 2001
 */
class CBufFIFO
{
public:

	CBufFIFO ();
	~CBufFIFO ();

	/// Push 'buffer' in the head of the FIFO
	void	 push (const std::vector<uint8> &buffer) { push (&buffer[0], buffer.size()); }

	void	 push (const SIPBASE::CMemStream &buffer) { push (buffer.buffer(), buffer.length()); }

	void	 push (const uint8 *buffer, uint32 size);

	/// Concate and push 'buffer1' and buffer2 in the head of the FIFO. The goal is to avoid a copy
	void	 push (const std::vector<uint8> &buffer1, const std::vector<uint8> &buffer2);

	/// Get the buffer in the tail of the FIFO and put it in 'buffer'
	void	 front (std::vector<uint8> &buffer);

	void	 front (SIPBASE::CMemStream &buffer);

	void	 front (uint8 *&buffer, uint32 &size);

	/// This function returns the last byte of the front message
	/// It is used by the network to know a value quickly without doing front()
	uint8	 frontLast ();


	/// Pop the buffer in the tail of the FIFO
	void	 pop ();

	/// Set the size of the FIFO buffer in byte
	void	 resize (uint32 size);

	/// Return true if the FIFO is empty
	bool	 empty () { return _Empty; }


	/// Erase the FIFO
	void	 clear ();

	/// Returns the size of the FIFO
	uint32	 size ();

	/// display the FIFO to stdout (used to debug the FIFO)
	void	 display ();

	/// display the FIFO statistics (speed, nbcall, etc...) to stdout
	void	 displayStats (CLog *log = InfoLog);

	// KSR_ADD
	uint32	fifoSize() { return	_Remained; };

	uint32	lastPoped() { return	_Poped; };

	uint32	lastPushed() { return	_Pushed; };

private:

	typedef uint32 TFifoSize;

	// pointer to the big buffer
	uint8	*_Buffer;
	// size of the big buffer
	uint32	 _BufferSize;

	// true if the bufffer is empty
	bool	 _Empty;

	// head of the FIFO
	uint8	*_Head;
	// tail of the FIFO
	uint8	*_Tail;
	// pointer to the rewinder of the FIFO
	uint8	*_Rewinder;

	// return true if we can put size bytes on the buffer
	// return false if we have to resize
	bool	 canFit (uint32 size);


	// statistics of the FIFO
	uint32 _BiggestBlock;
	uint32 _SmallestBlock;
	uint32 _BiggestBuffer;
	uint32 _SmallestBuffer;
	uint32 _Pushed ;
	uint32 _Poped;
	uint32 _Remained;
	uint32 _Fronted;
	uint32 _Resized;
	TTicks _PushedTime;
	TTicks _FrontedTime;
	TTicks _ResizedTime;

#ifdef BUFFIFO_TRACK_ALL_BUFFERS
	typedef std::set<CBufFIFO*> TAllBuffers;
	// All the buffer for debug output
	static TAllBuffers		_AllBuffers; // WARNING: not mutexed, can produce some crashes!
#endif

	SIPBASE_CATEGORISED_COMMAND_FRIEND(sip, dumpAllBuffers);
};

/// Synchronized FIFO buffer
typedef CSynchronized<SIPBASE::CBufFIFO> CSynchronizedFIFO;

/// Accessor of mutexed FIFO buffer
typedef CSynchronizedFIFO::CAccessor CFifoAccessor;

} // SIPBASE


#endif // __BUF_FIFO_H__

/* End of buf_fifo.h */
