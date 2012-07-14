
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRINGUTILS_H
#define __AGS_CN_UTIL__STRINGUTILS_H

#include <stdio.h>
#include <stdarg.h>

void removeBackslashBracket(char *lbuffer);
// Break up the text into lines, using normal Western left-right style
void split_lines_leftright(const char *todis, int wii, int fonnt);

//=============================================================================

void fputstring(char *sss, FILE *ddd);
void fgetstring_limit(char *sss, FILE *ddd, int bufsize);
void fgetstring(char *sss, FILE *ddd);

#endif // __AGS_CN_UTIL__STRINGUTILS_H
