#ifndef HTTP_H
#define HTTP_H

int createGETRequest(char* city, char* buf, int bufsize) ;
int parseHTTPResponse(char* response, char** headerBuf, char** bodyBuf);

#endif