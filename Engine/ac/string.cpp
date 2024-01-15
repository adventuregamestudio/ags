//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <algorithm>
#include <cstdio>
#include <allegro.h>
#include "ac/string.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/dynobj/dynobj_manager.h"
#include "font/fonts.h"
#include "debug/debug_log.h"
#include "script/runtimescriptvalue.h"
#include "util/string_compat.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern GameState play;
extern int longestline;

const char *CreateNewScriptString(const char *text)
{
    return static_cast<const char*>(ScriptString::Create(text).Obj);
}


int String_IsNullOrEmpty(const char *thisString) 
{
    if ((thisString == nullptr) || (thisString[0] == 0))
        return 1;

    return 0;
}

const char* String_Copy(const char *srcString) {
    return CreateNewScriptString(srcString);
}

const char* String_Append(const char *thisString, const char *extrabit) {
    const auto &header = ScriptString::GetHeader(thisString);
    int str2_len, str2_ulen;
    ustrlen2(extrabit, &str2_len, &str2_ulen);
    auto buf = ScriptString::CreateBuffer(header.Length + str2_len, header.ULength + str2_ulen);
    memcpy(buf.Get(), thisString, header.Length);
    memcpy(buf.Get() + header.Length, extrabit, str2_len + 1);
    return CreateNewScriptString(std::move(buf));
}

const char* String_AppendChar(const char *thisString, int extraOne) {
    char chr[5]{};
    const auto &header = ScriptString::GetHeader(thisString);
    size_t new_chw = usetc(chr, extraOne);
    auto buf = ScriptString::CreateBuffer(header.Length + new_chw, header.ULength + 1);
    memcpy(buf.Get(), thisString, header.Length);
    memcpy(buf.Get() + header.Length, chr, new_chw + 1);
    return CreateNewScriptString(std::move(buf));
}

const char* String_ReplaceCharAt(const char *thisString, int index, int newChar) {
    const auto &header = ScriptString::GetHeader(thisString);
    if ((index < 0) || ((size_t)index >= header.ULength))
        quit("!String.ReplaceCharAt: index outside range of string");

    size_t off = uoffset(thisString, index);
    int old_char = ugetc(thisString + off);
    size_t old_chw = ucwidth(old_char);
    char new_chr[5]{};
    size_t new_chw = usetc(new_chr, newChar);
    size_t new_len = header.Length + new_chw - old_chw;
    auto buf = ScriptString::CreateBuffer(new_len, header.ULength); // text length is the same
    memcpy(buf.Get(), thisString, off);
    memcpy(buf.Get() + off, new_chr, new_chw);
    memcpy(buf.Get() + off + new_chw, thisString + off + old_chw, header.Length - off - old_chw + 1);
    return CreateNewScriptString(std::move(buf));
}

const char* String_Truncate(const char *thisString, int length) {
    if (length < 0)
        quit("!String.Truncate: invalid length");
    const auto &header = ScriptString::GetHeader(thisString);
    if ((size_t)length >= header.ULength)
        return thisString;

    size_t new_len = uoffset(thisString, length);
    auto buf = ScriptString::CreateBuffer(new_len, length); // arg is a text length
    memcpy(buf.Get(), thisString, new_len);
    buf.Get()[new_len] = 0;
    return CreateNewScriptString(std::move(buf));
}

const char* String_Substring(const char *thisString, int index, int length) {
    if (length < 0)
        quit("!String.Substring: invalid length");
    const auto &header = ScriptString::GetHeader(thisString);
    if ((index < 0) || ((size_t)index > header.ULength))
        quit("!String.Substring: invalid index");
    size_t sublen = std::min<uint32_t>(length, header.ULength - index);
    size_t start = uoffset(thisString, index);
    size_t end = uoffset(thisString + start, sublen) + start;
    size_t copylen = end - start;

    auto buf = ScriptString::CreateBuffer(copylen, sublen);
    memcpy(buf.Get(), thisString + start, copylen);
    buf.Get()[copylen] = 0;
    return CreateNewScriptString(std::move(buf));
}

int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive) {

    if (caseSensitive) {
        return strcmp(thisString, otherString);
    }
    else {
        return ustricmp(thisString, otherString);
    }
}

int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    if (caseSensitive) {
        return (strncmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
    else {
        return (ustrnicmp(thisString, checkForString, ustrlen(checkForString)) == 0) ? 1 : 0;
    }
}

int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive) {
    // NOTE: we need size in bytes here
    const auto &header = ScriptString::GetHeader(thisString);
    size_t checklen = strlen(checkForString);
    if (checklen > header.Length)
        return 0;

    if (caseSensitive) 
    {
        return (strcmp(thisString + (header.Length - checklen), checkForString) == 0) ? 1 : 0;
    }
    else 
    {
        return (ustricmp(thisString + (header.Length - checklen), checkForString) == 0) ? 1 : 0;
    }
}

