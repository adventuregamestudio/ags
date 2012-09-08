
//=============================================================================
//
// Basic types definition
//
//=============================================================================
#ifndef __AGS_CN_CORE__TYPES_H
#define __AGS_CN_CORE__TYPES_H

#ifndef NULL
#define NULL 0
#endif

#if !defined (WINDOWS_VERSION)
#include <stdint.h>
#else  // WINDOWS_VERSION

#define int8_t       signed char
#define uint8_t      unsigned char
#define int16_t      signed short
#define uint16_t     unsigned short
#define int32_t      signed int
#define uint32_t     unsigned int
#define int64_t      signed __int64
#define uint64_t     unsigned __int64

#if defined (PLATFORM_64BIT) || defined (_WIN64)
#define intptr_t     int64_t
#define uintptr_t    uint64_t
#else // PLATFORM_32BIT
#define intptr_t     int32_t
#define uintptr_t    uint32_t
#endif
#define _INTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED

// NOTE: if you get errors when compiling windows C headers related to 'byte'
// symbol, ensure windows headers are included BEFORE the AGS types.h, so that
// AGS simply redefine byte symbol.
#define byte         unsigned char

#endif // WINDOWS_VERSION

#endif // __AGS_CN_CORE__TYPES_H
