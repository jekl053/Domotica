#include "domotica.h"

int main (void)
{
	int process_result;

	MYSQL_RES *res1;
	MYSQL_ROW row1;
	
	struct commando_struct cmd;
	char logtxt[250];
	
	logmsg("Domotica service gestart", "main");
	
// MySQL verbinding opzetten
    MYSQL *con1 = connect_domotica_db();

// Initialize !!
	initialize_domotica(con1);

	int x = piThreadCreate (processInputNodes);
	if (x != 0)
		printf("Thread processInputNodes did not start");

	int y = piThreadCreate (activityLed);
	if (y != 0)
		printf("Thread activityLed did not start");

	for (;;)
	{
		res1 = query_res_db(con1, "SELECT * FROM commando WHERE time <= NOW()");

		while ((row1 = mysql_fetch_row(res1)))
		{
			cmd.id = atol(row1[0]);
			strcpy(cmd.time, row1[1]);
			strcpy(cmd.cmd_type, row1[2]);
			strcpy(cmd.cmd, row1[3]);
			cmd.value = atol(row1[4]);
			
			sprintf(logtxt, "Commando verwerken: id: %i, time: %s, type: %s, cmd: %s, value: %i", cmd.id, cmd.time, cmd.cmd_type, cmd.cmd, cmd.value);
			logmsg(logtxt, cmd.cmd);
			
			if ((strcmp(cmd.cmd_type, "node") == 0))
			{
				process_result = process_node_cmd(cmd.cmd, cmd.value);
			} else if ((strcmp(cmd.cmd_type, "scene") == 0))
			{
				process_result = process_scene_cmd(cmd.cmd);
			} else if ((strcmp(cmd.cmd_type, "status") == 0))
			{
				process_result = process_status_cmd(cmd.cmd, cmd.value);
			} else if ((strcmp(cmd.cmd_type, "init") == 0))
			{
				process_result = process_init_cmd(cmd.cmd);
			}
			
			if (process_result == 0) {
				sprintf(logtxt, "Commando %s, waarde %i succesvol verwerkt", cmd.cmd, cmd.value);
				logmsg(logtxt, cmd.cmd);
			}
			else {
				sprintf(logtxt, "Fout bij verwerking commando %s waarde %i", cmd.cmd, cmd.value);
				logmsg(logtxt, cmd.cmd);
			}
				
			end_process_cmd(cmd.id, process_result);
		}
/*		if (row1 == NULL) {
			printf("Geen commando gevonden\n");
		}
*/		
		mysql_free_result(res1);

		delay(INT_MAIN_LOOP);
 	}

}

void initialize_domotica(MYSQL *con) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	struct node_struct node;
	char query[250];
	char logtxt[250];
	
	wiringPiSetup () ;
	
	// mcp23017 initialiseren
	sprintf(query, "SELECT * FROM node_type_comm WHERE type = 'mcp23017' AND actief = '1'");
	logmsg(query, "debug");
	res = query_res_db(con, query);
	while( (row = mysql_fetch_row(res) ) ) {
		mcp23017Setup (atoi(row[3]), (int)strtol(row[2], NULL, 0)) ;
		sprintf(logtxt, "Waarden 23017 setup: %i %i", atoi(row[3]), (int)strtol(row[2], NULL, 0));
		logmsg(logtxt, "debug");
	}
	//mcp23017Setup(101,0x20);
	
	// Tabel node_comm_init leeg maken
	sprintf(query, "TRUNCATE node_comm_init");
	query_db(con, query);
	
	// Tabel commando leeg maken
	sprintf(query, "TRUNCATE commando");
	query_db(con, query);
	
	sprintf(query, "proc_node_get_all_active()");
	res = call_proc_db(con, query);
	while ( (row = mysql_fetch_row(res)) ) {
		node = fill_node_struct(row);
		sprintf(query, "CALL proc_node_init('%s')", node.id);
		
		query_db(con, query);
	}
	
	// RF433 werkt anders, apart initialiseren
	initialize_rf433(con);
	
	mysql_free_result(res);
}

void initialize_rf433(MYSQL *con) {

	MYSQL_RES *res, *res2;
	MYSQL_ROW row, row2;
	char query[250];
	char logtxt[250];
	struct nt_rf433_equip_struct rf433_equip;
	struct node_type_comm_struct node_type_comm;
	int pinNr;
	
	sprintf(query, "SELECT * FROM nt_rf433_equip WHERE actief = 1");
	res = query_res_db(con, query);
	while ((row = mysql_fetch_row(res)) ) {
		strcpy(rf433_equip.id, row[0]);
		strcpy(rf433_equip.oms, row[1]);
		strcpy(rf433_equip.comm, row[2]);
		strcpy(rf433_equip.code, row[3]);
		strcpy(rf433_equip.io, row[4]);
		rf433_equip.actief = atol(row[5]);
		
		sprintf(query, "SELECT * FROM node_type_comm WHERE comm = '%s'", rf433_equip.comm);
		res2 = query_res_db(con, query);
		row2 = mysql_fetch_row(res2);
		strcpy(node_type_comm.comm, row2[0]);
		strcpy(node_type_comm.type, row2[1]);
		strcpy(node_type_comm.address, row2[2]);
		node_type_comm.base = atoi(row2[3]);
		node_type_comm.actief = atoi(row2[4]);
			
		if ( (strcmp(node_type_comm.type, "gpio") == 0)  || (strcmp(node_type_comm.type, "mcp23017") == 0) ) {
			pinNr = node_type_comm.base + atoi(rf433_equip.code);
			pinMode(pinNr, OUTPUT);
			sprintf(logtxt, "RF433 equip geinitialiseerd... pin nr %i", pinNr);
			logmsg(logtxt, "RF433 INIT");
		}
		else {
			sprintf(logtxt, "Verkeerde configuratie rf433 equip id %s, alleen gpio/mcp23017 mogelijk", rf433_equip.id);
			logmsg(logtxt, "RF433 INIT");
		}
	}
	
	mysql_free_result(res);
}

PI_THREAD(processInputNodes)
{
	MYSQL_RES *res2;
	MYSQL_ROW row2;
	char node[20];
	int process_result;
	char logtxt[200];

	// MySQL verbinding opzetten
    MYSQL *con2 = connect_domotica_db();

// Even wachten met starten, andere proces moet opgestart zijn
	delay(5000);
	
	for (;;) 
	{
		// Stored procedure aanroepen voor lezen van input nodes die gelezen moeten worden, interval verstreken
		res2 = call_proc_db(con2, "proc_nodes_input_get_next_read()");
		
		while ( (row2 = mysql_fetch_row(res2) ) ) {
			strcpy(node, row2[0]);
			process_result = read_input_node(node);

			if (process_result != 0) {
				sprintf(logtxt, "Fout bij lezen node %s", node);
				logmsg(logtxt, node);
			}
		}
		
		mysql_free_result(res2);
		delay(INT_NODE_LOOP);
	}
	mysql_close(con2);

}

PI_THREAD(activityLed) {

	int pinNr = 15;
	pinMode (pinNr, OUTPUT);
	
	for (;;) {
		digitalWrite(pinNr, 1);
		delay(1000);
		digitalWrite(pinNr, 0);
		delay(1000);
	}

}

void end_process_cmd(int id, int result) {

	MYSQL *lcon = connect_domotica_db();
	char query[250];
	
	sprintf(query, "DELETE FROM commando WHERE id = %i", id);
	query_db(lcon, query);
	
	mysql_close(lcon);
}
