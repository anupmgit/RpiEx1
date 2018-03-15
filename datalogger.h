#ifndef _datalogger_h_
#define _datalogger_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "circularbuffer.h"

void LogErrorToStdErr(const char *msg);
void LogStatus(const char *msg);

clock_t app_start_time;
unsigned int cWebrequests;
unsigned int cStateSwitches;

CircularBuffer* telemetryBuffer;

void InitializeDataLogger();

int CloseDataLogger();

int LogControllerChange(long time, TelemetryModeSwitch mode, float desired, float actual);

int LogExceptionTemp(float temperature);

int sendexceptionemail(const char *email);
#endif
