#ifndef NET_H
#define NET_H


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


int resolveSocket(char* hostname, char* service, struct addrinfo hints);

char* request(char* msg, int sockfd, struct sockaddr* ai_addr, size_t addrlen, double timeout);


#endif