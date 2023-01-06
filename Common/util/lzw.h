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
// LZW (un)compression functions.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__LZW_H
#define __AGS_CN_UTIL__LZW_H

#include "core/types.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

bool lzwcompress(Common::Stream *lzw_in, Common::Stream *out);
// Expands lzw-compressed data from src to dst.
// the dst buffer should be large enough, or the uncompression will not be complete.
bool lzwexpand(const uint8_t *src, size_t src_sz, uint8_t *dst, size_t dst_sz);

#endif // __AGS_CN_UTIL__LZW_H
