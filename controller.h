#ifndef _controller_h_
#define _controller_h_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "datalogger.h"
#include "ClientWebRequest.h"
#include "tempmanager.h"
#include "devices.h"


// keep hysteresis
typedef enum _HysteresisState { RISING, FALLING } HysteresisState;

#define HYSTERESISDELTA 3

#define REST_REQ_URL "https://l2brdmt4l3.execute-api.us-west-2.amazonaws.com/temperatureTest/autoconfig"
#define REST_REQ_URL_OLD "https://ozk26y77u2.execute-api.us-east-2.amazonaws.com/Test/gettimesimpl"

char lastModifiedConfig[255];
ControllerMode running;
TemperatureData *temperature_data;

int InitializeController();

void Shutdown();

void PrintStatistics();

void attach_shutdown_handler();

void attach_tipover_handler();

#endif

