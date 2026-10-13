#include "include/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

char* urlEncode(char* original) {
    int origLen = strlen(original);

    // worst case replace all with %xy
    char* output = malloc(3*origLen + 1);

    if (output == NULL) {
        fprintf(stderr, "tempest:http:urlEncode failed to allocate memory\n");
        exit(4);
    }

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


int createGETRequest(char* city, char* buf, int bufsize) {
    // places request in buf
    int totalsize = snprintf(
        buf, 
        bufsize,
        "GET /%s?0T HTTP/1.1\r\nHost: wttr.is\r\nConnection: close\r\n\r\n",
        urlEncode(city)
    );

    if (totalsize >= bufsize) {
        fprintf(stderr, "tempest:createGETRequest allocated buffer size insufficient\n");
        exit(4);
    } 

    return 0;
}


int parseHTTPResponse(char* response, char** headerBuf, char** bodyBuf) {
    // place headers in headerBuf
    // place body in bodyBuf
    // returns request status code

    char* sep = strstr(response, "\r\n\r\n");
    if (sep == NULL) {
        fprintf(stderr, "tempest:parseHTTPResponse malformed response (no header terminator)\n");
        exit(5);
    }

    int headerLen = sep - response;
    *headerBuf = malloc(headerLen + 1);
    if (*headerBuf == NULL) {
        fprintf(stderr, "tempest:http:parseHTTPResponse Failed to allocate memory for header buffer (%d bytes) \n", headerLen + 1);
        exit(4);
    }
    

    memcpy(*headerBuf, response, headerLen);
    (*headerBuf)[headerLen] = '\0';

    *bodyBuf = strdup(sep + 4); 

    if (*bodyBuf == NULL) {
        fprintf(stderr, "tempest:http:parseHTTPResponse Failed to allocate memory for body buffer (%ld bytes) \n", strlen(sep+4) + 1);
        exit(4);
    }

    // check against Content-Length for incomplete body
    char* line = *headerBuf;
    while (line != NULL) {
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            // found CL line
            long contentLength = strtol(line + 15, NULL, 10);
            size_t bodyLen = strlen(*bodyBuf);

            if (contentLength != (long)bodyLen) {
                fprintf(stderr, "tempest:http:parseHTTPResponse incomplete response (got %zu of %ld bytes)\n", bodyLen, contentLength);
                exit(5);
            }
            break;
        }

        line = strstr(line, "\r\n");
        if (line != NULL) line += 2;
    }


    if (strlen(response) <= 13) return -1; // status code = -1 if response is too short

    // parse status code
    char code[4];
    memcpy(code, response + 9, 3);  
    code[3] = '\0';
    return (int)atoi(code);


}
