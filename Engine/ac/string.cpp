//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/string.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptstring.h"
#include "debug/debug_log.h"
#include "util/string_utils.h"
#include "script/runtimescriptvalue.h"

extern char lines[MAXLINE][200];
extern int  numlines;
extern GameSetupStruct game;
extern GameState play;
extern int longestline;
extern ScriptString myScriptStringImpl;

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
    {
        return thisString;
    }

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
    vsprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    return CreateNewScriptString(displbuf);
}

int String_GetChars(const char *texx, int index) {
    if ((index < 0) || (index >= (int)strlen(texx)))
        return 0;
    return texx[index];
}

int StringToInt(const char*stino) {
    return atoi(stino);
}

int StrContains (const char *s1, const char *s2) {
    VALIDATE_STRING (s1);
    VALIDATE_STRING (s2);
    char *tempbuf1 = (char*)malloc(strlen(s1) + 1);
    char *tempbuf2 = (char*)malloc(strlen(s2) + 1);
    strcpy(tempbuf1, s1);
    strcpy(tempbuf2, s2);
    strlwr(tempbuf1);
    strlwr(tempbuf2);

    char *offs = strstr (tempbuf1, tempbuf2);
    free(tempbuf1);
    free(tempbuf2);

    if (offs == NULL)
        return -1;

    return (offs - tempbuf1);
}

//=============================================================================

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

void split_lines_rightleft (char *todis, int wii, int fonnt) {
    // start on the last character
    char *thisline = todis + strlen(todis) - 1;
    char prevlwas, *prevline = NULL;
    // work backwards
    while (thisline >= todis) {

        int needBreak = 0;
        if (thisline <= todis) 
            needBreak = 1;
        // ignore \[ sequence
        else if ((thisline > todis) && (thisline[-1] == '\\')) { }
        else if (thisline[0] == '[') {
            needBreak = 1;
            thisline++;
        }
        else if (wgettextwidth_compensate(thisline, fonnt) >= wii) {
            // go 'back' to the nearest word
            while ((thisline[0] != ' ') && (thisline[0] != 0))
                thisline++;

            if (thisline[0] == 0)
                quit("!Single word too wide for window");

            thisline++;
            needBreak = 1;
        }

        if (needBreak) {
            strcpy(lines[numlines], thisline);
            removeBackslashBracket(lines[numlines]);
            numlines++;
            if (prevline) {
                prevline[0] = prevlwas;
            }
            thisline--;
            prevline = thisline;
            prevlwas = prevline[0];
            prevline[0] = 0;
        }

        thisline--;
    }
    if (prevline)
        prevline[0] = prevlwas;
}

char *reverse_text(const char *text) {
    int stlen = strlen(text), rr;
    char *backwards = (char*)malloc(stlen + 1);
    for (rr = 0; rr < stlen; rr++)
        backwards[rr] = text[(stlen - rr) - 1];
    backwards[stlen] = 0;
    return backwards;
}

void wouttext_reverseifnecessary(Common::Graphics *g, int x, int y, int font, char *text) {
    char *backwards = NULL;
    char *otext = text;
    if (game.options[OPT_RIGHTLEFTWRITE]) {
        backwards = reverse_text(text);
        otext = backwards;
    }

    wouttext_outline(g, x, y, font, otext);

    if (backwards)
        free(backwards);
}

void break_up_text_into_lines(int wii,int fonnt, const char*todis) {
    if (fonnt == -1)
        fonnt = play.normal_font;

    //  char sofar[100];
    if (todis[0]=='&') {
        while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
        if (todis[0]==' ') todis++;
    }
    numlines=0;
    longestline=0;

    // Don't attempt to display anything if the width is tiny
    if (wii < 3)
        return;

    int rr;

    if (game.options[OPT_RIGHTLEFTWRITE] == 0)
    {
        split_lines_leftright(todis, wii, fonnt);
    }
    else {
        // Right-to-left just means reverse the text then
        // write it as normal
        char *backwards = reverse_text(todis);
        split_lines_rightleft (backwards, wii, fonnt);
        free(backwards);
    }

    for (rr=0;rr<numlines;rr++) {
        if (wgettextwidth_compensate(lines[rr],fonnt) > longestline)
            longestline = wgettextwidth_compensate(lines[rr],fonnt);
    }
}

