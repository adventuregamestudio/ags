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

#include "gfx/gfx_util.h"

// CHECKME: is this hack still relevant?
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
extern int psp_gfx_renderer;
#endif

namespace AGS
{
namespace Engine
{

namespace GfxUtil
{

void DrawSpriteWithTransparency(Bitmap *ds, Bitmap *sprite, int x, int y, int transparency)
{
    if (transparency >= 0xFF)
    {
        // fully transparent, don't draw it at all
        return;
    }

    int surface_depth = ds->GetColorDepth();
    int sprite_depth  = sprite->GetColorDepth();

    if (sprite_depth < surface_depth
        // CHECKME: what is the purpose of this hack and is this still relevant?
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
        || (ds->GetBPP() < surface_depth && psp_gfx_renderer > 0) // Fix for corrupted speechbox outlines with the OGL driver
#endif
        )
    {
        if (sprite_depth == 8 && surface_depth >= 24)
        {
            // 256-col sprite -> truecolor background
            // this is automatically supported by allegro, no twiddling needed
            ds->Blit(sprite, x, y, Common::kBitmap_Transparency);
            return;
        }

        // 256-col sprite -> hi-color background, or
        // 16-bit sprite -> 32-bit background
        Bitmap hctemp;
        hctemp.CreateCopy(sprite, surface_depth);
        if (sprite_depth == 8)
        {
            // only do this for 256-col -> hi-color, cos the Blit call converts
            // transparency for 16->32 bit
            color_t mask_color = hctemp.GetMaskColor();
            for (int scan_y = 0; scan_y < hctemp.GetHeight(); ++scan_y)
            {
                // we know this must be 1 bpp source and 2 bpp pixel destination
                const uint8_t *src_scanline = sprite->GetScanLine(scan_y);
                uint16_t      *dst_scanline = (uint16_t*)hctemp.GetScanLineForWriting(scan_y);
                for (int scan_x = 0; scan_x < hctemp.GetWidth(); ++scan_x)
                {
                    if (src_scanline[scan_x] == 0)
                    {
                        dst_scanline[scan_x] = mask_color;
                    }
                }
            }
        }
        ds->Blit(&hctemp, x, y, Common::kBitmap_Transparency);
    }
    else
    {
        if (transparency != 0 && surface_depth > 8 && sprite_depth > 8) 
        {
            set_trans_blender(0, 0, 0, transparency);
            ds->TransBlendBlt(sprite, x, y);
        }
        else
        {
            ds->Blit(sprite, x, y, Common::kBitmap_Transparency);
        }
    }
}

} // namespace GfxUtil

} // namespace Engine
} // namespace AGS
