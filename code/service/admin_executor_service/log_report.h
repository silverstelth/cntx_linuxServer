/** \file log_report.h
 * <File description>
 *
 * $Id: log_report.h,v 1.1 2007/10/20 07:52:54 Sim Exp $
 */


#ifndef SIP_LOG_REPORT_H
#define SIP_LOG_REPORT_H

#include "misc/types_sip.h"
#include "misc/log.h"
#include "misc/thread.h"
#include <string>
#include <vector>
#include <map>


class CLogReport;

/*
 *
 */
class CMakeLogTask : public SIPBASE::IRunnable
{
public:

	/// Constructor
	CMakeLogTask() : _Stopping(false), _Complete(false), _Thread(NULL), _OutputLogReport(NULL) {}

	/// Destructor
	~CMakeLogTask();

	/// Start (called in main thread)
	void	start();

	/// Ask for stop and wait until terminated (called in main thread)
	void	terminateTask();

	///
	bool	isRunning() const { return (_Thread != NULL) && (!_Complete); }

	///
	bool	isStopping() const { return _Stopping; }

	///
	bool	isComplete() const { return _Complete; }

	virtual void run();

	/// Set the path of logfile directory. 
	void setLogPath(const std::string & logPath);

	/// Set one or more paths of logfile directory. They will be processed when running the background thread.
	void setLogPaths(const std::vector<std::string>& logPaths);

	/// Set the path of logfile directory to default value
	void setLogPathToDefault();

	/// Return the log paths
	const std::vector<std::string> getLogPaths() const { return _LogPaths; }

	/** Set which files are to be browsed:
	 * v       --> log*.log (default if setLogTarget() not called or called with an empty string)
	 * v*      --> log*.log + v*_log*.log
	 * v<n>    --> v<n>_log*.log
	 * v<n>+   --> v<n>_log*.log + all matching greater <n> + log*.log
	 * v<n>-   --> v<n>_log*.log + all matching lower <n>
	 * *       --> *.log
	 */
	void setLogTarget(const std::string & logTarget) { _LogTarget = logTarget; }

	/// Return the log report (NULL if task not started yet)
	CLogReport*		getLogReport() { return _OutputLogReport; }

private:

	void	pleaseStop() { _Stopping = true; }
	void	clear();

	volatile bool	_Stopping;
	volatile bool	_Complete;
	std::string		_LogTarget;
	std::vector<std::string>	_LogPaths;

	SIPBASE::IThread	*_Thread;

	CLogReport		*_OutputLogReport;
};


const uint NB_LINES_PER_PAGE = 100;


/**
 *
 */
class CLogLineInfo
{
public:
	
	CLogLineInfo() : NbOccurences(0) {}

	void				addAnOccurence( const std::vector<std::string>& lineTokens );

	uint				NbOccurences;

	std::string			SampleLogText;
};


/**
 *
 */
class ILogReport
{
public:

	virtual void storeLine( const std::vector<std::string>& lineTokens, bool mainCountOnly ) = 0;

	virtual void report( SIPBASE::CLog *targetLog, bool detailed ) = 0;

	virtual uint getNbDistinctLines() const = 0;

	virtual uint getNbTotalLines( SIPBASE::CLog::TLogType logType ) = 0;

	virtual ~ILogReport() {}
};


/**
 * For one service (service name, not service instance), store info about log lines
 */
class CLogReportLeaf : public ILogReport
{
public:

	/// Constructor
	CLogReportLeaf( const std::string& service ) : _Service(service), _TotalLines(0) {}

	std::string		service() { return _Service; }

	virtual uint	getNbDistinctLines() const { return static_cast<uint>(_LogLineInfo.size()); }

	virtual uint	getNbTotalLines( SIPBASE::CLog::TLogType logType );

	virtual void	storeLine( const std::vector<std::string>& lineTokens, bool mainCountOnly );

	virtual void	report( SIPBASE::CLog *targetLog, bool detailed );

