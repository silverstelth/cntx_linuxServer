/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __O_XML_H__
#define __O_XML_H__

//#define SIP_DONT_USE_EXTERNAL_CODE
#undef SIP_DONT_USE_EXTERNAL_CODE

#ifndef SIP_DONT_USE_EXTERNAL_CODE

#include "types_sip.h"
#include "stream.h"

// Include from libxml2
#include <libxml/parser.h>

namespace SIPBASE {

/**
 * Output xml stream
 *
 * This class is an xml formated output stream.
 *
 * This stream use an internal stream to output final xml code.
 \code
	// Check exceptions
	try
	{
		// File stream
		COFile file;

		// Open the file
		file.open ("output.xml");

		// Create the XML stream
		COXml output;

		// Init
		if (output.init (&file, "1.0"))
		{
			// Serial the class
			myClass.serial (output);

			// Flush the stream, write all the output file
			output.flush ();
		}

		// Close the file
		file.close ();
	}
 	catch (Exception &e)
	{
	}
\endcode
 *
 * \date 2001
 */
class COXml : public IStream
{
	friend int xmlOutputWriteCallbackForSIP ( void *context, const char *buffer, int len );
	friend int xmlOutputCloseCallbackForSIP ( void *context );
	friend void xmlGenericErrorFuncWrite (void *ctx, const char *msg, ...);
public:

	/** Stream ctor
	  *
	  */
	COXml ();

	/** Stream initialisation
	  *
	  * \param stream is the stream the class will use to output xml code.
	  * this pointer is held by the class but won't be deleted.
	  * \param version is the version to write in the XML header. Default is 1.0.
	  * \return true if initialisation is successful, false if the stream passed is not an output stream.
	  */
	bool init (IStream *stream, const char *version="1.0");

	/** Return the error string.
	  * if not empty, something wrong appends
	  */
	const char *getErrorString () const;

	/** Default dstor
	  *
	  * Flush the stream.
	  */
	virtual ~COXml ();

	/** Flush the stream.
	  *
	  * You can only flush the stream when all xmlPushBegin - xmlPop have been closed.
	  */
	void flush ();

	/** Get root XML document pointer
	  */
	xmlDocPtr getDocument ();

	/** Return true if the string is valid to be stored in a XML property without modification.
	  */
	static bool		isStringValidForProperties (const char *str);

private:

	/// From IStream
	virtual void	serial(uint8 &b);
	virtual void	serial(sint8 &b);
	virtual void	serial(uint16 &b);
	virtual void	serial(sint16 &b);
	virtual void	serial(uint32 &b);
	virtual void	serial(sint32 &b);
	virtual void	serial(uint64 &b);
	virtual void	serial(sint64 &b);
	virtual void	serial(float &b);
	virtual void	serial(double &b);
	virtual void	serial(bool &b);
#ifndef SIP_OS_CYGWIN
	virtual void	serial(char &b);
#endif
	virtual void	serial(std::string &b);
	virtual void	serial(ucstring &b);
	virtual void	serialBuffer(uint8 *buf, uint len);
	virtual void	serialBit(bool &bit);

	virtual bool	xmlPushBeginInternal (const char *nodeName);
	virtual bool	xmlPushEndInternal ();
	virtual bool	xmlPopInternal ();
	virtual bool	xmlSetAttribInternal (const char *attribName);
	virtual bool	xmlBreakLineInternal ();
	virtual bool	xmlCommentInternal (const char *comment);

	// Internal functions
	void			serialSeparatedBufferOut( const char *value );
	inline void		flushContentString ();

	// Push mode
	bool			_PushBegin;

	// Attribute defined
	bool			_AttribPresent;

	// Attribute name
	std::string		_AttribName;

	// The internal stream
	IStream			*_InternalStream;
	
	// Document pointer
	xmlDocPtr		_Document;

	// Document version
	std::string		_Version;

	// Current nodes
	xmlNodePtr		_CurrentNode;

	// Current content string
	std::string		_ContentString;

	// Error message
	std::string		_ErrorString;
};


} // SIPBASE

#endif // SIP_DONT_USE_EXTERNAL_CODE

#endif // __O_XML_H__

/* End of o_xml.h */