int MAXSTRLEN = MAX_MAXSTRLEN;
void check_strlen(char*ptt) {
    MAXSTRLEN = MAX_MAXSTRLEN;
    long charstart = (long)&game.chars[0];
    long charend = charstart + sizeof(CharacterInfo)*game.numcharacters;
    if (((long)&ptt[0] >= charstart) && ((long)&ptt[0] <= charend))
        MAXSTRLEN=30;
}

/*void GetLanguageString(int indxx,char*buffr) {
VALIDATE_STRING(buffr);
char*bptr=get_language_text(indxx);
if (bptr==NULL) strcpy(buffr,"[language string error]");
else strncpy(buffr,bptr,199);
buffr[199]=0;
}*/

void my_strncpy(char *dest, const char *src, int len) {
    // the normal strncpy pads out the string with zeros up to the
    // max length -- we don't want that
    if (strlen(src) >= (unsigned)len) {
        strncpy(dest, src, len);
        dest[len] = 0;
    }
    else
        strcpy(dest, src);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/math.h"

// int (const char *thisString)
RuntimeScriptValue Sc_String_IsNullOrEmpty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(String_IsNullOrEmpty, const char);
}

// const char* (const char *thisString, const char *extrabit)
RuntimeScriptValue Sc_String_Append(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(const char, const char, myScriptStringImpl, String_Append, const char);
}

// const char* (const char *thisString, char extraOne)
RuntimeScriptValue Sc_String_AppendChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_AppendChar);
}

// int (const char *thisString, const char *otherString, bool caseSensitive)
RuntimeScriptValue Sc_String_CompareTo(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_CompareTo, const char);
}

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_StrContains(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(const char, StrContains, const char);
}

// const char* (const char *srcString)
RuntimeScriptValue Sc_String_Copy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Copy);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_EndsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_EndsWith, const char);
}

// const char* (const char *texx, ...)
RuntimeScriptValue Sc_String_Format(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(String_Format, 1);
    return RuntimeScriptValue().SetDynamicObject((void*)String_Format("%s", scsf_buffer), &myScriptStringImpl);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_LowerCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_LowerCase);
}

// const char* (const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
RuntimeScriptValue Sc_String_Replace(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ2_PBOOL(const char, const char, myScriptStringImpl, String_Replace, const char, const char);
}

// const char* (const char *thisString, int index, char newChar)
RuntimeScriptValue Sc_String_ReplaceCharAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_ReplaceCharAt);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_StartsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_StartsWith, const char);
}

// const char* (const char *thisString, int index, int length)
RuntimeScriptValue Sc_String_Substring(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_Substring);
}

// const char* (const char *thisString, int length)
RuntimeScriptValue Sc_String_Truncate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_Truncate);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_UpperCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_UpperCase);
}

// FLOAT_RETURN_TYPE (const char *theString);
RuntimeScriptValue Sc_StringToFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(const char, StringToFloat);
}

// int (char*stino)
RuntimeScriptValue Sc_StringToInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(const char, StringToInt);
}

// int (const char *texx, int index)
RuntimeScriptValue Sc_String_GetChars(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(const char, String_GetChars);
}

RuntimeScriptValue Sc_strlen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF(strlen)
    return RuntimeScriptValue().SetInt32(strlen((const char*)self));
}

//=============================================================================
//
// Exclusive API for Plugins
//
//=============================================================================

// const char* (const char *texx, ...)
const char *ScPl_String_Format(const char *texx, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, texx);
    const char *scsf_buffer = ScriptVSprintf(ScSfBuffer, 3000, get_translation(texx), arg_ptr);
    va_end(arg_ptr);
    return String_Format("%s", scsf_buffer);
}


