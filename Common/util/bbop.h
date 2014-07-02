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
// Various utility bit and byte operations
//
//=============================================================================
#ifndef __AGS_CN_UTIL__BBOP_H
#define __AGS_CN_UTIL__BBOP_H

#include "core/types.h"

namespace AGS
{
namespace Common
{

enum DataEndianess
{
    kBigEndian,
    kLittleEndian,
#if defined(AGS_BIG_ENDIAN) || defined (TEST_BIGENDIAN)
    kDefaultSystemEndianess = kBigEndian
#else
    kDefaultSystemEndianess = kLittleEndian
#endif
};

namespace BitByteOperations
{
    inline int16_t SwapBytesInt16(const int16_t val)
    {
        return ((val >> 8) & 0xFF) | ((val << 8) & 0xFF00);
    }

    inline int32_t SwapBytesInt32(const int32_t val)
    {
        return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val << 8) & 0xFF0000) | ((val << 24) & 0xFF000000);
    }

    inline int64_t SwapBytesInt64(const int64_t val)
    {
        return ((val >> 56) & 0xFF) | ((val >> 40) & 0xFF00) | ((val >> 24) & 0xFF0000) |
              ((val >> 8) & 0xFF000000) | ((val << 8) & 0xFF00000000LL) |
              ((val << 24) & 0xFF0000000000LL) | ((val << 40) & 0xFF000000000000LL) | ((val << 56) & 0xFF00000000000000LL);
    }

} // namespace BitByteOperations


// Aliases for easier calling
namespace BBOp  = BitByteOperations;


} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__BBOP_H
