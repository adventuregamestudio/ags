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

Bitmap *ConvertBitmap(Bitmap *src, int dst_color_depth, bool keep_mask)
{
    int src_col_depth = src->GetColorDepth();
    if (src_col_depth != dst_color_depth)
    {
        int old_conv = get_color_conversion();
        // TODO: find out what is this, and why do we need to call this every time (do we?)
        set_color_conversion((COLORCONV_KEEP_TRANS * keep_mask) | COLORCONV_TOTAL);
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

    const int surface_depth = ds->GetColorDepth();
    const int sprite_depth  = sprite->GetColorDepth();

    // Allegro does not support masked blit or blend between different formats
    // *except* when drawing 8-bit sprites.
    std::unique_ptr<Bitmap> conv_bm;
    if ((surface_depth != sprite_depth) && (sprite_depth > 8))
    {
        // use ConvertBitmap in order to keep mask pixels
        conv_bm.reset(ConvertBitmap(sprite, surface_depth));
        sprite = conv_bm.get();
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