void RegisterStringAPI()
{
    ccAddExternalStaticFunction("String::IsNullOrEmpty^1",  Sc_String_IsNullOrEmpty);
    ccAddExternalObjectFunction("String::Append^1",         Sc_String_Append);
    ccAddExternalObjectFunction("String::AppendChar^1",     Sc_String_AppendChar);
    ccAddExternalObjectFunction("String::CompareTo^2",      Sc_String_CompareTo);
    ccAddExternalObjectFunction("String::Contains^1",       Sc_StrContains);
    ccAddExternalObjectFunction("String::Copy^0",           Sc_String_Copy);
    ccAddExternalObjectFunction("String::EndsWith^2",       Sc_String_EndsWith);
    ccAddExternalStaticFunction("String::Format^101",       Sc_String_Format);
    ccAddExternalObjectFunction("String::IndexOf^1",        Sc_StrContains);
    ccAddExternalObjectFunction("String::LowerCase^0",      Sc_String_LowerCase);
    ccAddExternalObjectFunction("String::Replace^3",        Sc_String_Replace);
    ccAddExternalObjectFunction("String::ReplaceCharAt^2",  Sc_String_ReplaceCharAt);
    ccAddExternalObjectFunction("String::StartsWith^2",     Sc_String_StartsWith);
    ccAddExternalObjectFunction("String::Substring^2",      Sc_String_Substring);
    ccAddExternalObjectFunction("String::Truncate^1",       Sc_String_Truncate);
    ccAddExternalObjectFunction("String::UpperCase^0",      Sc_String_UpperCase);
    ccAddExternalObjectFunction("String::get_AsFloat",      Sc_StringToFloat);
    ccAddExternalObjectFunction("String::get_AsInt",        Sc_StringToInt);
    ccAddExternalObjectFunction("String::geti_Chars",       Sc_String_GetChars);
    ccAddExternalObjectFunction("String::get_Length",       Sc_strlen);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("String::IsNullOrEmpty^1",  (void*)String_IsNullOrEmpty);
    ccAddExternalFunctionForPlugin("String::Append^1",         (void*)String_Append);
    ccAddExternalFunctionForPlugin("String::AppendChar^1",     (void*)String_AppendChar);
    ccAddExternalFunctionForPlugin("String::CompareTo^2",      (void*)String_CompareTo);
    ccAddExternalFunctionForPlugin("String::Contains^1",       (void*)StrContains);
    ccAddExternalFunctionForPlugin("String::Copy^0",           (void*)String_Copy);
    ccAddExternalFunctionForPlugin("String::EndsWith^2",       (void*)String_EndsWith);
    ccAddExternalFunctionForPlugin("String::Format^101",       (void*)ScPl_String_Format);
    ccAddExternalFunctionForPlugin("String::IndexOf^1",        (void*)StrContains);
    ccAddExternalFunctionForPlugin("String::LowerCase^0",      (void*)String_LowerCase);
    ccAddExternalFunctionForPlugin("String::Replace^3",        (void*)String_Replace);
    ccAddExternalFunctionForPlugin("String::ReplaceCharAt^2",  (void*)String_ReplaceCharAt);
    ccAddExternalFunctionForPlugin("String::StartsWith^2",     (void*)String_StartsWith);
    ccAddExternalFunctionForPlugin("String::Substring^2",      (void*)String_Substring);
    ccAddExternalFunctionForPlugin("String::Truncate^1",       (void*)String_Truncate);
    ccAddExternalFunctionForPlugin("String::UpperCase^0",      (void*)String_UpperCase);
    ccAddExternalFunctionForPlugin("String::get_AsFloat",      (void*)StringToFloat);
    ccAddExternalFunctionForPlugin("String::get_AsInt",        (void*)StringToInt);
    ccAddExternalFunctionForPlugin("String::geti_Chars",       (void*)String_GetChars);
    ccAddExternalFunctionForPlugin("String::get_Length",       (void*)strlen);
}
