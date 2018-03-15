#ifndef _ClientWebRequest_h_
#define _ClientWebRequest_h_
 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>
#include "datalogger.h"

// function declarations

char* GetUrl(const char *http_url);

#endif
