CC=gcc

CFLAGS=-c -Wall `mysql_config --cflags`
LFLAGS=-lwiringPi -lcurl `mysql_config --libs`

all: domotica

domotica: domotica.o mysqlfunc.o log.o process_node_type.o process_status.o
	$(CC) $(LFLAGS) domotica.o mysqlfunc.o log.o process_node_type.o process_status.o -o domotica

domotica.o: domotica.c domotica.h
	$(CC) $(CFLAGS) domotica.c

mysqlfunc.o: mysqlfunc.c mysqlfunc.h
	$(CC) $(CFLAGS) mysqlfunc.c

log.o: log.c log.h
	$(CC) $(CFLAGS) log.c
	
process_node_type.o: process_node_type.c process_node_type.h
	$(CC) $(CFLAGS) process_node_type.c

process_status.o: process_status.c process_status.h
	$(CC) $(CFLAGS) process_status.c
	
install: domotica
	service domotica stop
	cp domotica /usr/local/bin
	service domotica start
	
clean:
	rm -rf *.o domotica 

