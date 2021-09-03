
// This file include common data both in ClientUI Update and NetManager!

#ifndef __UPDATEUIDATA_H__
#define __UPDATEUIDATA_H__


struct _infoShard
{
	uint16			shardId;
	ucstring		ucName;
	uint32			nbPlayers;
	ucstring		ucContent;
};

typedef	std::vector<_infoShard>		TShardAry;

#endif	//__UPDATEUIDATA_H__