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
// 
//
//=============================================================================
#ifndef __AGS_CN_GUI__GUISLIDER_H
#define __AGS_CN_GUI__GUISLIDER_H

#include "gui/guiobject.h"
#include "util/array.h"

namespace AGS
{
namespace Common
{

class GuiSlider:public GuiObject
{
public:
    GuiSlider();

    virtual bool IsOverControl(int x, int y, int leeway);
    virtual void Draw(Common::Bitmap *ds);

    virtual bool  OnMouseDown();
    virtual void OnMouseMove(int xp, int yp);
    virtual void OnMouseUp();

    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void WriteToSavedGame(Common::Stream *out);
    virtual void ReadFromSavedGame(Common::Stream *in, RuntimeGuiVersion gui_version);

// TODO: these members are currently public; hide them later
public:
    int32_t MinValue;
    int32_t MaxValue;
    int32_t Value;
    int32_t BackgroundImage;
    int32_t HandleImage;
    int32_t HandleOffset;
    bool    IsMousePressed;
    // The following variables are not persisted on disk
    // Cached coordinates of slider handle
    Rect    CachedHandleFrame;  
};

} // namespace Common
} // namespace AGS

extern AGS::Common::ObjectArray<AGS::Common::GuiSlider> guislider;
extern int numguislider;

#endif // __AGS_CN_GUI__GUISLIDER_H
