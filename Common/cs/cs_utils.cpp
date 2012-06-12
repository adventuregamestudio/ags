
#include <stdio.h>
#include "cs_utils.h"


void fputstring(char *sss, FILE *ddd) {
    int b = 0;
    while (sss[b] != 0) {
        fputc(sss[b], ddd);
        b++;
    }
    fputc(0,ddd);
}

void fgetstring_limit(char *sss, FILE *ddd, int bufsize) {
    int b = -1;
    do {
        if (b < bufsize - 1)
            b++;
        sss[b] = fgetc(ddd);
        if (feof(ddd))
            return;
    } while (sss[b] != 0);
}

void fgetstring(char *sss, FILE *ddd) {
    fgetstring_limit (sss, ddd, 50000000);
}
