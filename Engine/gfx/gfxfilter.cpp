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

#include <stdlib.h>                   // NULL
#include "gfx/gfxfilter.h"
#include "gfx/gfxfilter_allegro.h"
#include "gfx/gfxfilter_hq2x.h"
#include "gfx/gfxfilter_hq3x.h"
#include "gfx/gfxfilter_d3d.h"
#include "gfx/gfxfilter_aad3d.h"
#include "device/mousew32.h"

// Standard do-nothing filter

namespace AGS
{
namespace Engine
{

GfxFilter::~GfxFilter()
{
}

bool GfxFilter::Initialize(const int color_depth, String &err_str)
{
    return true;  // always succeeds
}

void GfxFilter::UnInitialize() {
    // do nothing
}

void GfxFilter::GetRealResolution(int *width, int *height)
{
    // no change
}

void GfxFilter::SetMouseArea(int x1, int y1, int x2, int y2)
{
    mgraphconfine(x1, y1, x2, y2);
}

void GfxFilter::SetMouseLimit(int x1, int y1, int x2, int y2)
{
    msetcursorlimit(x1, y1, x2, y2);
}

void GfxFilter::SetMousePosition(int x, int y)
{
    msetgraphpos(x, y);
}

const char *GfxFilter::GetVersionBoxText()
{
    return "";
}

const char *GfxFilter::GetFilterID()
{
    return "None";
}

} // namespace Engine
} // namespace AGS

using namespace AGS::Engine;
using namespace AGS::Engine::ALSW;
using namespace AGS::Engine::D3D;

Engine::GfxFilter *filter;

Engine::GfxFilter *gfxFilterList[11];
Engine::GfxFilter *gfxFilterListD3D[16];

Engine::GfxFilter **get_allegro_gfx_filter_list() {

    gfxFilterList[0] = new AllegroGfxFilter(1);
    gfxFilterList[1] = new AllegroGfxFilter(2);
    gfxFilterList[2] = new AllegroGfxFilter(3);
    gfxFilterList[3] = new AllegroGfxFilter(4);
    gfxFilterList[4] = new AllegroGfxFilter(5);
    gfxFilterList[5] = new AllegroGfxFilter(6);
    gfxFilterList[6] = new AllegroGfxFilter(7);
    gfxFilterList[7] = new AllegroGfxFilter(8);
    gfxFilterList[8] = new Hq2xGfxFilter();
    gfxFilterList[9] = new Hq3xGfxFilter();
    gfxFilterList[10] = NULL;

    return gfxFilterList;
}

Engine::GfxFilter **get_d3d_gfx_filter_list() {

    gfxFilterListD3D[0] = new D3DGfxFilter(1);
    gfxFilterListD3D[1] = new D3DGfxFilter(2);
    gfxFilterListD3D[2] = new D3DGfxFilter(3);
    gfxFilterListD3D[3] = new D3DGfxFilter(4);
    gfxFilterListD3D[4] = new D3DGfxFilter(5);
    gfxFilterListD3D[5] = new D3DGfxFilter(6);
    gfxFilterListD3D[6] = new D3DGfxFilter(7);
    gfxFilterListD3D[7] = new D3DGfxFilter(8);
    gfxFilterListD3D[8] = new AAD3DGfxFilter(2);
    gfxFilterListD3D[9] = new AAD3DGfxFilter(3);
    gfxFilterListD3D[10] = new AAD3DGfxFilter(4);
    gfxFilterListD3D[11] = new AAD3DGfxFilter(5);
    gfxFilterListD3D[12] = new AAD3DGfxFilter(6);
    gfxFilterListD3D[13] = new AAD3DGfxFilter(7);
    gfxFilterListD3D[14] = new AAD3DGfxFilter(8);
    gfxFilterListD3D[15] = NULL;

    return gfxFilterListD3D;
}
