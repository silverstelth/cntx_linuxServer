/** \file log_report.cpp
 * <File description>
 *
 * $Id: log_report.cpp,v 1.1 2007/10/20 07:52:54 Sim Exp $
 */

#include "log_report.h"
#include <functional>
#include "misc/common.h"
#include "misc/displayer.h"
#include "misc/file.h"
#include "misc/path.h"
#include "misc/variable.h"

using namespace SIPBASE;
using namespace std;


CVariable<string> LogPath( "LogReport","LogPath", "Path of the log files", ".", 0, true );

const uint MAX_LOG_LINE_SIZE = 1024;
//sipctassert(MAX_LOG_LINE_SIZE>0);

enum TLogLineHeader { LHDate, LHTime, LHType, LHThread, LHService, LHCodeFile, LHCodeLine, LHSeparator, LH_NB_FIELDS };


///
bool isLogFile( const std::string& filename )
{
	size_t len = filename.size();
	return (len >= 4 ) && (filename.substr( len-4 ) == ".log");
}

///
inline bool	isNumberChar( char c )
{
	return (c >= '0') && (c <= '9');
}

///
void sortLogFiles( vector<std::string>& filenames )
{
	uint i;
	for ( i=0; i!=filenames.size(); ++i )
	{
		// Ensure that a log file without number comes *after* the ones with a number
		string name = string(filenames[i]);
		size_t dotpos = name.find_last_of('.');
		if ( (dotpos!=string::npos) && (dotpos > 2) )
		{
			if ( ! (isNumberChar(name[dotpos-1]) && isNumberChar(name[dotpos-2]) && isNumberChar(name[dotpos-3])) )
			{
				name = name.substr( 0, dotpos ) + "ZZZ" + name.substr( dotpos );
				filenames[i] = name.c_str();
			}
		}
	}
	sort( filenames.begin(), filenames.end() );
	for ( i=0; i!=filenames.size(); ++i )
	{
		// Set the original names back
		string name = filenames[i];
		size_t tokenpos = name.find( "ZZZ." );
		if ( tokenpos != string::npos )
		{
			name = name.substr( 0, tokenpos ) + name.substr( tokenpos + 3 );
			filenames[i] = name.c_str();
		}
	}
}

void	CMakeLogTask::setLogPath(const std::string & logPath)
{
	_LogPaths.resize( 1 );
	_LogPaths[0] = logPath;
}

void	CMakeLogTask::setLogPaths(const std::vector<std::string>& logPaths)
{
	_LogPaths = logPaths;
}

void	CMakeLogTask::setLogPathToDefault()
{
	setLogPath( LogPath.get() );
}

/*
 *
 */
CMakeLogTask::~CMakeLogTask()
{
	if ( _Thread ) // implies && _OutputLogReport
	{
		if ( ! _Complete )
		{
			pleaseStop();
			_Thread->wait();
		}
		clear();
	}
}


/*
 *
 */
void	CMakeLogTask::start()
{
	if ( _Thread )
	{
		if ( _Complete )
			clear();
		else
			return;
	}
	_Stopping = false;
	_Complete = false;
	_Thread = SIPBASE::IThread::create( this );
	_OutputLogReport = new CLogReport();
	_Thread->start();
}


/*
 *
 */
void	CMakeLogTask::clear()
{
	if (_Thread)
	{
		delete _Thread;
		_Thread = NULL;
	}
	if (_OutputLogReport)
	{
		delete _OutputLogReport;
		_OutputLogReport = NULL;
	}
}

/*
 *
 */
void	CMakeLogTask::terminateTask()
{
	if (!_Thread) // _Thread _implies _OutputLogReport
		return;

	pleaseStop();
	_Thread->wait();

	clear();
}

//
bool isOfLogDotLogFamily( const std::string& filename )
{
	return ((filename == "sip.log") ||
			 ((filename.size() == 10) &&
			  (filename.substr( 0, 3 ) == "sip") &&
			  isNumberChar(filename[3]) && isNumberChar(filename[4]) && isNumberChar(filename[5]) &&
			  (filename.substr( 6, 4 ) == ".log")) );
}


enum TVersionTargetMode { TTMAll, TTMMatchAllV, TTMMatchExactV, TTMMatchGreaterV, TTMMatchLowerV } targetMode;
const uint CurrentVersion = ~0;

