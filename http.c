#include "include/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

char* urlEncode(char* original) {
    int origLen = strlen(original);

    // worst case replace all with %xy
    char* output = malloc(3*origLen + 1);

    if (output == NULL) return NULL;

    // hex[i] = hexadecimal for i
    const char* hex = "0123456789ABCDEF";


    int outpos = 0;
    for (int i = 0; i < origLen; i++) {
        unsigned char letter = (unsigned char) original[i];
        
        if (isalnum(letter)) {
            output[outpos++] = letter;
        } else {
            output[outpos++] = '%';
            output[outpos++] = hex[letter >> 4]; // top nibble
            output[outpos++] = hex[letter & 0b00001111]; // bottom nibble
        }
        
    }
    output[outpos] = '\0';

    return output;
}


int createGETRequest(char* city, char* buf) {
    // places request in buf
    int totalsize = snprintf(
        buf, 
        sizeof(buf),
        "GET /%s?0T HTTP/1.1\r\nHost: wttr.is\r\nConnection: close\r\n\r\n",
        urlEncode(city)
    );

    if (totalsize > buf) {
        fprintf(stderr, "tempest:createGETRequest allocated buffer size insufficient");
        exit(4);
        // TODO: move this block outside and return a status code instead
    } 

    return 0;
}


int parseHTTPResponse(char* response, char** headerBuf, char** bodyBuf) {
    // place headers in headerBuf
    // place body in bodyBuf

    char* sep = strstr(response, "\r\n\r\n");
    if (sep == NULL) {
        fprintf(stderr, "tempest:parseHTTPResponse malformed response");
        exit(5);
        // TODO: move this block outside and return a status code instead
    }

    int headerLen = sep - response;
    *headerBuf = malloc(headerLen + 1); // TODO: handle fail

    memcpy(*headerBuf, response, headerLen);
    (*headerBuf)[headerLen] = '\0';

    *bodyBuf = strdup(sep + 4); // TODO: handle fail 

    return 0; 


}

// TODO: mark functions as static