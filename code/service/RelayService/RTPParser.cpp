#include "RTPParser.h"

// RTP packet의 CSRC Count bit setting
int		CRTPParser::getContribSrcCount(uint8* rtpHeader) 
{
	return rtpHeader[0] & 0xF;
} 

//  RTP Packet중에서 Fixed Header Size만 골라낸다 
int		CRTPParser::getHeaderSize(uint8* rtpHeader)
{
	return 12 + 4 * getContribSrcCount(rtpHeader);
}

//  RTP Version값(2)  return
//  8 bit중 오른쪽  2bit를 얻어온다
uint	CRTPParser::getVersion(uint8* rtpHeader)
{
	return (rtpHeader[0]>>6) & 3;	//수신buffer의  RTP Version 얻어오기
}

//  Header의 확장 bit return
bool	CRTPParser::getExtension(uint8* rtpHeader)
{
	return (rtpHeader[0]&0x10) != 0;	//수신buffer의 Extension bit 얻어오기
}

//  RTP Packet Marker bit
bool	CRTPParser::getMarker(uint8* rtpHeader) 
{
	return (rtpHeader[1] & 0x80) != 0;
}

//  RTP Packet의 Payload m_type return
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
