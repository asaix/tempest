#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// error code 2: bad args
// error code 3: network error
// error code 4: memory error
// error code 5: malformed data

int main(int argc, char *argv[]) {

    char* city;
    bool rawEnabled;

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

    printf("DEBUG(main): city=%s rawEnabled=%d\n", city, rawEnabled);
    return 0;   
}