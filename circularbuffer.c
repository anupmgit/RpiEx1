/*
 * circularbuffer.c
 *
 *  Created on: Mar 3, 2018
 *      Author: anupm
 *
 *      Circular buffer implementation with overwrite of least frequently used data
 *
 */

#include "circularbuffer.h"

int InitializeCircularBuffer(CircularBuffer *buffer)
{
    if (buffer == NULL)
    {
        return 0;
    }

    buffer->startIdx = 0;
    buffer->endIdx = 0;
    buffer->flagEmpty = 1;
    return 1;
}

int isFull(const CircularBuffer *buffer)
{
	if (buffer == NULL)
	{
		return 0;
	}
	if (buffer->endIdx == buffer->startIdx && !buffer->flagEmpty)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int isEmpty(const CircularBuffer *buffer)
{
    if (buffer == NULL) { return 0; }
    // start and end will coincide when the buffer is full or when it is empty
    return ((buffer->startIdx == buffer->endIdx) && buffer->flagEmpty);
}

unsigned int size(const CircularBuffer *buffer)
{
    if (isFull(buffer))
        return TELEMETRYSIZE;    // both start and end will point at the same index,
                                 // but it is not empty in this case
    else
    {
        if (buffer->startIdx > buffer->endIdx)
        {
            return buffer->endIdx + TELEMETRYSIZE - buffer->startIdx ; // e.g. in a 5 element array, startIdx = 2, endIdx = 0, means that index 2, 3, 4 are filled, endIdx points past element 4, which is 0
                                                                       // hence the size in this case is 0 + 5 - 2,  i.e. 3
        }
        else // startIdx is <= endIdx
        {
            return buffer->endIdx - buffer->startIdx;
                    // could be 0 for empty,
                    // Also remember, endIdx points past the top of the buffer,
                    // e.g. endIdx = 4 and startIdx = 2 means there are 2 elements at index 2 and 3
        }
    }
}

int insert(CircularBuffer *buffer, const TelemetryData *data)
{
	if (buffer == NULL)
	{
		return 0;
	}
	if (isFull(buffer))
	{
		// buffer is full, start overwriting
		buffer->startIdx = (buffer->startIdx + 1) % TELEMETRYSIZE;
	}
	buffer->circularbuffer[buffer->endIdx].desiredtemp = data->desiredtemp;
	buffer->circularbuffer[buffer->endIdx].modeswitch = data->modeswitch;
	buffer->circularbuffer[buffer->endIdx].temp = data->temp;
	buffer->circularbuffer[buffer->endIdx].time = data->time;
	buffer->circularbuffer[buffer->endIdx].exception = data->exception;
	buffer->circularbuffer[buffer->endIdx].excessiveCPUTemp = data->excessiveCPUTemp;
	buffer->circularbuffer[buffer->endIdx].abnormalTemp = data->abnormalTemp;
	buffer->circularbuffer[buffer->endIdx].tooQuickSwitch = data->tooQuickSwitch;
	buffer->endIdx = (buffer->endIdx + 1) % TELEMETRYSIZE; // increment endIdx, endIdx will point one past the filled portion of the buffer
	buffer->flagEmpty = 0;
	return 1;
}

int removeitem(CircularBuffer *buffer, TelemetryData *data)
{
	if (buffer == NULL)
	{
		return 0;
	}
	if (isEmpty(buffer))
	{
		printf("Removing from empty buffer.\n");
		return 0;
	}

	// remember, endIndex points past the top of the valid buffer, so decrement it
	buffer->endIdx = buffer->endIdx - 1;
	if (buffer->endIdx < 0)
	{
		buffer->endIdx += TELEMETRYSIZE;
	}

	if (buffer->startIdx == buffer->endIdx)
	{
		// start and end became equal when removing, buffer is now empty
		buffer->flagEmpty = 1;
	}

	if (data != NULL)
	{
		// populate the data param passed with the current top of the buffer
		data->desiredtemp = buffer->circularbuffer[buffer->endIdx].desiredtemp;
		data->modeswitch = buffer->circularbuffer[buffer->endIdx].modeswitch;
		data->temp = buffer->circularbuffer[buffer->endIdx].temp;
		data->time = buffer->circularbuffer[buffer->endIdx].time;
		data->exception = buffer->circularbuffer[buffer->endIdx].exception;
		data->excessiveCPUTemp = buffer->circularbuffer[buffer->endIdx].excessiveCPUTemp;
		data->abnormalTemp = buffer->circularbuffer[buffer->endIdx].abnormalTemp;
		data->tooQuickSwitch = buffer->circularbuffer[buffer->endIdx].tooQuickSwitch;
	}
	else
	{
		// printf("data param is null in remove. Not populating values\n");
	}

	// clear out the data.
	buffer->circularbuffer[buffer->endIdx].desiredtemp = 0;
	buffer->circularbuffer[buffer->endIdx].modeswitch = TOFF;
	buffer->circularbuffer[buffer->endIdx].temp = 0;
	buffer->circularbuffer[buffer->endIdx].time = 0;
        buffer->circularbuffer[buffer->endIdx].exception = 0;
	buffer->circularbuffer[buffer->endIdx].excessiveCPUTemp = 0;
	buffer->circularbuffer[buffer->endIdx].abnormalTemp = 0;
	buffer->circularbuffer[buffer->endIdx].tooQuickSwitch = 0;

	printf("Removal: start:%d end:%d\n", buffer->startIdx, buffer->endIdx);
	return 1;
}

int printdatalogtofile(const CircularBuffer *buffer, const char *filename)
{
    assert(filename != NULL);
    assert(buffer != NULL);

    if(filename == NULL)
    {
        printf("filename null in printdatalogtofile of circular buffer");
        return 0;
    }

    if (buffer == NULL)
    {
        printf("buffer null in printdatalogtofile of circular buffer");
        return 0;
    }

    int start = buffer->startIdx;
    FILE* fp;

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "[%s,%d]Could not open datalogger file to write: %s", __FILE__, __LINE__, filename);
        return 0;
    }
    fprintf(fp, "Time, ModeSwitch, DesiredTemp, Temperature\n");

    if (!isEmpty(buffer))
    {
        do
        {
            if (buffer->circularbuffer[start].exception == 0)
            {
              // start and end can coincide when buffer is full too, hence the do loop to force 
              fprintf(fp, "%ld, %d, %f, %f\n",
                   buffer->circularbuffer[start].time,
                   buffer->circularbuffer[start].modeswitch,
                   buffer->circularbuffer[start].desiredtemp,
                   buffer->circularbuffer[start].temp);
            }
            start = (start + 1) % TELEMETRYSIZE;
        } while (start != buffer->endIdx);
    }

    fclose(fp);
    return 1;
}

