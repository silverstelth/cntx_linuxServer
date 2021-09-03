/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __EMAIL_H__
#define __EMAIL_H__

#include <string>

#include "misc/report.h"


namespace SIPNET {

/** Send an email
 * \param smtpServer must be a smtp email server.
 * \param from must be a valid email address. If empty, create a fake email address with anonymous@<ipaddress>.com
 * \param to must be a valid email address.
 * \param subject subject of the email. Can be empty.
 * \param body body of the email. Can be empty.
 * \param attachedFile a filename that will be send with the email. Can be empty.
 * \param onlyCheck If true, It'll not send the mail but only check if it could be send.
 */

bool sendEmail (const std::string &smtpServer, const std::string &from, const std::string &to, const std::string &subject, const std::string &body, const std::string &attachedFile = "", bool onlyCheck = false);

/**  If you call this function, the default from (when from is "") used in the sendEmail will be the one
 * you set by this function
 */
void setDefaultEmailParams (const std::string &smtpServer, const std::string &from, const std::string &to);

} // SIPNET


#endif // __EMAIL_H__

/* End of email.h */
