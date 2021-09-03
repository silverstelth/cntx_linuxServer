/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __INET_ADDRESS_H__
#define __INET_ADDRESS_H__

#include "misc/types_sip.h"

#include <string>
#include <vector>


struct sockaddr_in;
struct in_addr;


#ifdef SIP_OS_WINDOWS
// automatically add the win socket library if you use network part
#pragma comment(lib, "ws2_32.lib")
#endif

namespace SIPBASE
{
	class IStream;
}


namespace SIPNET
{

struct ESocket;


/**
 * Internet address (IP + port).
 * The structure sockaddr_in is internally in network byte order
 * \todo cado: Test big/little endian transfers to check if byte ordering is ok.
 * \date 2000
 */
class CInetAddress
{
public:

	/// Default Constructor. The address is set to INADDR_ANY
	CInetAddress();

	/// Alternate constructor (calls setByName())
	CInetAddress( const std::string& hostName, uint16 port );

	/// Alternate constructor (calls setByName())
	/// example: CInetAddress("www.samilpo.com:80")
	CInetAddress( const std::string& hostNameAndPort );
	CInetAddress( const std::basic_string<ucchar>& hostNameAndPort );

	/// Copy constructor
	CInetAddress( const CInetAddress& other );

	/// Assignment operator
	CInetAddress& operator=( const CInetAddress& other );

	/// Comparison == operator
	friend bool operator==( const CInetAddress& a1, const CInetAddress& a2 );

	/// Comparison != operator
	friend bool operator!=( const CInetAddress& a1, const CInetAddress& a2 );

	/// Comparison < operator
	friend bool operator<( const CInetAddress& a1, const CInetAddress& a2 );

	/// Destructor
	~CInetAddress();

	/// Resolves a name
	CInetAddress&		setByName( const std::string& hostname );

	/// Sets port
	void				setPort( uint16 port );

	/// Sets hostname and port (ex: www.samilpo.com:80)
	void				setNameAndPort( const std::string& hostNameAndPort );

	/** Sets internal socket address directly (contents is copied).
	 * It also retrieves the host name if CInetAddress::RetrieveNames is true.
	 */
	void				setSockAddr( const sockaddr_in* saddr );
	 
	/// Returns if object (address and port) is valid
	bool				isValid() const;
	
	/// Returns internal socket address (read only)
	const sockaddr_in	 *sockAddr() const;

	/// Returns internal IP address
	uint32				internalIPAddress() const;

	/// Returns the internal network address (it s the network address for example 192.168.0.0 for a C class) 
	uint32				internalNetAddress () const;

	/// Returns readable IP address. (ex: "195.68.21.195")
	std::string			ipAddress() const;

	/// Returns hostname. (ex: "www.samilpo.org")
	const std::string&	hostName() const;

	/// Returns port
	uint16				port() const;

	/// Returns hostname and port as a string. (ex: "www.samilpo.org:80 (195.68.21.195)")
	std::string			asString() const;

	/// Returns IP address and port as a string. (ex: "195.68.21.195:80")
	std::string			asIPString() const;
	std::string			asWebIPString() const;
	std::string			asUniqueString() const;

	/// Serialize
	void serial( SIPBASE::IStream& s );

	/// Returns true if this CInetAddress is 127.0.0.1
	bool is127001 () const;

	/// Creates a CInetAddress object with local host address, port=0
	static CInetAddress	localHost();

	/** Returns the list of the local host addresses (with port=0)
	 * (especially useful if the host is multihomed)
	 */
	static std::vector<CInetAddress> localAddresses();

	/// Returns true if strIP is in local addresses
	static bool	isLocalHostIP(std::string strIP);

protected:

	/// Constructor with ip address, port=0
	CInetAddress( const in_addr *ip );

private:

	// Called in all constructors. Calls CBaseSocket::init().
	void				init();

	std::string			_HostName;
	sockaddr_in			*_SockAddr;
	bool				_Valid;

};

/// Take a internet dot string and convert it in an uint32 internal format for example "128.64.32.16" -> 0xF0804020
uint32 stringToInternalIPAddress (const std::string &addr);

/// Take an internal address and convert it to a internet dot string
std::string internalIPAddressToString (uint32 addr);

std::string vectorCInetAddressToString(const std::vector<CInetAddress> &addrs);

}

#endif // __INET_ADDRESS_H__

/* End of inet_address.h */
