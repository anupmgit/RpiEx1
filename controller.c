#include "controller.h"
#define IGNORABLE 0

int autoshutdowninterval = -1; // seconds to auto shutdown for testing purposes
                        // specify -1 to not auto shutdown the app

int exceptiontemp = 100; // max temperature before exception (Fahrenheit)
const char *emailowner = "nuppy66@gmail.com";

void* testmode_abort_exec(void*);

pthread_t threadController, threadAbort;

// Temperature Controller.c

// returns 0 if current is not in policy, else returns unsigned short temperature value
unsigned short get_desired_temp(const TemperatureData *tempsdata)
{
    int ct = 0;
    TemperatureTimeSlice* slices = NULL;
    int isWeekday = 1, hr = 0, min = 0;
    short retval = 0;

    // convert current time to day of week
    time_t currentTime;
    struct tm* localtimeinfo = NULL;

    time(&currentTime);
    localtimeinfo = localtime(&currentTime);
    if (localtimeinfo == NULL)
    {
       // time functions return NULL if failure
       return retval;
    }

    isWeekday = (localtimeinfo->tm_wday > 0) && (localtimeinfo->tm_wday < 6); // 1..5
    hr = localtimeinfo->tm_hour;
    min = localtimeinfo->tm_min;

    if (isWeekday)
    {
      ct = tempsdata->ctweekday;
      slices = tempsdata->weekdaytemps;
    }
    else
    {
      ct = tempsdata->ctweekend;
      slices = tempsdata->weekendtemps;
    }

    if (slices == NULL)
    {
        return retval;
    }

    unsigned short starthr = 0, endhr = 0, startmin = 0, endmin = 0;
    for (int i = 0; i < ct; ++i)
    {
        starthr  = slices[i].starthr;
        endhr    = slices[i].endhr;
        startmin = slices[i].startmin;
        endmin   = slices[i].endmin;

        if (hr < starthr)
            continue;
        if (hr > endhr)
            continue;

        // now if start and end hours of the policy are equal,
        // then minutes have to be startmin <= {current} <= endmin 
        // e.g. is start time is 5:30 end time is 5:45 if start and end 
        // nominally the values could be start 5:30, end 6:00

        // e.g. start 5:30 end 7:15
        // in this case,
        // if hr == starthr,
        //    minute has to be >= startmin
        // if hr == endhr,
        //    min <= endmin
        if (starthr == hr && min < startmin)
        {
            continue;
        }
        if (endhr == hr && min > endmin)
        {
            continue;
        }
        // printf("Start%d:%d End %d:%d Current %d:%d\n", starthr, startmin, endhr, endmin, hr, min);
        retval = slices[i].temperature;
        return retval;
    }

    return 0;

}

int IsCurrentlyInTimeRange(const TemperatureData *tempsdata)
{
    unsigned short ret =  get_desired_temp(tempsdata);
    return  ret != 0;
}

void SelectTemperature (const TemperatureData *tempspolicy)
{
    // if mode is on, try to achieve desired temp
    // else it is a no-op

    ControllerMode desiredMode = OFF;
    float currentTemp = 0, desiredTemp = 0;

    // check if in time range for controlling
    if (!IsCurrentlyInTimeRange(tempspolicy))
    {
        LogStatus("Not currently in time range according to policy. Turning off");
        ControllerSwitch(OFF, desiredTemp, currentTemp);
        return;
    }

    // now we are in time range controlled by policy

    // get current temperature
    currentTemp = get_current_temp();
    desiredTemp = (float) get_desired_temp(tempspolicy);

    // display temperature data to LCD
    LCDDisplay(desiredTemp, currentTemp);
    printf("exp: %f act: %f\n", desiredTemp, currentTemp);


    // exception temperature detection
    if (currentTemp >= exceptiontemp || currentTemp < 32) // greater than too hot, or less than 0 deg C
    {
       // too hot or too cold. Log abnormal temp and shut down (either
       //  sensors are not working or temperature is too hot, so controller may be bad)
       LogErrorToStdErr("Too high temp detected");
       LogExceptionTemp(currentTemp);
       sendexceptionemail(emailowner);
       Shutdown();
    }

    float temp_delta = currentTemp - desiredTemp;


    // if temp_delta is +ve and greater than hysteresis delta
    // turn controller off
    // if temp_delta is -ve (too cold) and greater than hysteresis delta
    // turn controller on

    if (temp_delta >= 0)
    {
        // hotter than desired temp
        if (temp_delta > HYSTERESISDELTA)
        {
            ControllerSwitch(OFF, desiredTemp, currentTemp);
        }
        else
        {
            ControllerSwitch(ON, desiredTemp, currentTemp);
        }
    }
    else
    {
        // colder than desired temp, here make sure we don't go too bar below
        // desired. Hence HYSTERESISDELTA / 3.
        float abs_temp_delta = -1 * temp_delta;
        if (abs_temp_delta > HYSTERESISDELTA / 3)
        {
            ControllerSwitch(ON, desiredTemp, currentTemp);
        }
        else
        {
            ControllerSwitch(OFF, desiredTemp, currentTemp);
        }

    }

}

