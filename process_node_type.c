/**********************************************
* File: process_node_type.c
* Author: Jeroen Kloppers
* Omschrijving: Verwerking van commando's voor
*               de verschillende (specifieke)
*				node_types. Database Domotica,
*				tabellen nodes en node_types
***********************************************/
#include "process_node_type.h"

/********************************************************************************************
*																							*
*	INT PROCESS_NODE_CMD																	*
*																							*
*********************************************************************************************/
int process_node_cmd(char *node_in, int value_in) {

	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[250];
	
	char logtxt[250];
	int ret;
	int num;

	struct node_struct node;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();

	sprintf(logtxt, "Commando voor node %s verwerken, waarde %i", node_in, value_in);
	logmsg(logtxt, node_in);
	
// Node gegevens ophalen
	sprintf(query,"SELECT * FROM nodes WHERE id = '%s'", node_in);
	logmsg(query, node_in);
	result = query_res_db(con, query);
	if ( (num = mysql_num_rows(result)) == 0 ) {
		sprintf(logtxt, "Node %s bestaat niet, einde verwerking", node_in);
		logmsg(logtxt, node_in);
		mysql_free_result(result);
		mysql_close(con);
		return 0;
	}
	
	row = mysql_fetch_row(result);
	
	node = fill_node_struct(row);

	// Controle of node actief is
	if (node.actief != 1) {
		sprintf(logtxt, "Node %s is niet actief, einde verwerking", node.id);
		logmsg(logtxt, node.id);
		mysql_free_result(result);
		mysql_close(con);
		return 0;
	}

// Controleren of waarde gewijzigd is
/* Controle uitschakelen
	if (node.value == value_in) {
		sprintf(logtxt, "Node %s : huidige waarde %i, nieuwe waarde %i, geen aanpassing, einde verwerking", node.id, node.value, value_in);
		logmsg(logtxt, node.id);
		mysql_free_result(result);
		mysql_close(con);
		return 0;
	}
*/

// Welk node type ?
	if (strcmp(node.type, "rf433") == 0)  {
		ret = process_nt_rf433(node.id, node.nt_id, value_in);
	} else if ( strcmp(node.type, "relay") == 0 ) {
		ret = process_nt_general(node, value_in);	
	} else {
		sprintf(logtxt, "Onbekende node type: %s - %s", node.id, node.type);
		logmsg(logtxt, node.id);
		ret = -1;
	}

// Afhandeling interval bij Output nodes, default waarde is 0
	if ( node.io == 'O' && strcmp(node.interval, "00:00:00") != 0  ) {
		printf("Test interval OK!\n");
	
	}


	if (ret == 0) {
		log_to_firebase(node.id);
	}
	
	mysql_free_result(result);
	mysql_close(con);
	
	return ret;

}

/********************************************************************************************
*																							*
*	INT PROCESS_SCENE_CMD																	*
*																							*
*********************************************************************************************/
int process_scene_cmd(char *scene_in) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	MYSQL_RES *res_time;
	MYSQL_ROW row_time;
	char query[250];
	char logtxt[250];
	int num;
	char time[20];

// MySQL connectie maken
	MYSQL *con = connect_domotica_db();

	
// Scene lezen, definitie lezen en commando's in commando tabel plaatsen
	sprintf(query, "SELECT * FROM scene WHERE id = '%s'", scene_in );
	logmsg(query, scene_in);

	res = query_res_db(con, query);
	if ( (num = mysql_num_rows(res)) == 0 ) {
		sprintf(logtxt, "Scene %s bestaat niet, einde verwerking", scene_in);
		logmsg(logtxt, scene_in);
		mysql_free_result(res);
		mysql_close(con);
		return 1;
	}
	 
	
// Scene bestaat, regels opzoeken
	sprintf(query, "SELECT * FROM scene_def WHERE scene_id = '%s' ORDER BY volgnr", scene_in);
	logmsg(query, scene_in);
	res = query_res_db(con, query);
	num = mysql_num_rows(res);
	sprintf(logtxt, "Aantal regels %i", num);
	logmsg(logtxt, "debug");
	
	res_time = query_res_db(con, "SELECT NOW()");
	row_time = mysql_fetch_row(res_time);
	strcpy(time, row_time[0]);
	
	while ( (row = mysql_fetch_row(res)) ) {

	//  TODO: insert commando aanpassen, rekening houden met timing in een scenario
		sprintf(query, "SELECT ADDTIME('%s', '%s')", time, row[5]);
		res_time = query_res_db(con, query);
		row_time = mysql_fetch_row(res_time);
		strcpy(time, row_time[0]);
		
		sprintf(query, "INSERT INTO commando (time, cmd_type, cmd, value) VALUES ( '%s', '%s', '%s', %s )", time, row[2], row[3], row[4]);
		logmsg(query, "debug");
		query_db(con, query);
	}
	
	int ret = 0;

	return ret;
}

