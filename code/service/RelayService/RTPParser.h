#pragma once

#include "misc/types_sip.h"

class CRTPParser
{
public:
	static	int		getContribSrcCount(uint8* rtpHeader);
	static	int		getHeaderSize(uint8* rtpHeader);
	static	uint	getVersion(uint8* rtpHeader);
	static	bool	getExtension(uint8* rtpHeader);
	static	bool	getMarker(uint8* rtpHeader);
	static	uint	getPayloadType(uint8* rtpHeader);
	static	uint16	getSequenceNumber(uint8* rtpHeader);
	static	uint32	getTimestamp(uint8* rtpHeader);
	static	uint32	getSyncSource(uint8* rtpHeader); 
	static	uint32	getContribSource(uint8* rtpHeader, int idx);
};
