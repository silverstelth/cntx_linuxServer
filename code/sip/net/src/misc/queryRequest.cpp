/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/variable.h"
#include "misc/queryRequest.h"


namespace	SIPBASE
{

//////////////////////////////////////////////////////////////////////////
// TPARAM_C
//////////////////////////////////////////////////////////////////////////
TPARAM_C::TPARAM_C()
{
	_param = NULL;
}

TPARAM_C::TPARAM_C(char &chFormat)
{
	_param = NULL;
	setParamType(chFormat);
}

void	TPARAM_C::setParamType(char &chFormat)
{
	_chFormat = chFormat;
	switch ( _chFormat )
	{
	case 'd':
		{
			_ctype		= SQL_C_SLONG;
			_sqltype	= SQL_INTEGER;
		}
		break;
	case 'D':
		{
			_ctype		= SQL_C_ULONG;
			_sqltype	= SQL_INTEGER;
		}
		break;
	case 'w':
		{
			_ctype		= SQL_C_SSHORT;
			_sqltype	= SQL_SMALLINT;
		}
		break;
	case 'W':
		{
			_ctype		= SQL_C_USHORT;
			_sqltype	= SQL_SMALLINT;
		}
		break;
	case 'b':
		{
			_ctype		= SQL_C_STINYINT;
			_sqltype	= SQL_TINYINT;
		}
		break;
	case 'B':
		{
			_ctype		= SQL_C_UTINYINT;
			_sqltype	= SQL_TINYINT;
		}
		break;
	case 'u':
	case 'U':
		{
			_ctype		= SQL_C_BINARY;
			_sqltype	= SQL_BINARY;
		}
		break;
	case 'i':
	case 'I':
		{
			_ctype		= SQL_C_BINARY;
			_sqltype	= SQL_VARBINARY;
		}
		break;
	case 'P':
	case 'p':
		{
			_ctype		= SQL_C_DEFAULT;
			_sqltype	= SQL_C_DEFAULT;
		}
		break;
	case 's':
		{
			_ctype		= SQL_C_CHAR;
			_sqltype	= SQL_VARCHAR;
		}
		break;
	case 'S':
		{
			_ctype		= SQL_C_WCHAR;
			_sqltype	= SQL_WLONGVARCHAR;
		}
		break;
	default:
		sipwarning("Unknown Format %s", _chFormat);
		siperrornoex("Unknown Format %s", _chFormat);
		break;
	}
}

// void	TPARAM_C::init(uint32 mode, char &chFormat, void *param)
// {
// 	_mode	= mode;
// 	_param	= param;
// 
// 	setParamType(chFormat);
// }

void	TPARAM_C::serial(SIPBASE::IStream &s)
{
	s.serial(_mode);
	s.serial(_chFormat);
	if ( s.isReading())
	{
		switch ( _chFormat )
		{
			case 'd':
				{
					sint32	param;		s.serial(param);
					_param				= new sint32;
					*(sint32 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'D':
				{
					uint32	param;		s.serial(param);
					_param				= new uint32;
					*(uint32 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'w':
				{
					sint16	param;		s.serial(param);
					_param				= new sint16;
					*(sint16 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'W':
				{
					uint16	param;		s.serial(param);
					_param				= new uint16;
					*(uint16 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'b':
				{
					sint8	param;		s.serial(param);
					_param				= new sint8;
					*(sint8 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'B':
				{
					uint8	param;		s.serial(param);
					_param				= new uint8;
					*(uint8 *)_param	= param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 'u':
			case 'U':
				{
					uint32	size		= 0;					s.serial(size);
					_param				= new uint8[size];		s.serialBuffer((uint8 *)_param, size);
					_nbytes				= size;
					_len				= _nbytes;
				}
				break;
			case 'i':
			case 'I':
				{
					uint32	size		= 0;					s.serial(size);
					_param				= new uint8[size];		s.serialBuffer((uint8 *)_param, size);
					_nbytes				= size;
					_len				= _nbytes;
				}
				break;
			case 'P':
			case 'p':
				{
					uint64	param;		s.serial(param);
					_param				= (void *)param;
					_nbytes				= sizeof(param);
					_len				= _nbytes;
				}
				break;
			case 's':
				{
					std::string strParam;	s.serial(strParam);
					_len				= strParam.size();
					_nbytes				= _len+1;
					_param				= new char[_nbytes];
					//memset(_param, 0, _len + sizeof(char));
					memcpy(_param, strParam.c_str(), _nbytes);
				}
				break;
			case 'S':
				{
					ucstring ucParam;		s.serial(ucParam);
					_len				= ucParam.size();
					_nbytes				= (_len+1) * sizeof(ucchar);
					_param				= new ucchar[_len + 1];
					//memset(_param, 0, _nbytes + sizeof(ucchar));
					memcpy(_param, ucParam.c_str(), _nbytes);
				}
				break;
			default:
				sipwarning("Unknown Format. chFormat=%c", _chFormat);
				siperrornoex("Unknown Format. chFormat=%c", _chFormat);
				break;
		}
	}
	else // save
	{
		switch ( _chFormat )
		{
			case 'd':	{sint32	param = *(sint32 *)_param;		s.serial(param);}		break;
			case 'D':	{uint32	param = *(uint32 *)_param;		s.serial(param);}		break;
			case 'w':	{sint16	param = *(sint16 *)_param;		s.serial(param);}		break;
			case 'W':	{uint16	param = *(uint16 *)_param;		s.serial(param);}		break;
			case 'b':	{sint8	param = *(sint8 *)_param;		s.serial(param);}		break;
			case 'B':	{uint8	param = *(uint8 *)_param;		s.serial(param);}		break;
			case 'u':
			case 'U':	{s.serialBufferWithSize((uint8 *)_param,_nbytes);}				break;
			case 'i':
			case 'I':	{s.serialBufferWithSize((uint8 *)_param,_nbytes);}				break;
			case 'P':
			case 'p':	{uint64			param		= (uint64) _param;		s.serial(param);}		break;
			case 's':	{std::string	strParam	= (char *)_param;		s.serial(strParam);}	break;
			case 'S':	{ucstring		ucParam		= (wchar_t *)_param;	s.serial(ucParam);}		break;
			default:
				sipwarning("Unknown Format. chFormat=%c", _chFormat);
				siperrornoex("Unknown Format. chFormat=%c", _chFormat);
				break;
		}
	}
}

void	TPARAM_C::serialPtr(SIPBASE::IStream &s)
{
/*	if ( s.isReading() )
		crefs --;		
	else
		crefs ++;
*/
	s.serial(_mode);
	s.serial(_chFormat);
	s.serial(_nbytes);
	s.serial(_len);
	if ( s.isReading())
	{
		clear();
		uint8 *pNewParam = new uint8[_nbytes];
		s.serialBuffer(pNewParam, _nbytes);
		_param		= (lpvoid)pNewParam;
	}
	else // save
	{
		s.serialBuffer((uint8 *)_param, _nbytes);
	}
}

void	TPARAM_C::clear()
{
	if ( _param == NULL )
		return;
	switch ( _chFormat )
	{
	case 'd':	case 'D':
	case 'w':	case 'W':
	case 'b':	case 'B':
		free(_param);	_param = NULL;		break;
	case 'u':	case 'U':
	case 'i':	case 'I':
	case 's':	case 'S':
		free(_param);	_param = NULL;		break;
	case 'p':	case 'P':
		break;
	default:
		sipwarning("Unknown Format");
		siperrornoex("Unknown Format");
		break;
	}
}
TPARAM_C::~TPARAM_C()
{
	clear();
}

//////////////////////////////////////////////////////////////////////////
// TPARAM_SQL
TPARAM_SQL::TPARAM_SQL()
{
	// _param = NULL;
	memset(_param, 0, sizeof(_param));
}

TPARAM_SQL::TPARAM_SQL(char &chFormat)
{
	// _param = NULL;
	setParamType(chFormat);
	memset(_param, 0, sizeof(_param));
}

void	TPARAM_SQL::clear()
{
/*
	if ( _param == NULL )
		return;
	switch ( _sqltype )
	{
	case SQL_NUMERIC:
	case SQL_DECIMAL:
	case SQL_BIGINT:
	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
	case SQL_INTEGER:
	case SQL_SMALLINT:
	case SQL_TINYINT:
	case SQL_CHAR:
		delete		_param;	_param = NULL;
		break;

	case SQL_BINARY:
	case SQL_VARCHAR:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
		delete[]	_param;	_param = NULL;
		break;

	case SQL_DATETIME:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:
		delete[]	_param;	_param = NULL;
		break;

	default:
		sipwarning("Unknown Format. sqlType=%d", _sqltype);
		break;
	}
	*/
}
TPARAM_SQL::~TPARAM_SQL()
{
	clear();
}


void	TPARAM_SQL::serial(SIPBASE::IStream &s)
{
	if ( s.isReading())
	{
		switch ( _chFormat )
		{
		case 'd':
			{
				sint32	param;		s.serial(param);
				//_param				= new sint32;
				*(sint32 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'D':
			{
				uint32	param;		s.serial(param);
				//_param				= new uint32;
				*(uint32 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'w':
			{
				sint16	param;		s.serial(param);
				//_param				= new sint16;
				*(sint16 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'W':
			{
				uint16	param;		s.serial(param);
				//_param				= new uint16;
				*(uint16 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'b':
			{
				sint8	param;		s.serial(param);
				//_param				= new sint8;
				*(sint8 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'B':
			{
				uint8	param;		s.serial(param);
				//_param				= new uint8;
				*(uint8 *)_param	= param;
				_nbytes				= sizeof(param);
				_len				= _nbytes;
			}
			break;
		case 'i':
		case 'I':
			{
				uint32	size		= 0;					s.serial(size);
				//_param				= new uint8[size];		
				s.serialBuffer((uint8 *)_param, size);
				_nbytes				= size;
				_len				= _nbytes;
			}
			break;
		case 's':
			{
				std::string strParam;	s.serial(strParam);
				_len				= strParam.size();
				_nbytes				= _len;
				//_param				= new char[_len + 1];
				//memset(_param, 0, _len + sizeof(char));
				memcpy(_param, strParam.c_str(), _len+1);
			}
			break;
		case 'S':
			{
				ucstring ucParam;		s.serial(ucParam);
				_len				= ucParam.size();
				_nbytes				= (_len) * sizeof(ucchar);
				//_param				= new ucchar[_len + 1];
				//memset(_param, 0, _nbytes + sizeof(ucchar));
				memcpy(_param, ucParam.c_str(), (_len+1)*sizeof(ucchar));
			}
			break;

		default:
			sipwarning("Unknown Format _chFormat=%c", _chFormat);
			siperrornoex("Unknown Format _chFormat=%c", _chFormat);
			break;
		}
	}
	else
	{
		switch ( _chFormat )
		{
		case 'd':	{sint32	param = *(sint32 *)_param;		s.serial(param);}		break;
		case 'D':	{uint32	param = *(uint32 *)_param;		s.serial(param);}		break;
		case 'w':	{sint16	param = *(sint16 *)_param;		s.serial(param);}		break;
		case 'W':	{uint16	param = *(uint16 *)_param;		s.serial(param);}		break;
		case 'b':	{sint8	param = *(sint8 *)_param;		s.serial(param);}		break;
		case 'B':	{uint8	param = *(uint8 *)_param;		s.serial(param);}		break;
		case 'i':
		case 'I':	{s.serialBufferWithSize((uint8 *)_param,_nbytes);}				break;
		case 's':	{std::string strParam = (char *)_param;	s.serial(strParam);}	break;
		case 'S':	
			{
#ifdef SIP_OS_WINDOWS
				ucstring ucParam = (wchar_t *)_param;
#else
				ucstring ucParam;
				char16_t* params = (char16_t*)_param;
				int length = std::char_traits<char16_t>::length(params);
				for (int i = 0; i < length; i++)
				{
					ucParam += params[i];
				}
#endif
				s.serial(ucParam);
			}
				break;

		default:
			sipwarning("Unknown Format. _chFormat=%c", _chFormat);
			siperrornoex("Unknown Format. _chFormat=%c", _chFormat);
			break;
		}
	}
}

uint32	TPARAM_SQL::init(sint16 sqlType, int len)
{
	clear();
	_sqltype	= sqlType;
	_len		= len;

	setParamType(sqlType);

	switch ( sqlType )
	{
	case SQL_NUMERIC:
	case SQL_DECIMAL:
		break;
	case SQL_BIGINT:
	case SQL_FLOAT:
		//_param		= new float;
		_nbytes		= sizeof(float);
	case SQL_REAL:
		break;
	case SQL_DOUBLE:
		break;
	case SQL_INTEGER:
		{
			//_param		= new sint32;
			_nbytes		= sizeof(sint32);
		}
		break;
	case 'D':
		{
			//_param		= new uint32;
			_nbytes		= sizeof(uint32);
		}
		break;
	case SQL_SMALLINT:
		{
			//_param		= new sint16;
			_nbytes		= sizeof(sint16);
		}
		break;
	case 'W':
		{
			//_param		= new uint16;
			_nbytes		= sizeof(uint16);
		}
		break;
	case SQL_TINYINT:
		{
			//_param		= new sint8;
			_nbytes		= sizeof(sint8);
		}
		break;
	case 'B':
		{
			//_param		= new uint8;
			_nbytes		= sizeof(uint8);
		}
		break;
	case SQL_CHAR:
	case SQL_VARCHAR:
		{
			_nbytes		= len+1;
			//_param		= new char[_nbytes];
			sipassert(_nbytes <= MAX_DBCONTENTSLENGTH_BYTE);
		}
		break;
	case SQL_LONGVARBINARY:
	case SQL_VARBINARY:
	case SQL_BINARY:
		{
			_nbytes		= len;
			//_param		= new char[len];
			sipassert(_nbytes <= MAX_DBCONTENTSLENGTH_BYTE);
		}
		break;
	case SQL_WCHAR:
	case SQL_WVARCHAR:
		{
			_nbytes		= (len+1) * sizeof(SQLTCHAR);
			sipassert(_nbytes <= MAX_DBCONTENTSLENGTH_BYTE);
			//_param		= new ucchar[len + 1];
		}
		break;
	case SQL_DATETIME:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:
		{
 			_nbytes		= len+1;
 			//_param		= new char[_nbytes];

// 			_nbytes		= sizeof(SQL_DATE_STRUCT);
// 			_param		= new SQL_DATE_STRUCT;
		}
		break;

	default:
		sipwarning("Unknown Format sqlType=%d", sqlType);
		siperrornoex("Unknown Format sqlType=%d", sqlType);
		break;
	}
	memset(_param, 0, sizeof(_param));
	return _nbytes;
}

void	TPARAM_SQL::setParamType(sint16	sqlType)
{
	_sqltype	= sqlType;
	switch ( sqlType )
	{
	case SQL_INTEGER:			_chFormat = 'D';	break;
	case SQL_SMALLINT:			_chFormat = 'W';	break;
	case SQL_TINYINT:			_chFormat = 'B';	break;

	case SQL_CHAR:
	case SQL_VARCHAR:			_chFormat = 's';	break;

	case SQL_WCHAR:
	case SQL_WLONGVARCHAR:
	case SQL_WVARCHAR:			_chFormat = 'S';	break;

	case SQL_DATETIME:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:	_chFormat = 's';	break;

	case SQL_LONGVARBINARY:
	case SQL_VARBINARY:
	case SQL_BINARY:			_chFormat = 'I';	break;

	case SQL_NUMERIC:
	case SQL_DECIMAL:		//break;

	case SQL_BIGINT:
	case SQL_FLOAT:
	case SQL_REAL:			//break;

	case SQL_DOUBLE:		//break;
	default:	
		sipwarning("Unknown Parameter Type. sqlType=%d", sqlType);
		siperrornoex("Unknown Parameter Type. sqlType=%d", sqlType);
	}
}

void	TPARAM_SQL::GetRowColumn(ODBCRecord &row, int nColPos)
{
	memset(_param, 0, _nbytes);
	switch ( _sqltype )
	{
	case SQL_DATETIME:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:
	case SQL_CHAR:
	case SQL_VARCHAR:
		row.GetData(nColPos, _param, _nbytes, NULL, SQL_C_CHAR);
		break;
	case SQL_LONGVARBINARY:
	case SQL_VARBINARY:
	case SQL_BINARY:
		{
			SQLLEN	lBufSize = _len; // _nbytes;
			row.GetData(nColPos, _param, _len/*_nbytes*/, &lBufSize, SQL_C_BINARY);
			if (lBufSize >= 0)
				_nbytes = lBufSize;
			else
				_nbytes = 0;
		}
		break;
	case SQL_WCHAR:
	case SQL_WVARCHAR:
		row.GetData(nColPos, _param, _nbytes, NULL, SQL_C_WCHAR);
		break;
	default:
		row.GetData(nColPos, _param, _nbytes, NULL);
		break;
	}

}

//////////////////////////////////////////////////////////////////////////
// CQueryRequest
CQueryRequest::CQueryRequest(tstring &strQuery, TDBCompleteCallback callback, uint8 mode, uint8 prio, uint32 etime)
{
	_strQuery		= strQuery;
	_callback		= callback;
	_mode			= mode;
	_priority		= prio;
	_expiretimems	= etime;
	_requestTime	= CTime::getLocalTime(); 
}

CQueryRequest::CQueryRequest(const CQueryRequest& other)
{
	operator =(other);
}

CQueryRequest::~CQueryRequest()
{
}

CQueryRequest& CQueryRequest::operator =(const CQueryRequest& other)
{
	_callback	= other._callback;
	//	_hCompEvent = other._hCompEvent;
	_mode		= other._mode;
	_strQuery	= other._strQuery;
	_nParams	= other._nParams;
	_paramBuffer= other._paramBuffer;
	_strInFormat= other._strInFormat;
	_priority	= other._priority;
	_expiretimems = other._expiretimems;
	_requestTime = other._requestTime;
	
	return *this;
}

void	CQueryRequest::init()
{
	if ( _mode != EXECUTE_BLOCK )
		return;

	_nParams	= 0;
	_strInFormat	= "";

//	_hCompEvent = CreateEvent(NULL, FALSE, FALSE, _t("Req_CompletionEvent"));
}

void	CQueryRequest::setCompletionEvent()
{
//	SetEvent(_hCompEvent);
}

void	CQueryRequest::init(const char *strInFormat, va_list paramList)
{
	int	param_count = strlen(strInFormat);

	_nParams		= param_count;
	_strInFormat	= strInFormat;
	if ( strInFormat == "" )
		return;

	sipassert(!_paramBuffer.isReading());
	uint32		parammode = 0;
	for ( uint8 i = 0; i < param_count; i ++ )
	{
		parammode = va_arg(paramList, uint32);
		_paramBuffer.serial(parammode);

		char	c = strInFormat[i];
		_paramBuffer.serial(c);

		switch ( c )
		{
		case 'd':
			{
				sint32	param = va_arg(paramList, sint32);
				_paramBuffer.serial(param);
			}
			break;
		case 'D':
			{
				uint32	param = va_arg(paramList, uint32);
				_paramBuffer.serial(param);
			}
			break;
		case 'w':
			{
				sint16	param = va_arg(paramList, int);
				_paramBuffer.serial(param);
			}
			break;
		case 'W':
			{
				uint16	param = va_arg(paramList, unsigned int);
				_paramBuffer.serial(param);
			}
			break;
		case 'b':
			{
				sint8	param = va_arg(paramList, int);
				_paramBuffer.serial(param);
			}
			break;
		case 'B':
			{
				uint8	param = va_arg(paramList, unsigned int);
				_paramBuffer.serial(param);
			}
			break;
		case 'u':
		case 'U':
			{
				uint32	param	= va_arg(paramList, uint32);
				uint32	size	= va_arg(paramList, uint32);
				uint8 *buffer 	= (uint8 *)&param;
				// uint8 * buffer	= (uint8 *)param;
				// uint32 * buffer1= (uint32 *)param;
				_paramBuffer.serialBufferWithSize(buffer,size);
			}
			break;
		case 'P':
		case 'p':
			{
				uint64	param = va_arg(paramList, uint64);
				_paramBuffer.serial(param);
			}
			break;
		case 's':
			{
				char *param = va_arg(paramList, char *);
				std::string strParam(param);
				_paramBuffer.serial(strParam);
			}
			break;
		case 'S':
			{
				ucchar *param = va_arg(paramList, ucchar *);
				ucstring ucParam(param);
				_paramBuffer.serial(ucParam);
			}
			break;
		default:
			sipwarning("Unknown Format %s", c);
			siperrornoex("Unknown Format %s", c);
		}
	}
}

void	CQueryRequest::release()
{
	if ( _mode != EXECUTE_BLOCK )
		return;

//	if ( _hCompEvent != NULL )
//	{
//		SetEvent(_hCompEvent);

//		CloseHandle(_hCompEvent);
//		_hCompEvent = NULL;
//	}
}

void	CQueryRequest::serial (SIPBASE::IStream &s)
{
	s.serial(_strQuery);
	s.serial(_mode);
	s.serial(_priority);
	s.serial(_expiretimems);
	s.serial(_requestTime);
	ULONG_PTR	temp;
	if ( _mode == EXECUTE_BLOCK )
	{
//		if ( s.isReading() )
//		{
//			s.serial(temp);
//			_hCompEvent = (HANDLE) temp;
//		}
//		else
//		{
//			temp = (uint32) _hCompEvent;
//			s.serial(temp);
//		}
		return;
	}

	if ( s.isReading() )
	{
		s.serial(temp);
		_callback = (TDBCompleteCallback) temp;
	}
	else
	{
		temp = (ULONG_PTR) _callback;
		s.serial(temp);
	}
	s.serial(_strInFormat);
	s.serial(_nParams);
	if ( s.isReading() )
	{
		_paramBuffer.seek( 0, CMemStream::end );
		_paramBuffer.invert(); // write -> read, default : write mode
	}

	s.serialMemStream(_paramBuffer);
}

//////////////////////////////////////////////////////////////////////////
//CQueryResult
//////////////////////////////////////////////////////////////////////////
CQueryResult::CQueryResult(CQueryRequest &request, DBCONNECTION &DBCon)
{
	_strQuery	= request.Query();
	_callback	= request.Callback();
	_mode		= request.Mode();
	_ResultSet = NULL;
	_nRet = 0;

	//_DBStmt = new ODBCStmt(DBCon);
}

CQueryResult::CQueryResult(DBCONNECTION &DBCon)
:_callback(NULL)
,_mode(EXECUTE_NORMAL)
{
	//_DBStmt = new ODBCStmt(DBCon);
	_ResultSet = NULL;
	_nRet = 0;
}

CQueryResult::CQueryResult()
:_callback(NULL)
,_mode(EXECUTE_NORMAL)
{
	_ResultSet = NULL;
	_nRet = 0;
}

CQueryResult::~CQueryResult()
{
	delete _ResultSet;
	_ResultSet = NULL;
// 	if ( _DBStmt != NULL )
// 	{
// 		delete _DBStmt;
// 		_DBStmt = NULL;
// 	}
}

void	CQueryResult::init(CQueryRequest & request)
{
	_strQuery	= request.Query();
	_callback	= request.Callback();
	_mode		= request.Mode();

	CMemStream &	paramBuffer	= request.ParamBuffer();
	int				param_count	= request.ParamSize();
	std::string		strFormat	= request.ParamFormat();
	const char *	pkt_param	= strFormat.c_str();

	paramBuffer.seek( 0, CMemStream::begin );

	TParamPtr	paraminfo = new TPARAM_C;
	for ( uint8 i = 0; i < param_count; i ++ )
	{
		char	c = pkt_param[i];
		paraminfo->setParamType(c);
		paramBuffer.serial(*paraminfo);

		if ( paraminfo->isOUT_() || paraminfo->isCB_() )
			_ParamAry.push_back(paraminfo);
	}
	delete _ResultSet;
	_ResultSet = NULL;
}

void	CQueryResult::setResult(int nRet, CMemStream * resultset)
{
	_nRet		= nRet;
	_ResultSet	= resultset;
}

void	CQueryResult::release()
{
// 	if ( _DBStmt != NULL )
// 	{
// 		delete _DBStmt;
// 		_DBStmt = NULL;
// 	}

	if ( !_ParamAry.empty() )
		_ParamAry.clear();
}

void	CQueryResult::serial(SIPBASE::IStream &s)
{
	s.serial(_strQuery);
	s.serial(_mode);

	ULONG_PTR	temp;
	if ( s.isReading() )
	{
		s.serial(temp);
		_callback = (TDBCompleteCallback) temp;
	}
	else
	{
		temp = (ULONG_PTR) _callback;
		s.serial(temp);
	}

	s.serial(_nRet);

//	s.serialContPtr(_ParamAry);
}

void	CQueryResult::serialPtr(SIPBASE::IStream &s)
{
// 	if ( s.isReading() )
// 		crefs --;		
// 	else
// 		crefs ++;

	s.serial(_strQuery);
	s.serial(_mode);

	ULONG_PTR	temp;
	if ( s.isReading() )
	{
		s.serial(temp);
		_callback = (TDBCompleteCallback) temp;
		s.serial(temp);
		//_DBStmt = (DBSTMT *) temp;
		_ResultSet = (CMemStream *)temp;
	}
	else
	{
		temp = (ULONG_PTR) _callback;
		s.serial(temp);
		//temp = (uint32) _DBStmt;
		temp = (ULONG_PTR) _ResultSet;
		s.serial(temp);
	}

	s.serial(_nRet);

	uint32	nSize;
	if ( s.isReading() )
	{
		s.serial(nSize);
		for ( uint32 i = 0; i < nSize; i ++ )
		{
			TParamPtr	ptrParam = new TPARAM_C;
			TPARAM_C &	param = * ptrParam.getPtr();

			param.serialPtr(s);
			insertParamAry(ptrParam);
			//_ParamAry.push_back(ptrParam.getPtr());
		}

	}
	else
	{
		nSize = _ParamAry.size();
		s.serial(nSize);
		for ( uint32 i = 0; i < nSize; i ++ )
		{
			_ParamAry[i]->serialPtr(s);
		}
	}
}

void	CQueryResult::insertParamAry(TParamPtr paraminfo)
{
	_ParamAry.push_back(paraminfo);
}

} // SIPBASE
