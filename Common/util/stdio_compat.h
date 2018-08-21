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
#ifndef __AGS_CN_UTIL__STDIOCOMPAT_H
#define __AGS_CN_UTIL__STDIOCOMPAT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

// 64-bit fseek/ftell
#if defined(HAVE_FSEEKO) // Contemporary POSIX libc
#define file_off_t  off_t
#define fseek       fseeko
#define ftell       ftello
#elif defined(_MSC_VER) // MSVC
#define file_off_t  __int64
#define fseek       _fseeki64
#define ftell       _ftelli64
#else // No distinct interface with off_t
#define file_off_t  long
#endif

// 64-bit stat support for MSVC
#if defined(_MSC_VER) // MSVC
#define stat_t      _stat64
#define stat_fn     _stati64
#else
#define stat_t      stat
#define stat_fn     stat
#endif

#endif // __AGS_CN_UTIL__STDIOCOMPAT_H
