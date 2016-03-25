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
// Memory utils and algorithms
//
//=============================================================================
#ifndef __AGS_CN_UTIL__MEMORY_H
#define __AGS_CN_UTIL__MEMORY_H

#include <string.h>
#include "util/math.h"

namespace AGS
{
namespace Common
{

namespace Memory
{
    //-------------------------------------------------------------------------
    // Memory data manipulation
    //-------------------------------------------------------------------------
    // Copies block of 2d data from source into destination.
    inline void BlockCopy(uint8_t *dst, const size_t dst_pitch, const size_t dst_offset,
                   const uint8_t *src, const size_t src_pitch, const size_t src_offset,
                   const size_t height)
    {
        for (size_t y = 0; y < height; ++y, src += src_pitch, dst += dst_pitch)
            memcpy(dst + dst_offset, src + src_offset, Math::Min(dst_pitch - dst_offset, src_pitch - src_offset));
    }

} // namespace Memory

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__MEMORY_H