// Return true and logVersion, or false if not a log with version
bool getLogVersion( const std::string& filename, uint& logVersion )
{
	uint len = static_cast<uint>(filename.size());
	if ( (len > 4) && (filename.substr( len-4 ) == ".log") )
	{
		if ( filename.substr(0, 3) == "log" )
		{
			if ( (len == 7) ||
				 ((len == 10) && (isNumberChar(filename[3]) && isNumberChar(filename[4]) && isNumberChar(filename[5]))) )
			{
				logVersion = CurrentVersion;
				return true;
			}
		}
		else if ( filename[0] == 'v' )
		{
			string::size_type p = filename.find( "_", 1 );
			if ( p != string::npos )
			{
				if ( (len == p + 8) ||
					 ((len == p + 11) && (isNumberChar(filename[p+4]) && isNumberChar(filename[p+5]) && isNumberChar(filename[p+6]))) )
				{
					SIPBASE::fromString( filename.substr( 1, p-1 ), logVersion );
					return true;
				}
			}
		}
	}
	return false;
}

// Assumes filename is .log file
bool matchLogTarget( const std::string& filename, TVersionTargetMode targetMode, uint targetVersion )
{
	if ( targetMode == TTMAll )
		return true;

	uint version;

	// Get version or exclude non-standard log files
	if ( ! getLogVersion( filename, version ) )
		return false;

	// Exclude non-matching version
	switch ( targetMode )
	{
	case TTMMatchExactV:
		return (version == targetVersion); // break;
	case TTMMatchGreaterV:
		return (version >= targetVersion); // break;
	case TTMMatchLowerV:
		return (version <= targetVersion); // break;
	default: // TTMMatchAllV
		return true;
	}
}

/*
 *
 */
void	CMakeLogTask::run()
{
	// Parse log target
	uint targetVersion = CurrentVersion;
	uint lts = static_cast<uint>(_LogTarget.size());
	if ( _LogTarget.empty() || (_LogTarget == "v") )
	{
		targetMode = TTMMatchExactV;
	}
	else if ( _LogTarget == "v*" )
	{
		targetMode = TTMMatchAllV;
	}
	else if ( _LogTarget == "*" )
	{
		targetMode = TTMAll;
	}
	else if ( (lts > 1) && (_LogTarget[0] == 'v') )
	{
		uint additionalChars = 1;
		if ( _LogTarget[lts-1] == '+' )
			targetMode = TTMMatchGreaterV;
		else if ( _LogTarget[lts-1] == '-' )
			targetMode = TTMMatchLowerV;
		else
		{
			targetMode = TTMMatchExactV;
			additionalChars = 0;
		}
		
		SIPBASE::fromString( _LogTarget.substr( 1, lts-additionalChars-1 ), targetVersion );
	}
	else
	{
		sipwarning( "Invalid log target argument: %s", _LogTarget.c_str() );
		_Complete = true;
		return;
	}

	// Get log files and sort them
	vector<string> filenames;
	vector<string> filenamesOfPath;
	for ( vector<string>::const_iterator ilf=_LogPaths.begin(); ilf!=_LogPaths.end(); ++ilf )
	{
		string path = (*ilf);
		if ( (! path.empty()) && (path[path.size()-1]!='/') )
			path += "/";
		filenamesOfPath.clear();
		CPath::getPathContent( path, false, false, true, filenamesOfPath, NULL, true );
		vector<string>::iterator ilf2 = partition( filenamesOfPath.begin(), filenamesOfPath.end(), isLogFile );
		filenamesOfPath.erase( ilf2, filenamesOfPath.end() );
		sortLogFiles( filenamesOfPath );
		filenames.insert( filenames.end(), filenamesOfPath.begin(), filenamesOfPath.end() );
	}

	// Analyse log files
	_OutputLogReport->reset();
	uint nbLines = 0;
	char line [MAX_LOG_LINE_SIZE];

	uint nbSkippedFiles = 0;
	for ( vector<string>::const_iterator ilf=filenames.begin(); ilf!=filenames.end(); ++ilf )
	{
		string shortname = CFileA::getFilename( *ilf );

		// Filter log files based on filename before opening them
		if ( ! matchLogTarget( shortname, targetMode, targetVersion ) )
		{
			++nbSkippedFiles;
			continue;
		}

		sipinfo( "Processing %s (%u/%u)", (*ilf).c_str(), ilf-filenames.begin(), filenames.size() );
		CIFile logfile;
		if ( logfile.open( *ilf, true ) )
		{
			_OutputLogReport->setProgress( static_cast<uint>(ilf-filenames.begin()), static_cast<uint>(filenames.size()) );
			while ( ! logfile.eof() )
			{
				logfile.getline( line, MAX_LOG_LINE_SIZE );
				line[MAX_LOG_LINE_SIZE-1] = '\0'; // force valid end of line
				_OutputLogReport->pushLine( line );
				++nbLines;

				if ( isStopping() )
					return;
			}
		}
	}
	sipinfo( "%u lines processed", nbLines );
	if ( nbSkippedFiles != 0 )
		sipinfo( "%u log files skipped, not matching target %s", nbSkippedFiles, _LogTarget.c_str() );
	_Complete = true;
}


