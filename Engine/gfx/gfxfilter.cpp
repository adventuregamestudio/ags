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
#include "gfx/gfxfilter_scalingallegro.h"
#include "gfx/gfxfilter_hq2x.h"
#include "gfx/gfxfilter_hq3x.h"
#include "gfx/gfxfilter_d3d.h"
#include "gfx/gfxfilter_aad3d.h"
#include "device/mousew32.h"

// Standard do-nothing filter

const char* GFXFilter::Initialize(int width, int height, int colDepth) {
    return NULL;  // always succeeds
}

void GFXFilter::UnInitialize() {
    // do nothing
}

void GFXFilter::GetRealResolution(int *wid, int *hit) {
    // no change
}

void GFXFilter::SetMouseArea(int x1, int y1, int x2, int y2) {
    mgraphconfine(x1, y1, x2, y2);
}

void GFXFilter::SetMouseLimit(int x1, int y1, int x2, int y2) {
    msetcursorlimit(x1, y1, x2, y2);
}

void GFXFilter::SetMousePosition(int x, int y) {
    msetgraphpos(x, y);
}

const char *GFXFilter::GetVersionBoxText() {
    return "";
}

const char *GFXFilter::GetFilterID() {
    return "None";
}

GFXFilter::~GFXFilter()
{
}


GFXFilter *filter;

GFXFilter *gfxFilterList[11];
GFXFilter *gfxFilterListD3D[16];

GFXFilter **get_allegro_gfx_filter_list(bool checkingForSetup) {

    gfxFilterList[0] = new AllegroGFXFilter(checkingForSetup);
    gfxFilterList[1] = new ScalingAllegroGFXFilter(2, checkingForSetup);
    gfxFilterList[2] = new ScalingAllegroGFXFilter(3, checkingForSetup);
    gfxFilterList[3] = new ScalingAllegroGFXFilter(4, checkingForSetup);
    gfxFilterList[4] = new ScalingAllegroGFXFilter(5, checkingForSetup);
    gfxFilterList[5] = new ScalingAllegroGFXFilter(6, checkingForSetup);
    gfxFilterList[6] = new ScalingAllegroGFXFilter(7, checkingForSetup);
    gfxFilterList[7] = new ScalingAllegroGFXFilter(8, checkingForSetup);
    gfxFilterList[8] = new Hq2xGFXFilter(checkingForSetup);
    gfxFilterList[9] = new Hq3xGFXFilter(checkingForSetup);
    gfxFilterList[10] = NULL;

    return gfxFilterList;
}

GFXFilter **get_d3d_gfx_filter_list(bool checkingForSetup) {

    gfxFilterListD3D[0] = new D3DGFXFilter(checkingForSetup);
    gfxFilterListD3D[1] = new D3DGFXFilter(2, checkingForSetup);
    gfxFilterListD3D[2] = new D3DGFXFilter(3, checkingForSetup);
    gfxFilterListD3D[3] = new D3DGFXFilter(4, checkingForSetup);
    gfxFilterListD3D[4] = new D3DGFXFilter(5, checkingForSetup);
    gfxFilterListD3D[5] = new D3DGFXFilter(6, checkingForSetup);
    gfxFilterListD3D[6] = new D3DGFXFilter(7, checkingForSetup);
    gfxFilterListD3D[7] = new D3DGFXFilter(8, checkingForSetup);
    gfxFilterListD3D[8] = new AAD3DGFXFilter(2, checkingForSetup);
    gfxFilterListD3D[9] = new AAD3DGFXFilter(3, checkingForSetup);
    gfxFilterListD3D[10] = new AAD3DGFXFilter(4, checkingForSetup);
    gfxFilterListD3D[11] = new AAD3DGFXFilter(5, checkingForSetup);
    gfxFilterListD3D[12] = new AAD3DGFXFilter(6, checkingForSetup);
    gfxFilterListD3D[13] = new AAD3DGFXFilter(7, checkingForSetup);
    gfxFilterListD3D[14] = new AAD3DGFXFilter(8, checkingForSetup);
    gfxFilterListD3D[15] = NULL;

    return gfxFilterListD3D;
}

/*
const char *initialize_gfx_filter(int width, int height, int depth) {

//filter = new ScalingGFXFilter(2, false);

filter = new Super2xSAIGFXFilter(false);

//filter = new Hq2xGFXFilter(false);

//filter = new GFXFilter();

return filter->Initialize(width, height, depth);
}

*/
