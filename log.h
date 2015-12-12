#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <wiringPi.h>
#include "mysqlfunc.h"


void logmsg(char *msg, char *node);

#endif