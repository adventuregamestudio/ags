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
// Anti-aliased OGL filter
//
//=============================================================================

#ifndef __AGS_EE_GFX__AAOGLGFXFILTER_H
#define __AGS_EE_GFX__AAOGLGFXFILTER_H

#include "gfx/gfxfilter_ogl.h"

namespace AGS
{
namespace Engine
{
namespace OGL
{

class AAOGLGfxFilter : public OGLGfxFilter
{
public:
    const GfxFilterInfo &GetInfo() const override;

    bool UseLinearFiltering() const override;
    void GetFilteringForStandardSprite(int &filter, int &clamp) override;
    void SetFilteringForStandardSprite() override;

    static const GfxFilterInfo FilterInfo;
};

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__AAOGLGFXFILTER_H
