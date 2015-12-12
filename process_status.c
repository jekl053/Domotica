/**********************************************
* File: proces_status.c
* Author: Jeroen Kloppers
* Omschrijving: Verwerking van commando's voor
*               de verschillende (specifieke)
*				statussen.
***********************************************/

#include "process_status.h"

int process_status_cmd( char *status, int value) {

	MYSQL *lcon = connect_domotica_db();
	char query[250];
			
	sprintf(query, "UPDATE domotica.`status` SET value = %i WHERE status = '%s'", value, status);
	query_db(lcon, query);
	
	mysql_close(lcon);
	
	return 0;
	
}