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

#ifndef __AC_SCALINGGFXFILTER_H
#define __AC_SCALINGGFXFILTER_H

#include "gfx/gfxfilter.h"

struct MouseGetPosCallbackImpl;

struct ScalingGFXFilter : public GFXFilter {
protected:
    int MULTIPLIER;
    MouseGetPosCallbackImpl *mouseCallbackPtr;

    char filterName[100];
    char filterID[15];

    ScalingGFXFilter(int multiplier, bool justCheckingForSetup);

public:

    virtual const char* Initialize(int colDepth);
    virtual void UnInitialize();
    virtual int  GetScalingFactor() const;
    virtual void GetRealResolution(int *wid, int *hit);
    virtual void SetMouseArea(int x1, int y1, int x2, int y2);
    virtual void SetMouseLimit(int x1, int y1, int x2, int y2);
    virtual void SetMousePosition(int x, int y);
    virtual void AdjustPosition(int *x, int *y);
    virtual const char *GetVersionBoxText();
    virtual const char *GetFilterID();
    virtual ~ScalingGFXFilter();
};

#endif // __AC_SCALINGGFXFILTER_H
