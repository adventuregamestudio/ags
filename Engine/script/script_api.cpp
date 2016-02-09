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

#include <stdio.h>
#include <string.h>
#include "ac/game_version.h"
#include "script/cc_error.h"
#include "script/runtimescriptvalue.h"
#include "script/script_api.h"
#include "util/math.h"

namespace Math = AGS::Common::Math;

#if defined (WINDOWS_VERSION)
#define snprintf _snprintf
#endif

char ScSfBuffer[STD_BUFFER_SIZE];

enum FormatParseResult
{
    kFormatParseNone,
    kFormatParseInvalid,
    kFormatParseArgument,
    kFormatParseLiteralPercent,
};

const char *ScriptSprintf(char *buffer, size_t buf_length, const char *format, const RuntimeScriptValue *args, int32_t argc)
{
    if (!buffer)
    {
        cc_error("internal error in ScriptSprintf: buffer is null");
        return "";
    }
    if (!format)
    {
        cc_error("internal error in ScriptSprintf: format string is null");
        return "";
    }
    if (argc > 0 && !args)
    {
        cc_error("internal error in ScriptSprintf: args pointer is null");
        return "";
    }

    // Expected format character count:
    // percent sign:    1
    // flag:            1
    // field width      10 (an uint32 number)
    // precision sign   1
    // precision        10 (an uint32 number)
    // length modifier  2
    // type             1
    // NOTE: although width and precision will
    // not likely be defined by a 10-digit
    // number, such case is theoretically valid.
    const size_t fmtbuf_size = 27;
    char       fmtbuf[fmtbuf_size];
    char       *fmt_bufptr;
    char       *fmt_bufendptr = &fmtbuf[fmtbuf_size - 1];

    char       *out_ptr    = buffer;
    const char *out_endptr = buffer + buf_length;
    const char *fmt_ptr    = format;
    int32_t    arg_idx     = 0;

    ptrdiff_t  avail_outbuf;
    int        snprintf_res;
    FormatParseResult fmt_done;

    // Parse the format string, looking for argument placeholders
    while (*fmt_ptr && out_ptr != out_endptr)
    {
        // Try to put argument into placeholder
        if (*fmt_ptr == '%' && arg_idx < argc)
        {
            avail_outbuf = out_endptr - out_ptr;
            fmt_bufptr = fmtbuf;
            *(fmt_bufptr++) = '%';
            snprintf_res = 0;
            fmt_done = kFormatParseNone;
            const RuntimeScriptValue &arg = args[arg_idx];

            // Parse placeholder
            while (*(++fmt_ptr) && fmt_done == kFormatParseNone && fmt_bufptr != fmt_bufendptr)
            {
                *(fmt_bufptr++) = *fmt_ptr;
                switch (*fmt_ptr)
                {
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'c':
                    // Print integer
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.IValue);
                    fmt_done = kFormatParseArgument;
                    break;
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'a':
                case 'A':
                    // Print float
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.FValue);
                    fmt_done = kFormatParseArgument;
                    break;
                case 's':
                    if (!arg.Ptr)
                    {
                        if (loaded_game_file_version < kGameVersion_320)
                        {
                            // print "(null)" into the placeholder;
                            // NOTE: the behavior of printf("%s", 0) is undefined (MS version prints "(null)",
                            // but there's no guarantee others will), therefore we shouldn't let snprintf do
                            // all the job here.
                            *fmt_bufptr = 0;
                            strncpy(out_ptr, "(null)", avail_outbuf);
                            snprintf_res = Math::Min<ptrdiff_t>(avail_outbuf, 6);
                            fmt_done = kFormatParseArgument;
                            break;
                        }
                        else
                        {
                            cc_error("ScriptSprintf: argument %d is expected to be a string, but it is null pointer", arg_idx);
                            return "";
                        }
                    }
                    else if (arg.Ptr == buffer)
                    {
                        cc_error("ScriptSprintf: argument %d is a pointer to output buffer", arg_idx);
                        return "";
                    }
                    // fall through intended ---
                case 'p':
                    // Print string, or pointer value
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.Ptr);
                    fmt_done = kFormatParseArgument;
                    break;
                case '%':
                    // This may be a literal percent sign ('%%')
                    if (fmt_bufptr - fmtbuf == 2)
                    {
                        fmt_done = kFormatParseLiteralPercent;
                    }
                    // ...Otherwise we reached the next placeholder
                    else
                    {
                        fmt_ptr--;
                        fmt_bufptr--;
                        fmt_done = kFormatParseInvalid;
                    }
                    break;
                }
            }

            if (fmt_done == kFormatParseArgument)
            {
                out_ptr += snprintf_res >= 0 ? snprintf_res : avail_outbuf;
                arg_idx++;
            }
            else if (fmt_done == kFormatParseLiteralPercent)
            {
                *(out_ptr++) = '%';
            }
            // If placeholder was not valid, just copy stored format buffer as it is
            else
            {
                size_t copy_len = Math::Min(Math::Min<ptrdiff_t>(fmt_bufptr - fmtbuf, fmtbuf_size - 1), avail_outbuf);
                memcpy(out_ptr, fmtbuf, copy_len);
                out_ptr += copy_len;
            }
        }
        // If there's no placeholder, simply copy the character to output buffer
        else
        {
            *(out_ptr++) = *(fmt_ptr++);
        }
    }

    // Terminate the string
    *out_ptr = 0;
    return buffer;
}

