/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/queryRequest.h"
#include "misc/connectionPool.h"

namespace	SIPBASE
{
typedef		CSmartPtr<DBSTMT>	TDBStmtPtr;

// CDBConnector
CDBConnector::CDBConnector()
{
}

CDBConnector::~CDBConnector()
{
}

int		CDBConnector::_dbQuery(CQueryRequest & request, tstring &strQuery, CQueryResult *result)
{
	DB_EXE_QUERY(_DBConnection, DBStmt, strQuery.c_str());
	std::string		strFormat	= request.ParamFormat();
	const char *	pkt_param	= strFormat.c_str();
	CMemStream &	paramBuffer	= request.ParamBuffer();
	int				param_count	= request.ParamSize();

	if ( result == NULL )
		return nQueryResult;

	//	TPARAM_C		paraminfo;
	for ( uint8 i = 0; i < param_count; i ++ )
	{
		char	c = pkt_param[i];
		TParamPtr	paramPtr = new TPARAM_C(c);
		paramBuffer.serial(*paramPtr);

		if ( paramPtr->isOUT_() || paramPtr->isCB_() )
			result->insertParamAry(paramPtr);
	}

	CMemStream	*resBuffer = new CMemStream;
	_FetchBuffer(&DBStmt, resBuffer);

	BOOL	bFreeSuccess = DBStmt.FreeHandle();
	if (!bFreeSuccess)
		nQueryResult = false;

	result->setResult(nQueryResult, resBuffer);

	//delete	DBStmt;

	return nQueryResult;
}

#define MAX_STMTCOLUMNCOUNT 64
int		CDBConnector::_FetchBuffer(DBSTMT * DBStmt, CMemStream	*dbResSet)
{
	//////////////////////////////////////////////////////////////////////////
	// Fetch
	TPARAM_SQL	ResField[MAX_STMTCOLUMNCOUNT];
	do 
	{
		uint32		nLen		= DBStmt->GetColumnCount();
		if (nLen >= MAX_STMTCOLUMNCOUNT)
			siperrornoex("Over flow column count of record");
		
		// TPARAM_SQL	*ResField	= new TPARAM_SQL[nLen];
		//////////////////////////////////////////////////////////////////////////
		// BindCol

		int			_len_o	= 0;
		for ( uint32 i = 0; i < nLen; i ++ )
		{
			int		nCol	= i + 1;
			sint16	sqlType = DBStmt->GetColumnType(nCol);
			int		len		= DBStmt->GetColumnSize(nCol);

			uint32	nBytes = ResField[i].init(sqlType, len);
			//DBStmt->BindColumn((uint16) nCol, ResField[i].param(), (int)nBytes, &_len_o, sqlType);
		}

		uint32	nRowCount = 0;
		sint32	counterPos = dbResSet->reserve(sizeof(nRowCount));
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		while( true )
		{
			BOOL bFetch = DBStmt->Fetch();
			if (bFetch)
			{
				ODBCRecord	row((HSTMT)(*DBStmt));
				nRowCount ++;
				for ( uint32 i = 0; i < nLen; i ++ )
				{
					ResField[i].GetRowColumn(row, i+1);
					dbResSet->serial(ResField[i]);
				}
			}
			else
			{
				break;
			}
		}
		dbResSet->poke(nRowCount, counterPos);
		//delete[]	ResField;

	} while ( DBStmt->MoreResults() );

	return 1;
}

int		CDBConnector::_dbParamQuery(CQueryRequest & request, tstring &strQuery, CQueryResult *result)
{
	DB_EXE_PREPARESTMT(_DBConnection, DBStmt, strQuery.c_str());
	if ( ! nPrepareRet )
		return 0;
/*
	int nPrepareRet	= 1;
	int nRet		= 1;
#ifdef	MYSQL_USE
	TDBStmtPtr DBStmt = new CMysqlStmt(_DBConnection);
	ucstring query = strQuery;
	std::string strUtf8 =query.toUtf8();
	nPrepareRet = Stmt.Prepare(strUtf8.c_str());
	if ( ! nPrepareRet )
	{
		sipwarning("@@@@@DBCALL: %s: prepare statement failed!", strQuery.c_str());
		return 0;
	}
	int nBindResult = true;
	int nRet = true;
#else
	//TDBStmtPtr DBStmt = new ODBCStmt(_DBConnection);
	DBSTMT DBStmt(_DBConnection);
	nPrepareRet = DBStmt.Prepare(strQuery.c_str());
	if ( ! nPrepareRet )
	{
		sipwarning("@@@@@DBCALL: %s: prepare statement failed!", strQuery.c_str());
		return 0;
	}
	int nBindResult = TRUE;
#endif
*/
	std::string		strFormat	= request.ParamFormat();
	const char *	pkt_param	= strFormat.c_str();
sipinfo("pkt_param = %s, strQuery = %S", pkt_param, strQuery.c_str());
	CMemStream &	paramBuffer	= request.ParamBuffer();
	int				param_count	= request.ParamSize();

	int len[MAX_DB_PARAMS];

	for ( uint8 i = 0; i < param_count; i ++ )
	{
		// 			param_mode = va_arg(paramList, uint8);
		// 			paramBuffer.serial(param_mode);

		char	c = pkt_param[i];
		result->_paraminfo[i] = new TPARAM_C(c);
		TParamPtr	ptr = result->_paraminfo[i];
		paramBuffer.serial(*ptr);

		if ( ptr->isIO_() )
		{
			DB_EXE_BINDPARAM_IO(DBStmt, i+1, ptr->ctype(), ptr->sqltype(), ptr->len(), 0, ptr->param(), ptr->nbytes(), (SQLLEN*)&len[i]);
		}
		else
		{
			if ( ptr->isIN_())
			{
				len[i] = ptr->nbytes();
				//DB_EXE_BINDPARAM_IN(DBStmt, i+1, ptr->ctype(), ptr->sqltype(), 2048, 0, ptr->param(), 0, (SQLLEN*)&len[i]);
				DB_EXE_BINDPARAM_IN(DBStmt, i+1, ptr->ctype(), ptr->sqltype(), len[i], 0, ptr->param(), 0, (SQLLEN*)&len[i])
			}
			if ( ptr->isOUT_())
			{
				len[i] = 0;
				DB_EXE_BINDPARAM_OUT(DBStmt, i+1, ptr->ctype(), ptr->sqltype(), 0, 0, ptr->param(), ptr->nbytes(), (SQLLEN*)&len[i]);
			}
		}
 
		if ( ! nBindResult )
			return 0;

		if ( result )
			if ( ptr->isOUT_() || ptr->isCB_() )
				result->insertParamAry(ptr);
	}

	DB_EXE_PARAMQUERY(DBStmt, strQuery.c_str());

	CMemStream	*resBuffer = new CMemStream;
	_FetchBuffer(&DBStmt, resBuffer);

	BOOL	bFreeSuccess = DBStmt.FreeHandle();
	if (!bFreeSuccess)
		nQueryResult = false;

	result->setResult(nQueryResult, resBuffer);

	//delete	DBStmt;

	return nQueryResult;
}

int	CDBConnector::executeSP(CQueryRequest & request, CQueryResult *result)
{
	int		nRet = 0;
	tstring strQuery = request.Query();
	uint8	uMode = request.Mode();
	switch( uMode )
	{
	case EXECUTE_NORMAL:
	case EXECUTE_BLOCK:
		nRet = _dbQuery(request, strQuery, result);
		break;
	case EXECUTE_PARAM:
		nRet = _dbParamQuery(request, strQuery, result);
		break;
	default:
		sipwarning("@@@@@DBCALL: Unknown Execute Mode Query <%s>", strQuery.c_str());
	}

	// 	if ( result != NULL )
	// 	{
	// 		result->init(request);
	// 		result->setResult(nRet, _DBStmt);
	// 	}

	return nRet;
}

void	CDBConnector::pingDB()
{
	DB_EXE_QUERY(_DBConnection, DBStmt, _t("SELECT GETDATE()"));
}

bool	CDBConnector::connect(ucstring ucLogin, ucstring ucPassword, ucstring ucDBHost, ucstring ucDBName)
{
	_DBConnection.Release();
	BOOL	bConnect = _DBConnection.Connect(ucLogin.c_str(), ucPassword.c_str(), ucDBHost.c_str(), ucDBName.c_str());
	if(bConnect == true)
	{
		sipdebug("DBCallThread DBConnection Handle=0x%x", _DBConnection.m_hDBC);
	}
	return 	(bConnect == true);
}

void	CDBConnector::disconnect()
{
	if ( _DBConnection.IsConnected() )
		_DBConnection.Disconnect();
}

} // SIPBASE
