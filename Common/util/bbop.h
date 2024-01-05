//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Various utility bit and byte operations
//
//=============================================================================
#ifndef __AGS_CN_UTIL__BBOP_H
#define __AGS_CN_UTIL__BBOP_H

#include "core/platform.h"
#include "core/types.h"

#if AGS_PLATFORM_ENDIAN_BIG || defined (TEST_BIGENDIAN)
#define BITBYTE_BIG_ENDIAN
#endif

namespace AGS
{
namespace Common
{

enum DataEndianess
{
    kBigEndian,
    kLittleEndian,
#if defined (BITBYTE_BIG_ENDIAN)
    kDefaultSystemEndianess = kBigEndian
#else
    kDefaultSystemEndianess = kLittleEndian
#endif
};


//
// Various bit flags operations
//
// Converts from one flag into another:
// sets flag2 if flag1 IS set
// TODO: find more optimal way, using bitwise ops?
inline int FlagToFlag(int value, int flag1, int flag2)
{
    return ((value & flag1) != 0) * flag2;
}
// Sets flag2 if flag1 is NOT set
inline int FlagToNoFlag(int value, int flag1, int flag2)
{
    return ((value & flag1) == 0) * flag2;
}


namespace BitByteOperations
{
    struct IntFloatSwap
    {
        union
        {
            float    f;
            int32_t  i32;
        } val;

        explicit IntFloatSwap(int32_t i) { val.i32 = i; }
        explicit IntFloatSwap(float f) { val.f = f; }
    };

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

    inline float SwapBytesFloat(const float val)
    {
        IntFloatSwap swap(val);
        swap.val.i32 = SwapBytesInt32(swap.val.i32);
        return swap.val.f;
    }

    inline int16_t Int16FromLE(const int16_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return SwapBytesInt16(val);
#else
        return val;
#endif
    }

    inline int32_t Int32FromLE(const int32_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return SwapBytesInt32(val);
#else
        return val;
#endif
    }

    inline int64_t Int64FromLE(const int64_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return SwapBytesInt64(val);
#else
        return val;
#endif
    }

    inline float FloatFromLE(const float val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return SwapBytesFloat(val);
#else
        return val;
#endif
    }

    inline int16_t Int16FromBE(const int16_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return val;
#else
        return SwapBytesInt16(val);
#endif
    }

    inline int32_t Int32FromBE(const int32_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return val;
#else
        return SwapBytesInt32(val);
#endif
    }

    inline int64_t Int64FromBE(const int64_t val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return val;
#else
        return SwapBytesInt64(val);
#endif
    }

    inline float FloatFromBE(const float val)
    {
#if defined (BITBYTE_BIG_ENDIAN)
        return val;
#else
        return SwapBytesFloat(val);
#endif
    }

} // namespace BitByteOperations


// Aliases for easier calling
namespace BBOp  = BitByteOperations;


} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__BBOP_H
