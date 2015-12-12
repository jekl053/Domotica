#ifndef PROCESS_NODE_TYPE_H
#define PROCESS_NODE_TYPE_H

/* Header file bij process_node_type.c
*/
#include "mysqlfunc.h"
#include "log.h"
#include <string.h>
#include <wiringPi.h>
#include <curl/curl.h>

#define MAXTIMINGS    85

// Node structuur
struct node_struct {
	char id[20];
	char oms[50];
	char type[20];
	char io;
	int nt_id;
	int value;
	char value_oms[50];
	char locatie[10];
	int actief;
	char interval[30];
	char timestamp[30];
};

struct node_type_struct {
	char type[20];
	char table[50];
	char io;
	char oms[50];
};

// nt_rf433
struct nt_rf433_struct {
	int id;
	char type[10];
	char code[10];
};

// nt_rf433_equip
struct nt_rf433_equip_struct {
	char id[10];
	char oms[50];
	char comm[10];
	char code[10];
	char io[1];
	int actief;
};

// nt_rf433_type
struct nt_rf433_type_struct {
	char type[10];
	char oms[100];
};

struct nt_dht11_struct {
	int id;
	char oms[50];
	char comm[50];
	char code[10];
	float value_temp;
	float value_hum;
	char timestamp[20];
};

struct nt_ds18b20_struct {
	int id;
	char address[32];
	char oms[50];
	char comm[50];
	char code[10];
	float value;
	char timestamp[20];
};

struct node_type_comm_struct {
	char comm[10];
	char type[10];
	char address[10];
	int base;
	int actief;
};

int process_node_cmd(char *node, int value);
int process_scene_cmd(char *node);
int process_init_cmd(char *node);
int read_input_node(char *node);
void set_node_value(char* node, int value);
struct node_struct fill_node_struct(MYSQL_ROW row);
int process_nt_default(struct node_struct node);
int process_nt_rf433( char *node, int nt_id, int value);
int process_nt_pir(struct node_struct node);
int	process_nt_dht11(struct node_struct node);
int process_nt_ds18b20(struct node_struct node);
int process_nt_lightdet(struct node_struct node);
int process_nt_cputemp(struct node_struct node);
int process_nt_weewx();
int process_nt_general(struct node_struct node, int new_value);
void log_to_firebase(char *node);

#endif