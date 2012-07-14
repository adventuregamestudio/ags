
#include <stdlib.h>                   // NULL
#include "gfx/gfxfilter.h"
#include "util/wgt2allg.h"
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

GFXFilter *gfxFilterList[10];
GFXFilter *gfxFilterListD3D[10];

GFXFilter **get_allegro_gfx_filter_list(bool checkingForSetup) {

    gfxFilterList[0] = new AllegroGFXFilter(checkingForSetup);
    gfxFilterList[1] = new ScalingAllegroGFXFilter(2, checkingForSetup);
    gfxFilterList[2] = new ScalingAllegroGFXFilter(3, checkingForSetup);
    gfxFilterList[3] = new ScalingAllegroGFXFilter(4, checkingForSetup);
    gfxFilterList[4] = new Hq2xGFXFilter(checkingForSetup);
    gfxFilterList[5] = new Hq3xGFXFilter(checkingForSetup);
    gfxFilterList[6] = NULL;

    return gfxFilterList;
}

GFXFilter **get_d3d_gfx_filter_list(bool checkingForSetup) {

    gfxFilterListD3D[0] = new D3DGFXFilter(checkingForSetup);
    gfxFilterListD3D[1] = new D3DGFXFilter(2, checkingForSetup);
    gfxFilterListD3D[2] = new D3DGFXFilter(3, checkingForSetup);
    gfxFilterListD3D[3] = new D3DGFXFilter(4, checkingForSetup);
    gfxFilterListD3D[4] = new AAD3DGFXFilter(2, checkingForSetup);
    gfxFilterListD3D[5] = new AAD3DGFXFilter(3, checkingForSetup);
    gfxFilterListD3D[6] = new AAD3DGFXFilter(4, checkingForSetup);
    gfxFilterListD3D[7] = NULL;

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