/*
 * Add a log line to the report tree
 */
void	CLogReport::pushLine( const std::string& line, SIPBASE::CLog::TLogType onlyType, bool countOtherTypes )
{
	// Ignore session title
	if ( (line.size() > 14) && (line.substr( 0, 14 ) == "Log Starting [") )
		return;

	// Decode standard log line
	vector<string> lineTokens;
	explode( line, " ", lineTokens );
	if ( lineTokens.size() < LH_NB_FIELDS )
		return;

	// Filter log type
	if ( onlyType != CLog::LOG_UNKNOWN )
	{
		if ( lineTokens[LHType] != IDisplayerA::logTypeToString( onlyType ) )
		{
			if ( countOtherTypes )
				storeLine( lineTokens, true );
			return;
		}
	}

	// Store
	storeLine( lineTokens, false );
}


/*
 *
 */
void	CLogReportNode::storeLine( const std::vector<std::string>& lineTokens, bool mainCountOnly )
{
	// Get service name from "[machine/]serviceName-serviceId"
	string service = lineTokens[LHService];
	string::size_type p = service.find( '/' );
	if ( p != string::npos )
		service = service.substr( p+1 );
	p = service.find( '-' );
	if ( p != string::npos )
		service = service.substr( 0, p );

	// Store to appropriate child
	CLogReportLeaf *child = getChild( service );
	if ( ! child )
		child = addChild( service );
	child->storeLine( lineTokens, mainCountOnly );
}


/*
 *
 */
void	CLogReportLeaf::storeLine( const std::vector<std::string>& lineTokens, bool mainCountOnly )
{
	if ( ! mainCountOnly )
	{
		// Build key from "codeFile codeLine"
		string key = lineTokens[LHCodeFile] + ":" + lineTokens[LHCodeLine];
		_LogLineInfo[key].addAnOccurence( lineTokens );
	}
	++_Counts[lineTokens[LHType]];
	++_TotalLines;
}


/*
 *
 */
void	CLogLineInfo::addAnOccurence( const std::vector<std::string>& lineTokens )
{
	if ( NbOccurences == 0 )
	{
		for ( uint i=LH_NB_FIELDS; i<lineTokens.size(); ++i )
		{
			if ( i != LH_NB_FIELDS )
				SampleLogText += " ";
			SampleLogText += lineTokens[i];
		}
	}
	++NbOccurences;
}


/*
 *
 */
uint	CLogReportLeaf::getNbTotalLines( SIPBASE::CLog::TLogType logType )
{
	return (logType==SIPBASE::CLog::LOG_UNKNOWN) ? _TotalLines : _Counts[SIPBASE::IDisplayerA::logTypeToString( logType )];
}


/*
 * Get results for a service
 */
void	CLogReport::reportByService( const std::string& service, SIPBASE::CLog *targetLog )
{
	ILogReport *child = getChild( service );
	if ( child )
	{
		child->report( targetLog, true );
	}
	else
	{
		targetLog->displayNL( "Nothing found for service %s", service.c_str() );
	}
}


/*
 * Get results for a service (all distinct lines, sorted by occurence)
 */
void	CLogReportLeaf::report( SIPBASE::CLog *targetLog, bool )
{
	// Sort it
	typedef multimap< uint, pair< string, const CLogLineInfo * >, std::greater<uint> > CSortedByOccurenceLogLineInfoMap;
	CSortedByOccurenceLogLineInfoMap sortedByOccurence;
	for ( CLogLineInfoMap::const_iterator it=_LogLineInfo.begin(); it!=_LogLineInfo.end(); ++it )
	{
		const string &key = (*it).first;
		const CLogLineInfo& info = (*it).second;
		sortedByOccurence.insert( make_pair( info.NbOccurences, make_pair( key, &info ) ) );
	}

	// Display it
	for ( CSortedByOccurenceLogLineInfoMap::const_iterator iso=sortedByOccurence.begin(); iso!=sortedByOccurence.end(); ++iso )
	{
		const string &key = (*iso).second.first;
		const CLogLineInfo& info = *((*iso).second.second);
		targetLog->displayRawNL( "%s %6u %s : %s", _Service.c_str(), info.NbOccurences, key.c_str(), info.SampleLogText.c_str() );
	}
}


