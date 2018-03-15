#include "ClientWebRequest.h"

typedef struct curl_memstream
{
    char *data;
    size_t size;
} memstream;

size_t curl_callback(void* newdata, size_t size, size_t nmemb, void* userp)
{

    size_t size_adjust = size * nmemb;
    memstream* mem = (memstream*) userp;
    mem->data = realloc(mem->data, mem->size + size_adjust + 1);

    if (mem->data == NULL)
    {
        perror("Insufficient memory (curl_callback)");
        return 0;
    }

    memcpy(&(mem->data[mem->size]), newdata, size_adjust);
    mem->size += size_adjust;
    mem->data[mem->size] = 0; // null terminate it
    return size_adjust;
}

char* GetUrl(const char *http_url)
{
    assert(http_url != NULL);

    if (http_url == NULL)
    {
        return NULL;
    }

    CURL *curl_handle;
    CURLcode result;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init(); // use the easy api for non-multibind

    // allocate memory for chunked transfer http
    memstream chunk;
    chunk.data = malloc(1);
    chunk.data[0] = 0;
    chunk.size = 0;

    // set up curl options to get http data
    // curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.raspberrypi.org/forums/viewtopic.php?t=24686");
    curl_easy_setopt(curl_handle, CURLOPT_URL, http_url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*) &chunk);

    // always set a UserAgent string for http request. 
    // In this case, user libcurl, but you can change it 
    // to mimic different browsers
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // get the data
    result = curl_easy_perform(curl_handle);
    if (result != CURLE_OK)
    {
        LogErrorToStdErr("Error making REST webrequest using curl\n");
    }
    else
    {
        // debug print of data received
        // printf("Got:\n%s\n", chunk.data);
    }

    char *retval = calloc(strlen(chunk.data)+1, sizeof(char));
    assert(retval != NULL);
    if (retval != NULL)
    {
        strcpy (retval, chunk.data);
    }
Cleanup:
    free (chunk.data);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return retval;
}
