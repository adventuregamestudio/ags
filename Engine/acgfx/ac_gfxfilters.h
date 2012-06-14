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

#ifndef __AC_GFXFILTERS_H
#define __AC_GFXFILTERS_H

#include "acgfx/ac_gfxfilter.h"

GFXFilter **get_allegro_gfx_filter_list(bool checkingForSetup);
GFXFilter **get_d3d_gfx_filter_list(bool checkingForSetup);


extern GFXFilter *filter;

extern GFXFilter *gfxFilterList[10];
extern GFXFilter *gfxFilterListD3D[10];

#endif // __AC_GFXFILTERS_H