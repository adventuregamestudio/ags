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
//
// Graphics filter base class; does no processing
//
//=============================================================================

#ifndef __AGS_EE_GFX__GFXFILTER_H
#define __AGS_EE_GFX__GFXFILTER_H

#include "util/string.h"

namespace AGS
{
namespace Engine
{

using Common::String;

struct GfxFilterInfo
{
    String   Id;
    String   Name;
    uint32_t FixedScale;

    GfxFilterInfo();
    GfxFilterInfo(String id, String name, uint32_t fixed_scale = 0);
};

class GfxFilter
{
public:
    virtual ~GfxFilter();

    virtual const GfxFilterInfo &GetInfo() const = 0;

    virtual bool Initialize(const int color_depth, String &err_str);
    virtual void UnInitialize();
    virtual void GetRealResolution(int *width, int *height);
    virtual void SetMousePosition(int x, int y);
    // SetMouseArea shows the standard Windows cursor when the mouse moves outside
    // of it in windowed mode; SetMouseLimit does not
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
};

} // namespace Engine
} // namespace AGS

extern AGS::Engine::GfxFilter *filter;

#endif // __AGS_EE_GFX__GFXFILTER_H
