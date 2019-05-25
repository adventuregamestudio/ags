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

#ifndef __AGS_CN_CORE__ENDIANNESS_H
#define __AGS_CN_CORE__ENDIANNESS_H

#if !defined(AGS_BIG_ENDIAN) && !defined(AGS_LITTLE_ENDIAN)

#if defined(WINDOWS_VERSION)
#define AGS_LITTLE_ENDIAN

#elif defined(LINUX_VERSION)
#include <endian.h>
#if !(__BYTE_ORDER == __LITTLE_ENDIAN)
#define AGS_BIG_ENDIAN
#else
#define AGS_LITTLE_ENDIAN
#endif

#elif defined(MAC_VERSION)
#include <machine/endian.h>
#if !(BYTE_ORDER == LITTLE_ENDIAN)
#define AGS_BIG_ENDIAN
#else
#define AGS_LITTLE_ENDIAN
#endif

#else
#error Unsupported platform.
#endif

#endif // !AGS_BIG_ENDIAN && !AGS_LITTLE_ENDIAN

#endif // __AGS_CN_CORE__ENDIANNESS_H
