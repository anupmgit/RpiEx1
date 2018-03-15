/*
 * devices.c
 *
 *  Created on: Mar 2, 2018
 *      Author: anupm
 *
 *      Program to g
 *
 */

#include "devices.h"


int PT_GPIO=4;
int BTN_GPIO = 25;

int debounceTime = 200;
unsigned long lastButtonISR = 0;

int initFd = 0;
ControllerMode prevMode = OFF;
int lcdFd = 0, lcdinit = 0;
shutdownhandler shuthandler;

void initiateshutdown();

void wiringPiDevicesSetup()
{
    if (wiringPiSetup() == -1)
    {
        exit(1);
    }
    pinMode(PT_GPIO, OUTPUT);
    pinMode(BTN_GPIO, INPUT);

    // TODO: attach the shutdown button ISR

    initFd = wiringPiI2CSetup(0x40);
    assert(initFd != -1);

    wiringPiI2CWrite(initFd, 0xE3); // no hold master

    shuthandler = NULL;
    wiringPiISR(BTN_GPIO, INT_EDGE_RISING, &initiateshutdown);

}

void initiateshutdown()
{
    unsigned long currentTime = millis();
    if (currentTime - lastButtonISR > debounceTime)
    {
        lastButtonISR = currentTime;

        if (shuthandler != NULL)
        {
            shuthandler();
        }
    }

}

void shutdownbtn_attachhandler(shutdownhandler handler)
{
    shuthandler = handler;
}

void LCDDisplay(unsigned int cutoffTemp, unsigned int currentTemp)
{
  char msg[60];
  if (lcdinit == 0)
  {
    lcdFd = lcdInit(2,16,4,  2,3,  26,27,28,29,0,0,0,0);
    // lcdFd = lcdInit(2,16,4,  2,3,  12,16,20,21,0,0,0,0);
    if (lcdFd == -1)
    {
      LogErrorToStdErr("lcd could not be initialized\n");
    }
    else
    {
      LogStatus("Initialized LCD");
      lcdinit = 1;
    }
    // return;
  }

  lcdClear(lcdFd);
  memset(msg, 0, sizeof(msg));
  sprintf(msg, "Current: %d", currentTemp);
  lcdPosition(lcdFd, 0, 0);
  lcdPuts(lcdFd, msg);
  memset(msg, 0, sizeof(msg));
  sprintf(msg, "Cutoff: %d", cutoffTemp);
  lcdPosition(lcdFd, 0, 1);
  lcdPuts(lcdFd, msg);
}

void ClearLCDDisplay()
{
  if (lcdinit == 0) { return; }
  lcdClear(lcdFd);
}

void ControllerSwitch(ControllerMode mode, float desired, float actual)
{
    if (mode == prevMode)
    {
      // same as before, no change, no need to switch
      return;
    }

    if (mode == ON)
    {
      digitalWrite(PT_GPIO, HIGH);
      prevMode = ON;
      // DataLogger
      time_t seconds_past_epoch = time(0);
      LogControllerChange(seconds_past_epoch, TON, desired, actual);
    }
    else
    {
      digitalWrite(PT_GPIO, LOW);
      prevMode = OFF;
      // DataLogger
      time_t seconds_past_epoch = time(0);
      LogControllerChange(seconds_past_epoch, TOFF, desired, actual);
    }
}

float get_current_temp()
{
    // LogStatus("Reading current temperature.");
    wiringPiI2CWrite(initFd, 0xE3); // no hold master
    delay(500);
    int read = -1, temp1 = 0, temp2 = 0;
    int ctTry = 0, maxTries = 15;

    while (read < 0)
    {
      ctTry++;
      if (ctTry >= maxTries)
      {
         // too many tries, return sentinel value
         return -1;
      }
      temp1 = wiringPiI2CRead(initFd);
      temp2 = wiringPiI2CRead(initFd);
      int chksum = wiringPiI2CRead(initFd);
      // this sensor returns the MSB and LSB equal when the data is correct
      if (temp1 <= 0 || temp2 <= 0 || temp1 != temp2)
      {
        // couldn't read. retry
        usleep(1000000); // 1 sec
        continue;
      }
      read = 1;
    }
    // printf("Chk: %x %x %d %d\n", temp1, temp2, temp1, temp2);
    temp1 = (temp1 << 8) | temp2;
    float resultCelsius = (175.72*temp1/(float)65536) - 46.85;
    // printf("Read %6.2f celsius %6.2f fahrenheit\n", resultCelsius, resultCelsius*1.8 + 32);
    return resultCelsius *1.8 + 32;
}

/*

//  in situ test code

int main()
{
    wiringPiSetup();
    float currentTemp = 0;

    int i = 0;
    while (i < 12)
    {
      currentTemp = GetTemp();
      ++i;
      usleep(1000000); // 1 second
      if (currentTemp < cutoff)
      {
        led(ON);
      }
      else
      {
        led(OFF);
      }
      if ( i >= 2)
      {
        Display(cutoff, currentTemp);
      }
    }
    led(OFF);
    if (lcdinit)
    {
      lcdClear(lcdFd);
    }
    return 0;
}
*/
