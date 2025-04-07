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

Bitmap *ConvertBitmap(const Bitmap *src, int dst_color_depth, bool keep_mask)
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
    return const_cast<Bitmap*>(src);
}

void DrawSpriteBlend(Bitmap *ds, const Point &ds_at, const Bitmap *sprite,
                       BlendMode blend_mode, int alpha)
{
    if (alpha <= 0)
        return; // do not draw 100% transparent image

    if (blend_mode == kBlend_Copy)
    {
        // optimized variant for Copy "blend mode"
        ds->Blit(sprite, ds_at.X, ds_at.Y);
    }
    // support only 32-bit blending at the moment
    else if ((ds->GetColorDepth() == 32) && (sprite->GetColorDepth() == 32) &&
        SetBlender(blend_mode, alpha))
    {
        ds->TransBlendBlt(sprite, ds_at.X, ds_at.Y);
    }
    else
    {
        GfxUtil::DrawSpriteWithTransparency(ds, sprite, ds_at.X, ds_at.Y, alpha);
    }
}

void DrawSpriteWithTransparency(Bitmap *ds, const Bitmap *sprite, int x, int y, int alpha)
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
