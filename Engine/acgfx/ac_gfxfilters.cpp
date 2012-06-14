
#include "acgfx/ac_gfxfilters.h"
#include "wgt2allg.h"
#include "acgfx/ac_allegrogfxfilter.h"
#include "acgfx/ac_scalingallegrogfxfilter.h"
#include "acgfx/ac_hq2xgfxfilter.h"
#include "acgfx/ac_hq3xgfxfilter.h"
#include "acgfx/ac_d3dgfxfilter.h"
#include "acgfx/ac_aad3dgfxfilter.h"

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
