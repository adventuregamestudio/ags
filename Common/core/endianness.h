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

#if !defined (WINDOWS_VERSION)

#if defined (LINUX_VERSION)
#include <endian.h>
#endif

// Detect endianess on Linux
// The logic is inverted on purpose so that it assumes
// little endian if the defines have not been set
#if defined(__BYTE_ORDER)
#if !(__BYTE_ORDER == __LITTLE_ENDIAN)
#define AGS_BIG_ENDIAN
#endif
#endif

// gcc predefiend macros
#if defined(__BYTE_ORDER__)
#if !(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define AGS_BIG_ENDIAN
#endif
#endif

// Detect endianess on Mac
#if defined (__BIG_ENDIAN__) && !defined (AGS_BIG_ENDIAN)
#define AGS_BIG_ENDIAN
#endif

#endif // !WINDOWS_VERSION

#endif // __AGS_CN_CORE__ENDIANNESS_H
