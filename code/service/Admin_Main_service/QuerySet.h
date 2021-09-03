#pragma once

#ifdef MYSQL_USE

#else

// 모든 써버정보 얻기
#define MAKEQUERY_GETALLSERVER(strQ)				\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("SELECT * FROM tbl_Server"));
//// 모든 등록써비스정보 얻기
//#define MAKEQUERY_GETALLTGSERVICE(strQ)				\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT * FROM tbl_mstService"));
// 모든 동작써비스정보 얻기
#define MAKEQUERY_GETALLSERVICE(strQ)				\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("SELECT * FROM tbl_Service"));
//// 모든 지역싸이트정보 얻기
//#define MAKEQUERY_GETALLSHARD(strQ)					\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT * FROM tbl_Shard"));
// 모든 Zone정보 얻기
//#define MAKEQUERY_GETALLZONE(strQ)					\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT * FROM tbl_Zone"));

//// 모든 graph정보 얻기
//#define MAKEQUERY_GETALLGRAPH(strQ)					\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT VariableId, path, graph_update FROM tbl_Variables WHERE graph_update <> 0"));
// 모든 경고정보 얻기
//#define MAKEQUERY_GETALLALERT(strQ)					\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT VariableId, path, error_bound, alarm_order FROM tbl_Variables WHERE error_bound <> -1"));
// 모든 관리자정보 얻기
#define MAKEQUERY_GETALLMANAGER(strQ)				\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("SELECT * FROM tbl_Manager"));
//// 모든 Host관리자권한정보 얻기
//#define MAKEQUERY_GETALLMANAGEHOSTPOWER(strQ)				\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("SELECT * FROM tbl_mstMHP"));
// 모든 감시변수정보 얻기
#define MAKEQUERY_GETALLMONITORVARIABLE(strQ)		\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("SELECT * FROM tbl_MonitorVariable"));
// 감시변수삭제
#define MAKEQUERY_DELETEMV(strQ, uID)		\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("DELETE FROM tbl_MonitorVariable WHERE Id = %d"), uID);
// 감시변수추가
#define MAKEQUERY_ADDMV(strQ, sPath, uCycle, uDefault, uUp, uLow)		\
	tchar strQ[600] = _t("");						\
	smprintf(strQ, 256, _t("{call Admin_AddMV('%s', %d, %d, %d, %d)}"), sPath, uCycle, uDefault, uUp, uLow);
// 이름과 암호로 관리자정보 얻기
#define MAKEQUERY_GETMANAGERONNAME(strQ, strName, strPass)			\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("SELECT ManagerId, Role, ShardId FROM tbl_Manager WHERE ManagerAccount='%s' AND Password='%s'"), strName, strPass);
// 관리자암호변경
#define MAKEQUERY_UPDATEPWD(strQ, nID, sPassword)		\
	tchar strQ[300] = _t("");						\
	smprintf(strQ, 256, _t("UPDATE tbl_Manager SET Password='%s' WHERE ManagerId=%d"), sPassword, nID);
// 관리자이름변경
#define MAKEQUERY_UPDATENAME(strQ, nID, sName)		\
	tchar strQ[300] = _t("");						\
	smprintf(strQ, 256, _t("UPDATE tbl_Manager SET ManagerAccount='%s' WHERE ManagerId=%d"), sName, nID);
// 관리자추가
#define MAKEQUERY_INSERTMANAGER(strQ, strName, strPass, uPower)			\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("{call Admin_AddManager('%s', '%s', %d)}"), strName, strPass, uPower);
// 관리자삭제
#define MAKEQUERY_DELETEMANAGER(strQ, uID)			\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("UPDATE tbl_Manager SET DeleteFlag=1 WHERE ManagerId=%d"), uID);
// 관리로그얻기
#define MAKEQUERY_LOG(strQ, nPage, uPower)		\
	tchar strQ[300] = _t("");						\
	smprintf(strQ, 256, _t("{call Admin_Log(%d, %d)}"), nPage, uPower);
// 관리로그Reset
#define MAKEQUERY_LOGRESET(strQ)					\
	tchar strQ[300] = _t("");						\
	smprintf(strQ, 256, _t("DELETE FROM tbl_LogManage"));
// HostVersion갱신
#define MAKEQUERY_UPDATEVERSION(strQ, nID, sVersion)		\
	tchar strQ[300] = _t("");						\
	smprintf(strQ, 256, _t("UPDATE tbl_Server SET Version='%s' WHERE ServerId=%d"), sVersion, nID);