/********************************************************************************************
*																							*
*	INT PROCESS_INIT_CMD																	*
*																							*
*********************************************************************************************/
int process_init_cmd(char *node_in) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	char comm[20]; 
	char code[20] ;
	char comm_type[30];
	int pinnr;
	int pin_base;
	char logtxt[250];
	
	struct node_struct node;
	struct node_type_struct node_type;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(logtxt,"Init node %s", node_in);
	logmsg(logtxt, "init");

// node gegevens lezen
	sprintf(query, "proc_node_get('%s')", node_in);
	res = call_proc_db(con, query);
	row = mysql_fetch_row(res);
	node = fill_node_struct(row);
	
	if ((strcmp(node.type, "rf433") == 0) || (strcmp(node.type, "ds18b20") == 0 ) || (strcmp(node.type, "cpu_temp") == 0 ) || (strcmp(node.type, "weewx") == 0 ) )	{
		sprintf(logtxt, "Node type %s wordt separaat geinitialiseerd, einde verwerking", node.type);
		logmsg(logtxt, "init");
		mysql_free_result(res);
		mysql_close(con);
		return 0;
	}
	
// node type gegevens lezen
	sprintf(query, "proc_node_type_get('%s')", node.type);
	res = call_proc_db(con, query);
	row = mysql_fetch_row(res);
	strcpy(node_type.type, row[0]);
	strcpy(node_type.table, row[1]);
	node_type.io = row[2][0];
	strcpy(node_type.oms, row[3]);
	
	
	// bepalen gpio of i2c io	
	sprintf(query, "SELECT comm, code FROM %s WHERE id = %i", node_type.table, node.nt_id);
	res = query_res_db(con, query);
	row = mysql_fetch_row(res);
	strcpy(comm, row[0]);
	strcpy(code, row[1]);
	
	// bepalen type comm
	sprintf(query, "SELECT type, base FROM node_type_comm WHERE comm = '%s'", comm);
	logmsg(query, "debug");
	res = query_res_db(con, query);
	row = mysql_fetch_row(res);
	strcpy(comm_type, row[0]);
	pin_base = atoi(row[1]);

// poort of pinnr zetten op juiste mode (output/input)
	if ( (strcmp(comm_type, "gpio") == 0) || (strcmp(comm_type, "mcp23017") == 0) ) {
		pinnr = pin_base + atol(code);
		sprintf(logtxt, "SET GPIO pin %i, mode %c", pinnr, node.io);
		logmsg(logtxt, "init");
		if (node.io == 'I') 
			pinMode (pinnr, INPUT) ;
		else
			pinMode (pinnr, OUTPUT);
			
		sprintf(query, "INSERT INTO node_comm_init ( node, io, gpio_pin ) VALUES ( '%s', '%c', %i )", node.id, node.io, pinnr);
	}
	else {
// selecteren uit table nt_type_com en address van i2c lezen, poort op O of I zetten	
	}

	query_db(con, query);
	
	mysql_free_result(res);
	mysql_close(con);
	
	return 0;
}

/********************************************************************************************
*																							*
*	INT READ_INPUT_NODE																		*
*																							*
*********************************************************************************************/
int read_input_node(char *node_in) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	char logtxt[250];
	struct node_struct node;
	int process_result;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
