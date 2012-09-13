
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
#include <cstdlib> // for size_t
#include <limits.h> // for _WORDSIZE
#if !defined(PSP_VERSION)
#include <endian.h>
#endif
#endif

// Detect 64 bit environment
#if defined (_WIN64) || (__WORDSIZE == 64)
#define AGS_64BIT
#endif

// Detect endianess
// The logic is inverted on purpose so that it assumes
// little endian if the defines have not been set
#if !(__BYTE_ORDER == __LITTLE_ENDIAN)
#define AGS_BIGENDIAN
#endif

#if defined(WINDOWS_VERSION)
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

#endif // WINDOWS_VERSION

#endif // __AGS_CN_CORE__TYPES_H
