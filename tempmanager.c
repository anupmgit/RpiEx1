#include "tempmanager.h"

int get_timeslices(const json_t *day, TemperatureTimeSlice **timeslices, unsigned int* ctTimeSlices)
{
    if (!json_is_array(day))
    {
        fprintf(stderr, "error: day node is not an array\n");
        *ctTimeSlices = 0;
        *timeslices = NULL;
        return 0;
    }
    *ctTimeSlices = json_array_size(day);
    TemperatureTimeSlice *slices = (TemperatureTimeSlice*) calloc(*ctTimeSlices, sizeof(TemperatureTimeSlice));
    int ctParsed = 0;
    for (int i = 0; i < json_array_size(day); ++i)
    {
        json_t *timedata, *start, *end, *temperature;
        int starthr = 0, startmin = 0, endhr = 0, endmin = 0, tempval = 0;
        const char *timeval;
        char *timeval2 = NULL, *timeval3 = NULL;
        char *tok;
        timedata = json_array_get(day, i);
        if (!json_is_object(timedata))
        {
            fprintf(stderr, "error: could not get time data slice at index %d\n", i);
            continue;
        }

        // start
        start = json_object_get(timedata, "start");
        if (!json_is_string(start))
        {
            fprintf(stderr, "error: start is not a string at index %d\n", i);
            continue;
        }
        timeval = json_string_value(start);
        timeval2 = (char*) calloc(strlen(timeval) + 1, sizeof(char));
        strcpy(timeval2, timeval);
        // tokenize out timeval
        tok = strtok(timeval2, ":");

        if (tok != NULL)
        {
           starthr = atoi(tok);
        }
        tok = strtok(NULL, ":");
        if (tok != NULL)
        {
           startmin = atoi(tok);
        }

        // end
        end = json_object_get(timedata, "end");
        if (!json_is_string(end))
        {
            fprintf(stderr, "error:  end is not a string at index %d\n", i);
            continue;
        }
        timeval = json_string_value(end);
        timeval3 = (char*) calloc(strlen(timeval) + 1, sizeof(char));
        strcpy(timeval3, timeval);

        // tokenize out timeval
        unsigned int hr = 0, min = 0;
        tok = strtok(timeval3, ":");

        if (tok != NULL)
        {
           endhr = atoi(tok);
        }
        tok = strtok(NULL, ":");
        if (tok != NULL)
        {
           endmin = atoi(tok);
        }

        temperature = json_object_get(timedata, "temperature");
        if (!json_is_string(temperature))
        {
            fprintf(stderr, "error: temperature is not a string at index %d\n", i);
            continue;
        }
        timeval = json_string_value(temperature);
        tempval = atoi(timeval);

        slices[ctParsed].starthr = (unsigned short) starthr;
        slices[ctParsed].startmin = (unsigned short) startmin;
        slices[ctParsed].endhr = (unsigned short) endhr;
        slices[ctParsed].endmin = (unsigned short) endmin;
        slices[ctParsed].temperature = (unsigned short) tempval;

        assert(slices[ctParsed].starthr <= slices[ctParsed].endhr);
        if (slices[ctParsed].starthr == slices[ctParsed].endhr)
        {
            // start and end hr are same, the minute of endmin must be >= startmin
            assert(slices[ctParsed].startmin <= slices[ctParsed].endmin);
        }
        assert(slices[ctParsed].temperature > 0 && slices[ctParsed].temperature <= 100);

        ctParsed++;
        // debug statement
        // printf("Parsed %d records.\n", ctParsed);
        free (timeval2);
        free (timeval3);
    }
    // update returned number of timeslices to the ones that could be successfully parsed
    *ctTimeSlices = ctParsed;
    *timeslices = slices;
    return 1;
}

char* ReadTextFileContents(const char* filepath)
{
    FILE* fp = NULL;
    int bufferSize = 500, filepos = 0, BUFFER_MAX=32000;

    assert(filepath != NULL);

    if (filepath == NULL)
    {
        fprintf(stderr, "Null filename to read file");
    }

    fp = fopen(filepath, "r");
    char *filedata = (char*)calloc(bufferSize,sizeof(char));
    assert(filedata != NULL);

    if (fp == NULL)
    {
        fprintf(stderr, "[%s,%d]Could not open test file to read: %s",
                         __FILE__, __LINE__, filepath);
        return NULL;
    }
    while (1)
    {
        if (filedata == NULL)
        {
            fprintf(stderr, "[%s,%d]Could not allocate enough memory to read file '%s'",
                         __FILE__, __LINE__, filepath);
            return NULL;
        }
       char ch = fgetc(fp);
       if (feof(fp))
       {
          break; // last character is junk
       }
       filedata[filepos] = ch;
       ++filepos;
       // guarantee there is enough memory for the null terminator
       if (filepos >= bufferSize)
       {
           bufferSize *= 2; // expand the buffer
           // clamp to a max of BUFFER_MAX
           if (bufferSize >= BUFFER_MAX)
           {
               fprintf(stderr, "File size too big: %s. Truncating characters\n");
               filepos--;
               filedata[filepos] = '\0';
               break;
           }
           else
           {
               // realloc message
               // printf("Out of buffer, reallocating: %d\n", bufferSize);
               filedata = (char*) realloc(filedata, bufferSize * sizeof(char));
               assert(filedata != NULL);
           }
       }
    }
    filedata[filepos] = '\0';
    fclose(fp);
    return filedata;
}

