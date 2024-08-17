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
#include <cctype>
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
#include "util/string_utils.h"
#include "util/utf8.h"
#include "ac/dynobj/cc_dynamicarray.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern int longestline;

const char *CreateNewScriptString(const char *text)
{
    return static_cast<const char*>(ScriptString::Create(text).Obj);
}

// Gets a offset corresponding to a character index in a script String;
// tries to speed up utf-8 offset calculation by using recorded previous result
// of a offset search from the header. Updates header on success.
size_t GetStringCharOff(const char *thisString, int index)
{
    auto &header = ScriptString::GetHeader((void*)thisString);
    assert((index >= 0) && (static_cast<uint32_t>(index) < header.ULength));
    // No need to calculate anything or save last results in ASCII mode
    if (get_uformat() == U_ASCII)
    {
        return index;
    }

    int off;
    if (header.LastCharIdx <= index)
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
    return off;
}

int String_IsNullOrEmpty(const char *thisString) 
{
    if ((thisString == nullptr) || (thisString[0] == 0))
        return 1;

    return 0;
}

const char * String_Join(void* arrobj, const char *separator)
{
    if(arrobj == nullptr)
        return CreateNewScriptString("");

    std::vector<void*> items{};
    if (!DynamicArrayHelpers::ResolvePointerArray(arrobj, items))
    {
        // somehow this is not a managed array, this would have to be a compiler error
        return nullptr;
    }

    const size_t count = items.size();
    if(count == 0)
        return CreateNewScriptString("");

    if(separator == nullptr) {
        // I am not sure this can actually happen or the compiler will prevent this
        separator = "";
    }

    int sep_len, sep_ulen;
    ustrlen2(separator, &sep_len, &sep_ulen);

    int total_len, total_ulen;
    total_len = 0;
    total_ulen = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        auto address = static_cast<const char*>(items[i]);
        if (address != nullptr) {
            auto const& sh = ScriptString::GetHeader(address);
            total_len += sh.Length;
            total_ulen += sh.ULength;
        }
    }
    total_len += sep_len * (count - 1);
    total_ulen += sep_ulen * (count - 1);
    
    auto buf = ScriptString::CreateBuffer(total_len, total_ulen);

    // concatenate the array elements with a separator between elements: el1+sep+el2+sep...elN
    char* p = buf.Get();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto address = static_cast<const char*>(items[i]);
        if (address != nullptr) {
            auto const& sh = ScriptString::GetHeader(address);
            p = std::copy(address, address +sh.Length, p);
        }
        if (i != count - 1) {
            p = std::copy(separator, separator + sep_len, p);
        }
    }
    buf.Get()[total_len] = 0; // Null-terminate joint string
    return CreateNewScriptString(std::move(buf));
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

const char * TrimFront(const char *front, const char *back) {
    if (get_uformat() == U_UTF8) {
        for (int c = ugetc(front); front != back && uisspace(c);) {
            front += ucwidth(c);
            c = ugetc(front);
        }
    }
    else {
        for (; front != back && std::isspace(*front); ++front);
    }
    return front;
}

const char * TrimBack(const char *front, const char *back) {
    if (get_uformat() == U_UTF8) {
        const char* prev = Utf8::BackOneChar(back, front);
        for (int c = ugetc(prev); prev != front && uisspace(c); ) {
            back = prev;
            prev = Utf8::BackOneChar(prev, front);
            c = ugetc(prev);
        }
    }
    else {
        for (--back; back != front && std::isspace(*back); --back);
        ++back;
    }
    return back;
}