	uint			reportPart( uint beginIndex, uint maxNbLines, SIPBASE::CLog *targetLog );

protected:

	typedef std::map< std::string, CLogLineInfo > CLogLineInfoMap;

	std::string						_Service;

	CLogLineInfoMap					_LogLineInfo;

	std::map< std::string, uint >	_Counts; // indexed by log type string
	
	uint							_TotalLines;
};


/**
 * Store info about log lines for several services
 */
class CLogReportNode : public ILogReport
{
public:

	/// Constructor
	CLogReportNode() {}

	/// Destructor
	~CLogReportNode() { reset(); }

	/// Clear all
	void	reset()
	{
		for ( std::vector<CLogReportLeaf*>::iterator it=_Children.begin(); it!=_Children.end(); ++it )
			delete (*it);
		_Children.clear();
	}

protected:

	virtual void				storeLine( const std::vector<std::string>& lineTokens, bool mainCountOnly=false );

	CLogReportLeaf*				getChild( const std::string& service )
	{
		for ( std::vector<CLogReportLeaf*>::iterator it=_Children.begin(); it!=_Children.end(); ++it )
		{
			if ( (*it)->service() == service )
				return (*it);
		}
		return NULL;
	}

	CLogReportLeaf*				addChild( const std::string& service )
	{
		CLogReportLeaf *child = new CLogReportLeaf( service );
		_Children.push_back( child );
		return child;
	}

	virtual void	report( SIPBASE::CLog *targetLog, bool displayDetailsPerService );

	virtual uint	getNbDistinctLines() const
	{
		uint n = 0;
		for ( std::vector<CLogReportLeaf*>::const_iterator it=_Children.begin(); it!=_Children.end(); ++it )
			n += (*it)->getNbDistinctLines();
		return n;
	}

	virtual uint	getNbTotalLines( SIPBASE::CLog::TLogType logType=SIPBASE::CLogW::LOG_UNKNOWN )
	{
		uint n = 0;
		for ( std::vector<CLogReportLeaf*>::const_iterator it=_Children.begin(); it!=_Children.end(); ++it )
			n += (*it)->getNbTotalLines( logType );
		return n;
	}

	void			reportPage( uint pageNum, SIPBASE::CLog *targetLog );

private:

	std::vector<CLogReportLeaf*>	_Children;
};


/*
 * <Class description>
 * \date 2004
 */
class CLogReport : public CLogReportNode
{
public:

	/// Constructor
	CLogReport() : _CurrentFile(~0), _TotalFiles(~0) {}

	/// Clear all
	void	reset() { CLogReportNode::reset(); }

	/**
	 * Add a log line to the report tree.
	 * \param onlyType Type of log to study. If LOG_UNKNOWN, study all.
	 * \param countOtherTypes If true and onlyType not LOG_UNKNOWN, count the number of lines of each other type found.
	 */
	void	pushLine( const std::string& line, SIPBASE::CLog::TLogType onlyType=SIPBASE::CLog::LOG_WARNING, bool countOtherTypes=true );

	/// Set the current progress
	void	setProgress( uint currentFile, uint totalFiles ) { _CurrentFile = currentFile; _TotalFiles = totalFiles; }

	/// Get the current progress
	void	getProgress( uint& currentFile, uint& totalFiles ) { currentFile = _CurrentFile; totalFiles = _TotalFiles; }

	/// Get results for a service
	void	reportByService( const std::string& service, SIPBASE::CLog *targetLog );

	/// Get partial results (pageNum>=1)
	void	reportPage( uint pageNum, SIPBASE::CLog *targetLog ) { CLogReportNode::reportPage( pageNum, targetLog ); }

	/// Get summary of results
	virtual void report( SIPBASE::CLog *targetLog, bool displayDetailsPerService=false ) { CLogReportNode::report( targetLog, displayDetailsPerService ); }

private:

	uint	_CurrentFile;
	uint	_TotalFiles;
};


#endif // SIP_LOG_REPORT_H

/* End of log_report.h */