int InitializeController()
{
    memset(lastModifiedConfig, 0, 255);
    running = ON;
    temperature_data = NULL;

    // wiringPi init

    wiringPiDevicesSetup();
    InitializeDataLogger();

    // attach shutdown handler
    attach_shutdown_handler();

    // attach tipover handler
    attach_tipover_handler();

}

// ISR for shutdown
void ShutdownHandler()
{
    Shutdown();
}

// controller thread function
void* controller_exec(void* args)
{
   int tempmeasure_delta_ms = 15000;
   int clock_multiplier_for_policy = 20, policy_clock = 0; // 20*15 (5 min)
   double check_policy_delta_ms = 30000; // 240 seconds, 4 minutes
   clock_t lastCheckTimePolicy, lastCheckTimeTemp; // clock_t is of type long int on this platform
   lastCheckTimePolicy  = clock(); // clock for checking temperature policy over AWS REST API
   lastCheckTimeTemp  = clock(); // clock for checking temperature periodically

   // read temps from config file
   temperature_data = get_default_temps();

   // thread loop
   while (1)
   {
       if (running == OFF)
       {
           // exit this thread
           break;
       }

       // sleep(1); // watchdog_delta_ms / 1000);

       double elapsed = ((float)(clock() - lastCheckTimePolicy) / (float) CLOCKS_PER_SEC) * 1e3; // * 1e3;
       if (elapsed >= tempmeasure_delta_ms)
       {
           lastCheckTimePolicy = clock();
           if (policy_clock == 0)
           {
             // check for updates to time policy from web
             // if policy is different, then update

             printf("Update check over AWS for updated temperature policy\n");

             // make curl request for updated temperature data
             char *urldata = GetUrl(REST_REQ_URL);
             if (urldata == NULL) { continue; }
             char *urlModifiedDate = temperature_data_modified_date(urldata);
             if (urlModifiedDate == NULL)
             {
                 LogStatus("Unable to use modified date from REST API");
                 continue;
             }
             if (!strcmp(temperature_data->modified, urlModifiedDate)) // same values
             {
                 // web data was same as default values
                 continue;
             }
             else
             {
                 // uncomment to make it catch the REST API temperature policy
                 // temporarily commented for testing only to not be affected by
                 // transient policy from the web server

                 LogStatus("Updating temperature policy from REST API");
                 free(temperature_data);
                 free(temperature_data->modified);
                 free(temperature_data->weekdaytemps);
                 free(temperature_data->weekendtemps);
                 printf("URLData: %s\n", urldata);
                 temperature_data = get_temps_from_json(urldata);
             }

             free(urldata);
             free(urlModifiedDate);
           }
           policy_clock = (policy_clock + 1) % clock_multiplier_for_policy;
           SelectTemperature(temperature_data);

       }

   }

   LogStatus("Controller thread exiting.");
   return NULL;
}

void attach_shutdown_handler()
{
    shutdownbtn_attachhandler(ShutdownHandler);
}

void attach_tipover_handler()
{
    // TODO attach a tip over button that will Initiate shutdown as well
}

void PrintStatistics()
{
}

void Shutdown()
{
    running = OFF;
    ControllerSwitch(OFF, IGNORABLE, IGNORABLE);
    CloseDataLogger();
    free(temperature_data);
    LogStatus("Shutdown initiated.");
    ClearLCDDisplay();

    pthread_cancel(threadAbort);

    PrintStatistics();
}

// function to test reliability of app by initiating
// auto shutdown after specified time during testing phase
// specify -1 as autoshutdowninterval to disable auto shutdown
void* testmode_abort_exec(void* args)
{
    if (autoshutdowninterval != -1)
    {
        sleep(autoshutdowninterval); //  auto shutdown in x seconds
        Shutdown();
    }
    return NULL;
}

int main()
{
    InitializeController();

    pthread_create(&threadController, NULL, controller_exec, NULL); 
    pthread_create(&threadAbort, NULL, testmode_abort_exec, NULL); 

    pthread_join(threadController, NULL);
    pthread_join(threadAbort, NULL);
    LogStatus("App Exiting");

}

/*
in situ test code

void ControllerSwitch(ControllerMode mode)
{
    if (state == prevState)
    {
      // same as before, no change, no need to switch
      return;
    }

    if (state == ON)
    {
      digitalWrite(PT_GPIO, HIGH);
      prevState = ON;
    }
    else
    {
      digitalWrite(PT_GPIO, LOW);
      prevState = OFF;
    }
    */
    /*
    if (mode == ON)
    {
      LogStatus("Controller switch: ON");
    }
    else
    {
      LogStatus("Controller switch: OFF");
    }
}

float get_current_temp()
{
    return 56;
}

*/
