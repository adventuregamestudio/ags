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

class GfxFilter
{
public:
    virtual ~GfxFilter();

    virtual bool Initialize(const int color_depth, String &err_str);
    virtual void UnInitialize();
    virtual void GetRealResolution(int *width, int *height);
    virtual void SetMousePosition(int x, int y);
    // SetMouseArea shows the standard Windows cursor when the mouse moves outside
    // of it in windowed mode; SetMouseLimit does not
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
};

} // namespace Engine
} // namespace AGS


AGS::Engine::GfxFilter **get_allegro_gfx_filter_list();
AGS::Engine::GfxFilter **get_d3d_gfx_filter_list();

extern AGS::Engine::GfxFilter *filter;

extern AGS::Engine::GfxFilter *gfxFilterList[11];
extern AGS::Engine::GfxFilter *gfxFilterListD3D[16];

#endif // __AGS_EE_GFX__GFXFILTER_H
