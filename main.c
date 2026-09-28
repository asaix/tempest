#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/http.h"
#include "include/net.h"



// error code 2: bad args
// error code 3: network error
// error code 4: memory error
// error code 5: malformed data

int main(int argc, char *argv[]) {

    char* city;
    bool rawEnabled = false;

    if (argc == 1) {
        // err: no args
        fprintf(stderr, "tempest: no location provided\n");
        return 2; 
    }
    if (argc == 2) {
        // err: 1 arg && arg = --raw
        switch (strcmp("--raw", argv[1])) {
            case 0:
                fprintf(stderr, "tempest: no location provided\n");
                return 2;
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

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int sockfd = resolveSocket("wttr.is", "http", hints); 


    // == SEND HTTP REQUEST == 

    const double TIMEOUT = 10000; // 10s

    int responseSize = 1000;
    char* responseBuf = malloc(responseSize);
    if (responseBuf == NULL) {
        fprintf(stderr, "tempest:main Failed to allocate memory for response buffer (%d bytes) \n", responseSize);
        exit(4);
    }
    
    
    request(req, responseBuf, responseSize, sockfd, TIMEOUT);
    close(sockfd);


    // == PARSE RESPONSE AND PRINT == 

    char* headerBuf;
    char* bodyBuf;
    int responseCode = -1;
    // memory dynamically allocated inside parseHTTPResponse

    responseCode = parseHTTPResponse(responseBuf, &headerBuf, &bodyBuf);

    if (rawEnabled) {
        printf("%s", responseBuf);
    } else if (responseCode == 200) {
        printf("%s", bodyBuf);

    } else {
        fprintf(stderr, "tempest: invalid location\n");
        exit(5);
    } 


    return 0;   
}