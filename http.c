#include "include/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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
// TODO: mark functions as static