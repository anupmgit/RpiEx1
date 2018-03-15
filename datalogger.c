#include "datalogger.h"

const char *dataLogFile = "datalog.log";
const char *exceptionLogFile = "exceptionLog.log";

void LogErrorToStdErr(const char *msg)
{
    if (msg != NULL)
    {
        time_t current;
        time(&current);
        int len = 45 + strlen(msg) + 1;

        char* timemsg = (char*) calloc (len, sizeof(char));
        sprintf(timemsg, "Error: %s    %s\n", ctime(&current), msg);

        perror(timemsg);
        free(timemsg);
    }
}

void LogStatus(const char *msg)
{
    if (msg != NULL)
    {
        time_t current;
        time(&current);
        int len = 40 + strlen(msg) + 1;

        char* timemsg = (char*) calloc (len, sizeof(char));
        sprintf(timemsg, "%s    %s\n", ctime(&current), msg);
        printf(timemsg);
        free(timemsg);
    }
}

int LogControllerChange(long time, TelemetryModeSwitch mode, float desired, float actual)
{
    if (telemetryBuffer == NULL)
    {
        InitializeDataLogger();
    }
    TelemetryData data;
    data.time = time;
    data.modeswitch = mode;
    data.desiredtemp = desired;
    data.temp = actual;
    data.exception = 0;
    data.excessiveCPUTemp = 0;
    data.abnormalTemp = 0;
    data.tooQuickSwitch = 0;

    return insert(telemetryBuffer, &data);
}

int LogExceptionTemp(float temperature)
{
    if (telemetryBuffer == NULL)
    {
        InitializeDataLogger();
    }
    time_t seconds_past_epoch = time(0);
    TelemetryData data;
    data.time = seconds_past_epoch;
    data.modeswitch = TOFF;
    data.desiredtemp = 0;
    data.temp = 0;
    data.exception = 1;
    data.excessiveCPUTemp = -1;
    data.abnormalTemp = temperature;
    data.tooQuickSwitch = -1;

    return insert(telemetryBuffer, &data);

}

void InitializeDataLogger()
{
    telemetryBuffer = (CircularBuffer*) malloc(sizeof(CircularBuffer));
    InitializeCircularBuffer(telemetryBuffer);
}

int CloseDataLogger()
{
    int retval = printdatalogtofile(telemetryBuffer, dataLogFile);
    retval = printexceptionstofile(telemetryBuffer, exceptionLogFile);
    printall(telemetryBuffer);
    free(telemetryBuffer);
    return retval;
}

int sendexceptionemail(const char *email)
{
   if (email == NULL)
   {
      LogErrorToStdErr("email address null to send email to.");
      return 0;
   }
   char cmd[255];
   memset(cmd, 0, 255);
   sprintf(cmd, "ssmtp %s < exceptionemail.txt", email);
   return system(cmd);
}
