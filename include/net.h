#ifndef NET_H
#define NET_H


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


int resolveSocket(char* hostname, char* service, struct addrinfo hints);

char* request(char* msg, char* buf, int bufSize, int sockfd, double timeout);

#endif