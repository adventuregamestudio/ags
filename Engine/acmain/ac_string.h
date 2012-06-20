#ifndef __AC_STRING_H
#define __AC_STRING_H

const char* CreateNewScriptString(const char *fromText, bool reAllocate = true);
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
int StrGetCharAt (char *strin, int posn);
void StrSetCharAt (char *strin, int posn, int nchar);

#endif // __AC_STRING_H