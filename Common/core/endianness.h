#ifndef __AGS_CN_CORE__ENDIANNESS_H
#define __AGS_CN_CORE__ENDIANNESS_H

#if !defined (WINDOWS_VERSION)

#if defined (LINUX_VERSION)
#include <endian.h>
#endif

// Detect endianess on Linux
// The logic is inverted on purpose so that it assumes
// little endian if the defines have not been set
#if !(__BYTE_ORDER == __LITTLE_ENDIAN)
#define AGS_BIG_ENDIAN
#endif

// Detect endianess on Mac
#if defined (__BIG_ENDIAN__) && !defined (AGS_BIG_ENDIAN)
#define AGS_BIG_ENDIAN
#endif

#endif // !WINDOWS_VERSION

#endif // __AGS_CN_CORE__ENDIANNESS_H
