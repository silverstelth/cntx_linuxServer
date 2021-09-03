/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TKTYPES_H__
#define __TKTYPES_H__

namespace SIPBASE
{

typedef sint64             TKLong;
typedef sint64             TKLongID;
typedef sint64             TKRefCount;
typedef uint64    TKULong;

typedef uint32               TKDATE;

// undef const
const int   UNDEF_INT     = 0x80000000;
const uint32 UNDEF_DWORD   = 0xFFFFFFFF;


typedef struct TKuuid
{
    sint64     m_lo;
    sint64     m_hi;
}
    TKuuid;

#define /*BOOL*/ TKuuidEQ(u1,u2) (memcmp( u1, u2, sizeof(TKuuid) )==0)

struct TKTIME
{
    int     m_hour;
    int     m_minute;
    int     m_second;
    int     m_milliseconds;
};

// The TKTime structure holds an unsigned 64-bit date and time value.
// This value could represents the number of 100-nanosecond units since the beginning of January 1, 1601.
#ifdef SIP_OS_WINDOWS
class TKTime
{
public:
    TKTime( void );
    TKTime( LPCTSTR szGetSyWORDstemTimeDummy );  // szGetSystemTimeDummy is not used it's just to Get the system time in the constructor
    TKTime( const TKTime& time );
    TKTime( const FILETIME& fileTime );
    TKTime( const SYSTEMTIME& sysTime );
    TKTime( uint32 days, uint32 hours, uint32 minutes, uint32 seconds, uint32 millisseconds );
    TKTime( uint16 year, uint16 month, uint16 day, uint16 hour, uint16 minute, uint16 second );
    TKTime( TKLong time );

    ~TKTime( void ) {}

    TKTime& operator =( TKLong time );
    TKTime& operator +=( TKLong time );
    TKTime& operator -=( TKLong time );
    TKTime& operator /=( TKLong time );
    TKTime& operator *=( TKLong time );

    operator FILETIME( void );
    operator SYSTEMTIME( void );
    operator TKTIME( void );
    operator TKLong( void );

    TKTime& Set( uint32 days, uint32 hours, uint32 minutes, uint32 seconds, uint32 milliseconds );
    TKTime& Set( uint16 year, uint16 month, uint16 day, uint16 hour, uint16 minute, uint16 second );
    TKTime& Set( uint32 milliseconds );  // becarefull you set some milliseconds
    TKTime& SetSystemTime( void );

    void LocalToUTC( void );
    void UTCToLocal( void );

private:
    TKLong m_time;
};
#elif defined(SIP_OS_UNIX) // KSR_ADD 2010_2_2
//#error	(Sorry for your inconvenience. For Linux, TKTime is not prepared.)
#endif //SIP_OS_WINDOWS

}
#endif // end of guard __TKTYPES_H__
