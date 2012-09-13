
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
#if defined(AGS_BIGENDIAN)
    kDefaultSystemEndianess = kBigEndian
#else
    kDefaultSystemEndianess = kLittleEndian
#endif
};

namespace BitByteOperations
{
    inline void SwapBytesInt16(int16_t &val)
    {
        val = ((val >> 8) & 0xFF) | ((val << 8) & 0xFF00);
    }

    inline void SwapBytesInt32(int32_t &val)
    {
        val = ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val << 8) & 0xFF0000) | ((val << 24) & 0xFF000000);
    }

    inline void SwapBytesInt64(int64_t &val)
    {
        val = ((val >> 56) & 0xFF) | ((val >> 40) & 0xFF00) | ((val >> 24) & 0xFF0000) |
              ((val >> 8) & 0xFF000000) | ((val << 8) & 0xFF00000000LL) |
              ((val << 24) & 0xFF0000000000LL) | ((val << 40) & 0xFF000000000000LL) | ((val << 56) & 0xFF00000000000000LL);
    }

} // namespace BitByteOperations


// Aliases for easier calling
namespace BBOp  = BitByteOperations;


} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__BBOP_H