TemperatureData* get_temps_from_json(const char* json_formatted_data)
{
    TemperatureData *retval = NULL;
    json_t *root;
    json_error_t error;
    root = json_loads(json_formatted_data, 0, &error);

    if (!root)
    {
        fprintf(stderr, "Error on line %d: '%s' parsing JSON data\n", 
                                         error.line, error.text);
    }

    json_t *override, *overrideTemp, *modified, *weekday, *weekend, *times;
    const char *sOverride = NULL, *sModified = NULL;

    json_int_t iOverrideTemp;

    override = json_object_get(root, "override");
    if (!json_is_string(override))
    {
        fprintf(stderr, "error: override is not a string\n");
        json_decref(root);
        return NULL;
    }
    else
    {
        sOverride = json_string_value(override);
    }

    overrideTemp = json_object_get(root, "overridetemp");
    if (!json_is_number(overrideTemp))
    {
        fprintf(stderr, "error: overridetemp is not a number\n");
        json_decref(root);
        return NULL;
    }
    else
    {
        iOverrideTemp = json_integer_value(overrideTemp);
    }

    modified = json_object_get(root, "modified");
    if (!json_is_string(modified))
    {
        fprintf(stderr, "error: modified is not a string\n");
        json_decref(root);
        return NULL;
    }
    else
    {
        sModified = json_string_value(modified);
    }


    retval = (TemperatureData*) malloc(sizeof(TemperatureData));
    if (!strcmp("false", sOverride))
    {
       retval->isOverride = 0;
    }
    else
    {
       retval->isOverride = 1;
    }

    retval->overrideTemp = (int)iOverrideTemp;

    char* modifiedValue = (char*) calloc(strlen(sModified)+1, sizeof(char));
    strcpy(modifiedValue, sModified);
    retval->modified = modifiedValue;

    // weekday
    weekday = json_object_get(root, "weekday");
    times = json_object_get(weekday, "times");

    TemperatureTimeSlice *timeslices = NULL;
    unsigned int countTimeSlices = 0;
    if (!get_timeslices(times, &timeslices, &countTimeSlices))
    {
        fprintf(stderr, "error: Could not get time slices for weekday correctly\n");
    }
    retval->ctweekday = countTimeSlices;
    retval->weekdaytemps = timeslices;

    // weekend
    weekday = json_object_get(root, "weekend");
    times = json_object_get(weekday, "times");

    timeslices = NULL;
    countTimeSlices = 0;
    if (!get_timeslices(times, &timeslices, &countTimeSlices))
    {
        fprintf(stderr, "error: Could not get time slices for weekday correctly\n");
    }
    retval->ctweekend = countTimeSlices;
    retval->weekendtemps = timeslices;

    json_decref(root);
    return retval;

}

char* temperature_data_modified_date(const char* json_formatted_data)
{
    assert(json_formatted_data != NULL); // fatal
    if (json_formatted_data == NULL)
    {
        return NULL;
    }
    json_t *root, *modified;
    json_error_t error;
    const char *sModified = NULL;
    root = json_loads(json_formatted_data, 0, &error);

    if (!root)
    {
        //TODO: use the other api
        fprintf(stderr, "Error on line %d: '%s' parsing JSON data\n",
                                         error.line, error.text);
    }


    modified = json_object_get(root, "modified");
    if (!json_is_string(modified))
    {
        fprintf(stderr, "error: modified node not found or is not a string\n");
        json_decref(root);
        return NULL;
    }
    else
    {
        sModified = json_string_value(modified);
    }

    char* retval = (char*) calloc(strlen(sModified) + 1, sizeof(char));
    assert(retval != NULL);
    if (retval != NULL)
    {
       strcpy(retval, sModified);
    }
    json_decref(root);

    return retval;
}

TemperatureData* get_default_temps()
{
    char *data = NULL;
    data = ReadTextFileContents(DEFAULT_TEMPS);
    assert(data != NULL); // fatal
    if (data == NULL)
    {
        return NULL;
    }
    TemperatureData *values = get_temps_from_json(data);
    free(data);
    return values;
}

/*
TemperatureData* get_temps()
{
    TemperatureData *retval = NULL;
    // read the default data
    // make curl request for web data
    // up to bufferSize characters initially

    retval = get_default_temps();

    // make curl request for updated temperature data
    char *urldata = GetUrl(REST_REQ_URL);
    if (urldata != NULL)
    {
        printf("WebRequest: %s\n", urldata);
    }

    char *urlModifiedDate = temperature_data_modified_date(urldata);
    if (urlModifiedDate == NULL)
    {
        LogStatus("Unable to use temperatures from REST API");
        return retval;
    }
    if (!strcmp(retval->modified, urlModifiedDate)) // same values
    {
        // web data was same as default values
        return retval;
    }
    else
    {
        LogStatus("Using updated temperatures from REST API");
        free(retval);
        retval = get_temps_from_json(urldata);
    }

    free(urldata);
    free (urlModifiedData);
    return retval;
}
*/