const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
{
    char resultBuffer[STD_BUFFER_SIZE] = "";
    size_t outputSize = 0; // length in bytes
    if (caseSensitive)
    {
        size_t lookForLen = strlen(lookForText);
        size_t replaceLen = strlen(replaceWithText);
        for (const char *ptr = thisString; *ptr; ++ptr)
        {
            if (strncmp(ptr, lookForText, lookForLen) == 0)
            {
                memcpy(&resultBuffer[outputSize], replaceWithText, replaceLen);
                outputSize += replaceLen;
                ptr += lookForLen - 1;
            }
            else
            {
                resultBuffer[outputSize] = *ptr;
                outputSize++;
            }
        }
    }
    else
    {
        size_t lookForLen = ustrlen(lookForText);
        size_t lookForSz = strlen(lookForText); // length in bytes
        size_t replaceSz = strlen(replaceWithText); // length in bytes
        const char *p_cur = thisString;
        for (int c = ugetxc(&thisString); *p_cur; p_cur = thisString, c = ugetxc(&thisString))
        {
            if (ustrnicmp(p_cur, lookForText, lookForLen) == 0)
            {
                memcpy(&resultBuffer[outputSize], replaceWithText, replaceSz);
                outputSize += replaceSz;
                thisString = p_cur + lookForSz;
            }
            else
            {
                usetc(&resultBuffer[outputSize], c);
                outputSize += ucwidth(c);
            }
        }
    }

    resultBuffer[outputSize] = 0; // terminate
    return CreateNewScriptString(resultBuffer);
}

const char* String_LowerCase(const char *thisString) {
    const auto &header = ScriptString::GetHeader(thisString);
    auto buf = ScriptString::CreateBuffer(header.Length, header.ULength);
    memcpy(buf.Get(), thisString, header.Length + 1);
    ustrlwr(buf.Get());
    return CreateNewScriptString(std::move(buf));
}

const char* String_UpperCase(const char *thisString) {
    const auto &header = ScriptString::GetHeader(thisString);
    auto buf = ScriptString::CreateBuffer(header.Length, header.ULength);
    memcpy(buf.Get(), thisString, header.Length + 1);
    ustrupr(buf.Get());
    return CreateNewScriptString(std::move(buf));
}

int String_GetChars(const char *thisString, int index) {
    auto &header = ScriptString::GetHeader((void*)thisString);
    if ((index < 0) || (static_cast<uint32_t>(index) >= header.ULength))
        return 0;
    int off;
    if (get_uformat() == U_ASCII)
    {
        return thisString[index];
    }
    else if (header.LastCharIdx <= index)
    {
        off = uoffset(thisString + header.LastCharOff, index - header.LastCharIdx) + header.LastCharOff;
    }
    // TODO: support faster reverse iteration too? that would require reverse-dir uoffset
    else
    {
        off = uoffset(thisString, index);
    }
    // NOTE: works up to 64k chars/bytes, then stops; this is intentional to save a bit of mem
    if (off <= UINT16_MAX)
    {
        header.LastCharIdx = static_cast<uint16_t>(index);
        header.LastCharOff = static_cast<uint16_t>(off);
    }
    return ugetc(thisString + off);
}

int StringToInt(const char*stino) {
    return atoi(stino);
}

int StrContains (const char *s1, const char *s2) {
    VALIDATE_STRING (s1);
    VALIDATE_STRING (s2);
    char *tempbuf1 = ags_strdup(s1);
    char *tempbuf2 = ags_strdup(s2);
    ustrlwr(tempbuf1);
    ustrlwr(tempbuf2);

    char *offs = ustrstr(tempbuf1, tempbuf2);

    if (offs == nullptr)
    {
        free(tempbuf1);
        free(tempbuf2);
        return -1;
    }

    *offs = 0;
    int at = ustrlen(tempbuf1);
    free(tempbuf1);
    free(tempbuf2);
    return at;
}

int String_GetLength(const char *thisString) {
    return ScriptString::GetHeader(thisString).ULength;
}

//=============================================================================

