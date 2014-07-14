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

#ifndef __AC_GUISLIDER_H
#define __AC_GUISLIDER_H

#include "gui/guiobject.h"
#include "gui/dynamicarray.h"

struct GUISlider:public GUIObject
{
    int min, max;
    int value, mpressed;
    int handlepic, handleoffset, bgimage;
    // The following variables are not persisted on disk
    // Cached (x1, x2, y1, y2) co-ordinates of slider handle
    int cached_handtlx, cached_handbrx;
    int cached_handtly, cached_handbry;

    virtual void WriteToFile(Common::Stream *out);
    virtual void ReadFromFile(Common::Stream *in, GuiVersion gui_version);
    virtual void Draw(Common::Bitmap *ds);
    void MouseMove(int xp, int yp);

    void MouseOver()
    {
    }

    void MouseLeave()
    {
    }

    virtual int MouseDown()
    {
        mpressed = 1;
        // lock focus to ourselves
        return 1;
    }

    void MouseUp()
    {
        mpressed = 0;
    }

    void KeyPress(int kp)
    {
    }

    virtual int IsOverControl(int p_x, int p_y, int p_extra) {
        // check the overall boundary
        if (GUIObject::IsOverControl(p_x, p_y, p_extra))
            return 1;
        // now check the handle too
        if ((p_x >= cached_handtlx) && (p_y >= cached_handtly) &&
            (p_x < cached_handbrx) && (p_y < cached_handbry))
            return 1;
        return 0;
    }

    void reset()
    {
        GUIObject::init();
        min = 0;
        max = 10;
        value = 0;
        activated = 0;
        cached_handtlx = cached_handbrx = 0;
        cached_handtly = cached_handbry = 0;
        numSupportedEvents = 1;
        supportedEvents[0] = "Change";
        supportedEventArgs[0] = "GUIControl *control";
    }

    GUISlider() {
        reset();
    }
};

extern DynamicArray<GUISlider> guislider;
extern int numguislider;

#endif // __AC_GUISLIDER_H
