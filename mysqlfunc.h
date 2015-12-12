#ifndef MYSQLFUNC_H
#define MYSQLFUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "log.h"

MYSQL * connect_domotica_db();
void query_db(MYSQL *con, char *query);
MYSQL_RES * query_res_db(MYSQL *con, char *query);
MYSQL_RES * call_proc_db(MYSQL *con, char *proc);
void finish_with_error(MYSQL *con);


#endif