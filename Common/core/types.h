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
