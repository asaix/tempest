#include "include/net.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <poll.h>

int resolveSocket(char* hostname, char* service, struct addrinfo hints) { 
    // call getaddrinfo() and return sockfd = socket(...)

    struct addrinfo* res;

    int status;
    if ((status = getaddrinfo(hostname, service, &hints, &res)) != 0) {
        fprintf(stderr, "tempest:resolveSocket:getaddrinfo failed to connect [%s]", gai_strerror(status));
        exit(3);
    }

    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("tempest:resolveSocket:socket");
        exit(3);
    } 

    return sockfd;

}

int sendall(int sockfd, char *data, int len, double timeout, time_t startTime)
{
    // if startTime is NULL it is initialized in here

    int total = 0;        
    int bytesleft = len; 
    int n;

    time_t now;

    if (startTime == NULL) startTime = time(NULL);

    while(total < len) {
        n = send(sockfd, data+total, bytesleft, 0);
        
        now = time(NULL);
        if (difftime(now, startTime) > timeout) {
            fprintf(stderr, "tempest:sendall timeout %ds", timeout);
            return -1;
        }

        if (n == -1) {
            continue; // keep trying on failure until timeout 
        }
        total += n;
        bytesleft -= n;
    }

    return bytesleft; 
    // > 0: num bytes left
    // = 0: success
    // < 0: timeout
} 

char* request(char* msg, int sockfd, struct sockaddr* ai_addr, size_t addrlen, double timeout) {
    // calls connect(), sendall()
    // returns raw http response to be passed up to a parser

    // WARN: allocates char* buf to 250 bytes and returns it
    //       ideally we'd want to have it allocated outside the fn and passed down
    // TODO?

    time_t reqStart = time(NULL);

    int status;
    status = connect(sockfd, ai_addr, addrlen);

    if (status == -1) {
        perror("tempest:request:connect");
        exit(3);
    }

    int unsent;
    unsent = sendall(sockfd, msg, strlen(msg), timeout, reqStart);
    if (unsent != 0) {
        perror("tempest:request:sendall");
        exit(3);
    }

    //TODO: call recvhttp with remaining time and ret

    double timeLeftAfterSend = timeout - difftime(time(NULL), reqStart);
    
    char* buf = malloc(250);
    recvhttp(sockfd, buf, 250, timeLeftAfterSend);

    return buf;

}

int recvhttp(int sockfd, char* buf, int len, double timeout) {
    // this timeout is only for the recieve bit
    // pass the remaining time after request() has finished

    // TODO: where exactly do i place the timer start (in request too?
    time_t respStart = time(NULL);

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

    // keep polling until conn closed
    double runningTimeout = timeout;
    int pollstat;
    while (true) {

     

        // wait for poll to recv or timeout
        pollstat = poll(&pfd, 1, runningTimeout);
        if (pollstat == -1) {
            // case pollstat auto handled below
            // -1 = err
            perror("tempest:recvhttp:poll");
            exit(3);
        }

        // check for timeout
        runningTimeout = timeout - difftime(time(NULL), respStart);
        if (runningTimeout <= 0) {
            fprintf(stderr, "tempest:recvhttp: timeout %ds", timeout);
            return -1;
        }
        
        int nread;
        if (pfd.revents & POLLIN) {
            nread = recv(sockfd, buf, len, 0);
        } else {
            fprintf(stderr, "tempest:recvhttp:poll unexpected poll revent %d", pfd.revents);
            exit(3);
        }

        if (nread == -1) {
            perror("tempest:recvhttp:recv");
            exit(3);
        } else if (nread == 0) {
            break;
            // conn closed
            // proceed to check if entire response is valid
            // possible because this does not support persistent HTTP conns
        } else {
            continue;
        }
    }

    // TODO: close sockfd outside

    return 0;

    // -1: timeout
    //  0: OK
}

// TODO: mark priv as static?