size_t break_up_text_into_lines(const char *todis, bool apply_direction, SplitLines &lines, int wii, int fonnt, size_t max_lines)
{
    lines.Reset();
    longestline=0;

    // Don't attempt to display anything if the width is tiny
    if (wii < 3)
        return 0;

    split_lines(todis, lines, wii, fonnt, max_lines);

    int line_length;
    // Right-to-left just means reverse the text then
    // write it as normal
    if (apply_direction && (game.options[OPT_RIGHTLEFTWRITE] != 0))
        for (size_t rr = 0; rr < lines.Count(); rr++) {
            (get_uformat() == U_UTF8) ?
                lines[rr].ReverseUTF8() :
                lines[rr].Reverse();
            line_length = get_text_width_outlined(lines[rr].GetCStr(), fonnt);
            if (line_length > longestline)
                longestline = line_length;
        }
    else
        for (size_t rr = 0; rr < lines.Count(); rr++) {
            line_length = get_text_width_outlined(lines[rr].GetCStr(), fonnt);
            if (line_length > longestline)
                longestline = line_length;
        }
    return lines.Count();
}

// This is a somewhat ugly safety fix that tests whether the script tries
// to write inside the Character's struct (e.g. char.name?), and truncates
// the write limit accordingly.
size_t check_scstrcapacity(const char *ptr)
{
    const void *charstart = &game.chars[0];
    const void *charend = &game.chars[0] + game.chars.size();
    if ((ptr >= charstart) && (ptr <= charend))
        return sizeof(CharacterInfo::name);
    return MAX_MAXSTRLEN;
}

// Similar in principle to check_scstrcapacity, but this will sync
// legacy fixed-size name field with the contemporary property value.
void commit_scstr_update(const char *ptr)
{
    const void *charstart = &game.chars[0];
    const void *charend = &game.chars[0] + game.chars.size();
    if ((ptr >= charstart) && (ptr <= charend))
    {
        size_t char_index = ((uintptr_t)ptr - (uintptr_t)charstart) / sizeof(CharacterInfo);
        game.chars2[char_index].name_new = game.chars[char_index].name;
    }
}

const char *parse_voiceover_token(const char *text, int *voice_num)
{
    if (*text != '&')
    {
        if (voice_num)
            *voice_num = 0;
        return text; // no token
    }

    if (voice_num)
        *voice_num = atoi(&text[1]);
    // Skip the token and a single following space char
    for (; *text && *text != ' '; ++text) {}
    if (*text == ' ')
        ++text;
    return text;
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
    return RuntimeScriptValue().SetScriptObject((void*)CreateNewScriptString(scsf_buffer), &myScriptStringImpl);
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
    API_OBJCALL_FLOAT(const char, StringToFloat);
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

RuntimeScriptValue Sc_String_GetLength(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF(String_GetLength);
    return RuntimeScriptValue().SetInt32(String_GetLength((const char*)self));
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// const char* (const char *texx, ...)
const char *ScPl_String_Format(const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    return CreateNewScriptString(scsf_buffer);
}


void RegisterStringAPI()
{
    ScFnRegister string_api[] = {
        { "String::IsNullOrEmpty^1",  API_FN_PAIR(String_IsNullOrEmpty) },
        { "String::Format^101",       Sc_String_Format, ScPl_String_Format },

        { "String::Append^1",         API_FN_PAIR(String_Append) },
        { "String::AppendChar^1",     API_FN_PAIR(String_AppendChar) },
        { "String::CompareTo^2",      API_FN_PAIR(String_CompareTo) },
        { "String::Contains^1",       API_FN_PAIR(StrContains) },
        { "String::Copy^0",           API_FN_PAIR(String_Copy) },
        { "String::EndsWith^2",       API_FN_PAIR(String_EndsWith) },
        { "String::IndexOf^1",        API_FN_PAIR(StrContains) },
        { "String::LowerCase^0",      API_FN_PAIR(String_LowerCase) },
        { "String::Replace^3",        API_FN_PAIR(String_Replace) },
        { "String::ReplaceCharAt^2",  API_FN_PAIR(String_ReplaceCharAt) },
        { "String::StartsWith^2",     API_FN_PAIR(String_StartsWith) },
        { "String::Substring^2",      API_FN_PAIR(String_Substring) },
        { "String::Truncate^1",       API_FN_PAIR(String_Truncate) },
        { "String::UpperCase^0",      API_FN_PAIR(String_UpperCase) },
        { "String::get_AsFloat",      API_FN_PAIR(StringToFloat) },
        { "String::get_AsInt",        API_FN_PAIR(StringToInt) },
        { "String::geti_Chars",       API_FN_PAIR(String_GetChars) },
        { "String::get_Length",       API_FN_PAIR(String_GetLength) },
    };

    ccAddExternalFunctions(string_api);
}
