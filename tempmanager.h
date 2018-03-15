#ifndef _tempmanager_H_
#define _tempmanager_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <jansson.h>
#include <time.h>
#include "ClientWebRequest.h"
#include "datalogger.h"

#define DEFAULT_TEMPS "temps_default.json"

typedef struct timeslices
{
    unsigned short starthr;
    unsigned short endhr;
    unsigned short startmin;
    unsigned short endmin;
    unsigned short temperature;
} TemperatureTimeSlice;

typedef struct _temperaturedata
{
    int isOverride;
    int overrideTemp;
    char *modified;
    unsigned int ctweekday;
    unsigned int ctweekend;
    TemperatureTimeSlice *weekdaytemps;
    TemperatureTimeSlice *weekendtemps;
} TemperatureData;

int get_timeslices(const json_t *day, TemperatureTimeSlice **timeslices, unsigned int* ctTimeSlices);

char* ReadTextFileContents(const char* filepath);

TemperatureData* get_temps_from_json(const char* json_formatted_data);

char* temperature_data_modified_date(const char* json_formatted_data);

TemperatureData* get_default_temps();

#endif
