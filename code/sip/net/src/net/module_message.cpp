/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/module_message.h"

// a stupid function to remove some more stupid visual warnings
void foo_module_message() {};

namespace SIPNET
{
//	CModuleMessage::CModuleMessage(const CMessage &messageBody)
//		: MessageType(mt_invalid),
//		SenderModuleId(INVALID_MODULE_ID),
//		AddresseeModuleId(INVALID_MODULE_ID),
//		MessageBody(const_cast<CMessage&>(messageBody))
//	{
//
//	}
//
//
//	void CModuleMessage::serial(SIPBASE::IStream &s)
//	{
//		sipassert(mt_num_types < 0xFF);
//		sipassert(MessageType != mt_invalid);
//
//		s.serialBitField8(reinterpret_cast<uint8&>(MessageType));
//		s.serial(SenderModuleId);
//		s.serial(AddresseeModuleId);
//		MessageBody.serialMessage()
//	}


} // namespace SIPNET
