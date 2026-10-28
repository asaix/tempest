#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/http.h"
#include "include/net.h"

#include <time.h>

// error code 1: generic error
// error code 2: bad args
// error code 3: network error
// error code 4: memory error
// error code 5: malformed data

int main(int argc, char *argv[]) {

    char* city;
    bool rawEnabled = false;

    if (argc == 1) {
        city = "";
    }
    if (argc == 2) {
        switch (strcmp("--raw", argv[1])) {
            case 0: // only --raw, no location
                city = "";
                rawEnabled = true;
                break;
            default: // OK, SET city
                city = argv[1];
        }
    } else if (argc == 3) {
        // err: 2 args && arg1 = --raw
        if (strcmp("--raw", argv[1]) == 0) {
            fprintf(stderr, "tempest: misaligned flag --raw\n");
            return 2;
        } 
        // SET rawEnabled and city
        // (assume argv[2] is --raw because if it's not the program dies in the next if statement anyway)
        rawEnabled = true; 
        city = argv[1];
           
    } 
    if (
        // err: 3 or more args || second arg != raw
        argc > 3 || (
            argc == 3 &&
            (strcmp("--raw", argv[2]) != 0) 
        ) 
    ) {
        fprintf(stderr, "tempest: too many arguments\n");
        return 2;
    }

    // printf("DEBUG(main): city=%s rawEnabled=%d\n", city, rawEnabled);


    // no error handling here because it's all handled within the function calls 

    // == CRAFT REQUEST == 

    int reqSize = 1000;
    char* req = malloc(reqSize); 
    if (req == NULL) {
        fprintf(stderr, "tempest:main Failed to allocate memory for request buffer (%d bytes) \n", reqSize);
        exit(4);
    }
    
    createGETRequest(city, req, reqSize);


    // == CREATE SOCKET AND CONNECT ==

    const double TIMEOUT = 10; // 10s

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    time_t startTime = time(NULL);

    int sockfd = resolveSocket("wttr.is", "http", hints, TIMEOUT - difftime(time(NULL), startTime));


    // == SEND HTTP REQUEST ==

    int responseSize = 1000;
    char* responseBuf = malloc(responseSize);
    if (responseBuf == NULL) {
        fprintf(stderr, "tempest:main Failed to allocate memory for response buffer (%d bytes) \n", responseSize);
        exit(4);
    }
    
    
    request(req, responseBuf, responseSize, sockfd, TIMEOUT - difftime(time(NULL), startTime));
    close(sockfd);

    // print raw before parsing so it's still printed if the response is malformed
    if (rawEnabled) {
        char* body = strstr(responseBuf, "\r\n\r\n");
        int headerLen = (body != NULL) ? (int)(body + 4 - responseBuf) : (int)strlen(responseBuf);

        printf("\n\x1b[90m%s", req);
        printf("%.*s\x1b[0m", headerLen, responseBuf);
        printf("\x1b[38;5;141m%s\x1b[0m", responseBuf + headerLen);
    }

    // == PARSE RESPONSE AND PRINT == 

    char* headerBuf;
    char* bodyBuf;
    int responseCode = -1;
    // memory dynamically allocated inside parseHTTPResponse

    responseCode = parseHTTPResponse(responseBuf, &headerBuf, &bodyBuf);

    // printf("%d", responseCode);


    if (responseCode == 200) {
        if (!rawEnabled) {
            printf("\n\x1b[38;5;141m%s\x1b[0m", bodyBuf); // violet
        }
    }
    else if (responseCode == -1) {
        fprintf(stderr, "tempest: malformed response - could not read status code\n");
        exit(5);
    }
    else {
        // invalid location
        fprintf(stderr, "\ntempest: invalid location\n");
        exit(5);
    } 


    return 0;   
}