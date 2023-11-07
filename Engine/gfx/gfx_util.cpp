//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"
#include "gfx/gfx_util.h"
#include "gfx/blender.h"
#include <allegro/internal/aintern.h> // for blenders

namespace AGS
{
namespace Engine
{

using namespace Common;

namespace GfxUtil
{

Bitmap *ConvertBitmap(Bitmap *src, int dst_color_depth)
{
    int src_col_depth = src->GetColorDepth();
    if (src_col_depth != dst_color_depth)
    {
        int old_conv = get_color_conversion();
        // TODO: find out what is this, and why do we need to call this every time (do we?)
        set_color_conversion(COLORCONV_KEEP_TRANS | COLORCONV_TOTAL);
        Bitmap *dst = BitmapHelper::CreateBitmapCopy(src, dst_color_depth);
        set_color_conversion(old_conv);
        return dst;
    }
    return src;
}


// Array of blender descriptions
// NOTE: set NULL function pointer to fallback to common image blitting
typedef BLENDER_FUNC PfnBlenderCb;
static const PfnBlenderCb BlendModeSets[kNumBlendModes] =
{
    _argb2argb_blender,             // kBlend_Alpha
    _blender_masked_add32,          // kBlend_Add
    _blender_masked_darken32,       // kBlend_Darken
    _blender_masked_lighten32,      // kBlend_Lighten
    _blender_masked_multiply32,     // kBlend_Multiply
    _blender_masked_screen32,       // kBlend_Screen
    _blender_masked_burn32,         // kBlend_Burn
    _blender_masked_subtract32,     // kBlend_Subtract
    _blender_masked_exclusion32,    // kBlend_Exclusion
    _blender_masked_dodge32,        // kBlend_Dodge
};

static bool SetBlender(BlendMode blend_mode, int alpha)
{
    if (blend_mode < 0 || blend_mode >= kNumBlendModes)
        return false;
    const auto &blender = BlendModeSets[blend_mode];
    set_blender_mode(nullptr, nullptr, blender, 0, 0, 0, alpha);
    return true;
}

void DrawSpriteBlend(Bitmap *ds, const Point &ds_at, Bitmap *sprite,
                       BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return; // do not draw 100% transparent image

    // support only 32-bit blending at the moment
    if ((ds->GetColorDepth() == 32) && (sprite->GetColorDepth() == 32) &&
        // set blenders if applicable and tell if succeeded
        SetBlender(blend_mode, alpha))
    {
        ds->TransBlendBlt(sprite, ds_at.X, ds_at.Y);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, ds_at.X, ds_at.Y, alpha);
    }
}

void DrawSpriteWithTransparency(Bitmap *ds, Bitmap *sprite, int x, int y, int alpha)
{
    if (alpha <= 0)
    {
        // fully transparent, don't draw it at all
        return;
    }

    Bitmap hctemp;
    const int surface_depth = ds->GetColorDepth();
    const int sprite_depth  = sprite->GetColorDepth();

    if (sprite_depth < surface_depth)
    {
        // If sprite is lower color depth than destination surface, e.g.
        // 8-bit sprites drawn on 16/32-bit surfaces.
        if (sprite_depth == 8 && surface_depth >= 24)
        {
            // 256-col sprite -> truecolor background
            // this is automatically supported by allegro, no twiddling needed
            ds->Blit(sprite, x, y, kBitmap_Transparency);
            return;
        }

        // 256-col sprite -> hi-color background, or
        // 16-bit sprite -> 32-bit background
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
        sprite = &hctemp;
    }

    if ((alpha < 0xFF) && (surface_depth > 8) && (sprite_depth > 8))
    {
        set_trans_blender(0, 0, 0, alpha);
        ds->TransBlendBlt(sprite, x, y);
    }
    else
    {
        ds->Blit(sprite, x, y, kBitmap_Transparency);
    }
}

} // namespace GfxUtil

} // namespace Engine
} // namespace AGS
