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
//
// Graphic definitions and type/unit conversions.
//
// TODO: consider reorganizing this header; specifically move GraphicSpace to
// some other header, as it brings a bunch of matrix headers with itself.
//
//=============================================================================
#ifndef __AGS_CN_GFX__GFXDEF_H
#define __AGS_CN_GFX__GFXDEF_H

#include "util/geometry.h"

namespace AGS
{
namespace Common
{

// GraphicFlip tells how to flip (mirror) a sprite
enum GraphicFlip
{
    kFlip_None          = 0x0,
    kFlip_Horizontal    = 0x1, // this means - mirror over horizontal middle line
    kFlip_Vertical      = 0x2, // this means - mirror over vertical middle line
    kFlip_Both          = (kFlip_Horizontal | kFlip_Vertical) // mirror over diagonal
};

// Blend modes for object sprites
// This matches the blend modes in script API
enum BlendMode
{
    kBlend_Normal       = 0, // alpha-blend src to dest, combining src & dest alphas
    kBlend_Add,
    kBlend_Darken,
    kBlend_Lighten,
    kBlend_Multiply,
    kBlend_Screen,
    kBlend_Burn,
    kBlend_Subtract,
    kBlend_Exclusion,
    kBlend_Dodge,
    kBlend_Copy,
    kBlend_CopyRGB,
    kBlend_CopyAlpha,
    kNumBlendModes
};

// SpriteTransformFlags combine graphic effect options that do not have a value.
// These may be used both to store these options in memory and in serialization.
enum SpriteTransformFlags
{
    kSprTf_None     = 0x0000,
    kSprTf_FlipX    = 0x0001,
    kSprTf_FlipY    = 0x0002,
    kSprTf_FlipXY   = (kSprTf_FlipX | kSprTf_FlipY),
};

// GraphicResolution struct determines image size and color depth
struct GraphicResolution : Size
{
    int32_t ColorDepth; // color depth in bits per pixel

    GraphicResolution()
        : ColorDepth(0) {}

    GraphicResolution(const int32_t width, const int32_t height, const int32_t color_depth)
        : Size(width, height), ColorDepth(color_depth) {}

    GraphicResolution(const Size size, const int32_t color_depth)
        : Size(size), ColorDepth(color_depth) {}

    inline bool IsValid() const { return Width > 0 && Height > 0 && ColorDepth > 0; }
};


namespace GfxDef
{
    inline bool FlagsHaveFlip(const SpriteTransformFlags flags)
    {
        return (flags & kSprTf_FlipXY) != 0;
    }

    inline GraphicFlip GetFlipFromFlags(const SpriteTransformFlags flags)
    {
        switch (flags & kSprTf_FlipXY)
        {
        case kSprTf_FlipX: return kFlip_Horizontal;
        case kSprTf_FlipY: return kFlip_Vertical;
        case kSprTf_FlipXY: return kFlip_Both;
        default: return kFlip_None;
        }
    }

    inline SpriteTransformFlags GetFlagsFromFlip(const GraphicFlip flip)
    {
        return static_cast<SpriteTransformFlags>(kSprTf_FlipX * ((flip & kFlip_Horizontal) != 0)
            | kSprTf_FlipY * ((flip & kFlip_Vertical) != 0));
    }

    // Converts value from range of 100 to range of 250 (sic!);
    // uses formula that reduces precision loss and supports flawless forth &
    // reverse conversion for multiplies of 10%
    inline int Value100ToValue250(const int value100)
    {
        return (value100 * 25) / 10;
    }

    // Converts value from range of 250 to range of 100
    inline int Value250ToValue100(const int value100)
    {
        return (value100 * 10) / 25;
    }

    // Converts percentage of transparency into alpha
    inline int Trans100ToAlpha255(const int transparency)
    {
        return ((100 - transparency) * 255) / 100;
    }
    // Converts alpha into percentage of transparency
    inline int Alpha255ToTrans100(const int alpha)
    {
        return 100 - ((alpha * 100) / 255);
    }

    // Special formulae to reduce precision loss and support flawless forth &
    // reverse conversion for multiplies of 10%
    inline int Trans100ToAlpha250(const int transparency)
    {
        return ((100 - transparency) * 25) / 10;
    }

    inline int Alpha250ToTrans100(const int alpha)
    {
        return 100 - ((alpha * 10) / 25);
    }

    // Convert correct 100-ranged transparency into legacy 255-ranged
    // transparency; legacy inconsistent transparency value range:
    // 0   = opaque,
    // 255 = invisible,
    // 1 -to- 254 = barely visible -to- mostly visible (as proper alpha)
    inline int Trans100ToLegacyTrans255(const int transparency)
    {
        switch (transparency)
        {
        case 0:
            return 0; // this means opaque
        case 100:
            return 255; // this means invisible
        default:
            // the rest of the range works as alpha
            return Trans100ToAlpha250(transparency);
        }
    }

    // Convert legacy 255-ranged "incorrect" transparency into proper
    // 100-ranged transparency.
    inline int LegacyTrans255ToTrans100(const int legacy_transparency)
    {
        switch (legacy_transparency)
        {
        case 0:
            return 0; // this means opaque
        case 255:
            return 100; // this means invisible
        default:
            // the rest of the range works as alpha
            return Alpha250ToTrans100(legacy_transparency);
        }
    }

    // Convert legacy 100-ranged transparency into proper 255-ranged alpha
    // 0      => alpha 255
    // 100    => alpha 0
    // 1 - 99 => alpha 1 - 244
    inline int LegacyTrans100ToAlpha255(const int legacy_transparency)
    {
        switch (legacy_transparency)
        {
        case 0:
            return 255; // this means opaque
        case 100:
            return 0; // this means invisible
        default:
            // the rest of the range works as alpha (only 100-ranged)
            return legacy_transparency * 255 / 100;
        }
    }

    // Convert legacy 255-ranged transparency into proper 255-ranged alpha
    inline int LegacyTrans255ToAlpha255(const int legacy_transparency)
    {
        switch (legacy_transparency)
        {
        case 0:
            return 255; // this means opaque
        case 255:
            return 0; // this means invisible
        default:
            // the rest of the range works as alpha
            return legacy_transparency;
        }
    }

    // Convert 255-ranged alpha into legacy 255-ranged transparency
    inline int Alpha255ToLegacyTrans255(const int alpha)
    {
        switch (alpha)
        {
        case 255:
            return 0; // this means opaque
        case 0:
            return 255; // this means invisible
        default:
            // the rest of the range works as alpha
            return alpha;
        }
    }

    // Convert Alignment to the graphic anchor, which is a
    // normalized relative coordinates in [0; 1] range
    inline Pointf GetGraphicAnchorFromAlignment(FrameAlignment align)
    {
        float x;
        if ((align & kMAlignHCenter) != 0)
            x = 0.5f;
        else if((align & kMAlignRight) != 0)
            x = 1.f;
        else
            x = 0.f;

        float y;
        if ((align & kMAlignVCenter) != 0)
            y = 0.5f;
        else if ((align & kMAlignBottom) != 0)
            y = 1.f;
        else
            y = 0.f;

        return Pointf(x, y);
    }
} // namespace GfxDef

// Sets current blending mode, which will affect any further drawing
bool SetBlender(BlendMode blend_mode, int alpha);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__GFXDEF_H
