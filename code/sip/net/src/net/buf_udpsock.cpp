/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/hierarchical_timer.h"
#include "net/buf_udpsock.h"
#include "net/buf_server.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#elif defined SIP_OS_UNIX
#include <netinet/in.h>
#endif

using namespace SIPBASE;
using namespace std;

namespace SIPNET 
{
/*
 * Constructor
 */
CBufUdpSock::CBufUdpSock() :
	_LastFlushTime( 0 ),	_TriggerTime( 0 ),	_TriggerSize( -1 ),
	_SendFifoMutex("CBufUdpSock::_SendFifoAccessor"), _RecvedQueueMutex("CBufUdpSock::_RecvedQueueMutex"),
	_SendedBuffer("CBufUdpSock::_SendedBuffer"), _ResendTime( 2000 ), _CurAllIndex(1), 
	_RecvedAllPacket("CBufUdpSock::_RecvedAllPacket"), _Length(0)
{
	_LastFlushTime = CTime::getLocalTime();

	for (uint8 i = 0; i < UDPTYPENUM; i++)
		_SendFifo[i].curIndex = 0;
	for (uint8 i = 0; i < UDPTYPENUM; i++)
		_RecvedPacket[i].curIndex = 0;

}

/*
 * Destructor
 */
CBufUdpSock::~CBufUdpSock()
{
	sipassert (this != NULL);

	_LastFlushTime = 0;
	_TriggerTime = 0;
	_TriggerSize = 0;
}

bool CBufUdpSock::flush()
{
	// ������ ���� ���� ����Ʈ���� �����Ѵ�
	{
		SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	sendedBuffer( &_SendedBuffer );
		TLSTUDPBUFFER::iterator bi;
		for ( bi = sendedBuffer.value().begin(); bi != sendedBuffer.value().end(); bi++ )
		{
			
			if ( bi->second.uRepeatNum >= 30 )	// 30���� �������Ͽ�����
			{
				sendedBuffer.value().erase(bi);
				sipdebug ("LNETL1: There is no response UDP datagram");
				break;
			}
		}
	}
	// �������Ͽ��� �ϴ� �޽������� �������Ѵ�
	{
		SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	sendedBuffer( &_SendedBuffer );

		TLSTUDPBUFFER::iterator bi;
		for ( bi = sendedBuffer.value().begin(); bi != sendedBuffer.value().end(); bi++ )
		{
			TTime now = CTime::getLocalTime();
			TTime delta = now - bi->second.actiontime;
			if ( delta >= _ResendTime )
			{
				uint32	uSize = bi->second.buffer.size();
				send(bi->second.buffer.buffer(), uSize);
				bi->second.actiontime = now;
				bi->second.uRepeatNum ++;
				sipdebug( "LNETL1: resend datagram all index = %d", *(TUDPALLINDEX*)(bi->second.buffer.buffer() + UDPHEADERPOS2));
			}
		}
	}
	// Buffer�� ���� �޽������� �۽��Ѵ�
	_SendFifoMutex.enter();
	for (uint32 i = 0; i < UDPTYPENUM; i++)
	{
		uint32	uSize = UDPMESPOS;
		while ( !_SendFifo[i].buffer.empty() )
		{
			uint8 *tmpbuffer;
			uint32 size;
			_SendFifo[i].buffer.front( tmpbuffer, size );
			sipassert(size < MAX_UDPDATAGRAMLEN);
			if (uSize+size >= MAX_UDPDATAGRAMLEN)
				break;

			// �ڷ��� ���������� ���δ�
			*(TBlockSize*)&(_ReadyToSendBuffer[uSize]) = size;
			CFastMem::memcpy (&_ReadyToSendBuffer[uSize+sizeof(TBlockSize)], tmpbuffer, size);
			uSize += sizeof(TBlockSize) + size;
			_SendFifo[i].buffer.pop();
		}
		if (uSize == UDPMESPOS)
			continue;

		// �۽�����Ʈ�� Header�θ� �ۼ��Ѵ�
		// Tcpid�ۼ�
		
		// ��ü�������ۼ�
		*(TUDPALLINDEX*)&(_ReadyToSendBuffer[UDPHEADERPOS2]) = _CurAllIndex;
		// ����Ʈtype�ۼ�
		*(TUDPTYPE*)&(_ReadyToSendBuffer[UDPHEADERPOS3]) = (uint8)(UDP_TYPESTART + i + 1);
		// �������ۼ�
		*(TUDPINDEX*)&(_ReadyToSendBuffer[UDPHEADERPOS4]) = _SendFifo[i].curIndex;
		_SendFifo[i].curIndex ++;

		// �ŷڼ��� ����ǿ��� �ϴ� ����Ʈ�̸� ������Buffer�� �߰��Ѵ�
		if (i != UINDEX(UDP_UNRELIABILITY) && i != UINDEX(UDP_RESPONSE))
		{
			TUDPBUFFER newData;
			newData.buffer.resize(uSize);
			newData.buffer.serialBuffer( _ReadyToSendBuffer, uSize );
			newData.actiontime = CTime::getLocalTime();
			newData.uRepeatNum = 0;
			SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	sendedBuffer( &_SendedBuffer );
			sendedBuffer.value().insert(make_pair(_CurAllIndex, newData));
		}
		send(_ReadyToSendBuffer, uSize);
		// ��ü������ ����
		_CurAllIndex ++;
		if (_CurAllIndex == 0)
			_CurAllIndex = 1;
	}
	_SendFifoMutex.leave();

	return true;
}

void CBufUdpSock::setTimeFlushTrigger( sint32 ms )
{
	sipassert (this != NULL);
	_TriggerTime = ms;
	_LastFlushTime = CTime::getLocalTime();
}

bool CBufUdpSock::update()
{
	sipassert (this != NULL);

	// Time trigger
	if ( _TriggerTime != -1 )
	{
		TTime now = CTime::getLocalTime();
		if ( (sint32)(now-_LastFlushTime) >= _TriggerTime )
		{
			if ( flush() )
			{
				_LastFlushTime = now;
				return true;
			}
			else
				return false;
		}
	}
	// Size trigger
	if ( _TriggerSize != -1 )
	{
		sint32	bufsize = 0;
		_SendFifoMutex.enter();
		for (uint8 i = 0; i < UDPTYPENUM; i++)
		{
			bufsize += (sint32)_SendFifo[i].buffer.size();
		}
		_SendFifoMutex.leave();

		if ( bufsize > _TriggerSize )
			return flush();
	}

	return true;
}

bool	CBufUdpSock::pushBuffer( uint8 byType, const SIPBASE::CMemStream& buffer )
{
	sipassert( byType > UDP_TYPESTART && byType < UDP_TYPE_END );

	_SendFifoMutex.enter();
	_SendFifo[UINDEX(byType)].buffer.push(buffer);
	_SendFifoMutex.leave();

	bool	res = update();

	return res;
}

// ������ ����Ʈ�� ó���Ѵ�
void	CBufUdpSock::AddRecvPacket(uint8 *mes, uint32 uSize)
{
	// �������缺�� �˻��Ѵ�
	if (uSize < UDPMESPOS+sizeof(TBlockSize))
	{
		// ����Ʈ�� ������ �������̴�
		return;
	}

	TUDPTCPID	tcpid = *(TUDPTCPID *)&mes[UDPHEADERPOS1];
	TUDPALLINDEX	allindex = *(TUDPALLINDEX *)&mes[UDPHEADERPOS2];
	TUDPTYPE		packettype = *(TUDPTYPE *)&mes[UDPHEADERPOS3];
	TUDPINDEX		pindex = *(TUDPINDEX *)&mes[UDPHEADERPOS4];


	// ����Ʈ�� ���°��˻�
	if ( !(packettype > UDP_TYPESTART && packettype < UDP_TYPE_END) )
		return;
	
	// for debug
/*
	{
		TTime t = SIPBASE::CTime::getLocalTime();
		if (packettype != UDP_RESPONSE)
		{
			sipdebug( "receive datagram all index = %d, time = %d", allindex, t);
		}
		else
		{
			TUDPALLINDEX		index = *(TUDPALLINDEX *)&mes[UDPMESPOS+sizeof(TBlockSize)];
			sipdebug( "receive datagram all index = %d(response index = %d), time = %d", allindex, index, t);
		}
	}
*/
	// �˻��հ��� ���� ���缺������ �����Ѵ�
	
	// ������ ������
	if (packettype != UDP_UNRELIABILITY && packettype != UDP_RESPONSE)
	{
		CMemStream	resMem;
		resMem.serial(allindex);
		pushBuffer(UDP_RESPONSE, resMem);
	}

	{	
		SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	recvAllPacket( &_RecvedAllPacket );
		// �ߺ����� �ʾҴ°��� �˻��ϰ� �ߺ��ǿ����� Ż���Ѵ�
		TLSTUDPBUFFER::iterator doublei = recvAllPacket.value().find(allindex);
		if (doublei != recvAllPacket.value().end())
		{
			sipdebug ("LNETL1: Duplicate UDP datagram index %d", allindex);
			return;
		}
		// Recv buffer�� �߰��Ѵ�
		TUDPBUFFER newData;
		newData.buffer.resize(uSize);
		newData.buffer.serialBuffer( &mes[0], uSize );
		newData.actiontime = CTime::getLocalTime();
		recvAllPacket.value().insert(make_pair(allindex, newData));
	}

	// ����Ʈ���� �κ��ڷ���� �����Ѵ�
	TVU8	mesData;
	mesData.resize(uSize-UDPHEADERPOS4);
	CFastMem::memcpy (&mesData[0], &mes[UDPHEADERPOS4], uSize-UDPHEADERPOS4);
	_RecvedQueueMutex.enter();
	_RecvedPacket[UINDEX(packettype)].buffer.push_back(mesData);
	_RecvedQueueMutex.leave();

	RefreshCompleteBuffer();
}

// ����Ʈ�� �κ��ڷ���� �м��Ͽ� �ϼ��� ����Ʈ��Ͽ� �߰��Ѵ�
void	CBufUdpSock::AddPartPacket(vector<uint8> &mes, uint32 uSize)
{
	uint32	uCurPos = UDPMESPOS - UDPHEADERPOS4;
	while (uCurPos < uSize)
	{
		TBlockSize sz = *(TBlockSize *)&mes[uCurPos];
		uCurPos += sizeof(TBlockSize);
		if (uCurPos + sz <= uSize)
		{
			TVU8	newData;
			newData.resize(sz);
			CFastMem::memcpy (&newData[0], &mes[uCurPos], sz);
			_RecvComplete.push(newData);
			uCurPos += sz;
		}
		else
		{
			// ����Ʈ�� ������ �������̴�
			break;
		}
	}
	
}
// ����Ʈ���� ������ �����Ѵ�
// return value (0: equal), (1: i1 > i2), (2: i2 > i1)
static uint8	OrderUDP_REL_UPORDER(TUDPINDEX i1, TUDPINDEX i2)
{
	if (i1 == i2)
		return 0;
	sipassert(sizeof(TUDPINDEX) >= 2)
	uint32	uDelta = 16384;
	if (i1 < 16384 && i2 > 49152) // 49152 = 65536 - 16384
		return 1;
	if (i2 < 16384 && i1 > 49152) // 49152 = 65536 - 16384
		return 2;
	if (i1 > i2)
		return 1;
	return 2;
}

// ����buffer(����Ʈ���º�)�鿡�� �ϼ��� ����Ʈ���� ������ �ϼ�����Ʈ��Ͽ� �߰��Ѵ�
void	CBufUdpSock::RefreshCompleteBuffer()
{
	_RecvedQueueMutex.enter();

	// UDP����Ư�� �״���� �ŷڼ� ���� ����Ʈ�鿡 ���� ó��
	TUDPRECVQUEUE *rcBuffer;
	TBUFFER::iterator it;
	rcBuffer = &_RecvedPacket[UINDEX(UDP_UNRELIABILITY)];
	for (it = rcBuffer->buffer.begin(); it != rcBuffer->buffer.end(); it++)
		AddPartPacket( (*it), (*it).size());
	rcBuffer->buffer.clear();

	// �ŷڼ��� ����ǰ� ������ �������� ����Ʈ�鿡 ���� ó��
	rcBuffer = &_RecvedPacket[UINDEX(UDP_REL_NOORDER)];
	for (it = rcBuffer->buffer.begin(); it != rcBuffer->buffer.end(); it++)
		AddPartPacket( (*it), (*it).size());
	rcBuffer->buffer.clear();

	// �ŷڼ��� ����ǰ� ���� ������ ����Ʈ�� �����ϴ� ����Ʈ�鿡 ���� ó��
	rcBuffer = &_RecvedPacket[UINDEX(UDP_REL_UPORDER)];
	while (rcBuffer->buffer.size() > 0)
	{
		it = rcBuffer->buffer.begin();
		TUDPINDEX uIndex = *(TUDPINDEX *)&(*it)[0];
		if (OrderUDP_REL_UPORDER(uIndex, rcBuffer->curIndex) != 2)
		{
			AddPartPacket((*it), (*it).size());
			rcBuffer->curIndex = uIndex + 1;
		}
		rcBuffer->buffer.erase(it);
	}

	// �ŷڼ��� ����ǰ� �������� ����ǿ��� �ϴ� ����Ʈ�鿡 ���� ó��
	rcBuffer = &_RecvedPacket[UINDEX(UDP_REL_ORDER)];
	bool	bFind = true;
	while (bFind)
	{
		bFind = false;
		for (it = rcBuffer->buffer.begin(); it != rcBuffer->buffer.end(); it++)
		{
			TUDPINDEX uIndex = *(TUDPINDEX *)&(*it)[0];
			if (rcBuffer->curIndex == uIndex)
			{
				bFind = true;
				AddPartPacket((*it), (*it).size());
				rcBuffer->curIndex ++;
				rcBuffer->buffer.erase(it);
				break;
			}
		}
		if (bFind == false && rcBuffer->buffer.size() > 0)
		{
		}
	}

	// ������������Ʈ�鿡 ���� ó��
	rcBuffer = &_RecvedPacket[UINDEX(UDP_RESPONSE)];
	for (it = rcBuffer->buffer.begin(); it != rcBuffer->buffer.end(); it++)
	{
		uint32	uCurPos = UDPMESPOS - UDPHEADERPOS4;
		uint32	uSize = (*it).size();
		while (uCurPos < uSize)
		{
			TBlockSize sz = *(TBlockSize *)&((*it)[uCurPos]);
			uCurPos += sizeof(TBlockSize);
			if (uCurPos + sz <= uSize)
			{
				TUDPALLINDEX resIndex = *(TUDPALLINDEX *)&((*it)[uCurPos]);
				ProcResOfSended(resIndex);
				uCurPos += sz;
			}
			else
			{
				// ����Ʈ�� ������ �������̴�
				break;
			}
		}

	}
	rcBuffer->buffer.clear();

	_RecvedQueueMutex.leave();
	return;
}

bool	CBufUdpSock::receivePart( uint32 nbExtraBytes )
{
	if (_RecvComplete.empty())
		return false;

	uint8 *tmpbuffer;
	uint32 sz;
	_RecvComplete.front( tmpbuffer, sz );

	_ReceiveBuffer.resize (sz+nbExtraBytes);
	CFastMem::memcpy (&_ReceiveBuffer[0], tmpbuffer, sz);
	_RecvComplete.pop();

	return true;
}

// �������信 ���Ͽ� ��۽�Buffer�� �����Ѵ�
void	CBufUdpSock::ProcResOfSended(TUDPALLINDEX resIndex)
{
	SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	sendedBuffer( &_SendedBuffer );
	TLSTUDPBUFFER::iterator bi = sendedBuffer.value().find(resIndex);

	if (bi == sendedBuffer.value().end())
		return;

	const uint8 *pBuffer = bi->second.buffer.buffer();

	TUDPTCPID	tcpid = *(TUDPTCPID *)&pBuffer[UDPHEADERPOS1];
	TUDPALLINDEX	allindex = *(TUDPALLINDEX *)&pBuffer[UDPHEADERPOS2];
	TUDPTYPE		packettype = *(TUDPTYPE *)&pBuffer[UDPHEADERPOS3];
	TUDPINDEX		pindex = *(TUDPINDEX *)&pBuffer[UDPHEADERPOS4];

	sendedBuffer.value().erase(bi);
	
	// �ŷڼ��� ����ǰ� ���� ������ ����Ʈ�� �����ϴ� ����Ʈ�� ���� ������ ���
	// ���� ������ ����Ʈ���� ��۽�buffer���� �����Ѵ�
	if (packettype != UDP_REL_UPORDER)
		return;
	bool	bChange = true;
	while (bChange)
	{
		bChange = false;
		for ( bi = sendedBuffer.value().begin(); bi != sendedBuffer.value().end(); bi++ )
		{
			const uint8 *pBuffer2 = bi->second.buffer.buffer();
			TUDPTYPE	packettype2 = *(TUDPTYPE *)&pBuffer2[UDPHEADERPOS3];
			TUDPINDEX	pindex2 = *(TUDPINDEX *)&pBuffer2[UDPHEADERPOS4];
			if (packettype2 == UDP_REL_UPORDER)
			{
				if (OrderUDP_REL_UPORDER(pindex, pindex2) != 2)
				{
					sendedBuffer.value().erase(bi);
					bChange = true;
					break;
				}
			}
		}
	}
	
}

// ����Ʈ�� �ݺ������� �����ϱ� ���� ����buffer�� 1�п� �ѹ��� refresh�Ѵ�
void	CBufUdpSock::RefreshRecvAllPacket()
{
	SIPBASE::CSynchronized<TLSTUDPBUFFER>::CAccessor	recvAllPacket( &_RecvedAllPacket );

	bool	bChange = true;
	while (bChange)
	{
		bChange = false;
		TLSTUDPBUFFER::iterator it;
		for (it = recvAllPacket.value().begin(); it != recvAllPacket.value().end(); it ++)
		{
			TTime now = CTime::getLocalTime();
			if (now - it->second.actiontime > 60000)	// 1�е��ȸ� �ߺ�����Ʈ�˻縦 �����Ѵ�.
			{
				bChange = true;
				recvAllPacket.value().erase(it);
				break;
			}
		}
	}	
}

// ��������Ʈ�� ����� ��� �����Ѵ�
void	CBufUdpSock::SendResponsePacket(TUDPALLINDEX allindex, TUDPINDEX pindex, TUDPALLINDEX resindex, CUdpSimSock *sock, CInetAddress addr)
{
	SIPBASE::CObjectVector<uint8>				SendPacket;
	SendPacket.resize(UDPMESPOS+sizeof(TBlockSize)+sizeof(TUDPALLINDEX));
	
	// Checksum�ۼ�

	// ��ü�������ۼ�
	*(TUDPALLINDEX*)&(SendPacket[UDPHEADERPOS2]) = allindex;
	// ����Ʈtype�ۼ�
	*(TUDPTYPE*)&(SendPacket[UDPHEADERPOS3]) = UDP_RESPONSE;
	// �������ۼ�
	*(TUDPINDEX*)&(SendPacket[UDPHEADERPOS4]) = pindex;
	
	// �����ڷ��� ���̰�
	*(TBlockSize*)&(SendPacket[UDPMESPOS]) = sizeof(TUDPALLINDEX);
	// ���䰪
	*(TUDPALLINDEX*)&(SendPacket[UDPMESPOS+sizeof(TBlockSize)]) = resindex;
	
	if (sock != NULL)
	{
		uint32 len = SendPacket.size();
		sock->sendTo( &SendPacket[0], len, addr );
//		sipdebug( "Send response datagram_1 index = %d", resindex);
	}
	else
		send( &SendPacket[0], SendPacket.size() );
}

CBufUdpClient::CBufUdpClient() :
	CBufUdpSock()
{
	Sock = new CUdpSimSock(false);
}

CBufUdpClient::~CBufUdpClient()
{
	if (Sock != NULL)
		delete Sock;

	Sock = NULL;
}

void CBufUdpClient::send(const uint8 *buffer, uint32 len)
{
	sipassert(Sock != NULL);
	try
	{
		*(uint32 *)buffer = idInServer;
		Sock->send( buffer, len );
	}
	catch (Exception &e)
	{
		sipwarning ("LNETL1: Exception catched: '%s'", e.what());
	}
}

void CBufUdpServer::send(const uint8 *buffer, uint32 len)
{
	sipassert(ServerSock != NULL);
	try
	{
		// *(uint32 *)buffer = (uint32)((TTcpSockId *)TcpFriend);
		uint32 buf = *(uint32*)buffer;
		buf = (uint32)(uintptr_t)TcpFriend;
		ServerSock->sendTo( buffer, len, ClientAddress );
	}
	catch (Exception &e)
	{
		sipwarning ("LNETL1: Exception catched: '%s'", e.what());
	}
}

} // SIPNET

