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
// AGS specific color blending routines for transparency and tinting effects
//
//=============================================================================

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
