//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Basic types definition
//
//=============================================================================
#ifndef __AGS_CN_CORE__TYPES_H
#define __AGS_CN_CORE__TYPES_H

#include <stddef.h> // for size_t
#include <stdint.h>
#include <limits.h> // for _WORDSIZE

#ifndef NULL
#define NULL nullptr
#endif

// Not all compilers have this. Added in clang and gcc followed
#ifndef __has_attribute
    #define __has_attribute(x) 0
#endif

#ifndef FORCEINLINE
    #ifdef _MSC_VER
        #define FORCEINLINE __forceinline

    #elif defined (__GNUC__) || __has_attribute(__always_inline__)
        #define FORCEINLINE inline __attribute__((__always_inline__))

    #else
        #define FORCEINLINE inline

    #endif
#endif

// Format compile time guard for printf style variadic functions in AGS
//
// Usage:
// function: fmt is the Nth parameter, counting from 1, and "..." must immediately follow it.
//   void quitprintf(const char *fmt, ...) AGS_FORMAT_STRING(1);
//
// Non-static member function: GCC/Clang count the implicit "this" as 1, so the format string's needs a plus 1.
//   void Format(const char *fcstr, ...) AGS_FORMAT_STRING(2);
//
// va_list overload: nothing to check past the format string itself, use the _V variant.
//   void FormatV(const char *fcstr, va_list argptr) AGS_FORMAT_STRING_V(2);
//
// MSVC is different and SAL's _Printf_format_string_ is on the parameter itself.
// NOTE: requires /analyze rather than by default compilation.
//   void Format(AGS_FORMAT_STRING_ARG const char *fcstr, ...) AGS_FORMAT_STRING(2);
//
#if defined(__GNUC__) || defined(__clang__)
    // see https://stackoverflow.com/questions/79973899/mingw-w64-produces-warning-for-own-function-using-attribute-format-when
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #include <stdio.h>
        #define AGS_PRINTF_ARCHETYPE __MINGW_PRINTF_FORMAT
    #else
        #define AGS_PRINTF_ARCHETYPE printf
    #endif

    // fmt_idx: 1-based index of the format parameter, "this" is index 1 for non-static member functions.
    #define AGS_FORMAT_STRING(fmt_idx) __attribute__((format(AGS_PRINTF_ARCHETYPE, fmt_idx, fmt_idx + 1)))
    // Same, but for va_list overloads: no variadic args to count.
    #define AGS_FORMAT_STRING_V(fmt_idx) __attribute__((format(AGS_PRINTF_ARCHETYPE, fmt_idx, 0)))
    #define AGS_FORMAT_STRING_ARG
#elif defined(_MSC_VER) && (_MSC_VER >= 1600) // VS 2010+
    #include <sal.h>
    #define AGS_FORMAT_STRING(fmt_idx)
    #define AGS_FORMAT_STRING_V(fmt_idx)
    #define AGS_FORMAT_STRING_ARG _Printf_format_string_
#else
    #define AGS_FORMAT_STRING(fmt_idx)
    #define AGS_FORMAT_STRING_V(fmt_idx)
    #define AGS_FORMAT_STRING_ARG
#endif

// Stream offset type
typedef int64_t soff_t;

typedef int32_t fixed_t; // fixed point type
// FIXME: this typedef is used inconsistently throughout the engine code,
// because of a planning mistake during code refactor. Sometimes it's used
// to mean "color property" (which holds "color index"), and sometimes
// it's used to mean "resolved bitmap color", which holds RGB in bitmap's
// compatible format. We must fix this, clarify the purpose of this type,
// choose to use it for one of those meanings only. Use either "int" or
// a new distinct type for another case.
typedef int32_t color_t; // AGS color number type (meaning depends on game's setting)

// TODO: use distinct fixed point class
enum
{
    kShift    = 16,
    kUnit     = 1 << kShift
};

#endif // __AGS_CN_CORE__TYPES_H