const char *ScriptVSprintf(char *buffer, size_t buf_length, const char *format, va_list &arg_ptr)
{
    if (!buffer)
    {
        cc_error("internal error in ScriptSprintf: buffer is null");
        return "";
    }
    if (!format)
    {
        cc_error("internal error in ScriptSprintf: format string is null");
        return "";
    }

    const size_t fmtbuf_size = 27;
    char       fmtbuf[fmtbuf_size];
    char       *fmt_bufptr;
    char       *fmt_bufendptr = &fmtbuf[fmtbuf_size - 1];

    char       *out_ptr    = buffer;
    const char *out_endptr = buffer + buf_length;
    const char *fmt_ptr    = format;
    int32_t    arg_idx     = 0;

    ptrdiff_t  avail_outbuf;
    int        snprintf_res;
    FormatParseResult fmt_done;

    union VAR_ARG
    {
        int32_t     IValue;
        float       FValue;
        const char  *Ptr;
    } arg;

    // Parse the format string, looking for argument placeholders
    while (*fmt_ptr && out_ptr != out_endptr)
    {
        // Try to put argument into placeholder
        if (*fmt_ptr == '%')
        {
            avail_outbuf = out_endptr - out_ptr;
            fmt_bufptr = fmtbuf;
            *(fmt_bufptr++) = '%';
            snprintf_res = 0;
            fmt_done = kFormatParseNone;

            // Parse placeholder
            while (*(++fmt_ptr) && fmt_done == kFormatParseNone && fmt_bufptr != fmt_bufendptr)
            {
                *(fmt_bufptr++) = *fmt_ptr;
                switch (*fmt_ptr)
                {
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'c':
                    // Print integer
                    arg = va_arg(arg_ptr, VAR_ARG);
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.IValue);
                    fmt_done = kFormatParseArgument;
                    break;
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'a':
                case 'A':
                    // Print float
                    arg = va_arg(arg_ptr, VAR_ARG);
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.FValue);
                    fmt_done = kFormatParseArgument;
                    break;
                case 's':
                    arg = va_arg(arg_ptr, VAR_ARG);
                    if (!arg.Ptr)
                    {
                        if (loaded_game_file_version < kGameVersion_320)
                        {
                            *fmt_bufptr = 0;
                            strncpy(out_ptr, "(null)", avail_outbuf);
                            snprintf_res = Math::Min<ptrdiff_t>(avail_outbuf, 6);
                            fmt_done = kFormatParseArgument;
                            break;
                        }
                        else
                        {
                            cc_error("ScriptSprintf: argument %d is expected to be a string, but it is null pointer", arg_idx);
                            return "";
                        }
                    }
                    else if (arg.Ptr == buffer)
                    {
                        cc_error("ScriptSprintf: argument %d is a pointer to output buffer", arg_idx);
                        return "";
                    }
                    // fall through intended ---
                case 'p':
                    // Print string, or pointer value
                    *fmt_bufptr = 0;
                    snprintf_res = snprintf(out_ptr, avail_outbuf, fmtbuf, arg.Ptr);
                    fmt_done = kFormatParseArgument;
                    break;
                case '%':
                    // This may be a literal percent sign ('%%')
                    if (fmt_bufptr - fmtbuf == 2)
                    {
                        fmt_done = kFormatParseLiteralPercent;
                    }
                    // ...Otherwise we reached the next placeholder
                    else
                    {
                        fmt_ptr--;
                        fmt_bufptr--;
                        fmt_done = kFormatParseInvalid;
                    }
                    break;
                }
            }

            if (fmt_done == kFormatParseArgument)
            {
                out_ptr += snprintf_res >= 0 ? snprintf_res : avail_outbuf;
                arg_idx++;
            }
            else if (fmt_done == kFormatParseLiteralPercent)
            {
                *(out_ptr++) = '%';
            }
            // If placeholder was not valid, just copy stored format buffer as it is
            else
            {
                size_t copy_len = Math::Min(Math::Min<ptrdiff_t>(fmt_bufptr - fmtbuf, fmtbuf_size - 1), avail_outbuf);
                memcpy(out_ptr, fmtbuf, copy_len);
                out_ptr += copy_len;
            }
        }
        // If there's no placeholder, simply copy the character to output buffer
        else
        {
            *(out_ptr++) = *(fmt_ptr++);
        }
    }

    // Terminate the string
    *out_ptr = 0;
    return buffer;
}
