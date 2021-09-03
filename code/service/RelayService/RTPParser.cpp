#include "RTPParser.h"

// RTP packet�� CSRC Count bit setting
int		CRTPParser::getContribSrcCount(uint8* rtpHeader) 
{
	return rtpHeader[0] & 0xF;
} 

//  RTP Packet�߿��� Fixed Header Size�� ��󳽴� 
int		CRTPParser::getHeaderSize(uint8* rtpHeader)
{
	return 12 + 4 * getContribSrcCount(rtpHeader);
}

//  RTP Version��(2)  return
//  8 bit�� ������  2bit�� ���´�
uint	CRTPParser::getVersion(uint8* rtpHeader)
{
	return (rtpHeader[0]>>6) & 3;	//����buffer��  RTP Version ������
}

//  Header�� Ȯ�� bit return
bool	CRTPParser::getExtension(uint8* rtpHeader)
{
	return (rtpHeader[0]&0x10) != 0;	//����buffer�� Extension bit ������
}

//  RTP Packet Marker bit
bool	CRTPParser::getMarker(uint8* rtpHeader) 
{
	return (rtpHeader[1] & 0x80) != 0;
}

//  RTP Packet�� Payload m_type return
uint	CRTPParser::getPayloadType(uint8* rtpHeader)  
{
	return rtpHeader[1] & 0x7f;
}

uint16	CRTPParser::getSequenceNumber(uint8* rtpHeader) 
{
	return *(uint16 *)&rtpHeader[2];
}

uint32	CRTPParser::getTimestamp(uint8* rtpHeader) 
{
	return *(uint32 *)&rtpHeader[4];
}

uint32	CRTPParser::getSyncSource(uint8* rtpHeader) 
{
	return *(uint32 *)&rtpHeader[8];
}

uint32	CRTPParser::getContribSource(uint8* rtpHeader, int idx) 
{
	return ((uint32 *)&rtpHeader[12])[idx];
}
