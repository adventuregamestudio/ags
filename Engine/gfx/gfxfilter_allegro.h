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
// Standard software scaling filter
//
//=============================================================================

#ifndef __AGS_EE_GFX__ALLEGROGFXFILTER_H
#define __AGS_EE_GFX__ALLEGROGFXFILTER_H

#include "gfx/bitmap.h"
#include "gfx/gfxfilter_scaling.h"
#include "gfx/gfxdefines.h"

namespace AGS
{
namespace Engine
{
namespace ALSW
{

using Common::Bitmap;

class AllegroGfxFilter : public ScalingGfxFilter
{
public:
    AllegroGfxFilter(int multiplier = 1);

    virtual const GfxFilterInfo &GetInfo() const;
    
    virtual Bitmap *InitVirtualScreen(Bitmap *screen, int virtual_width, int virtual_height);
    virtual Bitmap *ShutdownAndReturnRealScreen(Bitmap *currentScreen);
    virtual void RenderScreen(Bitmap *toRender, int x, int y);
    virtual void RenderScreenFlipped(Bitmap *toRender, int x, int y, GlobalFlipType flipType);
    virtual void ClearRect(int x1, int y1, int x2, int y2, int color);
    virtual void GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap);
    virtual void GetCopyOfScreenIntoBitmap(Bitmap *copyBitmap, bool copy_with_yoffset);

    static const GfxFilterInfo FilterInfo;

protected:
    virtual Bitmap *PreRenderPass(Bitmap *toRender);

    // pointer to real screen bitmap
    Bitmap *realScreen;
    int lastBlitX;
    int lastBlitY;

    // bitmap the size of game resolution
    Bitmap *virtualScreen;
    // buffer for making a copy of video memory before stretching
    // for screen capture
    Bitmap *realScreenSizedBuffer;
    Bitmap *lastBlitFrom;
};

} // namespace ALSW
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALLEGROGFXFILTER_H
