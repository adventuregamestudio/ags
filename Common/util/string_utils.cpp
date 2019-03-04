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

#include <errno.h>
#include <stdlib.h>
#include "gui/guidefines.h" // MAXLINE
#include "util/math.h"
#include "util/string_utils.h"
#include "util/stream.h"

using namespace AGS::Common;

#define STD_BUFFER_SIZE 3000

// Turn [ into \n and turn \[ into [
void unescape(char *buffer) {
    char *offset;
    // Handle the special case of the first char
    if(buffer[0] == '[')
    {
        buffer[0] = '\n';
        offset = buffer + 1;
    }
    else
        offset = buffer;
    // Replace all other occurrences as they're found
    while((offset = strchr(offset, '[')) != NULL) {
        if(offset[-1] != '\\')
            offset[0] = '\n';
        else
            memmove(offset - 1, offset, strlen(offset) + 1);
        offset++;
    }
}

char lines[MAXLINE][200];
int  numlines;

// Project-dependent implementation
extern int wgettextwidth_compensate(const char *tex, int font);

// Break up the text into lines
void split_lines(const char *todis, int wii, int fonnt) {
    // v2.56.636: rewrote this function because the old version
    // was crap and buggy
    int i = 0;
    int nextCharWas;
    int splitAt;
    char *theline;
    // make a copy, since we change characters in the original string
    // and this might be in a read-only bit of memory
    char textCopyBuffer[STD_BUFFER_SIZE];
    strcpy(textCopyBuffer, todis);
    theline = textCopyBuffer;
    unescape(theline);

    while (1) {
        splitAt = -1;

        if (theline[i] == 0) {
            // end of the text, add the last line if necessary
            if (i > 0) {
                strcpy(lines[numlines], theline);
                numlines++;
            }
            break;
        }

        // temporarily terminate the line here and test its width
        nextCharWas = theline[i + 1];
        theline[i + 1] = 0;

        // force end of line with the \n character
        if (theline[i] == '\n')
            splitAt = i;
        // otherwise, see if we are too wide
        else if (wgettextwidth_compensate(theline, fonnt) >= wii) {
            int endline = i;
            while ((theline[endline] != ' ') && (endline > 0))
                endline--;

            // single very wide word, display as much as possible
            if (endline == 0)
                endline = i - 1;

            splitAt = endline;
        }

        // restore the character that was there before
        theline[i + 1] = nextCharWas;

        if (splitAt >= 0) {
            // add this line
            nextCharWas = theline[splitAt];
            theline[splitAt] = 0;
            strcpy(lines[numlines], theline);
            numlines++;
            theline[splitAt] = nextCharWas;
            if (numlines >= MAXLINE) {
                strcat(lines[numlines-1], "...");
                break;
            }
            // the next line starts from here
            theline += splitAt;
            // skip the space or new line that caused the line break
            if ((theline[0] == ' ') || (theline[0] == '\n'))
                theline++;
            i = -1;
        }

        i++;
    }
}

//=============================================================================

String cbuf_to_string_and_free(char *char_buf)
{
    String s = char_buf;
    free(char_buf);
    return s;
}


namespace AGS
{
namespace Common
{

String StrUtil::IntToString(int d)
{
    return String::FromFormat("%d", d);
}

int StrUtil::StringToInt(const String &s, int def_val)
{
    if (!s.GetCStr())
        return def_val;
    char *stop_ptr;
    int val = strtol(s.GetCStr(), &stop_ptr, 0);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

StrUtil::ConversionError StrUtil::StringToInt(const String &s, int &val, int def_val)
{
    val = def_val;
    if (!s.GetCStr())
        return StrUtil::kFailed;
    char *stop_ptr;
    errno = 0;
    long lval = strtol(s.GetCStr(), &stop_ptr, 0);
    if (stop_ptr != s.GetCStr() + s.GetLength())
        return StrUtil::kFailed;
    if (lval > INT_MAX || lval < INT_MIN || errno == ERANGE)
        return StrUtil::kOutOfRange;
    val = (int)lval;
    return StrUtil::kNoError;
}

String StrUtil::ReadString(Stream *in)
{
    size_t len = in->ReadInt32();
    if (len > 0)
        return String::FromStreamCount(in, len);
    return String();
}

void StrUtil::ReadString(char *cstr, Stream *in, size_t buf_limit)
{
    size_t len = in->ReadInt32();
    if (buf_limit > 0)
        len = Math::Min(len, buf_limit - 1);
    cstr[len] = 0;
    if (len > 0)
        in->Read(cstr, len);
    else
        cstr[0] = 0;
}

void StrUtil::ReadString(String &s, Stream *in)
{
    size_t len = in->ReadInt32();
    s.ReadCount(in, len);
}

void StrUtil::ReadString(char **cstr, Stream *in)
{
    size_t len = in->ReadInt32();
    *cstr = new char[len + 1];
    (*cstr)[len] = 0;
    if (len > 0)
        in->Read(*cstr, len);
    else
        (*cstr)[0] = 0;
}

void StrUtil::SkipString(Stream *in)
{
    size_t len = in->ReadInt32();
    in->Seek(len);
}

void StrUtil::WriteString(const String &s, Stream *out)
{
    size_t len = s.GetLength();
    out->WriteInt32(len);
    if (len > 0)
        out->Write(s.GetCStr(), len);
}

void StrUtil::WriteString(const char *cstr, Stream *out)
{
    size_t len = strlen(cstr);
    out->WriteInt32(len);
    if (len > 0)
        out->Write(cstr, len);
}

void StrUtil::ReadCStr(char *buf, Stream *in, size_t buf_limit)
{
    int ichar;
    size_t read_count = 0;
    char *ptr = buf;
    do
    {
        ichar = in->ReadByte();
        if (ichar < 0)
            break;
        if (++read_count > buf_limit)
            continue;
        *(ptr++) = static_cast<char>(ichar);
    }
    while (ichar > 0);
    buf[read_count > buf_limit ? buf_limit - 1 : read_count - 1] = 0;
}

void StrUtil::WriteCStr(const char *cstr, Stream *out)
{
    size_t len = strlen(cstr);
    out->Write(cstr, len + 1);
}

void StrUtil::WriteCStr(const String &s, Stream *out)
{
    out->Write(s.GetCStr(), s.GetLength() + 1);
}

} // namespace Common
} // namespace AGS