int printexceptionstofile(const CircularBuffer *buffer, const char *filename)
{
    assert(filename != NULL);
    assert(buffer != NULL);

    if(filename == NULL)
    {
        printf("filename null in printexceptiontofile of circular buffer");
        return 0;
    }

    if (buffer == NULL)
    {
        printf("buffer null in printexceptiontofile of circular buffer");
        return 0;
    }

    int start = buffer->startIdx;
    FILE* fp;

    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "[%s,%d]Could not open exception file to write: %s", __FILE__, __LINE__, filename);
        return 0;
    }
    fprintf(fp, "Time,  DesiredTemp, Temperature, ExcessiveSwitch\n");

    if (!isEmpty(buffer))
    {
        do
        {
            if (buffer->circularbuffer[start].exception == 1)
            {
              // start and end can coincide when buffer is full too, hence the do loop to force 
              fprintf(fp, "%ld, %f, %f, %d\n",
                   buffer->circularbuffer[start].time,
                   buffer->circularbuffer[start].excessiveCPUTemp,
                   buffer->circularbuffer[start].abnormalTemp,
                   buffer->circularbuffer[start].tooQuickSwitch);
            }
            start = (start + 1) % TELEMETRYSIZE;
        } while (start != buffer->endIdx);
    }

    fclose(fp);
    return 1;
}

