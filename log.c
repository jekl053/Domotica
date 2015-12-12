#include "log.h"

void logmsg(char *msg, char *node)
{
	char query[250];

// MySQL verbinding opzetten
	MYSQL *lcon = connect_domotica_db();
	
	sprintf(query, "INSERT INTO log ( msg, node ) VALUES ( '%s', '%s' )", msg, node);
	
	mysql_query(lcon, query);
	
 	mysql_close(lcon);

}
