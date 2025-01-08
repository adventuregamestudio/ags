//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Methods for loading and saving pixel data in various image formats.
//
//=============================================================================
#ifndef __AGS_CN_GFX__IMAGE_FILE_H
#define __AGS_CN_GFX__IMAGE_FILE_H

#include "gfx/bitmapdata.h"
#include "util/string.h"

struct RGB; // from Allegro 4

namespace AGS
{
namespace Common
{

namespace ImageFile
{
    // Reads PixelBuffer object from the stream, optionally filling in palette (if available).
    // "ext" parameter tells which image format to expect.
    PixelBuffer LoadImage(Stream *in, const String &ext, RGB *pal = nullptr);
    // Writes BitmapData object to the stream, optionally using a palette
    // "ext" parameter tells which image format to use.
    bool SaveImage(const BitmapData &bmdata, const RGB *pal, Stream *out, const String &ext);
} // namespace ImageFile

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__IMAGE_FILE_H