void printall(const CircularBuffer *buffer)
{
	if (buffer == NULL)
	{
		printf("Buffer is NULL\n");
		return;
	}
	int start = buffer->startIdx;
	printf("  Time       switch desired  actual  except cputemp abnorml fswitch\n");
	if (isEmpty(buffer))
	{
		return;
	}

	do
	{
		// start and end can coincide when buffer is full too, hence the do loop to force 
		const TelemetryData* data = &(buffer->circularbuffer[start]);
		printf("  %ld  %7d %7.2f %7.2f %7d %7.2f %7.2f %7d\n",
                          buffer->circularbuffer[start].time,
                          buffer->circularbuffer[start].modeswitch,
                          buffer->circularbuffer[start].desiredtemp,
                          buffer->circularbuffer[start].temp,
                          buffer->circularbuffer[start].exception,
                          buffer->circularbuffer[start].excessiveCPUTemp,
                          buffer->circularbuffer[start].abnormalTemp,
                          buffer->circularbuffer[start].tooQuickSwitch
                          );
		start = (start + 1) % TELEMETRYSIZE;
	} while (start != buffer->endIdx);
}


/*
     ////
    ////
   ////  Test code for ciruclarbuffer.c
  ////
 /////////////////////////////////////////

CircularBuffer* telemetryBuffer;

void InitializeTelemetry()
{
    telemetryBuffer = (CircularBuffer*) malloc(sizeof(CircularBuffer));
    telemetryBuffer->startIdx = 0;
    telemetryBuffer->endIdx = 0;
    telemetryBuffer->flagEmpty = 1;
}

int main()
{
	InitializeTelemetry();
	printall(telemetryBuffer);
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	time_t seconds_past_epoch = time(0);
	tm* locTime = localtime(&seconds_past_epoch);
	printf("Running this test on: %s", asctime(locTime));

	TelemetryData data  = { seconds_past_epoch, TON, 3, 3};
	TelemetryData data2 = { seconds_past_epoch+5, TOFF, 12, 16 };
	TelemetryData data3 = { seconds_past_epoch + 10, TOFF, 22, 25};
	insert(telemetryBuffer, &data);
	insert(telemetryBuffer, &data);
	insert(telemetryBuffer, &data);
	insert(telemetryBuffer, &data);
	insert(telemetryBuffer, &data2);
	insert(telemetryBuffer, &data3);
	insert(telemetryBuffer, &data2);
	insert(telemetryBuffer, &data3);
	printall(telemetryBuffer);
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	TelemetryData data4;
	remove(telemetryBuffer, &data4);
	printf("Got back: %f %d %f %ld\n", data4.desiredtemp, data4.modeswitch, data4.temp, data4.time);
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	remove(telemetryBuffer, &data4);
	remove(telemetryBuffer, &data4);
	remove(telemetryBuffer, NULL);
	printf("IsEmpty: %d, IsFull: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer));
	remove(telemetryBuffer, NULL);
	remove(telemetryBuffer, NULL);
	printf("IsEmpty: %d, IsFull: %d FirstIdx: %d LastIdx: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer), telemetryBuffer->startIdx, telemetryBuffer->endIdx);
	remove(telemetryBuffer, NULL);
	remove(telemetryBuffer, NULL);
	remove(telemetryBuffer, NULL);
	printf("IsEmpty: %d, IsFull: %d FirstIdx: %d LastIdx: %d\n", isEmpty(telemetryBuffer), isFull(telemetryBuffer), telemetryBuffer->startIdx, telemetryBuffer->endIdx);

	return 0;
}

////////// end test code

*/

