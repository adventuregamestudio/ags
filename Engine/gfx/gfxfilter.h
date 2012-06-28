/*
    AGS specific color blending routines for transparency and tinting
    effects

    Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
    All rights reserved.

    The AGS Editor Source Code is provided under the Artistic License 2.0
    http://www.opensource.org/licenses/artistic-license-2.0.php

    You MAY NOT compile your own builds of the engine without making it EXPLICITLY
    CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_GFXFILTER_H
#define __AC_GFXFILTER_H

struct GFXFilter {
public:

    virtual const char *Initialize(int width, int height, int colDepth);
    virtual void UnInitialize();
    virtual void GetRealResolution(int *wid, int *hit);
    virtual void SetMousePosition(int x, int y);
    // SetMouseArea shows the standard Windows cursor when the mouse moves outside
    // of it in windowed mode; SetMouseLimit does not
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
    virtual ~GFXFilter();
};

GFXFilter **get_allegro_gfx_filter_list(bool checkingForSetup);
GFXFilter **get_d3d_gfx_filter_list(bool checkingForSetup);


extern GFXFilter *filter;

extern GFXFilter *gfxFilterList[10];
extern GFXFilter *gfxFilterListD3D[10];

#endif // __AC_GFXFILTER_H
