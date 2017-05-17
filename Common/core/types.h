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
//
// Basic types definition
//
//=============================================================================
#ifndef __AGS_CN_CORE__TYPES_H
#define __AGS_CN_CORE__TYPES_H

#include "endianness.h"

#if defined (_WINDOWS) && !defined (WINDOWS_VERSION)
#define WINDOWS_VERSION
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef FORCEINLINE
#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#elif defined (__GNUC__)
#define FORCEINLINE inline __attribute__((__always_inline__))
#else
#define FORCEINLINE inline
#endif
#endif

#include <stddef.h>
#if !defined (WINDOWS_VERSION)
#include <stdint.h>
#include <cstdlib> // for size_t
#include <limits.h> // for _WORDSIZE
#endif

// Detect 64 bit environment
#if defined (_WIN64) || (__WORDSIZE == 64)
#define AGS_64BIT
#endif

#if defined(WINDOWS_VERSION)
#if defined(_MSC_VER) && (_MSC_VER >= 1600)

#include <stdint.h>

#else

#define int8_t       signed char
#define uint8_t      unsigned char
#define int16_t      signed short
#define uint16_t     unsigned short
#define int32_t      signed int
#define uint32_t     unsigned int
#define int64_t      signed __int64
#define uint64_t     unsigned __int64

#if defined (AGS_64BIT)
#define intptr_t     int64_t
#define uintptr_t    uint64_t
#else // AGS_32BIT
#define intptr_t     int32_t
#define uintptr_t    uint32_t
#endif
// tell Windows headers that we already have defined out intptr_t types
#define _INTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED

#endif // _MSC_VER >= 1600

#endif // WINDOWS_VERSION


#define fixed_t int32_t // fixed point type
#define color_t int32_t

// TODO: use distinct fixed point class
enum
{
    kShift    = 16,
    kUnit     = 1 << kShift
};

#endif // __AGS_CN_CORE__TYPES_H
