
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "wgt2allg.h"  //strupr and strlwr on Linux
#include "acmain/ac_maindefines.h"
#include "acmain/ac_string.h"
#include "ac/ac_common.h"
#include "acmain/ac_strings.h"
#include "acmain/ac_translation.h"
#include "ac/dynobj/scriptstring.h"
#include "bigend.h"    //stricmp()


// ** SCRIPT STRING

const char *CreateNewScriptString(const char *fromText, bool reAllocate) {
    ScriptString *str;
    if (reAllocate) {
        str = new ScriptString(fromText);
    }
    else {
        str = new ScriptString();
        str->text = (char*)fromText;
    }

    ccRegisterManagedObject(str->text, str);

    /*long handle = ccRegisterManagedObject(str->text, str);
    char buffer[1000];
    sprintf(buffer, "String %p (handle %d) allocated: '%s'", str->text, handle, str->text);
    write_log(buffer);*/

    return str->text;
}



int String_IsNullOrEmpty(const char *thisString) 
{
    if ((thisString == NULL) || (thisString[0] == 0))
        return 1;

    return 0;
}

const char* String_Copy(const char *srcString) {
    return CreateNewScriptString(srcString);
}

const char* String_Append(const char *thisString, const char *extrabit) {
    char *buffer = (char*)malloc(strlen(thisString) + strlen(extrabit) + 1);
    strcpy(buffer, thisString);
    strcat(buffer, extrabit);
    return CreateNewScriptString(buffer, false);
}

const char* String_AppendChar(const char *thisString, char extraOne) {
    char *buffer = (char*)malloc(strlen(thisString) + 2);
    sprintf(buffer, "%s%c", thisString, extraOne);
    return CreateNewScriptString(buffer, false);
}

const char* String_ReplaceCharAt(const char *thisString, int index, char newChar) {
    if ((index < 0) || (index >= (int)strlen(thisString)))
        quit("!String.ReplaceCharAt: index outside range of string");

    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    buffer[index] = newChar;
    return CreateNewScriptString(buffer, false);
}

const char* String_Truncate(const char *thisString, int length) {
    if (length < 0)
        quit("!String.Truncate: invalid length");

    if (length >= (int)strlen(thisString))
        return thisString;

    char *buffer = (char*)malloc(length + 1);
    strncpy(buffer, thisString, length);
    buffer[length] = 0;
    return CreateNewScriptString(buffer, false);
}

const char* String_Substring(const char *thisString, int index, int length) {
    if (length < 0)
        quit("!String.Substring: invalid length");
    if ((index < 0) || (index > (int)strlen(thisString)))
        quit("!String.Substring: invalid index");

    char *buffer = (char*)malloc(length + 1);
    strncpy(buffer, &thisString[index], length);
    buffer[length] = 0;
    return CreateNewScriptString(buffer, false);
}

int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive) {

    if (caseSensitive) {
        return strcmp(thisString, otherString);
    }
    else {
        return stricmp(thisString, otherString);
    }
}

int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    if (caseSensitive) {
        return (strncmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
    else {
        return (strnicmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
}

int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    int checkAtOffset = strlen(thisString) - strlen(checkForString);

    if (checkAtOffset < 0)
    {
        return 0;
    }

    if (caseSensitive) 
    {
        return (strcmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
    }
    else 
    {
        return (stricmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
    }
}

const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
{
    char resultBuffer[STD_BUFFER_SIZE] = "";
    int thisStringLen = (int)strlen(thisString);
    int outputSize = 0;
    for (int i = 0; i < thisStringLen; i++)
    {
        bool matchHere = false;
        if (caseSensitive)
        {
            matchHere = (strncmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
        }
        else
        {
            matchHere = (strnicmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
        }

        if (matchHere)
        {
            strcpy(&resultBuffer[outputSize], replaceWithText);
            outputSize += strlen(replaceWithText);
            i += strlen(lookForText) - 1;
        }
        else
        {
            resultBuffer[outputSize] = thisString[i];
            outputSize++;
        }
    }

    resultBuffer[outputSize] = 0;

    return CreateNewScriptString(resultBuffer, true);
}

const char* String_LowerCase(const char *thisString) {
    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    strlwr(buffer);
    return CreateNewScriptString(buffer, false);
}

const char* String_UpperCase(const char *thisString) {
    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    strupr(buffer);
    return CreateNewScriptString(buffer, false);
}

const char* String_Format(const char *texx, ...) {
    char displbuf[STD_BUFFER_SIZE];

    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    return CreateNewScriptString(displbuf);
}

int String_GetChars(const char *texx, int index) {
    if ((index < 0) || (index >= (int)strlen(texx)))
        return 0;
    return texx[index];
}


int StringToInt(char*stino) {
    return atoi(stino);
}

int StrGetCharAt (char *strin, int posn) {
    if ((posn < 0) || (posn >= (int)strlen(strin)))
        return 0;
    return strin[posn];
}

void StrSetCharAt (char *strin, int posn, int nchar) {
    if ((posn < 0) || (posn > (int)strlen(strin)) || (posn >= MAX_MAXSTRLEN))
        quit("!StrSetCharAt: tried to write past end of string");

    if (posn == (int)strlen(strin))
        strin[posn+1] = 0;
    strin[posn] = nchar;
}

