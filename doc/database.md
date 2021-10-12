- At first you must install freetds package. Use the below command.
	yum install freetds-devel

- Configurate ODBC Driver to use FreeTDS package.
	For this, you have to add the below lines to the end of /etc/odbcinst.ini
		[FreeTDS]	# driver name which you can use.
		Description	= description of this driver
		Driver		= indicate driver file to drive freetds connection. (Default path is /usr/lib64/libtdsodbc.so if you install freetds)
		Setup		= indecate setup file to drive freetds connection. (Default path is /usr/lib64/libtdsS.so if you install freetds-devel)
		FileUsage	= 1
		

If you setup the above configuration, then you can use like this.
	/*
	 * application call
	 */
	const char servername[] = "jdbc.sybase.com";
	sprintf(tmp, "DRIVER=FreeTDS;SERVER=%s;UID=%s;PWD=%s;DATABASE=%s;",
		servername, username, password, dbname);
	res = SQLDriverConnect(Connection, NULL, (SQLCHAR *) tmp, SQL_NTS,
				(SQLCHAR *) tmp, sizeof(tmp), &len, SQL_DRIVER_NOPROMPT);
	if (!SQL_SUCCEEDED(res)) {
		printf("Unable to open data source (ret=%d)\n", res);
		exit(1);
	}


- (Optional) You may set the servername by using freetds.conf(default path is /etc/freetds.conf) like this.
	[JDBC]
	host = jdbc.sybase.com
	port = 4444
	tds version = 5.0 (reference https://www.freetds.org/userguide/ChoosingTdsProtocol.html) to select a fit version.

If this configuration is selected, then you can use like this.
	/*
	 * application call
	 */
	const char servername[] = "JDBC";
	sprintf(tmp, "DRIVER=FreeTDS;SERVERNAME=%s;UID=%s;PWD=%s;DATABASE=%s;",
		servername, username, password, dbname);
	res = SQLDriverConnect(Connection, NULL, (SQLCHAR *) tmp, SQL_NTS,
				(SQLCHAR *) tmp, sizeof(tmp), &len, SQL_DRIVER_NOPROMPT);
	if (!SQL_SUCCEEDED(res)) {
		printf("Unable to open data source (ret=%d)\n", res);
		exit(1);
	}