/*
 * Return the number of lines displayed
 */
uint	CLogReportLeaf::reportPart( uint beginIndex, uint maxNbLines, SIPBASE::CLog *targetLog )
{
	uint i = 0;
	CLogLineInfoMap::const_iterator it;
	for ( it=_LogLineInfo.begin(); it!=_LogLineInfo.end(); ++it )
	{
		if ( i >= beginIndex )
		{
			if ( i >= maxNbLines )
				return i - beginIndex;

			const string &key = (*it).first;
			const CLogLineInfo& info = (*it).second;
			targetLog->displayRawNL( "%s %6u %s : %s", _Service.c_str(), info.NbOccurences, key.c_str(), info.SampleLogText.c_str() );
		}
		++i;
	}
	return i - beginIndex;
}


/*
 * Get summary of results
 */
void	CLogReportNode::report( SIPBASE::CLog *targetLog, bool displayDetailsPerService )
{
	uint nb1Sum=0, nb2Sum=0;
	for ( std::vector<CLogReportLeaf*>::const_iterator it=_Children.begin(); it!=_Children.end(); ++it )
	{
		CLogReportLeaf *pt = (*it);

		// Get distinct warnings
		uint nb1 = pt->getNbDistinctLines();
		nb1Sum += nb1;

		// Get total warnings, info... but filter out lines with no header
		uint sumTotalLinesNotUnknown = 0;
		uint nbTotalLines [CLog::LOG_UNKNOWN];
		for ( uint i=CLog::LOG_ERROR; i!=CLog::LOG_UNKNOWN; ++i )
		{
			nbTotalLines[i] = pt->getNbTotalLines( (CLog::TLogType)i );
			sumTotalLinesNotUnknown += nbTotalLines[i];
		}
		if ( sumTotalLinesNotUnknown != 0 )
		{
			targetLog->displayRawNL( "%s: \t%u distinct WRN, %u total WRN, %u INF, %u DBG, %u STT, %u AST, %u ERR, %u TOTAL",
				pt->service().c_str(), nb1, nbTotalLines[CLog::LOG_WARNING],
				nbTotalLines[CLog::LOG_INFO], nbTotalLines[CLog::LOG_DEBUG],
				nbTotalLines[CLog::LOG_STAT], nbTotalLines[CLog::LOG_ASSERT],
				nbTotalLines[CLog::LOG_ERROR], pt->getNbTotalLines( CLog::LOG_UNKNOWN ) );
			nb2Sum += nbTotalLines[CLog::LOG_WARNING];
		}
	}
	targetLog->displayRawNL( "=> %u distinct, %u total WRN (%u pages)", nb1Sum, nb2Sum, nb1Sum / NB_LINES_PER_PAGE + 1 );
	
	if ( displayDetailsPerService )
	{
		for ( std::vector<CLogReportLeaf*>::const_iterator it=_Children.begin(); it!=_Children.end(); ++it )
		{
			(*it)->report( targetLog, true );
		}
	}
}


/*
 * Get partial results (pageNum>=1)
 */
void	CLogReportNode::reportPage( uint pageNum, SIPBASE::CLog *targetLog )
{
	if ( _Children.empty() )
	{
		targetLog->displayRawNL( "[END OF LOG]" );
		return;
	}

	uint beginIndex = pageNum * NB_LINES_PER_PAGE;
	uint lineCounter = 0, prevLineCounter;
	for ( std::vector<CLogReportLeaf*>::const_iterator it=_Children.begin(); it!=_Children.end(); ++it )
	{
		CLogReportLeaf *pt = (*it);
		prevLineCounter = lineCounter;
		lineCounter += pt->getNbDistinctLines();
		if ( lineCounter >= beginIndex )
		{
			uint remainingLines = pageNum - (lineCounter - beginIndex );
			pt->reportPart( beginIndex - prevLineCounter, remainingLines, targetLog ); //
			while ( remainingLines != 0 )
			{
				++it;
				if ( it == _Children.end() )
				{
					targetLog->displayRawNL( "[END OF LOG]" );
					return;
				}
				pt = (*it);
				remainingLines -= pt->reportPart( 0, remainingLines, targetLog );
			}
			return;
		}
	}	
}
