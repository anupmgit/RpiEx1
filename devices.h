#ifndef _devices_h_
#define _devices_h_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <string.h>
#include <lcd.h>
#include <assert.h>
#include "datalogger.h"
// #include "controller.h"

#define usingDHT11 true
#define GPIO_DHT 22

typedef enum _ControllerMode { OFF, ON } ControllerMode;
typedef void (*shutdownhandler) ();

void wiringPiDevicesSetup();

void ControllerSwitch(ControllerMode mode, float desired, float actual);

void LCDDisplay(unsigned int cutoffTemp, unsigned int currentTemp);

void ClearLCDDisplay();

float get_current_temp();

void shutdownbtn_attachhandler(shutdownhandler handler);

#endif
