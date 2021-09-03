/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __REPORT_H__
#define __REPORT_H__

#include "types_sip.h"

namespace SIPBASE {

/** Display a custom message box.
 *
 * \param title set the title of the report. If empty, it'll display "SIP report".
 * \param header message displayed before the edit text box. If empty, it displays the default message.
 * \param body message displayed in the edit text box. This string will be sent by email.
 * \param debugButton 0 for disabling it, 1 for enable with default behaviors (generate a breakpoint), 2 for enable with no behavior
 *
 *
 * \return the button clicked or error
 */

enum TReportResult { ReportDebug, ReportIgnore, ReportQuit, ReportError };

TReportResult report (const std::string &title, const std::string &header, const std::string &subject, const std::string &body, bool enableCheckIgnore, uint debugButton, bool ignoreButton, sint quitButton, bool sendReportButton, bool &ignoreNextTime, const std::string &attachedFile = "");
TReportResult reportW(const std::basic_string<ucchar> &title, const std::basic_string<ucchar> &header, const std::basic_string<ucchar> &subject, const std::basic_string<ucchar> &body, bool enableCheckIgnore, uint debugButton, bool ignoreButton, sint quitButton, bool sendReportButton, bool &ignoreNextTime, const std::basic_string<ucchar> &attachedFile = L"");

/** call this in the main of your appli to enable email: setReportEmailFunction (sendEmail);
 */
void setReportEmailFunction (void *emailFunction);

} // SIPBASE

#endif // __REPORT_H__

/* End of report.h */
