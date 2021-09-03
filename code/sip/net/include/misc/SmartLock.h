/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SMARTLOCK_H__
#define __SMARTLOCK_H__

namespace SIPBASE
{

class SmartLock
{
public:
    SmartLock( LPCRITICAL_SECTION sect ){  EnterCriticalSection( pLock=sect ); };
    ~SmartLock(){ LeaveCriticalSection( pLock ); };
private:
    LPCRITICAL_SECTION      pLock;
};

}
#endif // end of guard __SMARTLOCK_H__