// Controleren of het Input node betreft, dan waarde lezen van node
	//printf("Node verwerking van node %s\n", node_in);

	sprintf(query, "proc_node_get('%s')", node_in);
	res = call_proc_db(con, query); 
	
	row = mysql_fetch_row(res);

	node = fill_node_struct(row);
	
	if ( (node.io != 'I') ) {
		sprintf(logtxt, "Node %s geen input node, einde verwerking", node_in);
		logmsg(logtxt, node_in);
		mysql_free_result(res);
		mysql_close(con);
		return 1;
	}
		
	if (strcmp(node.type, "pir") == 0 ) {
		process_result = process_nt_pir(node);
	} else if (strcmp(node.type, "dht11") == 0 ) {
		process_result = process_nt_dht11(node);
	} else if (strcmp(node.type, "ds18b20") == 0 ) {
		process_result = process_nt_ds18b20(node);
	} else if (strcmp(node.type, "lightdet") == 0 ) {
		process_result = process_nt_lightdet(node);
	} else if (strcmp(node.type, "cpu_temp") == 0) {
		process_result = process_nt_cputemp(node);
	} else if (strcmp(node.type, "weewx") == 0) {
		process_result = process_nt_weewx(node);
	} else {
		sprintf(logtxt, "Node type %s niet bekend, einde verwerking", node.type);
		logmsg(logtxt, node.id);
	}
	
	
	mysql_free_result(res);
	mysql_close(con);

	return process_result;
}

/********************************************************************************************
*																							*
*	VOID SET_NODE_VALUE																		*
*																							*
*********************************************************************************************/
void set_node_value(char* node, int value) {

	char query[250];

// MySQL connectie maken
	MYSQL *con = connect_domotica_db();

	sprintf(query, "UPDATE nodes SET value = %i WHERE id = '%s'", value, node);
	query_db(con, query);
	logmsg(query, node);
	
	mysql_close(con);
	
}

/********************************************************************************************
*																							*
*	STRUCT NODE_STRUCT FILL_NODE_STRUCT														*
*																							*
*********************************************************************************************/
struct node_struct fill_node_struct(MYSQL_ROW row) {

	struct node_struct node;
	
	// Node structuur
	strcpy(node.id, 		row[0]);
	strcpy(node.oms,		row[1]);
	strcpy(node.type,	 	row[2]);
	node.io = row[3][0]; // eerste character uit de string, node.io is char, niet char *
	node.nt_id = 			atol(row[4]);
	node.value = 			atol(row[5]);
	strcpy(node.value_oms,	row[6]);
	strcpy(node.locatie,	row[7]);
	node.actief =			atol(row[8]);
	strcpy(node.interval,	row[9]);
	strcpy(node.timestamp , row[10]);

	return node;
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_DEFAULT																	*
*																							*
*********************************************************************************************/
int process_nt_default(struct node_struct node) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	int value;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(query, "proc_node_get_comm('%s')", node.id);
	res = call_proc_db(con, query);
	row = mysql_fetch_row(res);
	
	if (strcmp(row[4], "0") == 0) { // GPIO pin
		value = digitalRead(atoi(row[3]));
		sprintf(query, "UPDATE nodes SET value = %i WHERE id = '%s'", value, node.id);
		query_db(con, query);
	} else {
	// i2c uitlezen
	}
	
	mysql_free_result(res);
	mysql_close(con);
	
