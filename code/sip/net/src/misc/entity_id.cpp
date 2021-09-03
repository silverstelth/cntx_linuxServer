/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/entity_id.h"

using namespace std;

namespace SIPBASE {

	const uint64 CEntityId::MaxEntityId = ((uint64)1 << (CEntityId::ID_SIZE + 1)) - (uint64)1;

	CEntityId CEntityId::_NextEntityId;

	uint8 CEntityId::_ServerId = 0;

	const CEntityId CEntityId::Unknown (CEntityId::UNKNOWN_TYPE, 0, 0, 0);

} // SIPBASE
