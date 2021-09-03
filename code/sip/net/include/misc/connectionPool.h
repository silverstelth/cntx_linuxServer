/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CONNECTIONPOOL_H__
#define __CONNECTIONPOOL_H__

namespace SIPBASE
{
	class CDBConnector
	{
	public:
		bool	connect(ucstring ucLogin, ucstring ucPassword, ucstring ucDBHost, ucstring ucDBName);
		void	disconnect();
		int		executeSP(CQueryRequest & request, CQueryResult *result = NULL);
		void	pingDB();
		void	SetReconnectFlag() { _DBConnection.SetReconectFlag(); }
		bool	IsReconnectFlag() { return _DBConnection.GetReconnectFlag(); }
		CDBConnector();
		~CDBConnector();

	public:
		DBCONNECTION&		DBConnection() { return _DBConnection; };

	protected:
		int		_dbQuery(CQueryRequest & request, tstring &strQuery, CQueryResult *result);
		int		_dbParamQuery(CQueryRequest & request, tstring &strQuery, CQueryResult *result);
		int		_FetchBuffer(DBSTMT * DBStmt, CMemStream	*dbResSet);

	private:
		//TParamPtr		paraminfo[10];

		DBCONNECTION	_DBConnection;
	};
} // SIPBASE
#endif // end of guard __CONNECTIONPOOL_H__
