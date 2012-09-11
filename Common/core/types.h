
//=============================================================================
//
// Basic types definition
//
//=============================================================================
#ifndef __AGS_CN_CORE__TYPES_H
#define __AGS_CN_CORE__TYPES_H

#if defined (_WINDOWS) && !defined (WINDOWS_VERSION)
#define WINDOWS_VERSION
#endif

#ifndef NULL
#define NULL 0
#endif

#if !defined (WINDOWS_VERSION)
#include <stdint.h>
#else  // WINDOWS_VERSION

#if defined (_WIN64) && !defined (AGS_64BIT)
#define AGS_64BIT
#endif

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

// NOTE: if you get errors when compiling windows C headers related to 'byte'
// symbol, ensure Windows headers are included BEFORE the AGS types.h, so that
// AGS simply redefine byte symbol.
#define byte         unsigned char

#endif // WINDOWS_VERSION

//#define TEST_64BIT

// special type for debugging 64-bit pointers on 32-bit build
#if defined (AGS_64BIT)

#define intptr_var_t   intptr_t

#else // AGS_32BIT

#if defined TEST_64BIT
#define intptr_var_t   int64_t
#else
#define intptr_var_t   intptr_t
#endif

#endif // AGS_32BIT


#endif // __AGS_CN_CORE__TYPES_H
