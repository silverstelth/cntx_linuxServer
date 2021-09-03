/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __QUERYREQUEST_H__
#define __QUERYREQUEST_H__

#include "misc/bit_mem_stream.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#undef max
#undef min
#endif

#include "misc/db_interface.h"

enum		ESPMODE{ EXECUTE_NORMAL, EXECUTE_PARAM, EXECUTE_BLOCK };
enum		ESPRIORITY { ESP_NORMAL, ESP_REST };
#define IN_			0x01
#define	OUT_		0x02
#define IO_			(IN_ | OUT_)
#define CB_			0x04

namespace SIPBASE
{
typedef		void (*TDBCompleteCallback)(/*DBSTMT &stmt, */int nQueryResult, uint32 argn, void *argv[], CMemStream *resSet);

class	TPARAM_C : public SIPBASE::CRefCount
{
	uint32		_mode;
	int			_ctype;
	int			_sqltype;
	uint32		_nbytes;
	uint32		_len;
	void		*_param;
	char		_chFormat;

public:
	void	serial(SIPBASE::IStream &s);
	void	serialPtr(SIPBASE::IStream &s);
	//void	init(uint32 mode, char &chFormat, void *param);
	void	setParamType(char &chFormat);

public:
	uint32	mode() { return _mode; };
	int		ctype() { return _ctype; };
	int		sqltype() { return _sqltype; };
	void	*param() { return _param; };
	uint32	nbytes() { return _nbytes; };
	uint32	len() { return _len; };

	bool	isIN_() { if (_mode & IN_) return true; return false; };
	bool	isOUT_(){ if(_mode & OUT_) return true; return false; };
	bool	isIO_() { if((_mode & IO_) == IO_) return true; return false; };
	bool	isCB_() { if(_mode & CB_) return true; return false; };

public:
	TPARAM_C();
	TPARAM_C(char &chFormat);
//	TPARAM_C(uint32 mode, int ctype, void *param)
//		: _mode(mode)
//		, _ctype(ctype)
//		, _param(param)
//	{};

	~TPARAM_C();

protected:
	void	clear();
};

typedef		SIPBASE::CSmartPtr<TPARAM_C>	TParamPtr;

#define MAX_DBCONTENTSLENGTH_BYTE	4096 + 2		// add space for null character

class	TPARAM_SQL : public SIPBASE::CRefCount
{
	int			_sqltype;
	uint32		_nbytes;
	uint32		_len;
	uint8		_param[MAX_DBCONTENTSLENGTH_BYTE];
	char		_chFormat;

public:
	void	serial(SIPBASE::IStream &s);
	uint32	init(sint16 sqlType, int len);
	void	setParamType(sint16	sqlType);
	void	GetRowColumn(ODBCRecord &row, int nColPos);
public:
	int		sqltype() { return _sqltype; };
	void	*param() { return (void *)_param; };
	uint32	nbytes() { return _nbytes; };
	uint32	len() { return _len; };

public:
	TPARAM_SQL();
	TPARAM_SQL(char &chFormat);
	~TPARAM_SQL();
protected:
	void	clear();
};

#define		MAX_DB_PARAMS	20

class CQueryRequest
{
protected:
	tstring					_strQuery;
	TDBCompleteCallback		_callback;
	uint8					_mode; // ESPMODE
//	HANDLE					_hCompEvent;
	uint8					_nParams;
	std::string				_strInFormat; 
	SIPBASE::CMemStream		_paramBuffer;
	uint8					_priority;
	uint32					_expiretimems;
	SIPBASE::TTime			_requestTime;
public:
	void	init(const char *strInFormat, va_list paramList);
	void	init();
	void	release();

	void	serial (SIPBASE::IStream &s);
	void	setCompletionEvent();

	// get/set function
	tstring &				Query() { return _strQuery; };
	TDBCompleteCallback		Callback() { return _callback; };
	uint8					Mode() { return _mode; }; // ESPMODE
	uint8					Priority() { return _priority; }
	uint32					Expiretime() { return _expiretimems; }
	SIPBASE::TTime			RequestTime() { return _requestTime; }
	uint8					ParamSize() { return _nParams; };
	std::string &			ParamFormat() { return _strInFormat; };
	SIPBASE::CMemStream&	ParamBuffer() { return _paramBuffer; };
//	HANDLE					Event() { return _hCompEvent; };

public:
	CQueryRequest() { _priority = ESP_NORMAL; _expiretimems = 0; _requestTime = SIPBASE::CTime::getLocalTime(); };
	CQueryRequest(tstring &strQuery, TDBCompleteCallback callback, uint8 mode, uint8 prio, uint32 etime=0);
	CQueryRequest(const CQueryRequest& other);
	CQueryRequest& operator =(const CQueryRequest& other);

	virtual	~CQueryRequest();
};

//typedef SIPBASE::CSmartPtr<DBSTMT>	TDBStmtPtr;
class	CQueryResult : public SIPBASE::CRefCount
{
protected:
	tstring					_strQuery;
	TDBCompleteCallback		_callback;
	uint8					_mode; // ESPMODE
	sint32					_nRet;

	typedef	std::vector<TParamPtr>		TPARAMARY;
	typedef	TPARAMARY::iterator			TPARAMARY_IT;
	TPARAMARY				_ParamAry;

	CMemStream *			_ResultSet;

public:
	TParamPtr				_paraminfo[MAX_DB_PARAMS];

public:
	void	init(CQueryRequest & request);
	void	release();
	void	setResult(int nRet, CMemStream * resultset);

	void	insertParamAry(TParamPtr paraminfo);

	void	serial(SIPBASE::IStream &s);
	void	serialPtr(SIPBASE::IStream &s);

	CQueryResult(CQueryRequest &request, DBCONNECTION &DBCon);
	CQueryResult(DBCONNECTION &DBCon);
	CQueryResult();
	~CQueryResult();

public:
	tstring &				Query() { return _strQuery; };
	TDBCompleteCallback		Callback() { return _callback; };
	uint8					Mode() { return _mode; }; // ESPMODE
	int						nResult() { return _nRet; };
	TPARAMARY&				ParamAry() { return _ParamAry; };
	//TDBStmtPtr				DBStmt() { return _StmtPtr; }
	//DBSTMT					*DBStmt() { return _DBStmt; }
	CMemStream				*ResultSet(){ return _ResultSet; };
};
}; // SIPBASE
#endif // end of guard __QUERYREQUEST_H__
