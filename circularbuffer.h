/*
 * circularbuffer.h
 *
 *  Created on: Mar 3, 2018
 *      Author: anupm
 *
 *      Circular buffer implementation with overwrite of least frequently used data
 *
 */

//  circularbuffer.h


#ifndef _circularbuffer_h_
#define _circularbuffer_h_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define TELEMETRYSIZE 35

typedef enum {TOFF, TON} TelemetryModeSwitch;

typedef struct _TelemetryData
{
    long time;
    TelemetryModeSwitch modeswitch; // 1 = on, 2 = off
    float temp;
    float desiredtemp;

    // sections related to exceptions
    int exception;
    float excessiveCPUTemp;
    float abnormalTemp;
    int tooQuickSwitch;
} TelemetryData;

typedef struct _circularBuffer
{
  // endIndex point 1 entry past the top of the buffer
  // in cases where start and end are equal, the buffer is either full, or empty,
  // hence the need for the empty flag flagEmpty

  int startIdx;
  int endIdx;
  int flagEmpty;
  TelemetryData circularbuffer[TELEMETRYSIZE];
} CircularBuffer;

int InitializeCircularBuffer(CircularBuffer *buffer);

int isFull(const CircularBuffer *buffer);

int isEmpty(const CircularBuffer *buffer);

unsigned int size(const CircularBuffer *buffer);

int insert(CircularBuffer *buffer, const TelemetryData *data);

int removeitem(CircularBuffer *buffer, TelemetryData *data);

void printall(const CircularBuffer *buffer);

int printdatalogtofile(const CircularBuffer *buffer, const char *filename);

int printexceptionstofile(const CircularBuffer *buffer, const char *filename);


#endif
