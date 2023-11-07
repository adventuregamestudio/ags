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
//
// Graphic definitions and type/unit conversions.
//
//=============================================================================
#ifndef __AGS_CN_GFX__GFXDEF_H
#define __AGS_CN_GFX__GFXDEF_H

#include <algorithm>
#include "util/geometry.h"
#include "util/matrix.h"

namespace AGS
{
namespace Common
{

enum GraphicFlip
{
    kFlip_None,
    kFlip_Horizontal, // this means - mirror over horizontal middle line
    kFlip_Vertical,   // this means - mirror over vertical middle line
    kFlip_Both        // mirror over diagonal (horizontal and vertical)
};

// Blend modes for object sprites
enum BlendMode
{
    kBlend_Normal,        // alpha-blend src to dest, combining src & dest alphas
    kBlend_Add,
    kBlend_Darken,
    kBlend_Lighten,
    kBlend_Multiply,
    kBlend_Screen,
    kBlend_Burn,
    kBlend_Subtract,
    kBlend_Exclusion,
    kBlend_Dodge,
    kNumBlendModes
};


// GraphicSpace provides information about object's graphic location and basic shape on screen;
// this may be used for e.g. hit and collision detection.
// TODO: better name?
class GraphicSpace
{
public:
    GraphicSpace() {}
    GraphicSpace(int x, int y, int src_w, int src_h, int dst_w, int dst_h, float rot)
    {
        const float sx = src_w != 0.f ? (float)dst_w / src_w : 1.f;
        const float sy = src_h != 0.f ? (float)dst_h / src_h : 1.f;
        assert((sx != 0.f) && (sy != 0.f));
        // World->local transform
        W2LTransform = glmex::make_inv_transform2d((float)-x, (float)-y, 1.f / sx, 1.f / sy,
            -Math::DegreesToRadians(rot), 0.5f * dst_w, 0.5f * dst_h);
        // Local->world transform + AABB
        L2WTransform = glmex::make_transform2d((float)x, (float)y, sx, sy,
            Math::DegreesToRadians(rot), -0.5f * dst_w, -0.5f * dst_h);
        Rect aabb = RectWH(0, 0, src_w, src_h);
        _AABB = glmex::full_transform(aabb, L2WTransform);
    }

    // Get axis-aligned bounding box
    inline const Rect &AABB() const { return _AABB; }

    // Converts world coordinate into local object space
    inline Point WorldToLocal(int x, int y) const
    {
        glm::vec4 v = W2LTransform * glmex::vec4((float)x, (float)y);
        return Point((int)v.x, (int)v.y); // TODO: better rounding
    }

    // Converts local object coordinates into world space
    inline Point LocalToWorld(int x, int y) const
    {
        glm::vec4 v = L2WTransform * glmex::vec4((float)x, (float)y);
        return Point((int)v.x, (int)v.y); // TODO: better rounding
    }

private:
    glm::mat4 W2LTransform; // transform from world to local space
    glm::mat4 L2WTransform; // transform from local to world space
    Rect _AABB; // axis-aligned bounding box
};


namespace GfxDef
{
    // Converts percentage of transparency into alpha
    inline int Trans100ToAlpha255(int transparency)
    {
        return ((100 - transparency) * 255) / 100;
    }
    // Converts alpha into percentage of transparency
    inline int Alpha255ToTrans100(int alpha)
    {
        return 100 - ((alpha * 100) / 255);
    }

    // Special formulae to reduce precision loss and support flawless forth &
    // reverse conversion for multiplies of 10%
    inline int Trans100ToAlpha250(int transparency)
    {
        return ((100 - transparency) * 25) / 10;
    }

    inline int Alpha250ToTrans100(int alpha)
    {
        return 100 - ((alpha * 10) / 25);
    }

    // Convert correct 100-ranged transparency into legacy 255-ranged
    // transparency; legacy inconsistent transparency value range:
    // 0   = opaque,
    // 255 = invisible,
    // 1 -to- 254 = barely visible -to- mostly visible (as proper alpha)
    inline int Trans100ToLegacyTrans255(int transparency)
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
    inline int LegacyTrans255ToTrans100(int legacy_transparency)
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
    inline int LegacyTrans100ToAlpha255(int legacy_transparency)
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
    inline int LegacyTrans255ToAlpha255(int legacy_transparency)
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
    inline int Alpha255ToLegacyTrans255(int alpha)
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
} // namespace GfxDef

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GFX__GFXDEF_H
