#include "include/net.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

int resolveSocket(char* hostname, char* service, struct addrinfo hints, double timeout) { 
    // call getaddrinfo() and return sockfd = socket(...)

    struct addrinfo* res;
    double runningTimeout = timeout;
    time_t startTime = time(NULL);

    int status;
    if ((status = getaddrinfo(hostname, service, &hints, &res)) != 0) {
        fprintf(stderr, "tempest:resolveSocket:getaddrinfo failed to connect [%s]\n", gai_strerror(status));
        exit(3);
    }

    int sockfd;
    struct addrinfo* p;
    int connStatus;
    bool connected = false;
    int flags;
    int pollstat;

    for (p = res; p != NULL; p = p->ai_next) {
        runningTimeout = timeout - difftime(time(NULL), startTime);

        if (runningTimeout <= 0) {
            fprintf(stderr, "tempest:resolveSocket: timeout %fs\n", timeout);
            exit(3);
        }
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        // get current flags
        flags = fcntl(sockfd, F_GETFL);
        // make the socket non blocking
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        connStatus = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (connStatus == -1 && errno == EINPROGRESS) {
            // wait for connect to finish or timeout
            struct pollfd pfd;
            pfd.fd = sockfd;
            pfd.events = POLLOUT;

            pollstat = poll(&pfd, 1, runningTimeout*1000); // ms
            if (pollstat == 0) {
                errno = ETIMEDOUT; // so perror reports the timeout, then try next addr
            } else if (pollstat == 1) {
                // connect finished, check if it succeeded
                int err;
                socklen_t errLen = sizeof(err);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errLen) == -1) err = errno;

                if (err == 0) connStatus = 0;
                else {
                    errno = err;
                }
            }
        }
        if (connStatus == -1) {
            close(sockfd);
            continue;
        }

        fcntl(sockfd, F_SETFL, flags); // back to blocking

        connected = true;
        break; // stop on first successful connect

    }

    if (!connected) {
        perror("tempest:resolveSocket");
        exit(3);
    }

    freeaddrinfo(res);
    return sockfd;

}

int sendall(int sockfd, char *data, int len, double timeout, time_t startTime)
{
    // if startTime is -1 it is initialized in here
    // timeout in seconds

    int total = 0;        
    int bytesleft = len; 
    int n;


    if (startTime == -1) startTime = time(NULL);

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT;

    double runningTimeout = timeout;
    int pollstat;

    while(total < len) {
        runningTimeout = timeout - difftime(time(NULL), startTime);

        if (runningTimeout <= 0) {
            fprintf(stderr, "tempest:sendall: timeout %fs\n", timeout);
            exit(3);
        }

        pollstat = poll(&pfd, 1, runningTimeout*1000); //ms
        if (pollstat == -1) {
            perror("tempest:sendall:poll");
            exit(3);
        } else if (pollstat == 0 ) {
            fprintf(stderr, "tempest:sendall:poll timeout %fs\n", timeout);
            exit(3);
        }


        n = send(sockfd, data+total, bytesleft, MSG_NOSIGNAL); // EPIPE instead of SIGPIPE on closed conn


        if (n == -1) {
            // temporary errors: retry until timeout
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;

            // fatal error
            perror("tempest:sendall:send");
            exit(3);
        }
        total += n;
        bytesleft -= n;
    }

    return bytesleft;
    // always 0: only returns once everything is sent
    // exits on timeout or error
}

int recvhttp(int sockfd, char* buf, int bufSize, double timeout) {
    // this timeout is only for the recieve bit
    // pass the remaining time after request() has finished

    // timeout in seconds

    time_t respStart = time(NULL);

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

    // keep polling until conn closed
    double runningTimeout = timeout;
    int pollstat;

    int totalRead = 0;

    while (true) {

        // calc remaining timeout
        runningTimeout = timeout - difftime(time(NULL), respStart);
        if (runningTimeout <= 0) {
            fprintf(stderr, "tempest:recvhttp: timeout %fs\n", timeout);
            exit(3);
        }

        // wait for poll to recv or timeout
        pollstat = poll(&pfd, 1, runningTimeout*1000); // convert to ms
        if (pollstat == -1) {
            // -1 = non-timeout error
            perror("tempest:recvhttp:poll");
            exit(3);
        } else if (pollstat == 0) {
            fprintf(stderr, "tempest:recvhttp:poll timeout %fs\n", timeout);
            exit(3);
        }

        
        
        int nread;
        if (pfd.revents & (POLLERR | POLLNVAL)) {
    
            fprintf(stderr, "tempest:recvhttp:poll unexpected poll revent %d\n", pfd.revents);
            exit(3);
        }

        // buffer full: read 1 more byte to see if the response is done or too large
        if (totalRead == bufSize - 1) {
            char extra;
            nread = recv(sockfd, &extra, 1, 0);

            if (nread == -1) {
                perror("tempest:recvhttp:recv");
                exit(3);
            } else if (nread == 0) {
                break; // conn closed, response fit exactly
            }

            fprintf(stderr, "tempest:recvhttp: response too large for buffer (%d bytes)\n", bufSize);
            exit(3);
        }
        // if POLLHUP or POLLIN, read

        nread = recv(sockfd, buf + totalRead, bufSize - 1 - totalRead, 0);


        if (nread == -1) {
            perror("tempest:recvhttp:recv");
            exit(3);
        } else if (nread == 0) {
            break;
            // conn closed
            // proceed to check if entire response is valid
            // possible because this does not support persistent HTTP conns
        } else {
            totalRead += nread;
            continue;
        }
    }

    buf[totalRead] = '\0';

    return 0;

    // always 0: only returns once the conn is closed
    // exits on timeout or error
}

int request(char* msg, char* buf, int bufSize, int sockfd, double timeout) {
    // calls sendall() using an ALREADY CONNECTED socket
    // places  raw http response to be passed up to a parser in buf

    // timeout in seconds


    time_t reqStart = time(NULL);
    
    sendall(sockfd, msg, strlen(msg), timeout, reqStart); // exits on failure


    double timeLeftAfterSend = timeout - difftime(time(NULL), reqStart);
    
    int stat;
    stat = recvhttp(sockfd, buf, bufSize, timeLeftAfterSend);

    return stat;
    // always 0: exits on timeout or error

}


