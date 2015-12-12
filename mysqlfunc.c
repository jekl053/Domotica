#include "mysqlfunc.h"

MYSQL * connect_domotica_db()
{
	MYSQL *con = mysql_init(NULL);

    if (con == NULL)
    {
        finish_with_error(con);
    }

    if (mysql_real_connect(con, "localhost", "domotica", "Home01", "domotica", 0, NULL, 0) == NULL)
    {
        finish_with_error(con);
    }

	return con;
}

void query_db(MYSQL *con, char *query)
{
	if (mysql_query(con, query))
	{
		logmsg(query, "mysql");
		finish_with_error(con);
	}
}

MYSQL_RES * query_res_db(MYSQL *con, char *query)
{
	
	MYSQL_RES *result;
	
	if (mysql_query(con, query))
	{
		logmsg(query, "mysql");
		finish_with_error(con);
	}

	result = mysql_store_result(con);
	if (result == NULL) 
	{
		finish_with_error(con);
	}
	
	return result;
}

MYSQL_RES * call_proc_db(MYSQL *con, char *proc)
{
	MYSQL_RES *result;
	char query[250];
	
	sprintf(query, "CALL %s", proc);
	result = query_res_db(con, query);

	mysql_next_result(con);
	
	return result;
}

void finish_with_error(MYSQL *con)
{
	char logtxt[250];

	sprintf(logtxt, "%s\n", mysql_error(con));
	printf("%s", logtxt);
	logmsg(logtxt, "mysql");
	mysql_close(con);
	exit(1);
}
