#ifndef __CS_UTILS_H
#define __CS_UTILS_H

// Note: those are used far not only in the compiler

extern void fputstring(char *sss, FILE *ddd);
extern void fgetstring_limit(char *sss, FILE *ddd, int bufsize);
extern void fgetstring(char *sss, FILE *ddd);

#endif // __CS_UTILS_H