const char* String_Trim(const char *thisString)
{
    const auto &this_header = ScriptString::GetHeader(thisString);
    const char* front = thisString;
    const char* back = front + this_header.Length;

    front = TrimFront(front, back);

    if (*front == '\0')
        return CreateNewScriptString("");

    back = TrimBack(front, back);

    size_t copylen = back - front;
    // if no trim happened, we can return the same string as AGS String is immutable
    if (copylen == this_header.Length)
        return thisString;

    auto buf = ScriptString::CreateBuffer(copylen, 0);
    memcpy(buf.Get(), front, copylen);
    buf.Get()[copylen] = 0;
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
    const auto &this_header = ScriptString::GetHeader(thisString);
    // For case-sensitive search select simple ascii "strstr", for strict byte-to-byte comparison;
    // For case-insensitive search select no-case unicode-compatible variant
    typedef const char* (*fn_strstr)(const char *, const char *);
    fn_strstr pfn_strstr = 
        caseSensitive ? reinterpret_cast<fn_strstr>(ags_strstr) : reinterpret_cast<fn_strstr>(ustrcasestr);
    int match_len, match_ulen;
    ustrlen2(lookForText, &match_len, &match_ulen);

    // Record positions of matches
    std::vector<size_t> matches; // TODO: use optimized container to avoid extra allocs on heap
    for (const char *match_ptr = pfn_strstr(thisString, lookForText);
        match_ptr;
        matches.push_back(match_ptr - thisString), match_ptr = pfn_strstr(match_ptr + match_len, lookForText));

    if (matches.size() == 0)
        return thisString; // nothing to replace, return original string

    int replace_len, replace_ulen;
    ustrlen2(replaceWithText, &replace_len, &replace_ulen);
    size_t final_len = this_header.Length - match_len * matches.size() + replace_len * matches.size();
    size_t final_ulen = this_header.ULength - match_ulen * matches.size() + replace_ulen * matches.size();
    auto buf = ScriptString::CreateBuffer(final_len, final_ulen);

    // For each found match...
    char *write_ptr = buf.Get();
    const char *prev_ptr = thisString;
    for (const auto &m : matches)
    {
        write_ptr = std::copy(prev_ptr, thisString + m, write_ptr); // copy unchanged part
        write_ptr = std::copy(replaceWithText, replaceWithText + replace_len, write_ptr); // copy replacement
        prev_ptr = thisString + m + match_len;
    }
    std::copy(prev_ptr, thisString + this_header.Length, write_ptr); // copy unchanged part (if any left)
    buf.Get()[final_len] = 0; // terminate
    return CreateNewScriptString(std::move(buf));
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

void * String_Split(const char *thisString, const char *separator, int splitOptions)
{
    if (thisString == nullptr)
        return nullptr;

    bool removeEmpty = (splitOptions & 0x0001) != 0;
    bool trim = (splitOptions & 0x0002) != 0;
    auto const &header = ScriptString::GetHeader((void*)thisString);

    if ((thisString[0] == 0) || (separator == nullptr) || (separator[0] == 0) || strlen(separator) > header.Length)
    {
        std::vector<DynObjectRef> objs{};
        const char* front = thisString;
        const char* back = thisString + header.Length;

        if (trim) {
            front = TrimFront(front, back);
            if (front != back) {
                back = TrimBack(front, back);
            }
        }
        size_t len = back - front;
        auto buf = ScriptString::CreateBuffer(len, 0);
        std::copy(front, back, buf.Get());
        buf.Get()[len] = 0;
        if(!(removeEmpty && len == 0)) {
            objs.push_back(ScriptString::Create(thisString));
        }
        DynObjectRef arr = DynamicArrayHelpers::CreateScriptArray(std::move(objs));
        return arr.Obj;
    }

    std::vector<ScriptString::Buffer> items{};
    size_t seplen = strlen(separator);
    const char *ptr = thisString;
    const char *end = thisString + header.Length;
    size_t len = 0;
    while (ptr <= end)
    {
        const char *found_cstr = strstr(ptr, separator);
        if (!found_cstr) {
            found_cstr = end;
        }

        const char* front = ptr;
        const char* back = found_cstr;

        if (trim) {
            front = TrimFront(front, back);
            if (front != back) {
                back = TrimBack(front, back);
            }
        }

        len = back - front;

        ptr = found_cstr + seplen;
        if (removeEmpty && len == 0)
            continue;

        auto buf = ScriptString::CreateBuffer(len, 0);
        std::copy(front, back, buf.Get());
        buf.Get()[len] = 0;
        items.push_back(std::move(buf));
    }

    DynObjectRef arr = DynamicArrayHelpers::CreateStringArrayFromBuffers(std::move(items));
    return arr.Obj;
}

int String_GetChars(const char *thisString, int index) {
    auto &header = ScriptString::GetHeader((void*)thisString);
    if ((index < 0) || (static_cast<uint32_t>(index) >= header.ULength))
        return 0;

    int off = GetStringCharOff(thisString, index);
    return ugetc(thisString + off);
}

int StringToInt(const char*stino) {
    return atoi(stino);
}

int String_IndexOf (const char *thisString, const char *lookForText, bool caseSensitive, int startIndex, int count) {
    VALIDATE_STRING(thisString);
    VALIDATE_STRING(lookForText);

    const auto &header = ScriptString::GetHeader(thisString);
    if ((startIndex < 0) || ((size_t) startIndex >= header.ULength)) {
        debug_script_warn("String.IndexOf: invalid start index %d, string length is %u.", startIndex, header.ULength);
        return -1;
    }

    if (count == 0 || count >= header.ULength - startIndex)
        count = header.ULength - startIndex;

    if (count < 0) {
        debug_script_warn("String.IndexOf: invalid count %d. Did you meant to use '0' for until string end?", count);
        return -1;
    }

    // Use GetStringCharOff to possibly speed up start offset search in utf-8
    size_t start = GetStringCharOff(thisString, startIndex);
    size_t end   = (startIndex + count == header.ULength) ?
        header.Length :
        uoffset(thisString + start, count); // end offset searched from the found start

    auto thistmpbuf = StrUtil::Substring(thisString, start, end - start);
    auto looktmpbuf = StrUtil::Duplicate(lookForText);

    if (!caseSensitive)
    {
        ustrlwr(thistmpbuf.get());
        ustrlwr(looktmpbuf.get());
    }

    char *offs = ustrstr(thistmpbuf.get(), looktmpbuf.get());

    // return -1 if it can't find it
    if (offs == nullptr)
        return -1;

    *offs = 0;
    return startIndex + ustrlen(thistmpbuf.get());
}

int StrContains (const char *s1, const char *s2) {
    return String_IndexOf(s1, s2, false, 0, 0);
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

// const char * (void* arrobj, const char *separator)
RuntimeScriptValue Sc_String_Join(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ2(const char, myScriptStringImpl, String_Join, void, const char);
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

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_String_IndexOf(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL_PINT2(const char, String_IndexOf, const char);
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

RuntimeScriptValue Sc_String_Trim(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Trim);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_UpperCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_UpperCase);
}

// const char* [] (const char *thisString, const char* separator)
RuntimeScriptValue Sc_String_Split(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ_PINT(const char, void, globalDynamicArray, String_Split, const char);
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
        { "String::Join^2",           API_FN_PAIR(String_Join) },

        { "String::Append^1",         API_FN_PAIR(String_Append) },
        { "String::AppendChar^1",     API_FN_PAIR(String_AppendChar) },
        { "String::CompareTo^2",      API_FN_PAIR(String_CompareTo) },
        { "String::Contains^1",       API_FN_PAIR(StrContains) },
        { "String::Copy^0",           API_FN_PAIR(String_Copy) },
        { "String::EndsWith^2",       API_FN_PAIR(String_EndsWith) },
        { "String::IndexOf^1",        API_FN_PAIR(StrContains) },
        { "String::IndexOf^4",        API_FN_PAIR(String_IndexOf) },
        { "String::LowerCase^0",      API_FN_PAIR(String_LowerCase) },
        { "String::Replace^3",        API_FN_PAIR(String_Replace) },
        { "String::ReplaceCharAt^2",  API_FN_PAIR(String_ReplaceCharAt) },
        { "String::StartsWith^2",     API_FN_PAIR(String_StartsWith) },
        { "String::Substring^2",      API_FN_PAIR(String_Substring) },
        { "String::Truncate^1",       API_FN_PAIR(String_Truncate) },
        { "String::Trim^0",           API_FN_PAIR(String_Trim) },
        { "String::UpperCase^0",      API_FN_PAIR(String_UpperCase) },
        { "String::Split^2",          API_FN_PAIR(String_Split) },
        { "String::get_AsFloat",      API_FN_PAIR(StringToFloat) },
        { "String::get_AsInt",        API_FN_PAIR(StringToInt) },
        { "String::geti_Chars",       API_FN_PAIR(String_GetChars) },
        { "String::get_Length",       API_FN_PAIR(String_GetLength) },
    };

    ccAddExternalFunctions(string_api);
}