//// 등록써비스추가
//#define MAKEQUERY_REGISTERSERVICE(strQ, strAlias, strExe, strShort, strLong)			\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("INSERT INTO tbl_mstService (AliasName, ExeName, ShortName, LongName) VALUES (N'%s', '%s', '%s', '%s')"), strAlias, strExe, strShort, strLong);
// 등록써비스삭제
//#define MAKEQUERY_DELETEREGSERVICE(strQ, uid)			\
//	tchar strQ[256] = _t("");						\
//	smprintf(strQ, 256, _t("DELETE FROM tbl_mstService WHERE ServiceId = %d"), uid);
// 동작써비스추가
#define MAKEQUERY_ADDSERVICE(strQ, tgsid, serverid, shardid)				\
	tchar strQ[256] = _t("");												\
	smprintf(strQ, 256, _t("{call Admin_AddService1(%d, %d, %d)}"), tgsid, serverid, shardid);
// 동작써비스삭제
#define MAKEQUERY_DELETESERVICE(strQ, uid)			\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("DELETE FROM tbl_Service WHERE ServiceInsId = %d"), uid);
// 써버추가
#define MAKEQUERY_ADDHOST(strQ, strHost)				\
	tchar strQ[256] = _t("");												\
	smprintf(strQ, 256, _t("{call Admin_AddHost('%s')}"), strHost);
// 써버삭제
#define MAKEQUERY_DELETESERVER(strQ, uid)			\
	tchar strQ[256] = _t("");						\
	smprintf(strQ, 256, _t("DELETE FROM tbl_Server WHERE ServerId = %d"), uid);
// 지역싸이트추가
#define MAKEQUERY_ADDSHARD(strQ, strShard, uZone)				\
	tchar strQ[256] = _t("");												\
	smprintf(strQ, 256, _t("{call Admin_AddShard(N'%s', %d)}"), strShard, uZone);
// 지역싸이트삭제
#define MAKEQUERY_DELSHARD(strQ, uid)				\
	tchar strQ[256] = _t("");												\
	smprintf(strQ, 256, _t("DELETE FROM tbl_Shard WHERE ShardId = %d"), uid);
//// 그라프추가
//#define MAKEQUERY_ADDGRAPH(strQ, strPath, uGraph)				\
//	tchar strQ[256] = _t("");												\
//	smprintf(strQ, 256, _t("INSERT INTO tbl_Variables(Path, Error_bound, Alarm_order, Graph_update) VALUES ('%s', '-1', 'gt', %d)"), strPath, uGraph);
//// 그라프삭제
//#define MAKEQUERY_DELGRAPH(strQ, uid)				\
//	tchar strQ[256] = _t("");												\
//	smprintf(strQ, 256, _t("DELETE FROM tbl_Variables WHERE VariableId = %d"), uid);
//// 경고추가
//#define MAKEQUERY_ADDALERT(strQ, strPath, ubound, strOrder)				\
//	tchar strQ[256] = _t("");												\
//	smprintf(strQ, 256, _t("INSERT INTO tbl_Variables(Path, Error_bound, Alarm_order, Graph_update) VALUES ('%s', %d, '%s', 0)"), strPath, ubound, strOrder);
//// 갱신써버정보얻기
//#define MAKEQUERY_GETSHARDUPDATESERVERINFO(strQ, ShardId)				\
//	tchar strQ[256] = _t("");												\
//	smprintf(strQ, 256, _t("SELECT LastVersion, RootUrl, Port, UserId, UserPassword, Path FROM tbl_ShardUpdateInfo WHERE ShardId=%d"), ShardId);
// 관리로그추가
#define MAKEQUERY_ADDLOG(strQ, uType, ucContent, uPower, ucAccount)					\
	tchar strQ[1000] = _t("");												\
	smprintf(strQ, 256, _t("INSERT INTO tbl_LogManage(Type, Content, Power, Account) VALUES (%d, N'%s', %d, '%s')"), uType, ucContent, uPower, ucAccount);
// Update정보얻기
#define MAKEQUERY_GETUPDATEINFO(strQ)					\
	tchar strQ[100] = _t("");												\
	smprintf(strQ, 256, _t("SELECT * FROM tbl_UpdateInfo"));
// Update정보갱신
#define MAKEQUERY_CHUPDATEINFO(strQ, ucServer, uPort, ucName, sPass, ucFile)					\
	tchar strQ[1000] = _t("");																	\
	smprintf(strQ, 256, _t("UPDATE tbl_UpdateInfo SET Address=N'%s', Port=%d, Username=N'%s', Password='%s', Path=N'%s'"), ucServer, uPort, ucName, sPass, ucFile);

#endif
