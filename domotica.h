#ifndef DOMOTICA_H
#define DOMOTICA_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <mcp23017.h>
#include "mysqlfunc.h"
#include "process_node_type.h"
#include "process_status.h"
#include "log.h"

#define INT_MAIN_LOOP 250
#define INT_NODE_LOOP 500

void initialize_domotica(MYSQL * con);
void initialize_rf433(MYSQL * con);
PI_THREAD(processInputNodes);
PI_THREAD(activityLed);
//PI_THREAD(read_db18s20);
void end_process_cmd(int id, int result);

struct commando_struct {
	int id;
	char time[20];
	char cmd_type[20];
	char cmd[20];
	int value;
};


#endif