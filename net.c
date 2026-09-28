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
        fprintf(stderr, "tempest:resolveSocket:getaddrinfo failed to connect [%s]\n", gai_strerror(status));
        exit(3);
    }

    int sockfd;
    struct addrinfo* p;
    int connStatus;
    bool connected = false;

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd == -1) continue;

        
        connStatus = connect(sockfd, res->ai_addr, res->ai_addrlen);
        if (connStatus == -1) continue;

        connected = true;
        break; // stop on first successful connect

    }

    if (!connected) {
        perror("tempest:resolveSocket");
        exit(3);
    }

 
    return sockfd;

}

int sendall(int sockfd, char *data, int len, double timeout, time_t startTime)
{
    // if startTime is -1 it is initialized in here

    int total = 0;        
    int bytesleft = len; 
    int n;

    time_t now;

    if (startTime == -1) startTime = time(NULL);

    while(total < len) {
        n = send(sockfd, data+total, bytesleft, 0);
        
        now = time(NULL);
        if (difftime(now, startTime) > timeout) {
            fprintf(stderr, "tempest:sendall timeout %fs\n", timeout);
            exit(3);
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

int recvhttp(int sockfd, char* buf, int bufSize, double timeout) {
    // this timeout is only for the recieve bit
    // pass the remaining time after request() has finished

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
            // -1 = non-timeout error
            perror("tempest:recvhttp:poll");
            exit(3);
        }

        // check for timeout
        runningTimeout = timeout - difftime(time(NULL), respStart);
        if (runningTimeout <= 0) {
            fprintf(stderr, "tempest:recvhttp: timeout %fs\n", timeout);
            exit(3);
        }
        
        int nread;
        if (pfd.revents & (POLLERR | POLLNVAL)) {
    
            fprintf(stderr, "tempest:recvhttp:poll unexpected poll revent %d\n", pfd.revents);
            exit(3);
        }
        // if POLLHUP or POLLIN, read

        nread = recv(sockfd, buf, bufSize, 0);


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

    return 0;

    // -1: timeout
    //  0: OK
}

int request(char* msg, char* buf, int bufSize, int sockfd, double timeout) {
    // calls sendall() using an ALREADY CONNECTED socket
    // places  raw http response to be passed up to a parser in buf

    time_t reqStart = time(NULL);
    
    int unsent;
    unsent = sendall(sockfd, msg, strlen(msg), timeout, reqStart);
    if (unsent != 0) {
        perror("tempest:request:sendall");
        exit(3);
    }


    double timeLeftAfterSend = timeout - difftime(time(NULL), reqStart);
    
    int stat;
    stat = recvhttp(sockfd, buf, bufSize, timeLeftAfterSend);

    return stat;
    // -1 = timeout
    //  0 = OK

}


