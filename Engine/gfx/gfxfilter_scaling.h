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
//
// Base class for graphic filter which provides virtual screen scaling
//
//=============================================================================

#ifndef __AGS_EE_GFX__SCALINGGFXFILTER_H
#define __AGS_EE_GFX__SCALINGGFXFILTER_H

#include "gfx/gfxfilter.h"
#include "util/scaling.h"

namespace AGS
{
namespace Engine
{

using AGS::Common::PlaneScaling;

class ScalingGfxFilter : public IGfxFilter
{
public:
    bool Initialize(const int color_depth, String &err_str) override;
    void UnInitialize() override;
    Rect SetTranslation(const Size src_size, const Rect dst_rect) override;
    Rect GetDestination() const override;

protected:
    Rect            _dstRect;
    PlaneScaling    _scaling;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__SCALINGGFXFILTER_H