	return 0;
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_RF433																	*
*																							*
*********************************************************************************************/
int process_nt_rf433( char *node, int nt_id, int value) {
	
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[250];
	char commando[250];
	
	struct nt_rf433_struct rf433;
	struct nt_rf433_type_struct rf433_type;
	struct nt_rf433_equip_struct rf433_equip;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
// Node type regel lezen
	sprintf(query, "SELECT * FROM nt_rf433 WHERE id = %i", nt_id);
	result = query_res_db(con, query); 
	row = mysql_fetch_row(result);
	
	rf433.id = atol(row[0]);
	strcpy(rf433.type, row[1]);
	strcpy(rf433.code, row[2]);
	
	mysql_free_result(result);

// Welk soort rf433 unit hebben we hier?
	sprintf(query, "SELECT * FROM nt_rf433_type WHERE type = '%s'", rf433.type);
	result = query_res_db(con, query); 
	row = mysql_fetch_row(result);
	
	strcpy(rf433_type.type, row[0]);
	strcpy(rf433_type.oms, row[1]);
	
	mysql_free_result(result);

// Transmitter selecteren
	sprintf(query, "SELECT * FROM nt_rf433_equip WHERE io = 'O' AND actief = 1");
	result = query_res_db(con, query);

	while ( (row = mysql_fetch_row(result)) ) {

// Struct vullen
		strcpy(rf433_equip.id, row[0]);
		strcpy(rf433_equip.oms, row[1]);
		strcpy(rf433_equip.comm, row[2]);
		strcpy(rf433_equip.code, row[3]);
		strcpy(rf433_equip.io, row[4]);
		rf433_equip.actief = atol(row[5]);
		
// Specifieke commando samenstellen en uitvoeren
		sprintf(commando, "/usr/local/bin/lights %s %s %s %s", rf433_equip.code, rf433_type.type, rf433.code, ((value) ? "on":"off") );
		logmsg(commando, node);
		system(commando);
	}
	
	mysql_free_result(result);

// Status node omzetten naar nieuwe waarde
	set_node_value(node, value);

	mysql_close(con);
	
	return 0;
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_PIR																		*
*																							*
*********************************************************************************************/
int process_nt_pir(struct node_struct node) {

	//printf("Functie PIR node %s\n", node.id);
	return process_nt_default(node);
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_DHT11																	*
*																							*
*********************************************************************************************/
int	process_nt_dht11(struct node_struct node) {

	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j    	    = 0, i;
 
	int dht11_dat[5] = { 0, 0, 0, 0, 0 };
 
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

	char ltemp[5], lhum[5];
	CURL *curl;
	char url[250]; 
	FILE *devnull = fopen("/dev/null", "w+");
	
	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	int pinnr;
	
	int data_ok = 0;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(query, "proc_node_get_comm('%s')", node.id);
	res = call_proc_db(con, query);
	row = mysql_fetch_row(res);
	pinnr = atol(row[3]);

    /* pull pin down for 18 milliseconds */
    pinMode( pinnr, OUTPUT );
    digitalWrite( pinnr, LOW );
    delay( 18 );
    /* then pull it up for 40 microseconds */
    digitalWrite( pinnr, HIGH );
    delayMicroseconds( 40 );
    /* prepare to read the pin */
    pinMode( pinnr, INPUT );
 
    /* detect change and read data */
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( pinnr ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( pinnr );
 
        if ( counter == 255 )
            break;
 
        /* ignore first 3 transitions */
        if ( (i >= 4) && (i % 2 == 0) )
        {
            /* shove each bit into the storage bytes */
            dht11_dat[j / 8] <<= 1;
            if ( counter > 16 )
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
 
    /*
     * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     * print it out if data is good
     */
    if ( (j >= 40) &&
         (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
    {
		sprintf(lhum, "%d,%d", dht11_dat[0], dht11_dat[1]);
		sprintf(ltemp, "%d,%d", dht11_dat[2], dht11_dat[3]);
		data_ok = 1;
    } else  {
//        printf( "Data not good, skip\n" );
		sprintf(lhum, "-1");
		sprintf(ltemp, "-1");
		data_ok = 0;
    }
	
	if (data_ok == 1) 
	{
		sprintf(query,"UPDATE nt_dht11 SET value_temp = '%s', value_hum = '%s' WHERE id = %i", ltemp, lhum, node.nt_id);
		query_db(con, query);

		curl = curl_easy_init();
		sprintf(url, "http://api.thingspeak.com/update?key=IR9PAM4G0JD0P770&field1=%s&field2=%s", ltemp, lhum);
		curl_easy_setopt(curl,CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	fclose(devnull);
	mysql_free_result(res);
	mysql_close(con);

	return 0;
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_DS18B20																	*
*																							*
*********************************************************************************************/
int process_nt_ds18b20(struct node_struct node) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];

	FILE *fp;
	char file[250];
	char line[256];
	char *token;
	char temp_value[6];
	struct nt_ds18b20_struct nt_ds18b20;
	
	int result = 1;

	CURL *curl;
	char url[250]; 
	FILE *devnull = fopen("/dev/null", "w+");

	char logtxt[250];
	
// MySQL verbinding opzetten
	MYSQL *con = connect_domotica_db();

	sprintf(query, "SELECT * FROM nt_ds18b20 WHERE id = %i", node.nt_id);
	res = query_res_db(con, query);
	row = mysql_fetch_row(res);
		
	// Struct vullen
	nt_ds18b20.id = atol(row[0]);
	strcpy(nt_ds18b20.address, row[1]);
	strcpy(nt_ds18b20.oms, row[2]);
	strcpy(nt_ds18b20.comm, row[3]);
	strcpy(nt_ds18b20.code, row[4]);
	nt_ds18b20.value = atof(row[5]);

	sprintf(file, "/sys/bus/w1/devices/%s/w1_slave", nt_ds18b20.address);
	
	fp = fopen(file, "r");
	if ((fp == NULL)) {
		sprintf(logtxt, "Sensor voor node %s niet (goed) aangesloten, node uitgeschakeld", node.id);
		logmsg(logtxt, node.id);
		// node uitschakelen
		sprintf(query, "UPDATE nodes SET actief = 0 WHERE id = '%s'", node.id );
		query_db(con, query);
		
		mysql_free_result(res);
		mysql_close(con);
		
		return 0;
	}
		
	while (fgets(line, sizeof(line), fp)) {
		// note that fgets don't strip the terminating \n, checking its
		//   presence would allow to handle lines longer that sizeof(line) 
		//printf("%s", line); 
		if (strstr(line, "YES") == 0) {
			token = strtok(line, "t=");
			token = strtok(NULL, "t=");
			sprintf(temp_value, "%c%c.%c%c%c",token[0], token[1], token[2], token[3], token[4]);

			sprintf(query, "UPDATE nt_ds18b20 SET value = %s WHERE address = '%s'", temp_value, nt_ds18b20.address );
			query_db(con, query);

			curl = curl_easy_init();
			sprintf(url, "http://api.thingspeak.com/update?key=I7RX6E53VBUWSGWG&field1=%s", temp_value);
			curl_easy_setopt(curl,CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			
			result = 0;
		}
	}
	// may check feof here to make a difference between eof and io failure -- network
	//   timeout for instance 
	
	fclose(fp);
	fclose(devnull);
	mysql_free_result(res);
	mysql_close(con);
			
	return result;
}

/********************************************************************************************
*																							*
*	INT PROCESS_NT_LIGHTDET																	*
*																							*
*********************************************************************************************/
int process_nt_lightdet(struct node_struct node) {

	//printf("Functie LIGHTDET node %s\n", node.id);
	return process_nt_default(node);
}
/********************************************************************************************
*																							*
*	INT PROCESS_NT_GENERAL																	*
*																							*
*********************************************************************************************/
int process_nt_general(struct node_struct node, int new_value) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	int value;
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(query, "proc_node_get_comm('%s')", node.id);
	res = call_proc_db(con, query);
	row = mysql_fetch_row(res);
	
	if (strcmp(row[4], "0") == 0) { // GPIO pin
		if ( node.io == 'O') {
			digitalWrite(atoi(row[3]), new_value);
		}
		value = digitalRead(atoi(row[3]));
		sprintf(query, "UPDATE nodes SET value = %i WHERE id = '%s'", value, node.id);
		query_db(con, query);
	} else {
	// i2c uitlezen
	}
	
	mysql_free_result(res);
	mysql_close(con);
	
	return 0;
}
/********************************************************************************************
*																							*
*	INT PROCESS_NT_CPUTEMP																	*
*																							*
*********************************************************************************************/
int process_nt_cputemp(struct node_struct node) {

//	int result;
	
	char command[50] = "/usr/local/bin/cpu_temp.php";
	
//	result = system(command);
	system(command);
	
	return 0;
}
/********************************************************************************************
*																							*
*	INT PROCESS_NT_WEEWX																	*
*																							*
*********************************************************************************************/
int process_nt_weewx(struct node_struct node) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(query, "SELECT field FROM nt_weewx WHERE id = %i", node.nt_id);

	res = query_res_db(con, query);
	row = mysql_fetch_row(res);

	sprintf(query, "UPDATE domotica.nt_weewx SET value = (SELECT archive.%s FROM weewx.archive ORDER BY dateTime DESC limit 1) WHERE id = %i", row[0], node.nt_id);
//	logmsg(query, "WEEWX");
	query_db(con, query);
	
	mysql_free_result(res);
	mysql_close(con);

	return 0;
}
/********************************************************************************************
*																							*
*	VOID LOG_TO_FIREBASE																	*
*																							*
*********************************************************************************************/
void log_to_firebase(char *node) {

	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[250];
	char cmdtxt[250];
	
// MySQL connectie maken
	MYSQL *con = connect_domotica_db();
	
	sprintf(query, "SELECT * FROM nodes WHERE id = '%s'", node);

	// check
	res = query_res_db(con, query);
	row = mysql_fetch_row(res);

	sprintf(cmdtxt,"curl -X PUT -d '{ \"value\": %s, \"text\": \"%s\", \"oms\": \"%s\", \"locatie\": \"%s\" }'  'https://blazing-heat-4389.firebaseio.com/nodes/%s.json'", row[5], row[6], row[1], row[7], row[0] );
	system(cmdtxt);

}