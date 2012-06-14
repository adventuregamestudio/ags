
#include <stdlib.h>                   // NULL
#include "acgfx/ac_gfxfilter.h"
#include "wgt2allg.h"
#include "mousew32.h"

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
