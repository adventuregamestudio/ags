//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__SPRITEFILEUTILS_H
#define __AGS_TOOL_DATA__SPRITEFILEUTILS_H

#include <regex>
#include <map>
#include <vector>
#include <allegro.h>
#include "ac/spritefile.h"
#include "data/game_utils.h"
#include "gfx/bitmapdata.h"
#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace DataUtil
{
    using HError = AGS::Common::HError;
    using String = AGS::Common::String;
    using PixelBuffer = AGS::Common::PixelBuffer;

    String GetCompressionName(Common::SpriteCompression compress);
    Common::SpriteCompression CompressionFromName(const String &compress_name);
    bool ResolveImageFilePattern(const String &pattern, String &res_pattern);
    bool ResolveImageFilePattern(const String &pattern, String &res_pattern, String &regex_pattern);
    // TODO: move elsewhere, more generic utils?
    HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, const std::regex &regex);

    struct GameColorSettings
    {
        GameColorDepth ColorDepth = kGameColorDepth_Undefined;
        AGS::Common::Palette Palette;
        std::array<PaletteColourType, PAL_SIZE> PalUses;

        GameColorSettings()
        {
            std::fill(Palette.begin(), Palette.end(), RGB{0,0,0,0});
            std::fill(PalUses.begin(), PalUses.end(), kPaletteColourType_Gamewide);
        }
    };

    using Palette = AGS::Common::Palette;

    // A cache of room background palettes
    typedef std::map<int, std::unique_ptr<Palette>> RoomPaletteCache;

    // Merges game and room palettes into the destination paletter, according to the "pal uses" rules
    void MergePalettes(Palette &dest_pal, const Palette &game_pal, const Palette &room_pal, const std::array<PaletteColourType, PAL_SIZE> &pal_uses);
    // Converts image in accordance to the sprite specs.
    // The final image will be located in dst_image.
    // If no format conversion is necessary, then moves original image to dst_image,
    // however, the pixel values may still be changed even then (transparency conversion).
    HError ConvertSpriteForGame(PixelBuffer &image, const Palette *pal,
        PixelBuffer &dst_image, const GameColorSettings &game_color_opts,
        const RoomPaletteCache *room_cache, const SpriteData &sprite);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__SPRITEFILEUTILS_H
