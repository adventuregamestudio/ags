
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__STRING_H
#define __AGS_EE_AC__STRING_H

#include <stdarg.h>

int String_IsNullOrEmpty(const char *thisString);
const char* String_Copy(const char *srcString);
const char* String_Append(const char *thisString, const char *extrabit);
const char* String_AppendChar(const char *thisString, char extraOne);
const char* String_ReplaceCharAt(const char *thisString, int index, char newChar);
const char* String_Truncate(const char *thisString, int length);
const char* String_Substring(const char *thisString, int index, int length);
int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive);
int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive);
int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive);
const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive);
const char* String_LowerCase(const char *thisString);
const char* String_UpperCase(const char *thisString);
const char* String_Format(const char *texx, ...);
int String_GetChars(const char *texx, int index);
int StringToInt(char*stino);
int StrContains (const char *s1, const char *s2);

//=============================================================================

const char* CreateNewScriptString(const char *fromText, bool reAllocate = true);
void removeBackslashBracket(char *lbuffer);
// Break up the text into lines, using normal Western left-right style
void split_lines_leftright(const char *todis, int wii, int fonnt);
void split_lines_rightleft (char *todis, int wii, int fonnt);
char *reverse_text(char *text);
void wouttext_reverseifnecessary(int x, int y, int font, char *text);
void break_up_text_into_lines(int wii,int fonnt,char*todis);
void check_strlen(char*ptt);
void my_strncpy(char *dest, const char *src, int len);
void my_sprintf(char *buffer, const char *fmt, va_list ap);;

#endif // __AGS_EE_AC__STRING_H
